/**
 * @file bignum_bit_test_benchmark_adapter.h
 * @brief Benchmark-framework adapter contract for bignum_bit_test.
 * @details The adapter maps generic workload fields to deterministic bignum
 * records and a selected zero-based bit index. It owns no heap memory and uses
 * the framework's callback status model.
 * @version 1.0.0
 */
#ifndef BIGNUM_BIT_TEST_BENCHMARK_ADAPTER_H
#define BIGNUM_BIT_TEST_BENCHMARK_ADAPTER_H

#include <benchmark_framework.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports validation and adapter-binding outcomes.
 * @details The adapter does not allocate or transfer ownership. A successful
 * status guarantees that the requested binding or validation completed. Error
 * statuses leave caller-owned objects unchanged and are safe to retry after
 * correcting the input.
 */
typedef enum bignum_bit_test_benchmark_status {
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS = 0, /**< The action completed; all documented outputs are valid. */
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer was NULL; caller-owned outputs remain unchanged. */
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_INVALID_PROFILE = 2 /**< A workload token was outside the adapter vocabulary; no state is published. */
} bignum_bit_test_benchmark_status_t;

/**
 * @brief Initializes the generic framework callbacks for bit testing.
 * @details Writes a complete callback binding for the benchmark-core lifecycle.
 * The framework retains no ownership of the binding's storage and the adapter
 * uses no mutable global state. On failure no field of `adapter` is written.
 * @param[out] adapter Caller-allocated, caller-owned binding; non-NULL and live
 * for the complete benchmark-core run; no aliasing with internal storage.
 * @return `BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS` when all callbacks and
 * constants are installed, or `BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT`
 * when `adapter` is NULL; output is unchanged on failure.
 * @pre `adapter` points to writable storage of `benchmark_adapter_t` size.
 * @post On success every callback, state size and success code are initialized.
 * @warning The caller must not mutate the binding while benchmark-core runs.
 * @thread_safety Safe for concurrent calls with independent bindings.
 * @complexity O(1) time and O(1) auxiliary space.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates the generic workload vocabulary accepted by this adapter.
 * @details Checks every workload axis against the finite vocabulary consumed by
 * the bignum adapter. The descriptor is inspected but never modified.
 * @param[in] workload Borrowed immutable framework descriptor; non-NULL and
 * valid for the call; ownership remains with benchmark-core and no aliasing is
 * retained after return.
 * @return `BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS` for supported values,
 * `..._NULL_ARGUMENT` for NULL, or `..._INVALID_PROFILE` for an unsupported
 * token; there are no output parameters and no partial state is published.
 * @pre All framework string members referenced by the descriptor are valid.
 * @post The workload remains byte-for-byte unchanged.
 * @thread_safety Safe for concurrent calls when descriptors are independent and
 * immutable.
 * @complexity O(vocabulary size) time and O(1) auxiliary space.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_BIT_TEST_BENCHMARK_ADAPTER_H */
