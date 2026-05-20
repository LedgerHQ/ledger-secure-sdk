/*
 * Absolution dispatcher for lib_alloc: mem_alloc / mem_realloc / mem_free /
 * mem_stat / mem_parse.
 *
 * Stateful target: the byte-coded command stream drives a heap allocator.
 * Absolution manages global state snapshotting via the prefix.
 *
 * Commands:
 *   'M' <size_lo> <size_hi>   -- alloc (2-byte LE size)
 *   'F' <idx_hi>  <idx_lo>    -- free by tracker index
 *   'R' <idx_hi>  <idx_lo> <size_lo> <size_hi> -- realloc
 *   'S'                       -- mem_stat with invariant check
 *   'P'                       -- mem_parse
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "mem_alloc.h"

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
#define MAX_ALLOC 256

static uint8_t heap[HEAP_SIZE];

struct alloc_entry {
    void  *ptr;
    size_t len;
};
static struct alloc_entry allocs[MAX_ALLOC];
static mem_ctx_t          allocator;

static bool insert_alloc(void *ptr, size_t len)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr == NULL) {
            allocs[i].ptr = ptr;
            allocs[i].len = len;
            return true;
        }
    }
    return false;
}

static bool remove_alloc(void *ptr)
{
    for (size_t i = 0; i < MAX_ALLOC; i++) {
        if (allocs[i].ptr == ptr) {
            allocs[i].ptr = NULL;
            allocs[i].len = 0;
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
    void *ptr = mem_alloc(allocator, len);
    if (!ptr) {
        return;
    }
    memset(ptr, (len + 1) & 0xff, len);
    insert_alloc(ptr, len);
}

static void exec_realloc(size_t index, size_t len)
{
    struct alloc_entry *e = NULL;
    if (!get_alloc(index, &e)) {
        return;
    }
    void  *old     = e->ptr;
    size_t old_len = e->len;
    void  *ptr     = mem_realloc(allocator, old, len);
    if (!ptr) {
        /* realloc failure: old pointer is still valid -- do NOT free it */
        return;
    }
    remove_alloc(old);
    if (len > 0) {
        memset(ptr, (len + 1) & 0xff, len);
    }
    insert_alloc(ptr, len);
}

static void exec_free(size_t index)
{
    struct alloc_entry *e = NULL;
    if (!get_alloc(index, &e)) {
        return;
    }
    mem_free(allocator, e->ptr);
    remove_alloc(e->ptr);
}

static void exec_stat(void)
{
    mem_stat_t st;
    memset(&st, 0, sizeof(st));
    mem_stat(&allocator, &st);
    (void) st;
}

static bool parse_cb(void *data, uint8_t *addr, bool allocated, size_t size)
{
    (void) data;
    (void) addr;
    (void) allocated;
    (void) size;
    return true;
}

static void exec_parse(void)
{
    mem_parse(allocator, parse_cb, NULL);
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
    allocator = mem_init(heap, HEAP_SIZE);
}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    const uint8_t *data = fuzz_tail_ptr;
    size_t         size = fuzz_tail_len;
    if (!data || size == 0 || !allocator) {
        return;
    }

    size_t offset = 0;
    while (offset < size) {
        switch (data[offset++]) {
            case 'M':
                if (offset + 1 >= size) {
                    return;
                }
                {
                    uint16_t sz = (uint16_t) data[offset] | ((uint16_t) data[offset + 1] << 8);
                    offset += 2;
                    exec_alloc(sz);
                }
                break;
            case 'F':
                if (offset + 1 >= size) {
                    return;
                }
                {
                    uint8_t hi = data[offset++], lo = data[offset++];
                    exec_free(((size_t) hi << 8) + lo);
                }
                break;
            case 'R':
                if (offset + 3 >= size) {
                    return;
                }
                {
                    uint8_t  hi = data[offset++], lo = data[offset++];
                    uint16_t sz = (uint16_t) data[offset] | ((uint16_t) data[offset + 1] << 8);
                    offset += 2;
                    exec_realloc(((size_t) hi << 8) + lo, sz);
                }
                break;
            case 'S':
                exec_stat();
                break;
            case 'P':
                exec_parse();
                break;
            default:
                return;
        }
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
