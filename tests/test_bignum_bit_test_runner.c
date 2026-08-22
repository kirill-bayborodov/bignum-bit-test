/**
 * @file test_bignum_bit_test_runner.c
 * @brief Static-library integration smoke test for bignum_bit_test.
 * @details The runner includes only the public module header, links the
 * packaged library, queries a known zero bit and verifies both status and value.
 * @version 1.0.0
 */
#include "bignum_bit_test.h"
#include <assert.h>
#include <stdio.h>

/** @brief Verifies public-header-only linkage and one known result. */
int main(void)
{
    bignum_t value = {0};
    int bit = -1;
    printf("Running test: test_bignum_bit_test_runner... ");
    assert(bignum_bit_test(&value, 5U, &bit) == BIGNUM_BIT_TEST_SUCCESS);
    assert(bit == 0);
    printf("PASSED\n");
    return 0;
}
