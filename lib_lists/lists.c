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
// Internal shared functions with strict validation
// ============================================================================

static bool push_front_internal(flist_node_t **list, flist_node_t *node, bool doubly_linked)
{
    if ((list == NULL) || (node == NULL)) {
        return false;
    }
    node->next = *list;
    *list      = node;
    if (doubly_linked) {
        if (node->next != NULL) {
            ((list_node_t *) node->next)->prev = (list_node_t *) node;
        }
        ((list_node_t *) node)->prev = NULL;
    }
    return true;
}

static bool pop_front_internal(flist_node_t **list, f_list_node_del del_func, bool doubly_linked)
{
    flist_node_t *tmp;

    if ((list == NULL) || (*list == NULL)) {
        return false;
    }
    tmp   = *list;
    *list = tmp->next;
    if (del_func != NULL) {
        del_func(tmp);
    }
    if (doubly_linked) {
        if (*list != NULL) {
            (*(list_node_t **) list)->prev = NULL;
        }
    }
    return true;
}

static bool push_back_internal(flist_node_t **list, flist_node_t *node, bool doubly_linked)
{
    flist_node_t *tmp;

    if ((list == NULL) || (node == NULL)) {
        return false;
    }
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
    return true;
}

static bool insert_after_internal(flist_node_t *ref, flist_node_t *node, bool doubly_linked)
{
    if ((ref == NULL) || (node == NULL)) {
        return false;
    }
    if (doubly_linked) {
        if (ref->next != NULL) {
            ((list_node_t *) (ref->next))->prev = (list_node_t *) node;
        }
        ((list_node_t *) node)->prev = (list_node_t *) ref;
    }
    node->next = ref->next;
    ref->next  = node;
    return true;
}

static bool remove_internal(flist_node_t  **list,
                            flist_node_t   *node,
                            f_list_node_del del_func,
                            bool            doubly_linked)
{
    flist_node_t *it;
    flist_node_t *tmp;

    if ((list == NULL) || (node == NULL) || (*list == NULL)) {
        return false;
    }
    if (node == *list) {
        // first element
        return pop_front_internal(list, del_func, doubly_linked);
    }
    it = *list;
    while ((it->next != node) && (it->next != NULL)) {
        it = it->next;
    }
    if (it->next == NULL) {
        // node not found
        return false;
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
    return true;
}

static size_t remove_if_internal(flist_node_t   **list,
                                 f_list_node_pred pred_func,
                                 f_list_node_del  del_func,
                                 bool             doubly_linked)
{
    flist_node_t *node;
    flist_node_t *tmp;
    size_t        count = 0;

    if ((list == NULL) || (pred_func == NULL)) {
        return 0;
    }
    node = *list;
    while (node != NULL) {
        tmp = node->next;
        if (pred_func(node)) {
            if (remove_internal(list, node, del_func, doubly_linked)) {
                count += 1;
            }
        }
        node = tmp;
    }
    return count;
}

static bool sort_internal(flist_node_t **list, f_list_node_cmp cmp_func, bool doubly_linked)
{
    flist_node_t **tmp;
    flist_node_t  *a, *b;
    bool           sorted;

    if ((list == NULL) || (cmp_func == NULL)) {
        return false;
    }
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
    return true;
}

static size_t unique_internal(flist_node_t       **list,
                              f_list_node_bin_pred pred_func,
                              f_list_node_del      del_func,
                              bool                 doubly_linked)
{
    size_t count = 0;

    if ((list == NULL) || (pred_func == NULL)) {
        return 0;
    }
    for (flist_node_t *ref = *list; ref != NULL; ref = ref->next) {
        flist_node_t *node = ref->next;
        while ((node != NULL) && (pred_func(ref, node))) {
            flist_node_t *tmp = node->next;
            if (remove_internal(list, node, del_func, doubly_linked)) {
                count += 1;
            }
            node = tmp;
        }
    }
    return count;
}

static bool reverse_internal(flist_node_t **list, bool doubly_linked)
{
    flist_node_t *node;
    flist_node_t *prev = NULL;
    flist_node_t *next;

    if (list == NULL) {
        return false;
    }
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
    return true;
}

// ============================================================================
// Forward list (singly-linked) functions
// ============================================================================

bool flist_push_front(flist_node_t **list, flist_node_t *node)
{
    return push_front_internal(list, node, false);
}

bool flist_pop_front(flist_node_t **list, f_list_node_del del_func)
{
    return pop_front_internal(list, del_func, false);
}

bool flist_push_back(flist_node_t **list, flist_node_t *node)
{
    return push_back_internal(list, node, false);
}

bool flist_pop_back(flist_node_t **list, f_list_node_del del_func)
{
    flist_node_t *tmp;

    if ((list == NULL) || (*list == NULL)) {
        return false;
    }
    tmp = *list;
    // only one element
    if (tmp->next == NULL) {
        return flist_pop_front(list, del_func);
    }
    while (tmp->next->next != NULL) {
        tmp = tmp->next;
    }
    if (del_func != NULL) {
        del_func(tmp->next);
    }
    tmp->next = NULL;
    return true;
}

bool flist_insert_after(flist_node_t **list, flist_node_t *ref, flist_node_t *node)
{
    UNUSED(list);
    return insert_after_internal(ref, node, false);
}

bool flist_remove(flist_node_t **list, flist_node_t *node, f_list_node_del del_func)
{
    return remove_internal(list, node, del_func, false);
}

size_t flist_remove_if(flist_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func)
{
    return remove_if_internal(list, pred_func, del_func, false);
}

bool flist_clear(flist_node_t **list, f_list_node_del del_func)
{
    flist_node_t *tmp;
    flist_node_t *next;

    if (list == NULL) {
        return false;
    }
    tmp = *list;
    while (tmp != NULL) {
        next = tmp->next;
        if (del_func != NULL) {
            del_func(tmp);
        }
        tmp = next;
    }
    *list = NULL;
    return true;
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

bool flist_sort(flist_node_t **list, f_list_node_cmp cmp_func)
{
    return sort_internal(list, cmp_func, false);
}

size_t flist_unique(flist_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func)
{
    return unique_internal(list, pred_func, del_func, false);
}

bool flist_reverse(flist_node_t **list)
{
    return reverse_internal(list, false);
}

// ============================================================================
// Doubly-linked list functions
// ============================================================================

bool list_push_front(list_node_t **list, list_node_t *node)
{
    return push_front_internal((flist_node_t **) list, (flist_node_t *) node, true);
}

bool list_pop_front(list_node_t **list, f_list_node_del del_func)
{
    return pop_front_internal((flist_node_t **) list, del_func, true);
}

bool list_push_back(list_node_t **list, list_node_t *node)
{
    return push_back_internal((flist_node_t **) list, (flist_node_t *) node, true);
}

bool list_pop_back(list_node_t **list, f_list_node_del del_func)
{
    return flist_pop_back((flist_node_t **) list, del_func);
}

bool list_insert_before(list_node_t **list, list_node_t *ref, list_node_t *node)
{
    if ((ref == NULL) || (node == NULL)) {
        return false;
    }
    if (ref->prev == NULL) {
        if ((list != NULL) && (*list == ref)) {
            return list_push_front(list, node);
        }
        return false;
    }
    return list_insert_after(list, ref->prev, node);
}

bool list_insert_after(list_node_t **list, list_node_t *ref, list_node_t *node)
{
    UNUSED(list);
    return insert_after_internal((flist_node_t *) ref, (flist_node_t *) node, true);
}

bool list_remove(list_node_t **list, list_node_t *node, f_list_node_del del_func)
{
    return remove_internal((flist_node_t **) list, (flist_node_t *) node, del_func, true);
}

size_t list_remove_if(list_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func)
{
    return remove_if_internal((flist_node_t **) list, pred_func, del_func, true);
}

bool list_clear(list_node_t **list, f_list_node_del del_func)
{
    return flist_clear((flist_node_t **) list, del_func);
}

size_t list_size(list_node_t *const *list)
{
    return flist_size((flist_node_t **) list);
}

bool list_empty(list_node_t *const *list)
{
    return list_size(list) == 0;
}

bool list_sort(list_node_t **list, f_list_node_cmp cmp_func)
{
    return sort_internal((flist_node_t **) list, cmp_func, true);
}

size_t list_unique(list_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func)
{
    return unique_internal((flist_node_t **) list, pred_func, del_func, true);
}

bool list_reverse(list_node_t **list)
{
    return reverse_internal((flist_node_t **) list, true);
}

#endif  // !defined(HAVE_ETH2) || defined(HAVE_SDK_LL_LIB)
