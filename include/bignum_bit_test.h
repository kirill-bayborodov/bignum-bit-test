/**
 * @file bignum_bit_test.h
 * @brief Public API for querying one bit of a fixed-capacity bignum.
 * @details The module reads an unsigned 2048-bit `bignum_t` without modifying it.
 * A bit index is numbered from zero at the least-significant bit of `words[0]`.
 * The API is caller-owned, allocation-free, reentrant, and safe for concurrent
 * calls when callers obey the core object's ordinary data-race rules.
 *
 * @version 1.0.0
 * @since 1.0.0
 */
#ifndef BIGNUM_BIT_TEST_H
#define BIGNUM_BIT_TEST_H

#include <bignum.h>
#include <stddef.h>

#ifndef BIGNUM_CAPACITY
#error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports the result of a bit-query operation.
 * @details A successful call writes exactly one value, `0` or `1`, to the
 * caller-provided output. Failure statuses leave the output value unchanged.
 */
typedef enum bignum_bit_test_status {
    BIGNUM_BIT_TEST_SUCCESS = 0, /**< Query completed; output is exactly 0 or 1. */
    BIGNUM_BIT_TEST_ERROR_NULL_ARG = -1, /**< `num` or `bit_value` is NULL; output is unchanged. */
    BIGNUM_BIT_TEST_ERROR_INDEX = -2, /**< `bit_index` is outside the fixed capacity; output is unchanged. */
    BIGNUM_BIT_TEST_ERROR_LENGTH = -3 /**< `num->len` exceeds capacity; output is unchanged. */
} bignum_bit_test_status_t;

/**
 * @brief Reads one bit from an unsigned fixed-capacity bignum.
 * @details The word index is `bit_index / 64` and the intra-word offset is
 * `bit_index % 64`. A valid index at or above the normalized `len` reads as
 * zero, so leading zero capacity and a zero value are handled without a
 * special caller-side branch. The function does not normalize, allocate, or
 * modify the input object.
 * @param[in] num Caller-owned bignum record; must be non-NULL and have
 * normalized `len <= BIGNUM_CAPACITY`.
 * @param[in] bit_index Zero-based bit position; valid range is
 * `[0, BIGNUM_CAPACITY * 64)`.
 * @param[out] bit_value Caller-allocated integer receiving `0` or `1` only on
 * success; its previous value is preserved on failure.
 * @return A named `bignum_bit_test_status_t` status code.
 * @pre `num` and `bit_value` point to live storage for the duration of the call.
 * @post On success, `*bit_value` equals the selected bit and `num` is unchanged.
 * @warning The function is a read-only query; callers must synchronize a
 * concurrent writer of `num`.
 * @thread_safety Reentrant and safe for independent concurrent calls.
 * @complexity O(1) time and O(1) auxiliary space.
 */
bignum_bit_test_status_t bignum_bit_test(const bignum_t *num, size_t bit_index,
                                         int *bit_value);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_BIT_TEST_H */
