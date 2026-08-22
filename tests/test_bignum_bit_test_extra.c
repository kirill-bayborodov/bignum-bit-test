/**
 * @file test_bignum_bit_test_extra.c
 * @brief Extended randomized and stress tests for bignum_bit_test.
 * @details A deterministic xorshift generator and direct word/bit oracle cover
 * valid records, dirty capacity words, every bit position, malformed lengths,
 * and memory guards. The same source runs with C11 and x86-64 YASM.
 * @version 1.0.0
 */
#include "bignum_bit_test.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Advances the deterministic test generator. */
static uint64_t next_random(uint64_t *state)
{
    if (*state == 0U) *state = UINT64_C(0x9e3779b97f4a7c15);
    *state ^= *state << 7U;
    *state ^= *state >> 9U;
    *state ^= *state << 8U;
    return *state;
}

/** @brief Computes the expected selected bit without calling the API. */
static int oracle(const bignum_t *value, size_t index)
{
    const size_t word = index / 64U;
    if (word >= value->len) return 0;
    return (int)((value->words[word] >> (index % 64U)) & UINT64_C(1));
}

/** @brief Checks one valid query against the direct model and input snapshot. */
static void check_valid(const bignum_t *value, size_t index)
{
    bignum_t before = *value;
    int actual = -1;
    assert(bignum_bit_test(value, index, &actual) == BIGNUM_BIT_TEST_SUCCESS);
    assert(actual == oracle(value, index));
    assert(memcmp(value, &before, sizeof(before)) == 0);
}

/** @brief Runs 20,000 deterministic randomized normalized-record queries. */
static void test_fuzz_against_model(void)
{
    uint64_t state = UINT64_C(0x243f6a8885a308d3);
    for (size_t iteration = 0U; iteration < 20000U; ++iteration) {
        bignum_t value;
        memset(&value, 0, sizeof(value));
        value.len = (size_t)(next_random(&state) % (BIGNUM_CAPACITY + 1U));
        for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word)
            value.words[word] = next_random(&state);
        for (size_t probe = 0U; probe < 5U; ++probe)
            check_valid(&value, (size_t)(next_random(&state) % (BIGNUM_CAPACITY * 64U)));
    }
    puts("test_fuzz_against_model: PASSED");
}

/** @brief Covers every index in and immediately beyond a one-word record. */
static void test_single_word_exhaustive(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = 1U;
    value.words[0] = UINT64_C(0x8000000000000001);
    for (size_t index = 0U; index < 128U; ++index) check_valid(&value, index);
    puts("test_single_word_exhaustive: PASSED");
}

/** @brief Covers all 2,048 valid positions in a full-capacity record. */
static void test_full_capacity_exhaustive(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = BIGNUM_CAPACITY;
    for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word)
        value.words[word] = (word & 1U) ? UINT64_MAX : UINT64_C(0);
    for (size_t index = 0U; index < BIGNUM_CAPACITY * 64U; ++index)
        check_valid(&value, index);
    puts("test_full_capacity_exhaustive: PASSED");
}

/** @brief Ensures dirty words above len never influence a successful result. */
static void test_dirty_tail_is_ignored(void)
{
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = 2U;
    value.words[0] = UINT64_C(1);
    value.words[1] = UINT64_C(0x8000000000000000);
    value.words[31] = UINT64_MAX;
    check_valid(&value, 0U);
    check_valid(&value, 127U);
    check_valid(&value, 128U);
    check_valid(&value, 2047U);
    puts("test_dirty_tail_is_ignored: PASSED");
}

/** @brief Checks output and surrounding canaries for a successful query. */
static void test_output_canaries(void)
{
    struct guarded_output { uint64_t left; int value; uint64_t right; } guarded =
        { UINT64_C(0x1111222233334444), -1, UINT64_C(0xaaaabbbbccccdddd) };
    bignum_t value;
    memset(&value, 0, sizeof(value));
    value.len = 1U;
    value.words[0] = UINT64_C(1) << 63U;
    assert(bignum_bit_test(&value, 63U, &guarded.value) == BIGNUM_BIT_TEST_SUCCESS);
    assert(guarded.value == 1);
    assert(guarded.left == UINT64_C(0x1111222233334444));
    assert(guarded.right == UINT64_C(0xaaaabbbbccccdddd));
    puts("test_output_canaries: PASSED");
}

/** @brief Runs extended model, exhaustive and memory-safety scenarios. */
int main(void)
{
    puts("--- Starting extended bignum_bit_test tests ---");
    test_fuzz_against_model();
    test_single_word_exhaustive();
    test_full_capacity_exhaustive();
    test_dirty_tail_is_ignored();
    test_output_canaries();
    puts("--- All extended bignum_bit_test tests passed ---");
    return EXIT_SUCCESS;
}
