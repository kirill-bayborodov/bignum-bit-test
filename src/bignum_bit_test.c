/**
 * @file bignum_bit_test.c
 * @brief C11 reference implementation for querying one bignum bit.
 * @details The implementation uses bounded division by the core word width to
 * locate one word, then masks one bit. It intentionally treats capacity words
 * above a normalized `len` as zero and never changes the input record.
 * @version 1.0.0
 */
#include "bignum_bit_test.h"

/**
 * @brief Validates the fixed-capacity representation before reading a word.
 * @param[in] num Caller-owned input record.
 * @return Non-zero if `num->len` is within the compile-time capacity.
 */
static int valid_length(const bignum_t *num)
{
    return num->len <= BIGNUM_CAPACITY;
}

/**
 * @brief Reads a selected bit from one validated word.
 * @param[in] word Source word containing the selected bit.
 * @param[in] offset Intra-word bit offset in the range [0, 63].
 * @return The selected bit as an integer 0 or 1.
 */
static int extract_bit(uint64_t word, size_t offset)
{
    return (int)((word >> offset) & UINT64_C(1));
}

bignum_bit_test_status_t bignum_bit_test(const bignum_t *num, size_t bit_index,
                                         int *bit_value)
{
    const size_t total_bits = BIGNUM_CAPACITY * (size_t)64U;
    const size_t word_index = bit_index / (size_t)64U;
    const size_t bit_offset = bit_index % (size_t)64U;

    if (num == NULL || bit_value == NULL) return BIGNUM_BIT_TEST_ERROR_NULL_ARG;
    if (bit_index >= total_bits) return BIGNUM_BIT_TEST_ERROR_INDEX;
    if (!valid_length(num)) return BIGNUM_BIT_TEST_ERROR_LENGTH;

    /* Words above normalized len are logically zero; do not inspect stale storage. */
    *bit_value = word_index < num->len ? extract_bit(num->words[word_index], bit_offset) : 0;
    return BIGNUM_BIT_TEST_SUCCESS;
}
