/**
 * @file test_bignum_bit_test_mt.c
 * @brief Thread-safety tests for bignum_bit_test.
 * @details Eight workers, each executing 10,000 iterations, repeatedly query
 * independent immutable records. Every result is compared with a local
 * word/bit oracle and the input snapshot is checked after each call. The test
 * intentionally does not share mutable bignum state, proving the documented
 * independent-object scope rather than permitting concurrent writers.
 * @version 1.0.0
 */
#include "bignum_bit_test.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NUM_THREADS 8U
#define NUM_ITERATIONS 10000U

/**
 * @brief Holds one worker's independent test object and failure flag.
 * @details The main thread initializes one instance per worker and never shares
 * an instance between workers. The worker writes only its own flag; the test
 * joins all workers before reading the flags.
 */
typedef struct thread_data {
    bignum_t value; /**< [in] Immutable worker-owned operand snapshot. */
    int failed; /**< [out] Worker-owned failure indicator, zero until mismatch. */
} thread_data_t;

/**
 * @brief Computes the expected bit for a valid record.
 * @param[in] value Borrowed immutable worker record.
 * @param[in] index Valid zero-based bit position.
 * @return Independent word/mask oracle value, exactly 0 or 1.
 */
static int oracle(const bignum_t *value, size_t index)
{
    size_t word = index / 64U;
    if (word >= value->len) return 0;
    return (int)((value->words[word] >> (index % 64U)) & UINT64_C(1));
}

/** @brief Repeatedly queries one independent record in a worker thread. */
static void *worker_thread(void *opaque)
{
    thread_data_t *data = opaque;
    bignum_t before = data->value;
    for (size_t iteration = 0U; iteration < NUM_ITERATIONS; ++iteration) {
        size_t index = (iteration * 131U + 17U) % (BIGNUM_CAPACITY * 64U);
        int actual = -1;
        if (bignum_bit_test(&data->value, index, &actual) != BIGNUM_BIT_TEST_SUCCESS ||
            actual != oracle(&data->value, index) ||
            memcmp(&data->value, &before, sizeof(before)) != 0) {
            data->failed = 1;
            return NULL;
        }
    }
    return NULL;
}

/**
 * @brief Runs independent worker threads and checks all results and ownership.
 * @details Creates eight workers, joins every worker, and fails if any oracle
 * result or byte-for-byte input snapshot differs. The join is the publication
 * boundary for each worker's failure flag.
 */
int main(void)
{
    pthread_t threads[NUM_THREADS];
    thread_data_t cases[NUM_THREADS];
    puts("--- Starting MT bignum_bit_test tests ---");
    for (size_t i = 0U; i < NUM_THREADS; ++i) {
        memset(&cases[i], 0, sizeof(cases[i]));
        cases[i].value.len = BIGNUM_CAPACITY;
        for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word)
            cases[i].value.words[word] = UINT64_C(0x0101010101010101) * (i + word + 1U);
        assert(pthread_create(&threads[i], NULL, worker_thread, &cases[i]) == 0);
    }
    for (size_t i = 0U; i < NUM_THREADS; ++i) assert(pthread_join(threads[i], NULL) == 0);
    for (size_t i = 0U; i < NUM_THREADS; ++i) assert(cases[i].failed == 0);
    puts("--- MT bignum_bit_test passed ---");
    return 0;
}
