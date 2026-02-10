/**
 * @file
 * @brief Generic linked list implementation
 *
 * Provides both singly-linked (flist) and doubly-linked (list) implementations.
 */

#if !defined(HAVE_ETH2) && !defined(USE_LIB_ETHEREUM) || defined(HAVE_SDK_LL_LIB)

#include "lists.h"
#include "os_helpers.h"

// ============================================================================
// Internal functions used by both singly and doubly linked lists
// ============================================================================

static void push_front_internal(flist_node_t **list, flist_node_t *node, bool doubly_linked)
{
    if ((list != NULL) && (node != NULL)) {
        node->next = *list;
        *list      = node;
        if (doubly_linked) {
            if (node->next != NULL) {
                ((list_node_t *) node->next)->prev = (list_node_t *) node;
            }
            ((list_node_t *) node)->prev = NULL;
        }
    }
}

static void pop_front_internal(flist_node_t **list, f_list_node_del del_func, bool doubly_linked)
{
    flist_node_t *tmp;

    if (list != NULL) {
        tmp = *list;
        if (tmp != NULL) {
            *list = tmp->next;
            if (del_func != NULL) {
                del_func(tmp);
            }
            if (doubly_linked) {
                if (*list != NULL) {
                    (*(list_node_t **) list)->prev = NULL;
                }
            }
        }
    }
}

static void push_back_internal(flist_node_t **list, flist_node_t *node, bool doubly_linked)
{
    flist_node_t *tmp;

    if ((list != NULL) && (node != NULL)) {
        node->next = NULL;
        if (*list == NULL) {
            if (doubly_linked) {
                ((list_node_t *) node)->prev = NULL;
            }
            *list = node;
        }
        else {
            tmp = *list;
            while (tmp->next != NULL) {
                tmp = tmp->next;
            }
            if (doubly_linked) {
                ((list_node_t *) node)->prev = (list_node_t *) tmp;
            }
            tmp->next = node;
        }
    }
}

static void insert_after_internal(flist_node_t *ref, flist_node_t *node, bool doubly_linked)
{
    if ((ref != NULL) && (node != NULL)) {
        if (doubly_linked) {
            if (ref->next != NULL) {
                ((list_node_t *) (ref->next))->prev = (list_node_t *) node;
            }
            ((list_node_t *) node)->prev = (list_node_t *) ref;
        }
        node->next = ref->next;
        ref->next  = node;
    }
}

static void remove_internal(flist_node_t  **list,
                            flist_node_t   *node,
                            f_list_node_del del_func,
                            bool            doubly_linked)
{
    flist_node_t *it;
    flist_node_t *tmp;

    if ((list != NULL) && (node != NULL)) {
        if (node == *list) {
            // first element
            pop_front_internal(list, del_func, doubly_linked);
        }
        else {
            it = *list;
            if (it != NULL) {
                while ((it->next != node) && (it->next != NULL)) {
                    it = it->next;
                }
                if (it->next == NULL) {
                    // node not found
                    return;
                }
                tmp = it->next->next;
                if (doubly_linked) {
                    if (tmp != NULL) {
                        ((list_node_t *) tmp)->prev = ((list_node_t *) node)->prev;
                    }
                }
                if (del_func != NULL) {
                    del_func(it->next);
                }
                it->next = tmp;
            }
        }
    }
}

static size_t remove_if_internal(flist_node_t   **list,
                                 f_list_node_pred pred_func,
                                 f_list_node_del  del_func,
                                 bool             doubly_linked)
{
    flist_node_t *node;
    flist_node_t *tmp;
    size_t        count = 0;

    if ((list != NULL) && (pred_func != NULL)) {
        node = *list;
        while (node != NULL) {
            tmp = node->next;
            if (pred_func(node)) {
                remove_internal(list, node, del_func, doubly_linked);
                count += 1;
            }
            node = tmp;
        }
    }
    return count;
}

static void sort_internal(flist_node_t **list, f_list_node_cmp cmp_func, bool doubly_linked)
{
    flist_node_t **tmp;
    flist_node_t  *a, *b;
    bool           sorted;

    if ((list != NULL) && (cmp_func != NULL)) {
        do {
            sorted = true;
            for (tmp = list; (*tmp != NULL) && ((*tmp)->next != NULL); tmp = &(*tmp)->next) {
                a = *tmp;
                b = a->next;
                if (cmp_func(a, b) == false) {
                    *tmp    = b;
                    a->next = b->next;
                    b->next = a;
                    if (doubly_linked) {
                        ((list_node_t *) b)->prev = ((list_node_t *) a)->prev;
                        ((list_node_t *) a)->prev = (list_node_t *) b;
                        if (a->next != NULL) {
                            ((list_node_t *) a->next)->prev = (list_node_t *) a;
                        }
                    }
                    sorted = false;
                }
            }
        } while (!sorted);
    }
}

static size_t unique_internal(flist_node_t       **list,
                              f_list_node_bin_pred pred_func,
                              f_list_node_del      del_func,
                              bool                 doubly_linked)
{
    size_t count = 0;

    if ((list != NULL) && (pred_func != NULL)) {
        for (flist_node_t *ref = *list; ref != NULL; ref = ref->next) {
            flist_node_t *node = ref->next;
            while ((node != NULL) && (pred_func(ref, node))) {
                flist_node_t *tmp = node->next;
                remove_internal(list, node, del_func, doubly_linked);
                count += 1;
                node = tmp;
            }
        }
    }
    return count;
}

static void reverse_internal(flist_node_t **list, bool doubly_linked)
{
    flist_node_t *node;
    flist_node_t *prev = NULL;
    flist_node_t *next;

    if (list != NULL) {
        node = *list;
        while (node != NULL) {
            next       = node->next;
            node->next = prev;
            if (doubly_linked) {
                ((list_node_t *) node)->prev = (list_node_t *) next;
            }
            *list = node;
            prev  = node;
            node  = next;
        }
    }
}

// ============================================================================
// Forward list (singly-linked) functions
// ============================================================================

void flist_push_front(flist_node_t **list, flist_node_t *node)
{
    push_front_internal(list, node, false);
}

void flist_pop_front(flist_node_t **list, f_list_node_del del_func)
{
    pop_front_internal(list, del_func, false);
}

void flist_push_back(flist_node_t **list, flist_node_t *node)
{
    push_back_internal(list, node, false);
}

void flist_pop_back(flist_node_t **list, f_list_node_del del_func)
{
    flist_node_t *tmp;

    if ((list != NULL) && (*list != NULL)) {
        tmp = *list;
        // only one element
        if (tmp->next == NULL) {
            flist_pop_front(list, del_func);
        }
        else {
            while (tmp->next->next != NULL) {
                tmp = tmp->next;
            }
            if (del_func != NULL) {
                del_func(tmp->next);
            }
            tmp->next = NULL;
        }
    }
}

void flist_insert_after(flist_node_t **list, flist_node_t *ref, flist_node_t *node)
{
    UNUSED(list);
    insert_after_internal(ref, node, false);
}

void flist_remove(flist_node_t **list, flist_node_t *node, f_list_node_del del_func)
{
    remove_internal(list, node, del_func, false);
}

size_t flist_remove_if(flist_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func)
{
    return remove_if_internal(list, pred_func, del_func, false);
}

void flist_clear(flist_node_t **list, f_list_node_del del_func)
{
    flist_node_t *tmp;
    flist_node_t *next;

    if (list != NULL) {
        tmp = *list;
        while (tmp != NULL) {
            next = tmp->next;
            if (del_func != NULL) {
                del_func(tmp);
            }
            tmp = next;
        }
        *list = NULL;
    }
}

size_t flist_size(flist_node_t *const *list)
{
    size_t size = 0;

    if (list != NULL) {
        for (flist_node_t *tmp = *list; tmp != NULL; tmp = tmp->next) {
            size += 1;
        }
    }
    return size;
}

bool flist_empty(flist_node_t *const *list)
{
    return flist_size(list) == 0;
}

void flist_sort(flist_node_t **list, f_list_node_cmp cmp_func)
{
    sort_internal(list, cmp_func, false);
}

size_t flist_unique(flist_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func)
{
    return unique_internal(list, pred_func, del_func, false);
}

void flist_reverse(flist_node_t **list)
{
    reverse_internal(list, false);
}

// ============================================================================
// Doubly-linked list functions
// ============================================================================

void list_push_front(list_node_t **list, list_node_t *node)
{
    push_front_internal((flist_node_t **) list, (flist_node_t *) node, true);
}

void list_pop_front(list_node_t **list, f_list_node_del del_func)
{
    pop_front_internal((flist_node_t **) list, del_func, true);
}

void list_push_back(list_node_t **list, list_node_t *node)
{
    push_back_internal((flist_node_t **) list, (flist_node_t *) node, true);
}

void list_pop_back(list_node_t **list, f_list_node_del del_func)
{
    flist_pop_back((flist_node_t **) list, del_func);
}

void list_insert_before(list_node_t **list, list_node_t *ref, list_node_t *node)
{
    if ((ref != NULL) && (node != NULL)) {
        if (ref->prev == NULL) {
            if ((list != NULL) && (*list == ref)) {
                list_push_front(list, node);
            }
        }
        else {
            list_insert_after(list, ref->prev, node);
        }
    }
}

void list_insert_after(list_node_t **list, list_node_t *ref, list_node_t *node)
{
    UNUSED(list);
    insert_after_internal((flist_node_t *) ref, (flist_node_t *) node, true);
}

void list_remove(list_node_t **list, list_node_t *node, f_list_node_del del_func)
{
    remove_internal((flist_node_t **) list, (flist_node_t *) node, del_func, true);
}

size_t list_remove_if(list_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func)
{
    return remove_if_internal((flist_node_t **) list, pred_func, del_func, true);
}

void list_clear(list_node_t **list, f_list_node_del del_func)
{
    flist_clear((flist_node_t **) list, del_func);
}

size_t list_size(list_node_t *const *list)
{
    return flist_size((flist_node_t **) list);
}

bool list_empty(list_node_t *const *list)
{
    return list_size(list) == 0;
}

void list_sort(list_node_t **list, f_list_node_cmp cmp_func)
{
    sort_internal((flist_node_t **) list, cmp_func, true);
}

size_t list_unique(list_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func)
{
    return unique_internal((flist_node_t **) list, pred_func, del_func, true);
}

void list_reverse(list_node_t **list)
{
    reverse_internal((flist_node_t **) list, true);
}

#endif  // !defined(HAVE_ETH2) || defined(HAVE_SDK_LL_LIB)
