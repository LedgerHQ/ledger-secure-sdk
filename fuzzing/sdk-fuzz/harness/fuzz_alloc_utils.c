/*
 * Absolution dispatcher for app_mem_utils (high-level memory utilities).
 *
 * Stateful target: byte-coded stream exercises alloc / calloc / strdup / free.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "app_mem_utils.h"

#define FUZZ_PREFIX_SIZE_FALLBACK 0
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed)
{
    return fuzz_custom_mutator(data, size, max_size, seed);
}

#include "fuzz_harness.h"

/* ── Allocator state ─────────────────────────────────────────────────────── */

#define HEAP_SIZE 0x7FF0
#define MAX_ALLOC 0x100

static uint8_t heap[HEAP_SIZE];

struct alloc_entry {
    void   *ptr;
    size_t  len;
    uint8_t type;
};
static struct alloc_entry allocs[MAX_ALLOC];

static bool insert_alloc(void *ptr, size_t len, uint8_t type)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr == NULL) {
            allocs[i] = (struct alloc_entry){ptr, len, type};
            return true;
        }
    }
    return false;
}

static bool remove_alloc(void *ptr)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr == ptr) {
            allocs[i] = (struct alloc_entry){0};
            return true;
        }
    }
    return false;
}

static bool get_alloc(size_t index, struct alloc_entry **out)
{
    if (index >= MAX_ALLOC || allocs[index].ptr == NULL) {
        return false;
    }
    *out = &allocs[index];
    return true;
}

static void exec_alloc(size_t len)
{
    void *ptr = APP_MEM_ALLOC(len);
    if (!ptr) {
        return;
    }
    memset(ptr, len & 0xff, len);
    insert_alloc(ptr, len, 0);
}

static void exec_buffer_allocate(size_t index, size_t len)
{
    struct alloc_entry *e      = NULL;
    void               *buffer = NULL;
    if (get_alloc(index, &e) && e->type == 1) {
        buffer = e->ptr;
        remove_alloc(buffer);
    }
    if (!APP_MEM_CALLOC(&buffer, len)) {
        return;
    }
    if (buffer && len > 0) {
        for (size_t i = 0; i < len; i++) {
            assert(((uint8_t *) buffer)[i] == 0);
        }
        memset(buffer, len & 0xff, len);
        insert_alloc(buffer, len, 1);
    }
}

static void exec_strdup(const uint8_t *data, size_t len)
{
    if (!len) {
        return;
    }
    char   str[256];
    size_t copy_len = len < 255 ? len : 255;
    memcpy(str, data, copy_len);
    str[copy_len]   = '\0';
    const char *dup = APP_MEM_STRDUP(str);
    if (!dup) {
        return;
    }
    assert(strcmp(dup, str) == 0);
    insert_alloc((void *) dup, strlen(dup) + 1, 2);
}

static void exec_free(size_t index)
{
    struct alloc_entry *e = NULL;
    if (!get_alloc(index, &e)) {
        return;
    }
    if (e->type == 0) {
        for (size_t i = 0; i < e->len; i++) {
            assert(((uint8_t *) e->ptr)[i] == (e->len & 0xff));
        }
    }
    void *saved = e->ptr;
    if (e->type == 1) {
        APP_MEM_FREE_AND_NULL(&e->ptr);
        assert(e->ptr == NULL);
    }
    else {
        APP_MEM_FREE(e->ptr);
    }
    remove_alloc(saved);
}

/* ── Absolution harness wiring ───────────────────────────────────────────── */

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void)
{
    memset(heap, 0, HEAP_SIZE);
    memset(allocs, 0, sizeof(allocs));
    mem_utils_init(heap, HEAP_SIZE);
}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    const uint8_t *data = fuzz_tail_ptr;
    size_t         size = fuzz_tail_len;
    if (!data || size == 0) {
        return;
    }

    size_t offset = 0;
    while (offset < size) {
        uint8_t op = data[offset++];
        switch (op % 5) {
            case 0:
                if (offset >= size) {
                    return;
                }
                exec_alloc(data[offset++]);
                break;
            case 1:
                if (offset + 1 >= size) {
                    return;
                }
                {
                    uint8_t idx  = data[offset++];
                    size_t  blen = data[offset++];
                    exec_buffer_allocate(idx, blen);
                }
                break;
            case 2:
                if (offset >= size) {
                    return;
                }
                {
                    size_t slen = data[offset++];
                    if (offset + slen > size) {
                        slen = size - offset;
                    }
                    exec_strdup(data + offset, slen);
                    offset += slen;
                }
                break;
            case 3:
                if (offset >= size) {
                    return;
                }
                exec_free(data[offset++]);
                break;
            case 4:
                exec_free(offset % MAX_ALLOC);
                break;
        }
    }

    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr) {
            if (allocs[i].type == 1) {
                APP_MEM_FREE_AND_NULL(&allocs[i].ptr);
            }
            else {
                APP_MEM_FREE(allocs[i].ptr);
            }
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
