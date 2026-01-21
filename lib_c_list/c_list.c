/**
 * @file
 * @brief Generic linked list implementation
 *
 * Provides both singly-linked (c_flist) and doubly-linked (c_list) implementations.
 */

#include "c_list.h"
#include "os_helpers.h"

// ============================================================================
// Internal shared functions with strict validation
// ============================================================================

static bool push_front_internal(c_flist_node_t **list, c_flist_node_t *node, bool doubly_linked)
{
    if ((list == NULL) || (node == NULL)) {
        return false;
    }
    node->next = *list;
    *list      = node;
    if (doubly_linked) {
        if (node->next != NULL) {
            ((c_dlist_node_t *) node->next)->prev = (c_dlist_node_t *) node;
        }
        ((c_dlist_node_t *) node)->prev = NULL;
    }
    return true;
}

static bool pop_front_internal(c_flist_node_t **list, c_list_node_del del_func, bool doubly_linked)
{
    c_flist_node_t *tmp;

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
            (*(c_dlist_node_t **) list)->prev = NULL;
        }
    }
    return true;
}

static bool push_back_internal(c_flist_node_t **list, c_flist_node_t *node, bool doubly_linked)
{
    c_flist_node_t *tmp;

    if ((list == NULL) || (node == NULL)) {
        return false;
    }
    node->next = NULL;
    if (*list == NULL) {
        if (doubly_linked) {
            ((c_dlist_node_t *) node)->prev = NULL;
        }
        *list = node;
    }
    else {
        tmp = *list;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        if (doubly_linked) {
            ((c_dlist_node_t *) node)->prev = (c_dlist_node_t *) tmp;
        }
        tmp->next = node;
    }
    return true;
}

static bool insert_after_internal(c_flist_node_t *ref, c_flist_node_t *node, bool doubly_linked)
{
    if ((ref == NULL) || (node == NULL)) {
        return false;
    }
    if (doubly_linked) {
        if (ref->next != NULL) {
            ((c_dlist_node_t *) (ref->next))->prev = (c_dlist_node_t *) node;
        }
        ((c_dlist_node_t *) node)->prev = (c_dlist_node_t *) ref;
    }
    node->next = ref->next;
    ref->next  = node;
    return true;
}

static bool remove_internal(c_flist_node_t **list,
                            c_flist_node_t  *node,
                            c_list_node_del  del_func,
                            bool             doubly_linked)
{
    c_flist_node_t *it;
    c_flist_node_t *tmp;

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
            ((c_dlist_node_t *) tmp)->prev = ((c_dlist_node_t *) node)->prev;
        }
    }
    if (del_func != NULL) {
        del_func(it->next);
    }
    it->next = tmp;
    return true;
}

static size_t remove_if_internal(c_flist_node_t **list,
                                 c_list_node_pred pred_func,
                                 c_list_node_del  del_func,
                                 bool             doubly_linked)
{
    c_flist_node_t *node;
    c_flist_node_t *tmp;
    size_t          count = 0;

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

static bool sort_internal(c_flist_node_t **list, c_list_node_cmp cmp_func, bool doubly_linked)
{
    c_flist_node_t **tmp;
    c_flist_node_t  *a, *b;
    bool             sorted;

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
                    ((c_dlist_node_t *) b)->prev = ((c_dlist_node_t *) a)->prev;
                    ((c_dlist_node_t *) a)->prev = (c_dlist_node_t *) b;
                    if (a->next != NULL) {
                        ((c_dlist_node_t *) a->next)->prev = (c_dlist_node_t *) a;
                    }
                }
                sorted = false;
            }
        }
    } while (!sorted);
    return true;
}

static size_t unique_internal(c_flist_node_t     **list,
                              c_list_node_bin_pred pred_func,
                              c_list_node_del      del_func,
                              bool                 doubly_linked)
{
    size_t count = 0;

    if ((list == NULL) || (pred_func == NULL)) {
        return 0;
    }
    for (c_flist_node_t *ref = *list; ref != NULL; ref = ref->next) {
        c_flist_node_t *node = ref->next;
        while ((node != NULL) && (pred_func(ref, node))) {
            c_flist_node_t *tmp = node->next;
            if (remove_internal(list, node, del_func, doubly_linked)) {
                count += 1;
            }
            node = tmp;
        }
    }
    return count;
}

static bool reverse_internal(c_flist_node_t **list, bool doubly_linked)
{
    c_flist_node_t *node;
    c_flist_node_t *prev = NULL;
    c_flist_node_t *next;

    if (list == NULL) {
        return false;
    }
    node = *list;
    while (node != NULL) {
        next       = node->next;
        node->next = prev;
        if (doubly_linked) {
            ((c_dlist_node_t *) node)->prev = (c_dlist_node_t *) next;
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

bool c_flist_push_front(c_flist_node_t **list, c_flist_node_t *node)
{
    return push_front_internal(list, node, false);
}

bool c_flist_pop_front(c_flist_node_t **list, c_list_node_del del_func)
{
    return pop_front_internal(list, del_func, false);
}

bool c_flist_push_back(c_flist_node_t **list, c_flist_node_t *node)
{
    return push_back_internal(list, node, false);
}

bool c_flist_pop_back(c_flist_node_t **list, c_list_node_del del_func)
{
    c_flist_node_t *tmp;

    if ((list == NULL) || (*list == NULL)) {
        return false;
    }
    tmp = *list;
    // only one element
    if (tmp->next == NULL) {
        return c_flist_pop_front(list, del_func);
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

bool c_flist_insert_after(c_flist_node_t **list, c_flist_node_t *ref, c_flist_node_t *node)
{
    UNUSED(list);
    return insert_after_internal(ref, node, false);
}

bool c_flist_remove(c_flist_node_t **list, c_flist_node_t *node, c_list_node_del del_func)
{
    return remove_internal(list, node, del_func, false);
}

size_t c_flist_remove_if(c_flist_node_t **list,
                         c_list_node_pred pred_func,
                         c_list_node_del  del_func)
{
    return remove_if_internal(list, pred_func, del_func, false);
}

bool c_flist_clear(c_flist_node_t **list, c_list_node_del del_func)
{
    c_flist_node_t *tmp;
    c_flist_node_t *next;

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

size_t c_flist_size(c_flist_node_t *const *list)
{
    size_t size = 0;

    if (list != NULL) {
        for (c_flist_node_t *tmp = *list; tmp != NULL; tmp = tmp->next) {
            size += 1;
        }
    }
    return size;
}

bool c_flist_empty(c_flist_node_t *const *list)
{
    return c_flist_size(list) == 0;
}

bool c_flist_sort(c_flist_node_t **list, c_list_node_cmp cmp_func)
{
    return sort_internal(list, cmp_func, false);
}

size_t c_flist_unique(c_flist_node_t     **list,
                      c_list_node_bin_pred pred_func,
                      c_list_node_del      del_func)
{
    return unique_internal(list, pred_func, del_func, false);
}

bool c_flist_reverse(c_flist_node_t **list)
{
    return reverse_internal(list, false);
}

// ============================================================================
// Doubly-linked list functions
// ============================================================================

bool c_dlist_push_front(c_dlist_node_t **list, c_dlist_node_t *node)
{
    return push_front_internal((c_flist_node_t **) list, (c_flist_node_t *) node, true);
}

bool c_dlist_pop_front(c_dlist_node_t **list, c_list_node_del del_func)
{
    return pop_front_internal((c_flist_node_t **) list, del_func, true);
}

bool c_dlist_push_back(c_dlist_node_t **list, c_dlist_node_t *node)
{
    return push_back_internal((c_flist_node_t **) list, (c_flist_node_t *) node, true);
}

bool c_dlist_pop_back(c_dlist_node_t **list, c_list_node_del del_func)
{
    return c_flist_pop_back((c_flist_node_t **) list, del_func);
}

bool c_dlist_insert_before(c_dlist_node_t **list, c_dlist_node_t *ref, c_dlist_node_t *node)
{
    if ((ref == NULL) || (node == NULL)) {
        return false;
    }
    if (ref->prev == NULL) {
        if ((list != NULL) && (*list == ref)) {
            return c_dlist_push_front(list, node);
        }
        return false;
    }
    return c_dlist_insert_after(list, ref->prev, node);
}

bool c_dlist_insert_after(c_dlist_node_t **list, c_dlist_node_t *ref, c_dlist_node_t *node)
{
    UNUSED(list);
    return insert_after_internal((c_flist_node_t *) ref, (c_flist_node_t *) node, true);
}

bool c_dlist_remove(c_dlist_node_t **list, c_dlist_node_t *node, c_list_node_del del_func)
{
    return remove_internal((c_flist_node_t **) list, (c_flist_node_t *) node, del_func, true);
}

size_t c_dlist_remove_if(c_dlist_node_t **list,
                         c_list_node_pred pred_func,
                         c_list_node_del  del_func)
{
    return remove_if_internal((c_flist_node_t **) list, pred_func, del_func, true);
}

bool c_dlist_clear(c_dlist_node_t **list, c_list_node_del del_func)
{
    return c_flist_clear((c_flist_node_t **) list, del_func);
}

size_t c_dlist_size(c_dlist_node_t *const *list)
{
    return c_flist_size((c_flist_node_t **) list);
}

bool c_dlist_empty(c_dlist_node_t *const *list)
{
    return c_dlist_size(list) == 0;
}

bool c_dlist_sort(c_dlist_node_t **list, c_list_node_cmp cmp_func)
{
    return sort_internal((c_flist_node_t **) list, cmp_func, true);
}

size_t c_dlist_unique(c_dlist_node_t     **list,
                      c_list_node_bin_pred pred_func,
                      c_list_node_del      del_func)
{
    return unique_internal((c_flist_node_t **) list, pred_func, del_func, true);
}

bool c_dlist_reverse(c_dlist_node_t **list)
{
    return reverse_internal((c_flist_node_t **) list, true);
}
