#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include "mem_alloc.h"
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

int main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {cmocka_unit_test(test_alloc),
                                       cmocka_unit_test(test_corrupt_invalid),
                                       cmocka_unit_test(test_corrupt_overflow),
                                       cmocka_unit_test(test_fragmentation),
                                       cmocka_unit_test(test_init)};
    return cmocka_run_group_tests(tests, NULL, NULL);
}
