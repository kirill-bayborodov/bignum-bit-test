/**
 * @file bignum_bit_test_benchmark_adapter.c
 * @brief Benchmark-framework callbacks for bignum_bit_test.
 * @details Each state contains one deterministic bignum, a selected bit index,
 * and the returned bit. The checksum observes every state field so the query
 * remains part of the measured workload.
 * @version 1.0.0
 */
#include "bignum_bit_test_benchmark_adapter.h"
#include "bignum_bit_test.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define BIT_TEST_FNV_OFFSET UINT64_C(1469598103934665603)
#define BIT_TEST_FNV_PRIME UINT64_C(1099511628211)

/**
 * @brief Holds one immutable benchmark source record and its mutable query state.
 * @details benchmark-core initializes one source record and copies it before
 * each measured operation. The adapter owns no heap storage; all fields remain
 * valid for the lifetime of the record supplied by benchmark-core.
 */
typedef struct bit_test_benchmark_state {
    bignum_t value; /**< [in,out] Caller/framework-owned normalized operand; valid during the callback. */
    size_t bit_index; /**< [in,out] Zero-based bit position in [0, BIGNUM_CAPACITY * 64); updated after query. */
    int bit_value; /**< [out] Last query result, -1 before operation and 0/1 after success. */
} bit_test_benchmark_state_t;

/**
 * @brief Compares two non-NULL workload tokens.
 * @details Equality is required for vocabulary checks; NULL is rejected rather
 * than dereferenced, so malformed framework metadata becomes a validation miss.
 * @param[in] left Borrowed NUL-terminated token; NULL is invalid.
 * @param[in] right Borrowed NUL-terminated token; NULL is invalid.
 * @return Non-zero only when both tokens are non-NULL and byte-equal.
 * @complexity O(min(strlen(left), strlen(right))) time and O(1) space.
 */
static int equal_text(const char *left, const char *right)
{
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

/**
 * @brief Advances the deterministic pseudo-random state.
 * @details The xorshift sequence is deterministic for a given non-zero seed and
 * is used only to construct reproducible records, not as a security primitive.
 * @param[in,out] state Caller-owned generator state; non-NULL and writable.
 * @return The next generated 64-bit value; no allocation occurs.
 * @pre `state` points to live storage.
 * @post `*state` contains the next generator state and is never zero after the
 * zero-state replacement.
 * @complexity O(1) time and O(1) space.
 */
static uint64_t next_value(uint64_t *state)
{
    if (*state == 0U) *state = UINT64_C(0x9e3779b97f4a7c15);
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/**
 * @brief Tests whether a token belongs to a NULL-terminated vocabulary.
 * @param[in] value Borrowed token to validate; NULL is rejected.
 * @param[in] list Caller-owned NULL-terminated token list; NULL is rejected.
 * @return Non-zero when an exact token match exists, otherwise zero.
 * @post Neither input is modified and no ownership is retained.
 * @complexity O(number of vocabulary entries × token length) time and O(1) space.
 */
static int allowed(const char *value, const char *const *list)
{
    if (value == NULL || list == NULL) return 0;
    for (size_t i = 0U; list[i] != NULL; ++i) if (equal_text(value, list[i])) return 1;
    return 0;
}

/**
 * @brief Maps a validated size token to a representable word length.
 * @details The mapping keeps every result within capacity; near-capacity uses
 * the full valid capacity and variable uses a deterministic bounded value.
 * @param[in] workload Borrowed validated workload descriptor.
 * @param[in,out] state Caller-owned deterministic generator state.
 * @return A word count in [1, BIGNUM_CAPACITY].
 * @pre `workload` and `state` are non-NULL and workload vocabulary is valid.
 * @complexity O(1) time and O(1) space.
 */
static size_t choose_length(const benchmark_workload_t *workload, uint64_t *state)
{
    if (equal_text(workload->size_profile, "one") ||
        equal_text(workload->size_profile, "tiny")) return 1U;
    if (equal_text(workload->size_profile, "quarter") ||
        equal_text(workload->size_profile, "small")) return BIGNUM_CAPACITY / 4U;
    if (equal_text(workload->size_profile, "half") ||
        equal_text(workload->size_profile, "medium")) return BIGNUM_CAPACITY / 2U;
    if (equal_text(workload->size_profile, "near-capacity") ||
        equal_text(workload->size_profile, "large")) return BIGNUM_CAPACITY;
    return 1U + (size_t)(next_value(state) % (BIGNUM_CAPACITY / 2U));
}

/**
 * @brief Fills one normalized deterministic bignum record.
 * @details The complete record is cleared first so stale words cannot influence
 * the checksum. A non-zero top word preserves the normalized-length invariant.
 * @param[out] value Caller/framework-owned record to overwrite; non-NULL.
 * @param[in] length Requested word count; bounded by BIGNUM_CAPACITY.
 * @param[in,out] state Caller-owned deterministic generator state.
 * @param[in] zero Non-zero requests the canonical zero record.
 * @post `value` is normalized, contains no uninitialized words, and ownership is unchanged.
 * @complexity O(BIGNUM_CAPACITY) time and O(1) auxiliary space.
 */
static void fill_value(bignum_t *value, size_t length, uint64_t *state, int zero)
{
    memset(value, 0, sizeof(*value));
    if (zero) return;
    value->len = length == 0U ? 1U : length;
    for (size_t word = 0U; word < value->len; ++word) value->words[word] = next_value(state);
    if (value->words[value->len - 1U] == 0U) value->words[value->len - 1U] = 1U;
}

/**
 * @brief Initializes one framework source state from validated metadata.
 * @details Combines the workload seed with the sequence index, maps the input
 * and size profiles, fills the immutable source record, and chooses a valid bit
 * index. Returning a framework input error publishes no successful state.
 * @param[out] opaque Framework-owned mutable record of adapter state size.
 * @param[in] index Zero-based dataset sequence index.
 * @param[in] workload Borrowed immutable workload metadata.
 * @param[in] context Optional adapter context; unused and not owned.
 * @return Named benchmark adapter status; success publishes a complete state,
 * input error leaves the record unsuitable for measurement.
 * @pre `opaque` and `workload` are non-NULL and workload validates.
 * @post On success `value`, `bit_index` and sentinel `bit_value` are initialized.
 * @thread_safety Safe for independent records; no shared mutable state.
 * @complexity O(BIGNUM_CAPACITY) time and O(1) auxiliary space.
 */
static benchmark_adapter_status_t initialize(void *opaque, uint64_t index,
                                              const benchmark_workload_t *workload, void *context)
{
    bit_test_benchmark_state_t *state = opaque;
    uint64_t random_state;
    int zero;
    (void)context;
    if (state == NULL || workload == NULL ||
        bignum_bit_test_benchmark_validate_workload(workload) != BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS)
        return BENCHMARK_ADAPTER_STATUS_INPUT_ERROR;
    random_state = workload->seed ^ (index + UINT64_C(0x9e3779b97f4a7c15));
    zero = equal_text(workload->input_kind, "zero") ||
        (equal_text(workload->input_kind, "mixed") && (index & 1U));
    fill_value(&state->value, choose_length(workload, &random_state), &random_state, zero);
    state->bit_index = (size_t)(next_value(&random_state) % (BIGNUM_CAPACITY * 64U));
    state->bit_value = -1;
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Executes one selected-bit query on the mutable framework state.
 * @details Calls the public bignum operation, records its 0/1 result, and then
 * advances the index so repeated calls cannot be optimized into one observation.
 * @param[in,out] opaque Framework-owned mutable state; non-NULL.
 * @param[in] iteration Framework iteration number used for deterministic index advance.
 * @param[in] workload Borrowed workload metadata; unused by this callback.
 * @param[in] context Optional adapter context; unused and not owned.
 * @return Named framework adapter status; operation error leaves no successful sample.
 * @post On success `bit_value` is 0/1 and the next index remains in capacity.
 * @thread_safety Safe for independent framework state copies.
 * @complexity O(1) time and O(1) auxiliary space.
 */
static benchmark_adapter_status_t operation(void *opaque, uint64_t iteration,
                                             const benchmark_workload_t *workload, void *context)
{
    bit_test_benchmark_state_t *state = opaque;
    (void)workload; (void)context;
    if (state == NULL || bignum_bit_test(&state->value, state->bit_index, &state->bit_value) != BIGNUM_BIT_TEST_SUCCESS)
        return BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR;
    state->bit_index = (state->bit_index + (size_t)(iteration & 63U)) % (BIGNUM_CAPACITY * 64U);
    return BENCHMARK_ADAPTER_STATUS_SUCCESS;
}

/**
 * @brief Hashes source, index and output bit into the framework checksum.
 * @details FNV-style accumulation observes every capacity word and metadata so
 * the benchmark result remains data-dependent and resistant to dead-code removal.
 * @param[in] opaque Borrowed immutable post-operation state; NULL yields checksum zero.
 * @param[in] iteration Framework iteration mixed into the result.
 * @param[in] context Optional adapter context; unused and not owned.
 * @return Deterministic 64-bit checksum; no output pointer or ownership transfer.
 * @thread_safety Safe for independent immutable state reads.
 * @complexity O(BIGNUM_CAPACITY) time and O(1) space.
 */
static uint64_t checksum(const void *opaque, uint64_t iteration, void *context)
{
    const bit_test_benchmark_state_t *state = opaque;
    uint64_t hash = BIT_TEST_FNV_OFFSET;
    (void)context;
    if (state == NULL) return 0U;
    for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word) {
        hash ^= state->value.words[word];
        hash *= BIT_TEST_FNV_PRIME;
    }
    hash ^= (uint64_t)state->value.len; hash *= BIT_TEST_FNV_PRIME;
    hash ^= (uint64_t)state->bit_index; hash *= BIT_TEST_FNV_PRIME;
    hash ^= (uint64_t)(state->bit_value & 1); hash *= BIT_TEST_FNV_PRIME;
    return hash ^ iteration;
}

/**
 * @brief Implements adapter workload validation.
 * @details Exact matching prevents generic framework metadata from silently
 * selecting an unsupported bignum scenario. The static vocabularies are read-only
 * and the caller's descriptor is never modified; the complete public parameter,
 * status and ownership contract is declared in the adapter header.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_validate_workload(
    const benchmark_workload_t *workload)
{
    static const char *const input[] = { "zero", "nonzero", "mixed", NULL };
    static const char *const operation[] = {
        "bit", "bit-zero", "bit-word", "bit-test", "bit-random", "bit-mixed",
        "noop", "default", "mixed", NULL
    };
    static const char *const measure[] = { "end-to-end", "kernel-only", NULL };
    static const char *const size[] = {
        "one", "quarter", "half", "variable", "near-capacity",
        "tiny", "small", "medium", "large", NULL
    };
    static const char *const capacity[] = { "normal", "near-capacity", NULL };
    if (workload == NULL) return BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT;
    if (!allowed(workload->input_kind, input) || !allowed(workload->operation_kind, operation) ||
        !allowed(workload->measure_mode, measure) || !allowed(workload->size_profile, size) ||
        !allowed(workload->capacity_profile, capacity)) return BIGNUM_BIT_TEST_BENCHMARK_STATUS_INVALID_PROFILE;
    return BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS;
}

/**
 * @brief Installs the bit-test callbacks into one framework adapter binding.
 * @details The accepted vocabulary includes both project-specific matrix tokens
 * and benchmark-core legacy aliases. The latter are produced by `--data-mode`
 * before the adapter receives the workload and must remain valid for Makefile
 * `bench`, `bench_full`, and `bench_cl` workflows.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_adapter_init(benchmark_adapter_t *adapter)
{
    if (adapter == NULL) return BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT;
    *adapter = (benchmark_adapter_t){
        .benchmark_name = "bignum_bit_test",
        .state_size = sizeof(bit_test_benchmark_state_t),
        .success_code = BENCHMARK_ADAPTER_STATUS_SUCCESS,
        .adapter_context = NULL,
        .initialize = initialize,
        .operation = operation,
        .checksum = checksum
    };
    return BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS;
}
