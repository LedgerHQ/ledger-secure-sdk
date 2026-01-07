/**
 * @file fuzzer_app_mem_utils.c
 * @brief Fuzzer for app_mem_utils high-level memory utilities
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_mem_utils.h"

#define HEAP_SIZE 0x7FF0
#define MAX_ALLOC 0x100

// #define DEBUG_CRASH

static uint8_t heap[HEAP_SIZE] = {0};

struct alloc_entry {
    void   *ptr;
    size_t  len;
    uint8_t type;  // 0=alloc, 1=buffer, 2=strdup, 3=format_uint
};
static struct alloc_entry allocs[MAX_ALLOC] = {0};

bool insert_alloc(void *ptr, size_t len, uint8_t type)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr != NULL) {
            continue;
        }
        allocs[i].ptr  = ptr;
        allocs[i].len  = len;
        allocs[i].type = type;
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
        allocs[i].ptr  = NULL;
        allocs[i].len  = 0;
        allocs[i].type = 0;
        return true;
    }
    return false;
}

bool get_alloc(size_t index, struct alloc_entry **alloc)
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

void exec_alloc(size_t len)
{
    void *ptr = APP_MEM_ALLOC(len);
    if (ptr == NULL) {
        return;
    }
#ifdef DEBUG_CRASH
    printf("[ALLOC] ptr = %p | length = %lu\n", ptr, len);
#endif
    memset(ptr, len & 0xff, len);
    insert_alloc(ptr, len, 0);
}

void exec_buffer_allocate(size_t index, size_t len)
{
    struct alloc_entry *entry  = NULL;
    void               *buffer = NULL;

    // Try to get existing buffer or create new one
    if (get_alloc(index, &entry) && entry->type == 1) {
        buffer = entry->ptr;
        remove_alloc(buffer);
    }

    bool result = APP_MEM_CALLOC(&buffer, len);
    if (!result) {
        return;
    }

#ifdef DEBUG_CRASH
    printf("[BUFFER_ALLOC] ptr = %p | length = %lu\n", buffer, len);
#endif

    if (buffer != NULL && len > 0) {
        // Verify zero-initialization
        for (size_t i = 0; i < len; i++) {
            assert(((uint8_t *) buffer)[i] == 0);
        }
        memset(buffer, len & 0xff, len);
        insert_alloc(buffer, len, 1);
    }
}

void exec_strdup(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return;
    }

    // Create null-terminated string
    char   str[256];
    size_t copy_len = len < 255 ? len : 255;
    memcpy(str, data, copy_len);
    str[copy_len] = '\0';

    const char *dup = APP_MEM_STRDUP(str);
    if (dup == NULL) {
        return;
    }

#ifdef DEBUG_CRASH
    printf("[STRDUP] ptr = %p | length = %lu\n", (void *) dup, strlen(dup));
#endif

    assert(strcmp(dup, str) == 0);
    insert_alloc((void *) dup, strlen(dup) + 1, 2);
}

void exec_free(size_t index)
{
    struct alloc_entry *entry = NULL;
    if (!get_alloc(index, &entry)) {
        return;
    }

#ifdef DEBUG_CRASH
    printf("[FREE] ptr = %p | length = %lu | type = %u\n", entry->ptr, entry->len, entry->type);
#endif

    // Verify memory content
    for (size_t i = 0; i < entry->len; i++) {
        if (entry->type == 0) {  // Only check for simple allocs
            assert(((uint8_t *) entry->ptr)[i] == (entry->len & 0xff));
        }
    }

    if (entry->type == 1) {
        // Use buffer cleanup for buffers
        APP_MEM_FREE_AND_NULL(&entry->ptr);
        assert(entry->ptr == NULL);
    }
    else {
        APP_MEM_FREE(entry->ptr);
    }

    remove_alloc(entry->ptr);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef DEBUG_CRASH
    printf("[INIT] size = %lu\n", HEAP_SIZE);
#endif
    memset(heap, 0, HEAP_SIZE);
    memset(allocs, 0, sizeof(allocs));

    bool init_result = mem_utils_init(heap, HEAP_SIZE);
    if (!init_result) {
        return 0;
    }

    size_t offset = 0;
    while (offset < size) {
        if (offset >= size) {
            break;
        }

        uint8_t op = data[offset++];

        switch (op % 5) {
            case 0:  // Simple alloc
                if (offset >= size) {
                    return 0;
                }
                size_t len = data[offset++];
                exec_alloc(len);
                break;

            case 1:  // Buffer allocate/reallocate
                if (offset + 1 >= size) {
                    return 0;
                }
                uint8_t index   = data[offset++];
                size_t  buf_len = data[offset++];
                exec_buffer_allocate(index, buf_len);
                break;

            case 2:  // strdup
                if (offset >= size) {
                    return 0;
                }
                size_t str_len = data[offset++];
                if (offset + str_len > size) {
                    str_len = size - offset;
                }
                exec_strdup(data + offset, str_len);
                offset += str_len;
                break;

            case 3:  // Free specific
                if (offset >= size) {
                    return 0;
                }
                uint8_t free_idx = data[offset++];
                exec_free(free_idx);
                break;

            case 4:  // Free random
                // Free a random allocation to create fragmentation
                exec_free(offset % MAX_ALLOC);
                break;
        }
    }

    // Cleanup remaining allocations
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr != NULL) {
            if (allocs[i].type == 1) {
                APP_MEM_FREE_AND_NULL(&allocs[i].ptr);
            }
            else {
                APP_MEM_FREE(allocs[i].ptr);
            }
        }
    }

    return 0;
}
