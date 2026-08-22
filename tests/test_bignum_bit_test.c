/**
 * @file test_bignum_bit_test.c
 * @brief Deterministic contract tests for bignum_bit_test.
 * @details The same source and assertions run against C11 and x86-64 YASM.
 * It checks every status, bit boundary, normalized-length rule, immutability,
 * and output preservation on failure.
 * @version 1.0.0
 */
#include "bignum_bit_test.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/** @brief Initializes a normalized scalar bignum record. */
static void set_u64(bignum_t *value, uint64_t word)
{
    memset(value, 0, sizeof(*value));
    if (word != 0U) { value->words[0] = word; value->len = 1U; }
}

/** @brief Checks one successful query and confirms that the input is unchanged. */
static void expect_bit(const bignum_t *input, size_t index, int expected)
{
    bignum_t before = *input;
    int actual = -1;
    assert(bignum_bit_test(input, index, &actual) == BIGNUM_BIT_TEST_SUCCESS);
    assert(actual == expected);
    assert(memcmp(input, &before, sizeof(before)) == 0);
}

/** @brief Covers zero values and the first, last and cross-word bit positions. */
static void test_boundaries(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    expect_bit(&value, 0U, 0);
    expect_bit(&value, 63U, 0);
    expect_bit(&value, 64U, 0);
    expect_bit(&value, 2047U, 0);

    value.len = BIGNUM_CAPACITY;
    value.words[0] = UINT64_C(1);
    value.words[1] = UINT64_C(1) << 63U;
    value.words[15] = UINT64_C(1) << 7U;
    value.words[31] = UINT64_C(1) << 63U;
    expect_bit(&value, 0U, 1);
    expect_bit(&value, 1U, 0);
    expect_bit(&value, 127U, 1);
    expect_bit(&value, 967U, 1);
    expect_bit(&value, 1024U, 0);
    expect_bit(&value, 2047U, 1);
    puts("test_boundaries: PASSED");
}

/** @brief Confirms words above normalized len are treated as logical zero. */
static void test_normalized_prefix(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = 1U;
    value.words[0] = UINT64_C(0x8000000000000001);
    value.words[7] = UINT64_MAX;
    expect_bit(&value, 0U, 1);
    expect_bit(&value, 63U, 1);
    expect_bit(&value, 64U, 0);
    expect_bit(&value, 511U, 0);
    puts("test_normalized_prefix: PASSED");
}

/** @brief Checks all NULL, range and malformed-length statuses transactionally. */
static void test_invalid_arguments(void)
{
    bignum_t value;
    int bit = 7;
    set_u64(&value, 1U);
    assert(bignum_bit_test(NULL, 0U, &bit) == BIGNUM_BIT_TEST_ERROR_NULL_ARG);
    assert(bit == 7);
    assert(bignum_bit_test(&value, 0U, NULL) == BIGNUM_BIT_TEST_ERROR_NULL_ARG);
    assert(bignum_bit_test(&value, BIGNUM_CAPACITY * 64U, &bit) == BIGNUM_BIT_TEST_ERROR_INDEX);
    assert(bit == 7);
    assert(bignum_bit_test(&value, SIZE_MAX, &bit) == BIGNUM_BIT_TEST_ERROR_INDEX);
    assert(bit == 7);
    value.len = BIGNUM_CAPACITY + 1U;
    assert(bignum_bit_test(&value, 0U, &bit) == BIGNUM_BIT_TEST_ERROR_LENGTH);
    assert(bit == 7);
    puts("test_invalid_arguments: PASSED");
}

/** @brief Checks every intra-word offset using complementary bit patterns. */
static void test_intra_word_pattern(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = 2U;
    value.words[0] = UINT64_C(0xAAAAAAAAAAAAAAAA);
    value.words[1] = UINT64_C(0x5555555555555555);
    for (size_t offset = 0U; offset < 64U; ++offset) {
        expect_bit(&value, offset, (int)(offset & 1U));
        expect_bit(&value, 64U + offset, (int)((offset + 1U) & 1U));
    }
    puts("test_intra_word_pattern: PASSED");
}

/** @brief Runs the deterministic public API contract suite. */
int main(void)
{
    puts("--- Starting deterministic bignum_bit_test tests ---");
    test_boundaries();
    test_normalized_prefix();
    test_invalid_arguments();
    test_intra_word_pattern();
    puts("--- All deterministic bignum_bit_test tests passed ---");
    return 0;
}
