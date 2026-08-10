#include <setjmp.h>
#include <string.h>
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include "mem_alloc.h"
#include "errors.h"

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
    TEST_ASSERT_EQUAL_INT(mapping.nb_chunks, nb_chunks);
    TEST_ASSERT_EQUAL_INT(mapping.nb_allocated, nb_allocs);
    TEST_ASSERT_EQUAL_INT(mapping.total_size, malloc_buffer_size);
}

void test_alloc(void)
{
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    TEST_ASSERT_NOT_NULL(chunk1);
    assert_state(ctx, 2, 1);
    char *chunk2 = mem_alloc(ctx, 120);
    TEST_ASSERT_NOT_NULL(chunk2);
    assert_state(ctx, 3, 2);

    mem_free(ctx, chunk1);
    // expect 3 chunks and 1 allocated
    assert_state(ctx, 3, 1);

    chunk1 = mem_alloc(ctx, 12);
    TEST_ASSERT_NOT_NULL(chunk1);
    // expect 3 chunks and 2 allocated
    assert_state(ctx, 3, 2);

    mem_free(ctx, chunk2);
    // expect 2 chunks and 1 allocated (because coalesce)
    assert_state(ctx, 2, 1);

    mem_free(ctx, chunk1);
    assert_state(ctx, 1, 0);

    // ask too much and expect NULL
    chunk1 = mem_alloc(ctx, (4096 + 96) * 4);
    TEST_ASSERT_NULL(chunk1);
}

void test_re_alloc(void)
{
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // Assert only one big empty chunk
    assert_state(ctx, 1, 0);

    // Allocate a chunk
    char *chunk1 = mem_alloc(ctx, 64);
    TEST_ASSERT_NOT_NULL(chunk1);
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
    TEST_ASSERT_NOT_NULL(chunk2);
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
    TEST_ASSERT_NOT_NULL(chunk3);
    TEST_ASSERT_EQUAL_PTR(chunk3, chunk2);
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
    TEST_ASSERT_NOT_NULL(chunk4);
    TEST_ASSERT_EQUAL_PTR(chunk4, chunk3);
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
    TEST_ASSERT_NULL(chunk5);
    /**
     * Expect 2 chunks :
     * - original chunk from init,
     * - free leftover coalesced.
     * Expect 0 allocated.
     */
    assert_state(ctx, 2, 0);

    // Reallocate NULL pointer to allocate
    char *chunk6 = mem_realloc(ctx, NULL, 256);
    TEST_ASSERT_NOT_NULL(chunk6);
    /**
     * Expect 3 chunks:
     * - original from init,
     * - free leftover coalesced (from previous realloc),
     * - allocated
     * Expect 1 allocated :
     * - the allocated chunk
     */
    assert_state(ctx, 3, 1);

    // Reallocate with an overflowing size should return NULL and not change state
    char *chunk7 = mem_realloc(ctx, chunk6, (size_t) -1);
    TEST_ASSERT_NULL(chunk7);
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

void test_corrupt_invalid(void)
{
    uint32_t throw_raised_code = 0;

    memset(buffer_jmp_testing, 0, sizeof(jmp_buf));
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    TEST_ASSERT_NOT_NULL(chunk1);
    char *chunk2 = mem_alloc(ctx, 120);
    TEST_ASSERT_NOT_NULL(chunk2);
    assert_state(ctx, 3, 2);

    // overflow of first chunk to destroy second chunk header with invalid size
    memset(&chunk1[12], 0xAA, 20);
    if (setjmp(buffer_jmp_testing) == 0) {
        mem_free(ctx, chunk2);
    }
    else {
        throw_raised_code = get_throw_value();
    }

    TEST_ASSERT_EQUAL_INT(throw_raised_code, EXCEPTION_CORRUPT);
}

void test_corrupt_overflow(void)
{
    uint32_t throw_raised_code = 0;

    memset(buffer_jmp_testing, 0, sizeof(jmp_buf));
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    // assert only one big empty chunk
    assert_state(ctx, 1, 0);

    char *chunk1 = mem_alloc(ctx, 12);
    TEST_ASSERT_NOT_NULL(chunk1);
    char *chunk2 = mem_alloc(ctx, 120);
    TEST_ASSERT_NOT_NULL(chunk2);
    assert_state(ctx, 3, 2);

    // overflow of first chunk to destroy next chunk and try to free it
    memset(&chunk1[10], 0x88, 20);
    if (setjmp(buffer_jmp_testing) == 0) {
        mem_free(ctx, chunk2);
    }
    else {
        throw_raised_code = get_throw_value();
    }

    TEST_ASSERT_EQUAL_INT(throw_raised_code, EXCEPTION_CORRUPT);
}

void test_fragmentation(void)
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
        TEST_ASSERT_EQUAL_INT(mapping.free_size, malloc_buffer_size - 96 - (16 + 64 + 128) * i);
        small_chunks[i] = mem_alloc(ctx, 12);
        TEST_ASSERT_NOT_NULL(small_chunks[i]);
        middle_chunks[i] = mem_alloc(ctx, 60);
        TEST_ASSERT_NOT_NULL(middle_chunks[i]);
        large_chunks[i] = mem_alloc(ctx, 124);
        TEST_ASSERT_NOT_NULL(large_chunks[i]);
    }
    assert_state(ctx, 24, 24);
    // release all small chunks and try to allocate a middle chunk
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, small_chunks[i]);
    }
    assert_state(ctx, 24, 16);
    middle_chunks[8] = mem_alloc(ctx, 60);
    TEST_ASSERT_NULL(middle_chunks[8]);
    // release all middle chunks and try to allocate a large chunk
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, middle_chunks[i]);
    }
    assert_state(ctx, 16, 8);
    large_chunks[8] = mem_alloc(ctx, 124);
    TEST_ASSERT_NULL(large_chunks[8]);
    middle_chunks[0] = mem_alloc(ctx, 60);
    TEST_ASSERT_NOT_NULL(middle_chunks[0]);
    middle_chunks[1] = mem_alloc(ctx, 60);
    TEST_ASSERT_NOT_NULL(middle_chunks[1]);
    assert_state(ctx, 18, 10);

    // release all large chunks
    for (int i = 0; i < 8; i++) {
        mem_free(ctx, large_chunks[i]);
    }
    // assert 5 chunks and 2 allocated
    assert_state(ctx, 5, 2);
}

void test_init(void)
{
    mem_ctx_t ctx;

    // try with too big
    malloc_buffer_size = 0x8000 + 96;  // 96 is size of heap header
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    TEST_ASSERT_NULL(ctx);

    // try with biggest possible
    malloc_buffer_size = 0x7FF8 + 96;  // 96 is size of heap header
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    TEST_ASSERT_NOT_NULL(ctx);

    // try with too small
    malloc_buffer_size = 192;
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    TEST_ASSERT_NULL(ctx);

    // try with non multiple of 8
    malloc_buffer_size = 201;
    ctx                = mem_init(malloc_buffer, malloc_buffer_size);
    TEST_ASSERT_NULL(ctx);
}

void test_alloc_oversized_request(void)
{
    malloc_buffer_size = sizeof(malloc_buffer);
    mem_ctx_t ctx      = mem_init(malloc_buffer, malloc_buffer_size);

    TEST_ASSERT_NOT_NULL(ctx);

    // Requests larger than the maximum representable chunk must fail
    // instead of wrapping into a tiny allocation.
    TEST_ASSERT_NULL(mem_alloc(ctx, (size_t) -1));
    assert_state(ctx, 1, 0);

    // The allocator should remain usable after rejecting the request.
    void *ptr = mem_alloc(ctx, 12);
    TEST_ASSERT_NOT_NULL(ptr);
    assert_state(ctx, 2, 1);
}

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_alloc);
    RUN_TEST(test_re_alloc);
    RUN_TEST(test_corrupt_invalid);
    RUN_TEST(test_corrupt_overflow);
    RUN_TEST(test_fragmentation);
    RUN_TEST(test_init);
    RUN_TEST(test_alloc_oversized_request);
    return UNITY_END();
}
