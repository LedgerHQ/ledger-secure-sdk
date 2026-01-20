#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "c_list.h"

#define MAX_NODES    1000
#define MAX_TRACKERS 100

// #define DEBUG_CRASH

// Test node structures for both list types
typedef struct fuzz_flist_node_s {
    c_flist_node_t node;
    uint32_t       id;
    uint8_t        data[16];
} fuzz_flist_node_t;

typedef struct fuzz_dlist_node_s {
    c_dlist_node_t node;
    uint32_t       id;
    uint8_t        data[16];
} fuzz_dlist_node_t;

// Track allocated nodes for cleanup
struct node_tracker {
    void    *ptr;
    uint32_t id;
    bool     in_list;
    bool     is_doubly;  // Track which type of list
};

static struct node_tracker trackers[MAX_TRACKERS] = {0};
static uint32_t            next_id                = 1;

// Helper to create a new tracked forward list node
fuzz_flist_node_t *create_tracked_flist_node(const uint8_t *data, size_t len)
{
    fuzz_flist_node_t *node = malloc(sizeof(fuzz_flist_node_t));
    if (!node) {
        return NULL;
    }

    node->node.next = NULL;
    node->id        = next_id++;

    size_t copy_len = len < sizeof(node->data) ? len : sizeof(node->data);
    memcpy(node->data, data, copy_len);

    // Track the node
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == NULL) {
            trackers[i].ptr       = node;
            trackers[i].id        = node->id;
            trackers[i].in_list   = false;
            trackers[i].is_doubly = false;
#ifdef DEBUG_CRASH
            printf("[CREATE FLIST] node id=%u ptr=%p\n", node->id, (void *) node);
#endif
            return node;
        }
    }

    free(node);
    return NULL;
}

// Helper to create a new tracked doubly-linked list node
fuzz_dlist_node_t *create_tracked_dlist_node(const uint8_t *data, size_t len)
{
    fuzz_dlist_node_t *node = malloc(sizeof(fuzz_dlist_node_t));
    if (!node) {
        return NULL;
    }

    node->node._list.next = NULL;
    node->node.prev       = NULL;
    node->id              = next_id++;

    size_t copy_len = len < sizeof(node->data) ? len : sizeof(node->data);
    memcpy(node->data, data, copy_len);

    // Track the node
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == NULL) {
            trackers[i].ptr       = node;
            trackers[i].id        = node->id;
            trackers[i].in_list   = false;
            trackers[i].is_doubly = true;
#ifdef DEBUG_CRASH
            printf("[CREATE DLIST] node id=%u ptr=%p\n", node->id, (void *) node);
#endif
            return node;
        }
    }

    free(node);
    return NULL;
}

// Mark node as in list
void mark_in_list(void *node)
{
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i].in_list = true;
            return;
        }
    }
}

// Mark node as removed from list
void mark_removed_from_list(void *node)
{
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i].in_list = false;
            return;
        }
    }
}

// Get a tracked node by index (for operations requiring existing nodes)
void *get_tracked_node(uint8_t index, bool must_be_in_list, bool is_doubly)
{
    size_t idx = index % MAX_TRACKERS;
    if (trackers[idx].ptr == NULL) {
        return NULL;
    }
    if (trackers[idx].is_doubly != is_doubly) {
        return NULL;  // Type mismatch
    }
    if (must_be_in_list && !trackers[idx].in_list) {
        return NULL;
    }
    return trackers[idx].ptr;
}

// Deletion callback for forward lists
void delete_flist_node(c_flist_node_t *node)
{
    if (!node) {
        return;
    }

    fuzz_flist_node_t *fnode = (fuzz_flist_node_t *) node;

#ifdef DEBUG_CRASH
    printf("[DELETE FLIST] node id=%u ptr=%p\n", fnode->id, (void *) fnode);
#endif

    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == fnode) {
            trackers[i].ptr       = NULL;
            trackers[i].id        = 0;
            trackers[i].in_list   = false;
            trackers[i].is_doubly = false;
            break;
        }
    }

    free(fnode);
}

// Deletion callback for doubly-linked lists
void delete_dlist_node(c_flist_node_t *node)
{
    if (!node) {
        return;
    }

    fuzz_dlist_node_t *dnode = (fuzz_dlist_node_t *) node;

#ifdef DEBUG_CRASH
    printf("[DELETE DLIST] node id=%u ptr=%p\n", dnode->id, (void *) dnode);
#endif

    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == dnode) {
            trackers[i].ptr       = NULL;
            trackers[i].id        = 0;
            trackers[i].in_list   = false;
            trackers[i].is_doubly = false;
            break;
        }
    }

    free(dnode);
}

// Comparison functions
bool compare_flist_nodes(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const fuzz_flist_node_t *na = (const fuzz_flist_node_t *) a;
    const fuzz_flist_node_t *nb = (const fuzz_flist_node_t *) b;
    return na->id <= nb->id;
}

bool compare_dlist_nodes(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const fuzz_dlist_node_t *na = (const fuzz_dlist_node_t *) a;
    const fuzz_dlist_node_t *nb = (const fuzz_dlist_node_t *) b;
    return na->id <= nb->id;
}

// Cleanup all tracked nodes
void cleanup_all_flist(c_flist_node_t **list)
{
    if (list && *list) {
        c_flist_clear(list, delete_flist_node);
    }

    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr && !trackers[i].in_list && !trackers[i].is_doubly) {
#ifdef DEBUG_CRASH
            printf("[CLEANUP FLIST] node id=%u ptr=%p\n", trackers[i].id, trackers[i].ptr);
#endif
            free(trackers[i].ptr);
            trackers[i].ptr       = NULL;
            trackers[i].id        = 0;
            trackers[i].is_doubly = false;
        }
    }
}

void cleanup_all_dlist(c_dlist_node_t **list)
{
    if (list && *list) {
        c_dlist_clear(list, delete_dlist_node);
    }

    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr && !trackers[i].in_list && trackers[i].is_doubly) {
#ifdef DEBUG_CRASH
            printf("[CLEANUP DLIST] node id=%u ptr=%p\n", trackers[i].id, trackers[i].ptr);
#endif
            free(trackers[i].ptr);
            trackers[i].ptr       = NULL;
            trackers[i].id        = 0;
            trackers[i].is_doubly = false;
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2) {
        return 0;
    }

    // First byte determines list type: 0 = forward list, 1 = doubly-linked list
    bool use_doubly = (data[0] & 0x80) != 0;

    c_flist_node_t *flist = NULL;
    c_dlist_node_t *dlist = NULL;

    memset(trackers, 0, sizeof(trackers));
    next_id = 1;

#ifdef DEBUG_CRASH
    printf("\n[FUZZING] Starting %s list with %zu bytes\n",
           use_doubly ? "DOUBLY-LINKED" : "FORWARD",
           size);
#endif

    size_t offset = 1;
    while (offset < size) {
        uint8_t op = data[offset++];

#ifdef DEBUG_CRASH
        size_t current_size = use_doubly ? c_dlist_size(&dlist) : c_flist_size(&flist);
        printf("[OP] offset=%zu op=0x%02x list_size=%zu\n", offset - 1, op, current_size);
#endif

        if (use_doubly) {
            // Doubly-linked list operations
            switch (op & 0x0F) {
                case 0x00: {  // push_front
                    if (offset + 16 > size) {
                        goto cleanup;
                    }
                    fuzz_dlist_node_t *node = create_tracked_dlist_node(&data[offset], 16);
                    offset += 16;
                    if (node && c_dlist_push_front(&dlist, &node->node)) {
                        mark_in_list(node);
                    }
                    break;
                }

                case 0x01: {  // push_back
                    if (offset + 16 > size) {
                        goto cleanup;
                    }
                    fuzz_dlist_node_t *node = create_tracked_dlist_node(&data[offset], 16);
                    offset += 16;
                    if (node && c_dlist_push_back(&dlist, &node->node)) {
                        mark_in_list(node);
                    }
                    break;
                }

                case 0x02: {  // pop_front
                    c_dlist_pop_front(&dlist, delete_dlist_node);
                    break;
                }

                case 0x03: {  // pop_back
                    c_dlist_pop_back(&dlist, delete_dlist_node);
                    break;
                }

                case 0x04: {  // insert_after
                    if (offset + 17 > size) {
                        goto cleanup;
                    }
                    uint8_t            ref_idx = data[offset++];
                    fuzz_dlist_node_t *ref     = get_tracked_node(ref_idx, true, true);
                    if (ref) {
                        fuzz_dlist_node_t *node = create_tracked_dlist_node(&data[offset], 16);
                        if (node && c_dlist_insert_after(&dlist, &ref->node, &node->node)) {
                            mark_in_list(node);
                        }
                    }
                    offset += 16;
                    break;
                }

                case 0x05: {  // insert_before
                    if (offset + 17 > size) {
                        goto cleanup;
                    }
                    uint8_t            ref_idx = data[offset++];
                    fuzz_dlist_node_t *ref     = get_tracked_node(ref_idx, true, true);
                    if (ref) {
                        fuzz_dlist_node_t *node = create_tracked_dlist_node(&data[offset], 16);
                        if (node && c_dlist_insert_before(&dlist, &ref->node, &node->node)) {
                            mark_in_list(node);
                        }
                    }
                    offset += 16;
                    break;
                }

                case 0x06: {  // remove
                    if (offset >= size) {
                        goto cleanup;
                    }
                    uint8_t            node_idx = data[offset++];
                    fuzz_dlist_node_t *node     = get_tracked_node(node_idx, true, true);
                    if (node) {
                        c_dlist_remove(&dlist, &node->node, delete_dlist_node);
                    }
                    break;
                }

                case 0x07: {  // clear
                    c_dlist_clear(&dlist, delete_dlist_node);
                    break;
                }

                case 0x08: {  // sort
                    c_dlist_sort(&dlist, compare_dlist_nodes);
                    break;
                }

                case 0x09: {  // size
                    size_t s = c_dlist_size(&dlist);
                    (void) s;
                    break;
                }

                case 0x0A: {  // reverse
                    c_dlist_reverse(&dlist);
                    break;
                }

                case 0x0B: {  // empty
                    bool empty = c_dlist_empty(&dlist);
                    (void) empty;
                    break;
                }

                default:
                    break;
            }

            if (c_dlist_size(&dlist) > MAX_NODES) {
                goto cleanup;
            }
        }
        else {
            // Forward list operations
            switch (op & 0x0F) {
                case 0x00: {  // push_front
                    if (offset + 16 > size) {
                        goto cleanup;
                    }
                    fuzz_flist_node_t *node = create_tracked_flist_node(&data[offset], 16);
                    offset += 16;
                    if (node && c_flist_push_front(&flist, &node->node)) {
                        mark_in_list(node);
                    }
                    break;
                }

                case 0x01: {  // push_back
                    if (offset + 16 > size) {
                        goto cleanup;
                    }
                    fuzz_flist_node_t *node = create_tracked_flist_node(&data[offset], 16);
                    offset += 16;
                    if (node && c_flist_push_back(&flist, &node->node)) {
                        mark_in_list(node);
                    }
                    break;
                }

                case 0x02: {  // pop_front
                    c_flist_pop_front(&flist, delete_flist_node);
                    break;
                }

                case 0x03: {  // pop_back
                    c_flist_pop_back(&flist, delete_flist_node);
                    break;
                }

                case 0x04: {  // insert_after
                    if (offset + 17 > size) {
                        goto cleanup;
                    }
                    uint8_t            ref_idx = data[offset++];
                    fuzz_flist_node_t *ref     = get_tracked_node(ref_idx, true, false);
                    if (ref) {
                        fuzz_flist_node_t *node = create_tracked_flist_node(&data[offset], 16);
                        if (node && c_flist_insert_after(&flist, &ref->node, &node->node)) {
                            mark_in_list(node);
                        }
                    }
                    offset += 16;
                    break;
                }

                case 0x05: {  // remove
                    if (offset >= size) {
                        goto cleanup;
                    }
                    uint8_t            node_idx = data[offset++];
                    fuzz_flist_node_t *node     = get_tracked_node(node_idx, true, false);
                    if (node) {
                        c_flist_remove(&flist, &node->node, delete_flist_node);
                    }
                    break;
                }

                case 0x06: {  // clear
                    c_flist_clear(&flist, delete_flist_node);
                    break;
                }

                case 0x07: {  // sort
                    c_flist_sort(&flist, compare_flist_nodes);
                    break;
                }

                case 0x08: {  // size
                    size_t s = c_flist_size(&flist);
                    (void) s;
                    break;
                }

                case 0x09: {  // reverse
                    c_flist_reverse(&flist);
                    break;
                }

                case 0x0A: {  // empty
                    bool empty = c_flist_empty(&flist);
                    (void) empty;
                    break;
                }

                default:
                    break;
            }

            if (c_flist_size(&flist) > MAX_NODES) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (use_doubly) {
        cleanup_all_dlist(&dlist);
    }
    else {
        cleanup_all_flist(&flist);
    }
    return 0;
}
