/*
 * Absolution dispatcher for lib_lists (singly + doubly linked lists).
 *
 * Stateful target: byte-coded command stream exercises push/pop/insert/remove/
 * sort/reverse/clear on both forward and doubly-linked lists.
 */

#include "mocks.h"
#include "parser.h"
#include "scenario_layout.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "lists.h"

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

/* ── Node tracking ───────────────────────────────────────────────────────── */

#define MAX_NODES    1000
#define MAX_TRACKERS 100

typedef struct {
    flist_node_t node;
    uint32_t     id;
    uint8_t      data[16];
} fuzz_flist_node_t;
typedef struct {
    list_node_t node;
    uint32_t    id;
    uint8_t     data[16];
} fuzz_list_node_t;

struct node_tracker {
    void    *ptr;
    uint32_t id;
    bool     in_list;
    bool     is_doubly;
};
static struct node_tracker trackers[MAX_TRACKERS];
static uint32_t            next_id;

static fuzz_flist_node_t *create_flist_node(const uint8_t *d, size_t len)
{
    fuzz_flist_node_t *n = malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }
    n->node.next = NULL;
    n->id        = next_id++;
    memcpy(n->data, d, len < 16 ? len : 16);
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (!trackers[i].ptr) {
            trackers[i] = (struct node_tracker){n, n->id, false, false};
            return n;
        }
    }
    free(n);
    return NULL;
}

static fuzz_list_node_t *create_list_node(const uint8_t *d, size_t len)
{
    fuzz_list_node_t *n = malloc(sizeof(*n));
    if (!n) {
        return NULL;
    }
    n->node._list.next = NULL;
    n->node.prev       = NULL;
    n->id              = next_id++;
    memcpy(n->data, d, len < 16 ? len : 16);
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (!trackers[i].ptr) {
            trackers[i] = (struct node_tracker){n, n->id, false, true};
            return n;
        }
    }
    free(n);
    return NULL;
}

static void mark_in_list(void *p)
{
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == p) {
            trackers[i].in_list = true;
            return;
        }
    }
}
static void *get_tracked(uint8_t idx, bool must_in, bool dbl)
{
    size_t i = idx % MAX_TRACKERS;
    if (!trackers[i].ptr || trackers[i].is_doubly != dbl) {
        return NULL;
    }
    if (must_in && !trackers[i].in_list) {
        return NULL;
    }
    return trackers[i].ptr;
}

static void del_flist_node(flist_node_t *node)
{
    if (!node) {
        return;
    }
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i] = (struct node_tracker){0};
            break;
        }
    }
    free(node);
}
static void del_list_node(flist_node_t *node)
{
    if (!node) {
        return;
    }
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i] = (struct node_tracker){0};
            break;
        }
    }
    free(node);
}
static bool cmp_flist(const flist_node_t *a, const flist_node_t *b)
{
    return ((const fuzz_flist_node_t *) a)->id <= ((const fuzz_flist_node_t *) b)->id;
}
static bool cmp_list(const flist_node_t *a, const flist_node_t *b)
{
    return ((const fuzz_list_node_t *) a)->id <= ((const fuzz_list_node_t *) b)->id;
}

static uint8_t g_pred_mod;
static bool    pred_flist(const flist_node_t *node)
{
    return (g_pred_mod > 0) && (((const fuzz_flist_node_t *) node)->id % g_pred_mod == 0);
}
static bool pred_list(const flist_node_t *node)
{
    return (g_pred_mod > 0) && (((const fuzz_list_node_t *) node)->id % g_pred_mod == 0);
}
static bool eq_flist(const flist_node_t *a, const flist_node_t *b)
{
    return ((const fuzz_flist_node_t *) a)->id == ((const fuzz_flist_node_t *) b)->id;
}
static bool eq_list(const flist_node_t *a, const flist_node_t *b)
{
    return ((const fuzz_list_node_t *) a)->id == ((const fuzz_list_node_t *) b)->id;
}

static void cleanup_flist(flist_node_t **fl)
{
    if (fl && *fl) {
        flist_clear(fl, del_flist_node);
    }
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr && !trackers[i].in_list && !trackers[i].is_doubly) {
            free(trackers[i].ptr);
            trackers[i] = (struct node_tracker){0};
        }
    }
}
static void cleanup_dlist(list_node_t **dl)
{
    if (dl && *dl) {
        list_clear(dl, del_list_node);
    }
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr && !trackers[i].in_list && trackers[i].is_doubly) {
            free(trackers[i].ptr);
            trackers[i] = (struct node_tracker){0};
        }
    }
}

/* ── Absolution harness wiring ───────────────────────────────────────────── */

const fuzz_command_spec_t fuzz_commands[] = {
    {.cla = 0x00, .ins = 0x01},
};
const size_t fuzz_n_commands = 1;

void fuzz_app_reset(void)
{
    memset(trackers, 0, sizeof(trackers));
    next_id = 1;
}

void fuzz_app_dispatch(void *cmd)
{
    (void) cmd;
    const uint8_t *data = fuzz_tail_ptr;
    size_t         size = fuzz_tail_len;
    if (!data || size < 2) {
        return;
    }

    bool          use_doubly = (data[0] & 0x80) != 0;
    flist_node_t *flist      = NULL;
    list_node_t  *dlist      = NULL;

    size_t offset = 1;
    while (offset < size) {
        uint8_t op = data[offset++];
        if (use_doubly) {
            switch (op & 0x0F) {
                case 0x00: {
                    if (offset + 16 > size) {
                        goto done;
                    }
                    fuzz_list_node_t *n = create_list_node(&data[offset], 16);
                    offset += 16;
                    if (n && list_push_front(&dlist, &n->node)) {
                        mark_in_list(n);
                    }
                    break;
                }
                case 0x01: {
                    if (offset + 16 > size) {
                        goto done;
                    }
                    fuzz_list_node_t *n = create_list_node(&data[offset], 16);
                    offset += 16;
                    if (n && list_push_back(&dlist, &n->node)) {
                        mark_in_list(n);
                    }
                    break;
                }
                case 0x02:
                    list_pop_front(&dlist, del_list_node);
                    break;
                case 0x03:
                    list_pop_back(&dlist, del_list_node);
                    break;
                case 0x04: {
                    if (offset + 17 > size) {
                        goto done;
                    }
                    uint8_t           ri = data[offset++];
                    fuzz_list_node_t *r  = get_tracked(ri, true, true);
                    if (r) {
                        fuzz_list_node_t *n = create_list_node(&data[offset], 16);
                        if (n && list_insert_after(&dlist, &r->node, &n->node)) {
                            mark_in_list(n);
                        }
                    }
                    offset += 16;
                    break;
                }
                case 0x05: {
                    if (offset + 17 > size) {
                        goto done;
                    }
                    uint8_t           ri = data[offset++];
                    fuzz_list_node_t *r  = get_tracked(ri, true, true);
                    if (r) {
                        fuzz_list_node_t *n = create_list_node(&data[offset], 16);
                        if (n && list_insert_before(&dlist, &r->node, &n->node)) {
                            mark_in_list(n);
                        }
                    }
                    offset += 16;
                    break;
                }
                case 0x06: {
                    if (offset >= size) {
                        goto done;
                    }
                    fuzz_list_node_t *n = get_tracked(data[offset++], true, true);
                    if (n) {
                        list_remove(&dlist, &n->node, del_list_node);
                    }
                    break;
                }
                case 0x07:
                    list_clear(&dlist, del_list_node);
                    break;
                case 0x08:
                    list_sort(&dlist, cmp_list);
                    break;
                case 0x09:
                    (void) list_size(&dlist);
                    break;
                case 0x0A:
                    list_reverse(&dlist);
                    break;
                case 0x0B:
                    (void) list_empty(&dlist);
                    break;
                case 0x0C: {
                    if (offset >= size) {
                        goto done;
                    }
                    g_pred_mod = data[offset++];
                    list_remove_if(&dlist, pred_list, del_list_node);
                    break;
                }
                case 0x0D:
                    list_unique(&dlist, eq_list, del_list_node);
                    break;
                default:
                    break;
            }
            if (list_size(&dlist) > MAX_NODES) {
                goto done;
            }
        }
        else {
            switch (op & 0x0F) {
                case 0x00: {
                    if (offset + 16 > size) {
                        goto done;
                    }
                    fuzz_flist_node_t *n = create_flist_node(&data[offset], 16);
                    offset += 16;
                    if (n && flist_push_front(&flist, &n->node)) {
                        mark_in_list(n);
                    }
                    break;
                }
                case 0x01: {
                    if (offset + 16 > size) {
                        goto done;
                    }
                    fuzz_flist_node_t *n = create_flist_node(&data[offset], 16);
                    offset += 16;
                    if (n && flist_push_back(&flist, &n->node)) {
                        mark_in_list(n);
                    }
                    break;
                }
                case 0x02:
                    flist_pop_front(&flist, del_flist_node);
                    break;
                case 0x03:
                    flist_pop_back(&flist, del_flist_node);
                    break;
                case 0x04: {
                    if (offset + 17 > size) {
                        goto done;
                    }
                    uint8_t            ri = data[offset++];
                    fuzz_flist_node_t *r  = get_tracked(ri, true, false);
                    if (r) {
                        fuzz_flist_node_t *n = create_flist_node(&data[offset], 16);
                        if (n && flist_insert_after(&flist, &r->node, &n->node)) {
                            mark_in_list(n);
                        }
                    }
                    offset += 16;
                    break;
                }
                case 0x05: {
                    if (offset >= size) {
                        goto done;
                    }
                    fuzz_flist_node_t *n = get_tracked(data[offset++], true, false);
                    if (n) {
                        flist_remove(&flist, &n->node, del_flist_node);
                    }
                    break;
                }
                case 0x06:
                    flist_clear(&flist, del_flist_node);
                    break;
                case 0x07:
                    flist_sort(&flist, cmp_flist);
                    break;
                case 0x08:
                    (void) flist_size(&flist);
                    break;
                case 0x09:
                    flist_reverse(&flist);
                    break;
                case 0x0A:
                    (void) flist_empty(&flist);
                    break;
                case 0x0B: {
                    if (offset >= size) {
                        goto done;
                    }
                    g_pred_mod = data[offset++];
                    flist_remove_if(&flist, pred_flist, del_flist_node);
                    break;
                }
                case 0x0C:
                    flist_unique(&flist, eq_flist, del_flist_node);
                    break;
                default:
                    break;
            }
            if (flist_size(&flist) > MAX_NODES) {
                goto done;
            }
        }
    }
done:
    if (use_doubly) {
        cleanup_dlist(&dlist);
    }
    else {
        cleanup_flist(&flist);
    }
}

int fuzz_entry(const uint8_t *data, size_t size)
{
    return fuzz_harness_entry(data, size);
}
