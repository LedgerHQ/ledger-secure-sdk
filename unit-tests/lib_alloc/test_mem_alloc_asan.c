/* Checks the AddressSanitizer poison state that lib_alloc maintains on its heap. */

#include <setjmp.h>
#include <string.h>
#include "unity.h"
#include "mem_alloc.h"

#if defined(__SANITIZE_ADDRESS__)
#define TEST_ASAN
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define TEST_ASAN
#endif
#endif

// size/phys_prev word in front of every payload
#define CHUNK_HEADER_SIZE 4
// size of heap header
#define HEAP_HEADER_SIZE  96

// poisoned gap that mem_alloc() reserves after every allocation
#if defined(FUZZING)
#define REDZONE_SIZE 8
#else
#define REDZONE_SIZE 0
#endif

void __wrap_os_longjmp(unsigned int error_code)
{
    TEST_FAIL_MESSAGE("unexpected allocator exception");
    (void) error_code;
}

#ifdef TEST_ASAN
#include <sanitizer/asan_interface.h>

static uint64_t heap_buffer[2048];

typedef struct {
    const uint8_t *block;
    size_t         size;
    bool           allocated;
} chunk_t;

static bool find_chunk_cb(void *data, uint8_t *addr, bool allocated, size_t size)
{
    chunk_t *chunk = (chunk_t *) data;
    if (chunk->block == NULL || addr == chunk->block) {
        chunk->block     = addr;
        chunk->size      = size;
        chunk->allocated = allocated;
        return true;
    }
    return false;
}

// chunk starting at block, or the first chunk of the heap when block is NULL
static chunk_t get_chunk(mem_ctx_t ctx, const uint8_t *block)
{
    chunk_t chunk = {.block = block};
    mem_parse(ctx, find_chunk_cb, &chunk);
    TEST_ASSERT_NOT_EQUAL_UINT(0, chunk.size);
    return chunk;
}

static chunk_t chunk_of(mem_ctx_t ctx, const void *payload)
{
    return get_chunk(ctx, ((const uint8_t *) payload) - CHUNK_HEADER_SIZE);
}

static void assert_addressable(const void *addr, size_t size)
{
    TEST_ASSERT_NULL(__asan_region_is_poisoned((void *) addr, size));
}

static void assert_poisoned(const void *addr, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        TEST_ASSERT_TRUE(__asan_address_is_poisoned(((const uint8_t *) addr) + i));
    }
}

// the caller's bytes are addressable, the rest of the chunk is poisoned
static void assert_allocated(mem_ctx_t ctx, const uint8_t *ptr, size_t size)
{
    chunk_t chunk = chunk_of(ctx, ptr);
    TEST_ASSERT_TRUE(chunk.allocated);
    size_t slack = chunk.size - CHUNK_HEADER_SIZE - size;
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(REDZONE_SIZE, slack);
    assert_addressable(chunk.block, CHUNK_HEADER_SIZE + size);
    assert_poisoned(ptr + size, slack);
}

// only the header word of a free chunk is addressable
static void assert_free_chunk(mem_ctx_t ctx, const uint8_t *block)
{
    chunk_t chunk = get_chunk(ctx, block);
    TEST_ASSERT_FALSE(chunk.allocated);
    assert_addressable(chunk.block, CHUNK_HEADER_SIZE);
    assert_poisoned(chunk.block + CHUNK_HEADER_SIZE, chunk.size - CHUNK_HEADER_SIZE);
}

void test_init_poisons_heap(void)
{
    mem_ctx_t ctx = mem_init(heap_buffer, sizeof(heap_buffer));
    TEST_ASSERT_NOT_NULL(ctx);

    assert_addressable(heap_buffer, HEAP_HEADER_SIZE);
    chunk_t chunk = get_chunk(ctx, NULL);
    TEST_ASSERT_EQUAL_size_t(sizeof(heap_buffer) - HEAP_HEADER_SIZE, chunk.size);
    assert_free_chunk(ctx, chunk.block);
}

void test_alloc_boundaries(void)
{
    mem_ctx_t ctx = mem_init(heap_buffer, sizeof(heap_buffer));
    TEST_ASSERT_NOT_NULL(ctx);

    // exact-fit (12, 20, ...) and unaligned sizes, next to a live chunk
    for (size_t size = 1; size <= 200; size++) {
        uint8_t *ptr   = mem_alloc(ctx, size);
        uint8_t *guard = mem_alloc(ctx, 8);
        TEST_ASSERT_NOT_NULL(ptr);
        TEST_ASSERT_NOT_NULL(guard);
        assert_allocated(ctx, ptr, size);
        if (REDZONE_SIZE > 0) {
            TEST_ASSERT_TRUE(__asan_address_is_poisoned(ptr + size));
        }
        mem_free(ctx, ptr);
        mem_free(ctx, guard);
    }
}

void test_free_poisons_payload(void)
{
    mem_ctx_t ctx = mem_init(heap_buffer, sizeof(heap_buffer));
    TEST_ASSERT_NOT_NULL(ctx);

    uint8_t *first  = mem_alloc(ctx, 4);
    uint8_t *second = mem_alloc(ctx, 4);
    uint8_t *guard  = mem_alloc(ctx, 4);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_NULL(guard);

    // live neighbours on both sides: no coalescing, the free-list links must be poisoned too
    mem_free(ctx, second);
    assert_free_chunk(ctx, second - CHUNK_HEADER_SIZE);
    TEST_ASSERT_TRUE(__asan_address_is_poisoned(second));

    // coalesced with the previous chunk: the absorbed header is poisoned as well
    mem_free(ctx, first);
    assert_free_chunk(ctx, first - CHUNK_HEADER_SIZE);
    TEST_ASSERT_TRUE(__asan_address_is_poisoned(second - CHUNK_HEADER_SIZE));

    // coalesced on both sides, up to the end of the heap
    mem_free(ctx, guard);
    chunk_t chunk = get_chunk(ctx, NULL);
    TEST_ASSERT_EQUAL_size_t(sizeof(heap_buffer) - HEAP_HEADER_SIZE, chunk.size);
    assert_free_chunk(ctx, chunk.block);

    // a reused chunk is addressable again
    uint8_t *again = mem_alloc(ctx, 4);
    TEST_ASSERT_EQUAL_PTR(first, again);
    assert_allocated(ctx, again, 4);
}

void test_realloc_transitions(void)
{
    mem_ctx_t ctx = mem_init(heap_buffer, sizeof(heap_buffer));
    TEST_ASSERT_NOT_NULL(ctx);

    uint8_t *ptr   = mem_alloc(ctx, 40);
    uint8_t *guard = mem_alloc(ctx, 8);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_NOT_NULL(guard);
    memset(ptr, 0x5A, 40);

    // shrink in place: the split remainder becomes a poisoned free chunk
    uint8_t *shrunk = mem_realloc(ctx, ptr, 10);
    TEST_ASSERT_EQUAL_PTR(ptr, shrunk);
    assert_allocated(ctx, shrunk, 10);
    chunk_t chunk = chunk_of(ctx, shrunk);
    assert_free_chunk(ctx, chunk.block + chunk.size);

    // grow: moved, content kept, old payload poisoned
    uint8_t *grown = mem_realloc(ctx, shrunk, 200);
    TEST_ASSERT_NOT_NULL(grown);
    TEST_ASSERT_NOT_EQUAL(shrunk, grown);
    TEST_ASSERT_EACH_EQUAL_UINT8(0x5A, grown, 10);
    assert_allocated(ctx, grown, 200);
    TEST_ASSERT_TRUE(__asan_address_is_poisoned(shrunk));

    mem_free(ctx, grown);
    mem_free(ctx, guard);

    // every new size, in place or moved, keeps the content and the poisoned tail
    for (size_t size = 1; size <= 64; size++) {
        uint8_t *buf = mem_alloc(ctx, 16);
        TEST_ASSERT_NOT_NULL(buf);
        memset(buf, 0xA5, 16);
        uint8_t *res = mem_realloc(ctx, buf, size);
        TEST_ASSERT_NOT_NULL(res);
        TEST_ASSERT_EACH_EQUAL_UINT8(0xA5, res, size < 16 ? size : 16);
        assert_allocated(ctx, res, size);
        mem_free(ctx, res);
    }
}
#endif  // TEST_ASAN

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
#ifdef TEST_ASAN
    RUN_TEST(test_init_poisons_heap);
    RUN_TEST(test_alloc_boundaries);
    RUN_TEST(test_free_poisons_payload);
    RUN_TEST(test_realloc_transitions);
#endif
    return UNITY_END();
}
