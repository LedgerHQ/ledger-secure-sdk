#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "mem_alloc.h"

#define HEAP_SIZE 0x7FF0
#define MAX_ALLOC 0xffff

// #define DEBUG_CRASH

static uint8_t heap[HEAP_SIZE] = {0};

struct alloc {
    void  *ptr;
    size_t len;
};
static struct alloc allocs[MAX_ALLOC] = {0};

bool insert_alloc(void *ptr, size_t len)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr != NULL) {
            continue;
        }
        allocs[i].ptr = ptr;
        allocs[i].len = len;
        return true;
    }
    return false;
}

bool remove_alloc(void *ptr)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr != ptr) {
            continue;
        }
        allocs[i].ptr = NULL;
        allocs[i].len = 0;
        return true;
    }
    return false;
}

bool get_alloc(size_t index, struct alloc **alloc)
{
    if (index >= MAX_ALLOC) {
        return false;
    }
    if (allocs[index].ptr == NULL) {
        return false;
    }
    *alloc = &allocs[index];
    return true;
}

void exec_alloc(mem_ctx_t *allocator, size_t len)
{
    void *ptr = mem_alloc(allocator, len);
    if (ptr == NULL) {
        return;
    }
#ifdef DEBUG_CRASH
    printf("[ALLOC] ptr = %p | length = %lu\n", ptr, len);
#endif
    for (size_t i = 0; i < len; i++) {
        assert(((uint8_t *) ptr)[i] == 0);
    }
    memset(ptr, len & 0xff, len);
    assert(insert_alloc(ptr, len) == true);
}

void exec_realloc(mem_ctx_t *allocator, size_t index, size_t len)
{
    struct alloc *alloc = NULL;
    if (!get_alloc(index, &alloc)) {
        return;
    }
#ifdef DEBUG_CRASH
    printf(
        "[REALLOC_INIT] ptr = %p | length = %lu | new_length = %lu\n", alloc->ptr, alloc->len, len);
#endif
    for (size_t i = 0; i < alloc->len; i++) {
        assert(((uint8_t *) alloc->ptr)[i] == (alloc->len & 0xff));
    }
    memset(alloc->ptr, 0, alloc->len);

    void *ptr = mem_realloc(allocator, alloc->ptr, len);
#ifdef DEBUG_CRASH
    printf("[REALLOC_FINI] ptr = %p | length = %lu\n", ptr, len);
#endif
    if (ptr == NULL) {
        assert(remove_alloc(alloc->ptr) == true);
        return;
    }

    assert(remove_alloc(alloc->ptr) == true);

    for (size_t i = 0; i < len; i++) {
        assert(((uint8_t *) ptr)[i] == 0);
    }

    memset(ptr, len & 0xff, len);
    assert(insert_alloc(ptr, len) == true);
}

void exec_free(mem_ctx_t *allocator, size_t index)
{
    struct alloc *alloc = NULL;
    if (!get_alloc(index, &alloc)) {
        return;
    }
#ifdef DEBUG_CRASH
    printf("[FREE] ptr = %p | length = %lu\n", alloc->ptr, alloc->len);
#endif
    for (size_t i = 0; i < alloc->len; i++) {
        assert(((uint8_t *) alloc->ptr)[i] == (alloc->len & 0xff));
    }
    memset(alloc->ptr, 0, alloc->len);
    mem_free(allocator, alloc->ptr);
    assert(remove_alloc(alloc->ptr) == true);
}

void exec_stat(mem_ctx_t *allocator)
{
    mem_stat_t stat = {0};
    mem_stat(allocator, &stat);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef DEBUG_CRASH
    printf("[INIT] size = %lu\n", HEAP_SIZE);
#endif
    memset(heap, 0, HEAP_SIZE);
    memset(allocs, 0, sizeof(allocs));
    mem_ctx_t *allocator = mem_init(heap, HEAP_SIZE);

    if (!allocator) {
        return 0;
    }

    size_t  offset = 0;
    uint8_t high   = 0;
    uint8_t low    = 0;
    size_t  index  = 0;
    size_t  len    = 0;
    while (offset < size) {
        mem_stat_t stat = {0};
        mem_stat(allocator, &stat);
#ifdef DEBUG_CRASH
        printf(
            "[STATS] total_size = %d | free_size = %d | allocated_size = %d | nb_chunks = %d | "
            "nb_allocate = %d\n",
            stat.total_size,
            stat.free_size,
            stat.allocated_size,
            stat.nb_chunks,
            stat.nb_allocated);
#endif
        switch (data[offset++]) {
            case 'M':
                if (offset >= size) {
                    return 0;
                }
                len = data[offset++];
                exec_alloc(allocator, len);
                break;
            case 'F':
                if (offset + 1 >= size) {
                    return 0;
                }
                high  = data[offset++];
                low   = data[offset++];
                index = (high << 8) + low;
                exec_free(allocator, index);
                break;
            case 'R':
                if (offset + 2 >= size) {
                    return 0;
                }
                high  = data[offset++];
                low   = data[offset++];
                len   = data[offset++];
                index = (high << 8) + low;
                exec_realloc(allocator, index, len);
                break;
            default:
                return 0;
        }
    }
    return 0;
}
