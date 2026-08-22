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

typedef struct bit_test_benchmark_state {
    bignum_t value;
    size_t bit_index;
    int bit_value;
} bit_test_benchmark_state_t;

/** @brief Compares two optional workload tokens. */
static int equal_text(const char *left, const char *right)
{
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

/** @brief Advances the deterministic adapter generator. */
static uint64_t next_value(uint64_t *state)
{
    if (*state == 0U) *state = UINT64_C(0x9e3779b97f4a7c15);
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/** @brief Tests whether a token belongs to a NULL-terminated vocabulary. */
static int allowed(const char *value, const char *const *list)
{
    if (value == NULL || list == NULL) return 0;
    for (size_t i = 0U; list[i] != NULL; ++i) if (equal_text(value, list[i])) return 1;
    return 0;
}

/** @brief Maps a workload size token to a valid bignum word length. */
static size_t choose_length(const benchmark_workload_t *workload, uint64_t *state)
{
    if (equal_text(workload->size_profile, "one")) return 1U;
    if (equal_text(workload->size_profile, "quarter")) return BIGNUM_CAPACITY / 4U;
    if (equal_text(workload->size_profile, "half")) return BIGNUM_CAPACITY / 2U;
    if (equal_text(workload->size_profile, "near-capacity")) return BIGNUM_CAPACITY;
    return 1U + (size_t)(next_value(state) % (BIGNUM_CAPACITY / 2U));
}

/** @brief Fills one normalized deterministic bignum record. */
static void fill_value(bignum_t *value, size_t length, uint64_t *state, int zero)
{
    memset(value, 0, sizeof(*value));
    if (zero) return;
    value->len = length == 0U ? 1U : length;
    for (size_t word = 0U; word < value->len; ++word) value->words[word] = next_value(state);
    if (value->words[value->len - 1U] == 0U) value->words[value->len - 1U] = 1U;
}

/** @brief Initializes one framework source state from validated metadata. */
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

/** @brief Executes one selected-bit query on the mutable framework state. */
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

/** @brief Hashes source, index and output bit into the framework checksum. */
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

bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_validate_workload(
    const benchmark_workload_t *workload)
{
    static const char *const input[] = { "zero", "nonzero", "mixed", NULL };
    static const char *const operation[] = { "bit", "bit-zero", "bit-word", "bit-test", "bit-random", "bit-mixed", NULL };
    static const char *const measure[] = { "end-to-end", "kernel-only", NULL };
    static const char *const size[] = { "one", "quarter", "half", "variable", "near-capacity", NULL };
    static const char *const capacity[] = { "normal", "near-capacity", NULL };
    if (workload == NULL) return BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT;
    if (!allowed(workload->input_kind, input) || !allowed(workload->operation_kind, operation) ||
        !allowed(workload->measure_mode, measure) || !allowed(workload->size_profile, size) ||
        !allowed(workload->capacity_profile, capacity)) return BIGNUM_BIT_TEST_BENCHMARK_STATUS_INVALID_PROFILE;
    return BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS;
}

/** @brief Installs the bit-test callbacks into one framework adapter binding. */
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
