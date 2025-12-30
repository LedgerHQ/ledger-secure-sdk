#include "c_list.h"

/**
 * @brief Add a new node at the front of the list
 *
 * @param[in,out] list pointer to the list
 * @param[out] node new node to add
 */
void c_list_push_front(c_list_node_t **list, c_list_node_t *node)
{
    if ((list != NULL) && (node != NULL)) {
        node->next = *list;
        *list      = node;
    }
}

/**
 * @brief Remove the first node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 */
void c_list_pop_front(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp = NULL;

    if (list != NULL) {
        tmp = *list;
        if (tmp != NULL) {
            *list = tmp->next;
            if (func != NULL) {
                func(tmp);
            }
        }
    }
}

/**
 * @brief Add a new node at the back of the list
 *
 * @param[in,out] list pointer to the list
 * @param[in,out] node new node to add
 */
void c_list_push_back(c_list_node_t **list, c_list_node_t *node)
{
    c_list_node_t *tmp = NULL;

    if ((list != NULL) && (node != NULL)) {
        if (*list == NULL) {
            *list = node;
        }
        else {
            tmp = *list;
            if (tmp != NULL) {
                for (; tmp->next != NULL; tmp = tmp->next)
                    ;
                tmp->next = node;
            }
        }
    }
}

/**
 * @brief Remove the last node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 */
void c_list_pop_back(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp = NULL;

    if (list != NULL) {
        tmp = *list;
        if (tmp != NULL) {
            // only one element
            if (tmp->next == NULL) {
                c_list_pop_front(list, func);
            }
            else {
                for (; tmp->next->next != NULL; tmp = tmp->next)
                    ;
                if (func != NULL) {
                    func(tmp->next);
                }
                tmp->next = NULL;
            }
        }
    }
}

/**
 * @brief Insert a new node after a given list node (reference)
 *
 * @param[in,out] ref reference node
 * @param[in,out] node new node to add
 */
void c_list_insert_after(c_list_node_t *ref, c_list_node_t *node)
{
    if ((ref != NULL) && (node != NULL)) {
        node->next = ref->next;
        ref->next  = node;
    }
}

/**
 * @brief Remove a given node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[out] node node to remove
 * @param[in] func pointer to the node deletion function
 */
void c_list_remove(c_list_node_t **list, c_list_node_t *node, f_list_node_del func)
{
    c_list_node_t *it  = NULL;
    c_list_node_t *tmp = NULL;

    if ((list != NULL) && (node != NULL)) {
        if (node == *list) {
            // first element
            c_list_pop_front(list, func);
        }
        else {
            it = *list;
            if (it != NULL) {
                for (; (it->next != node) && (it->next != NULL); it = it->next)
                    ;
                if (it->next == NULL) {
                    // node not found
                    return;
                }
                tmp = it->next->next;
                if (func != NULL) {
                    func(it->next);
                }
                it->next = tmp;
            }
        }
    }
}

/**
 * @brief Remove all nodes from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 */
void c_list_clear(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp  = NULL;
    c_list_node_t *next = NULL;

    if (list != NULL) {
        tmp = *list;
        while (tmp != NULL) {
            next = tmp->next;
            if (func != NULL) {
                func(tmp);
            }
            tmp = next;
        }
        *list = NULL;
    }
}

/**
 * @brief Sort the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node comparison function
 */
void c_list_sort(c_list_node_t **list, f_list_node_cmp func)
{
    c_list_node_t **tmp    = NULL;
    c_list_node_t  *a      = NULL;
    c_list_node_t  *b      = NULL;
    bool            sorted = false;

    if ((list != NULL) && (func != NULL)) {
        do {
            sorted = true;
            for (tmp = list; (*tmp != NULL) && ((*tmp)->next != NULL); tmp = &(*tmp)->next) {
                a = *tmp;
                b = a->next;
                if (func(a, b) == false) {
                    *tmp    = b;
                    a->next = b->next;
                    b->next = a;
                    sorted  = false;
                }
            }
        } while (!sorted);
    }
}

/**
 * @brief Get the list size
 *
 * @param[in] list pointer to the list
 */
size_t c_list_size(c_list_node_t *const *list)
{
    size_t size = 0;

    if (list != NULL) {
        for (c_list_node_t *tmp = *list; tmp != NULL; tmp = tmp->next) {
            size += 1;
        }
    }
    return size;
}
