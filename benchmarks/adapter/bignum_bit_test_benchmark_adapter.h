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

/** @brief Project-owned validation statuses for adapter setup. */
typedef enum bignum_bit_test_benchmark_status {
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_SUCCESS = 0, /**< Adapter action succeeded. */
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer was NULL. */
    BIGNUM_BIT_TEST_BENCHMARK_STATUS_INVALID_PROFILE = 2 /**< A workload token was unsupported. */
} bignum_bit_test_benchmark_status_t;

/**
 * @brief Initializes the generic framework callbacks for bit testing.
 * @param[out] adapter Caller-owned callback binding to initialize.
 * @return A named project adapter status; NULL leaves the binding unchanged.
 * @thread_safety Safe because no mutable global state is used.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates the generic workload vocabulary accepted by this adapter.
 * @param[in] workload Borrowed immutable framework workload descriptor.
 * @return Success, NULL_ARGUMENT, or INVALID_PROFILE.
 * @thread_safety Safe for concurrent calls.
 */
bignum_bit_test_benchmark_status_t bignum_bit_test_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_BIT_TEST_BENCHMARK_ADAPTER_H */
