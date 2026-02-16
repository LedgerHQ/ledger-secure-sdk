#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include "mem_alloc.h"
#include "app_mem_utils.h"
#include "errors.h"

#define UNUSED(x) (void) x

static uint32_t malloc_buffer[4096 + 96];
static uint32_t malloc_buffer_size;
bool            display_parse = false;

jmp_buf buffer_jmp_testing;

static unsigned int throw_value;

// Wrapper function for throw
void __wrap_os_longjmp(unsigned int error_code)
{
    throw_value = error_code;
    longjmp(buffer_jmp_testing, 1);
}

unsigned int get_throw_value(void)
{
    return throw_value;
}

static void assert_state(mem_ctx_t ctx, uint32_t nb_chunks, uint32_t nb_allocs)
{
    mem_stat_t mapping;
    mem_stat(ctx, &mapping);
    assert_int_equal(mapping.nb_chunks, nb_chunks);
    assert_int_equal(mapping.nb_allocated, nb_allocs);
    assert_int_equal(mapping.total_size, malloc_buffer_size);
}

static void test_alloc(void **state __attribute__((unused)))
{
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    assert_non_null(chunk1);
    assert_state(ctx, 2, 1);
    char *chunk2 = mem_alloc(ctx, 120);
    assert_non_null(chunk2);
    assert_state(ctx, 3, 2);

    mem_free(ctx, chunk1);
    // expect 3 chunks and 1 allocated
    assert_state(ctx, 3, 1);

    chunk1 = mem_alloc(ctx, 12);
    assert_non_null(chunk1);
    // expect 3 chunks and 2 allocated
    assert_state(ctx, 3, 2);

    mem_free(ctx, chunk2);
    // expect 2 chunks and 1 allocated (because coalesce)
    assert_state(ctx, 2, 1);

    mem_free(ctx, chunk1);
    assert_state(ctx, 1, 0);

    // ask too much and expect NULL
    chunk1 = mem_alloc(ctx, (4096 + 96) * 4);
    assert_null(chunk1);
}

static void test_re_alloc(void **state __attribute__((unused)))
{
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // Assert only one big empty chunk
    assert_state(ctx, 1, 0);

    // Allocate a chunk
    char *chunk1 = mem_alloc(ctx, 64);
    assert_non_null(chunk1);
    /**
     * Expect 2 chunks:
     * - original from init,
     * - allocated
     * Expect 1 allocated :
     * - the allocated chunk
     */
    assert_state(ctx, 2, 1);

    // Reallocate to a bigger size
    char *chunk2 = mem_realloc(ctx, chunk1, 128);
    assert_non_null(chunk2);
    /**
     * Expect 3 chunks :
     * - original from init,
     * - allocated,
     * - free leftover.
     * Expect 1 allocated :
     * - the allocated chunk
     */
    assert_state(ctx, 3, 1);

    // Reallocate to a smaller size but not enough to split
    char *chunk3 = mem_realloc(ctx, chunk2, 127);
    assert_non_null(chunk3);
    assert_ptr_equal(chunk3, chunk2);
    /**
     * Expect 3 chunks :
     * - original chunk from init,
     * - reallocated,
     * - free leftover (from previous realloc).
     * Expect 1 allocated :
     * - the reallocated chunk
     */
    assert_state(ctx, 3, 1);

    // Reallocate to a smaller size
    char *chunk4 = mem_realloc(ctx, chunk3, 32);
    assert_non_null(chunk4);
    assert_ptr_equal(chunk4, chunk3);
    /**
     * Expect 4 chunks :
     * - original chunk from init,
     * - reallocated,
     * - split free leftover (from shrink optimization),
     * - free leftover (from previous realloc).
     * Expect 1 allocated :
     * - the reallocated chunk
     *
     * Free function not called so no coalescing done.
     */
    assert_state(ctx, 4, 1);

    // Reallocate with size 0 to free
    char *chunk5 = mem_realloc(ctx, chunk4, 0);
    assert_null(chunk5);
    /**
     * Expect 2 chunks :
     * - original chunk from init,
     * - free leftover coalesced.
     * Expect 0 allocated.
     */
    assert_state(ctx, 2, 0);

    // Reallocate NULL pointer to allocate
    char *chunk6 = mem_realloc(ctx, NULL, 256);
    assert_non_null(chunk6);
    /**
     * Expect 3 chunks:
     * - original from init,
     * - free leftover coalesced (from previous realloc),
     * - allocated
     * Expect 1 allocated :
     * - the allocated chunk
     */
    assert_state(ctx, 3, 1);

    // Reallocate with too big size should return NULL and not change state
    char *chunk7 = mem_realloc(ctx, chunk6, malloc_buffer_size * 2);
    assert_null(chunk7);
    /**
     * Expect 3 chunks:
     * - original from init,
     * - free leftover coalesced (from previous realloc),
     * - allocated
     * Expect 1 allocated :
     * - the allocated chunk (from previous realloc)
     */
    assert_state(ctx, 3, 1);
}

static void test_corrupt_invalid(void **state __attribute__((unused)))
{
    uint32_t throw_raised_code = 0;

    memset(buffer_jmp_testing, 0, sizeof(jmp_buf));
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    assert_non_null(chunk1);
    char *chunk2 = mem_alloc(ctx, 120);
    assert_non_null(chunk2);
    assert_state(ctx, 3, 2);

    // overflow of first chunk to destroy second chunk header with invalid size
    memset(&chunk1[12], 0xAA, 20);
    if (setjmp(buffer_jmp_testing) == 0) {
        mem_free(ctx, chunk2);
    }
    else {
        throw_raised_code = get_throw_value();
    }

    assert_int_equal(throw_raised_code, EXCEPTION_CORRUPT);
}

static void test_corrupt_overflow(void **state __attribute__((unused)))
{
    uint32_t throw_raised_code = 0;

    memset(buffer_jmp_testing, 0, sizeof(jmp_buf));
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    assert_non_null(chunk1);
    char *chunk2 = mem_alloc(ctx, 120);
    assert_non_null(chunk2);
    assert_state(ctx, 3, 2);

    // overflow of first chunk to destroy next chunk and try to free it
    memset(&chunk1[10], 0x88, 20);
    if (setjmp(buffer_jmp_testing) == 0) {
        mem_free(ctx, chunk2);
    }
    else {
        throw_raised_code = get_throw_value();
    }

    assert_int_equal(throw_raised_code, EXCEPTION_CORRUPT);
}

static void test_fragmentation(void **state __attribute__((unused)))
{
    char *small_chunks[16];
    char *middle_chunks[16];
    char *large_chunks[16];
    malloc_buffer_size = (16 + 64 + 128) * 8 + 96;  // 96 is size of heap header
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    // allocate chunks to fill entirely the heap
    for (int i = 0; i < 8; i++) {
        mem_stat_t mapping;
        mem_stat(ctx, &mapping);
        assert_int_equal(mapping.free_size, malloc_buffer_size - 96 - (16 + 64 + 128) * i);
        small_chunks[i] = mem_alloc(ctx, 12);
        assert_non_null(small_chunks[i]);
        middle_chunks[i] = mem_alloc(ctx, 60);
        assert_non_null(middle_chunks[i]);
        large_chunks[i] = mem_alloc(ctx, 124);
        assert_non_null(large_chunks[i]);
    }
    assert_state(ctx, 24, 24);
    // release all small chunks and try to allocate a middle chunk
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, small_chunks[i]);
    }
    assert_state(ctx, 24, 16);
    middle_chunks[8] = mem_alloc(ctx, 60);
    assert_null(middle_chunks[8]);
    // release all middle chunks and try to allocate a large chunk
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, middle_chunks[i]);
    }
    assert_state(ctx, 16, 8);
    large_chunks[8] = mem_alloc(ctx, 124);
    assert_null(large_chunks[8]);
    middle_chunks[0] = mem_alloc(ctx, 60);
    assert_non_null(middle_chunks[0]);
    middle_chunks[1] = mem_alloc(ctx, 60);
    assert_non_null(middle_chunks[1]);
    assert_state(ctx, 18, 10);

    // release all large chunks
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, large_chunks[i]);
    }
    // assert 5 chunks and 2 allocated
    assert_state(ctx, 5, 2);
}

static void test_init(void **state __attribute__((unused)))
{
    mem_ctx_t ctx;

    // try with too big
    malloc_buffer_size = 0x8000 + 96;  // 96 is size of heap header
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    assert_null(ctx);

    // try with biggest possible
    malloc_buffer_size = 0x7FF8 + 96;  // 96 is size of heap header
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    assert_non_null(ctx);

    // try with too small
    malloc_buffer_size = 192;
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    assert_null(ctx);

    // try with non multiple of 8
    malloc_buffer_size = 201;
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    assert_null(ctx);
}

// New tests using app_mem_utils wrapper

static void test_utils_init(void **state __attribute__((unused)))
{
    // Test successful initialization
    bool result = mem_utils_init(malloc_buffer, sizeof(malloc_buffer));
    assert_true(result);

    // Test NULL pointer
    result = mem_utils_init(NULL, sizeof(malloc_buffer));
    assert_false(result);
}

static void test_utils_basic_alloc(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *ptr1 = APP_MEM_ALLOC(64);
    assert_non_null(ptr1);

    void *ptr2 = APP_MEM_ALLOC(128);
    assert_non_null(ptr2);

    void *ptr3 = APP_MEM_ALLOC(256);
    assert_non_null(ptr3);

    // Free in different order
    APP_MEM_FREE(ptr2);
    APP_MEM_FREE(ptr1);
    APP_MEM_FREE(ptr3);
}

static void test_utils_zero_size(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *ptr = APP_MEM_ALLOC(0);
    assert_null(ptr);
}

static void test_utils_buffer_allocate(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *buffer = NULL;
    bool  result = APP_MEM_CALLOC(&buffer, 100);
    assert_true(result);
    assert_non_null(buffer);

    // Check zero-initialization
    uint8_t *byte_buffer = (uint8_t *) buffer;
    for (size_t i = 0; i < 100; i++) {
        assert_int_equal(byte_buffer[i], 0);
    }

    APP_MEM_FREE_AND_NULL(&buffer);
    assert_null(buffer);
}

static void test_utils_buffer_calloc(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *buffer = NULL;
    bool  result = APP_MEM_CALLOC(&buffer, 64);
    assert_true(result);
    assert_non_null(buffer);

    APP_MEM_FREE_AND_NULL(&buffer);
    assert_null(buffer);
}

static void test_utils_buffer_realloc(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    // Realloc to invalid pointer should return NULL
    void *result_invalid = APP_MEM_REALLOC((void *) 0x12345678, 100);
    assert_null(result_invalid);

    // Realloc NULL pointer should behave like alloc
    void *result1 = APP_MEM_REALLOC(NULL, 50);
    assert_non_null(result1);
    // Write some data
    memset(result1, 0xAB, 50);

    // Realloc to larger size
    void *result2 = APP_MEM_REALLOC(result1, 150);
    assert_non_null(result2);
    assert_non_null(result1);
    // Check previous data is intact
    uint8_t *byte_buffer = (uint8_t *) result2;
    for (size_t i = 0; i < 50; i++) {
        assert_int_equal(byte_buffer[i], 0xAB);
    }

    // Realloc to really tiny size (should shrink
    // and trigger alignment logic)
    void *result3 = APP_MEM_REALLOC(result2, 1);
    assert_non_null(result3);
    assert_non_null(result2);
    // Check previous data is intact
    byte_buffer = (uint8_t *) result3;
    assert_int_equal(byte_buffer[0], 0xAB);

    // Realloc to zero size should free the buffer
    void *result4 = APP_MEM_REALLOC(result3, 0);
    assert_null(result4);
}

static void test_utils_buffer_zero_size(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *buffer = NULL;
    bool  result = APP_MEM_CALLOC(&buffer, 0);
    assert_true(result);
    assert_null(buffer);  // Allocating zero size does not allocate anything
}

static void test_utils_strdup(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    const char *original  = "Hello, World!";
    const char *duplicate = APP_MEM_STRDUP(original);
    assert_non_null(duplicate);
    assert_string_equal(duplicate, original);
    assert_ptr_not_equal(duplicate, original);

    APP_MEM_FREE((void *) duplicate);
}

static void test_utils_large_alloc(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *large = APP_MEM_ALLOC(sizeof(malloc_buffer) / 2);
    assert_non_null(large);

    APP_MEM_FREE(large);
}

static void test_utils_out_of_memory(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    void *blocks[100];
    int   count = 0;

    for (int i = 0; i < 100; i++) {
        blocks[i] = APP_MEM_ALLOC(512);
        if (blocks[i] == NULL) {
            break;
        }
        count++;
    }

    assert_true(count > 0);
    assert_true(count < 100);

    void *should_fail = APP_MEM_ALLOC(512);
    assert_null(should_fail);

    for (int i = 0; i < count; i++) {
        APP_MEM_FREE(blocks[i]);
    }
}

static void test_utils_stress(void **state __attribute__((unused)))
{
    mem_utils_init(malloc_buffer, sizeof(malloc_buffer));

    for (int iteration = 0; iteration < 50; iteration++) {
        void *ptrs[20];

        for (int i = 0; i < 20; i++) {
            size_t size = 16 + (i * 32);
            ptrs[i]     = APP_MEM_ALLOC(size);
            if (ptrs[i] != NULL) {
                memset(ptrs[i], i, size);
            }
        }

        for (int i = 19; i >= 0; i -= 2) {
            if (ptrs[i] != NULL) {
                APP_MEM_FREE(ptrs[i]);
            }
        }
        for (int i = 0; i < 20; i += 2) {
            if (ptrs[i] != NULL) {
                APP_MEM_FREE(ptrs[i]);
            }
        }
    }
}

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {// Original mem_alloc tests
                                       cmocka_unit_test(test_alloc),
                                       cmocka_unit_test(test_re_alloc),
                                       cmocka_unit_test(test_corrupt_invalid),
                                       cmocka_unit_test(test_corrupt_overflow),
                                       cmocka_unit_test(test_fragmentation),
                                       cmocka_unit_test(test_init),
                                       // New app_mem_utils tests
                                       cmocka_unit_test(test_utils_init),
                                       cmocka_unit_test(test_utils_basic_alloc),
                                       cmocka_unit_test(test_utils_zero_size),
                                       cmocka_unit_test(test_utils_buffer_allocate),
                                       cmocka_unit_test(test_utils_buffer_calloc),
                                       cmocka_unit_test(test_utils_buffer_realloc),
                                       cmocka_unit_test(test_utils_buffer_zero_size),
                                       cmocka_unit_test(test_utils_strdup),
                                       cmocka_unit_test(test_utils_large_alloc),
                                       cmocka_unit_test(test_utils_out_of_memory),
                                       cmocka_unit_test(test_utils_stress)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
