#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "c_list.h"

#define MAX_NODES    1000
#define MAX_TRACKERS 100

// #define DEBUG_CRASH

// Test node structure
typedef struct fuzz_node_s {
    c_list_node_t node;
    uint32_t      id;
    uint8_t       data[16];
} fuzz_node_t;

// Track allocated nodes for cleanup
struct node_tracker {
    fuzz_node_t *ptr;
    uint32_t     id;
    bool         in_list;
};

static struct node_tracker trackers[MAX_TRACKERS] = {0};
static uint32_t            next_id                = 1;

// Helper to create a new tracked node
fuzz_node_t *create_tracked_node(const uint8_t *data, size_t len)
{
    fuzz_node_t *node = malloc(sizeof(fuzz_node_t));
    if (!node) {
        return NULL;
    }

    node->node.next = NULL;
    node->id        = next_id++;

    // Fill data from fuzzer input
    size_t copy_len = len < sizeof(node->data) ? len : sizeof(node->data);
    memcpy(node->data, data, copy_len);

    // Track the node
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == NULL) {
            trackers[i].ptr     = node;
            trackers[i].id      = node->id;
            trackers[i].in_list = false;
#ifdef DEBUG_CRASH
            printf("[CREATE] node id=%u ptr=%p\n", node->id, (void *) node);
#endif
            return node;
        }
    }

    // No space in tracker, free node
    free(node);
    return NULL;
}

// Mark node as in list
void mark_in_list(fuzz_node_t *node)
{
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i].in_list = true;
            return;
        }
    }
}

// Mark node as removed from list
void mark_removed_from_list(fuzz_node_t *node)
{
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == node) {
            trackers[i].in_list = false;
            return;
        }
    }
}

// Get a tracked node by index (for operations requiring existing nodes)
fuzz_node_t *get_tracked_node(uint8_t index, bool must_be_in_list)
{
    size_t idx = index % MAX_TRACKERS;
    if (trackers[idx].ptr == NULL) {
        return NULL;
    }
    if (must_be_in_list && !trackers[idx].in_list) {
        return NULL;
    }
    return trackers[idx].ptr;
}

// Deletion callback
void delete_node(c_list_node_t *node)
{
    fuzz_node_t *fnode = (fuzz_node_t *) node;
    if (!fnode) {
        return;
    }

#ifdef DEBUG_CRASH
    printf("[DELETE] node id=%u ptr=%p\n", fnode->id, (void *) fnode);
#endif

    // Remove from tracker
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr == fnode) {
            trackers[i].ptr     = NULL;
            trackers[i].id      = 0;
            trackers[i].in_list = false;
            break;
        }
    }

    free(fnode);
}

// Comparison function for sorting
bool compare_nodes(const c_list_node_t *a, const c_list_node_t *b)
{
    const fuzz_node_t *na = (const fuzz_node_t *) a;
    const fuzz_node_t *nb = (const fuzz_node_t *) b;
    return na->id <= nb->id;
}

// Cleanup all tracked nodes
void cleanup_all(c_list_node_t **list)
{
    // First clear the list
    if (list && *list) {
        c_list_clear(list, delete_node);
    }

    // Then free any remaining tracked nodes not in list
    for (size_t i = 0; i < MAX_TRACKERS; i++) {
        if (trackers[i].ptr && !trackers[i].in_list) {
#ifdef DEBUG_CRASH
            printf("[CLEANUP] node id=%u ptr=%p\n", trackers[i].id, (void *) trackers[i].ptr);
#endif
            free(trackers[i].ptr);
            trackers[i].ptr     = NULL;
            trackers[i].id      = 0;
            trackers[i].in_list = false;
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0) {
        return 0;
    }

    c_list_node_t *list = NULL;
    memset(trackers, 0, sizeof(trackers));
    next_id = 1;

#ifdef DEBUG_CRASH
    printf("\n[FUZZING] Starting new iteration with %zu bytes\n", size);
#endif

    size_t offset = 0;
    while (offset < size) {
        uint8_t op = data[offset++];

#ifdef DEBUG_CRASH
        printf("[OP] offset=%zu op=0x%02x list_size=%zu\n", offset - 1, op, c_list_size(&list));
#endif

        switch (op & 0x0F) {  // Use lower 4 bits for operation
            case 0x00: {      // push_front
                if (offset + 16 > size) {
                    goto cleanup;
                }
                fuzz_node_t *node = create_tracked_node(&data[offset], 16);
                offset += 16;
                if (node && c_list_push_front(&list, &node->node)) {
                    mark_in_list(node);
                }
                break;
            }

            case 0x01: {  // push_back
                if (offset + 16 > size) {
                    goto cleanup;
                }
                fuzz_node_t *node = create_tracked_node(&data[offset], 16);
                offset += 16;
                if (node && c_list_push_back(&list, &node->node)) {
                    mark_in_list(node);
                }
                break;
            }

            case 0x02: {  // pop_front
                c_list_pop_front(&list, delete_node);
                break;
            }

            case 0x03: {  // pop_back
                c_list_pop_back(&list, delete_node);
                break;
            }

            case 0x04: {  // insert_after
                if (offset + 17 > size) {
                    goto cleanup;
                }
                uint8_t      ref_idx = data[offset++];
                fuzz_node_t *ref     = get_tracked_node(ref_idx, true);
                if (ref) {
                    fuzz_node_t *node = create_tracked_node(&data[offset], 16);
                    if (node && c_list_insert_after(&ref->node, &node->node)) {
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
                uint8_t      ref_idx = data[offset++];
                fuzz_node_t *ref     = get_tracked_node(ref_idx, true);
                if (ref) {
                    fuzz_node_t *node = create_tracked_node(&data[offset], 16);
                    if (node && c_list_insert_before(&list, &ref->node, &node->node)) {
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
                uint8_t      node_idx = data[offset++];
                fuzz_node_t *node     = get_tracked_node(node_idx, true);
                if (node) {
                    c_list_remove(&list, &node->node, delete_node);
                }
                break;
            }

            case 0x07: {  // clear
                c_list_clear(&list, delete_node);
                break;
            }

            case 0x08: {  // sort
                c_list_sort(&list, compare_nodes);
                break;
            }

            case 0x09: {  // size
                size_t s = c_list_size(&list);
                (void) s;
#ifdef DEBUG_CRASH
                printf("[SIZE] list has %zu nodes\n", s);
#endif
                break;
            }

            case 0x0A: {  // traverse and verify integrity
                size_t          count   = 0;
                c_list_node_t  *current = list;
                c_list_node_t **seen    = calloc(MAX_NODES, sizeof(c_list_node_t *));
                if (!seen) {
                    goto cleanup;
                }

                while (current && count < MAX_NODES) {
                    // Check for cycles
                    for (size_t i = 0; i < count; i++) {
                        if (seen[i] == current) {
#ifdef DEBUG_CRASH
                            printf("[ERROR] Cycle detected at node %p\n", (void *) current);
#endif
                            free(seen);
                            goto cleanup;
                        }
                    }
                    seen[count++] = current;
                    current       = current->next;
                }
                free(seen);
                break;
            }

            default:
                // Unknown operation, skip
                break;
        }

        // Prevent infinite loops
        if (c_list_size(&list) > MAX_NODES) {
#ifdef DEBUG_CRASH
            printf("[ERROR] List size exceeded MAX_NODES\n");
#endif
            goto cleanup;
        }
    }

cleanup:
    cleanup_all(&list);
    return 0;
}
