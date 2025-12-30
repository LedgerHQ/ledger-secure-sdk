#include "c_list.h"

/**
 * @brief Add a new node at the front of the list
 *
 * @param[in,out] list pointer to the list
 * @param[out] node new node to add
 * @return true on success, false on error
 */
bool c_list_push_front(c_list_node_t **list, c_list_node_t *node)
{
    if ((list == NULL) || (node == NULL) || (node->next != NULL)) {
        return false;
    }
    node->next = *list;
    *list      = node;
    return true;
}

/**
 * @brief Remove the first node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 * @return true on success, false if list is empty or NULL
 */
bool c_list_pop_front(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp = NULL;

    if ((list == NULL) || (*list == NULL)) {
        return false;
    }
    tmp   = *list;
    *list = tmp->next;
    if (func != NULL) {
        func(tmp);
    }
    return true;
}

/**
 * @brief Add a new node at the back of the list
 *
 * @param[in,out] list pointer to the list
 * @param[in,out] node new node to add
 * @return true on success, false on error
 */
bool c_list_push_back(c_list_node_t **list, c_list_node_t *node)
{
    c_list_node_t *tmp = NULL;

    if ((list == NULL) || (node == NULL) || (node->next != NULL)) {
        return false;
    }
    if (*list == NULL) {
        *list = node;
    }
    else {
        tmp = *list;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = node;
    }
    return true;
}

/**
 * @brief Remove the last node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 * @return true on success, false if list is empty or NULL
 */
bool c_list_pop_back(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp = NULL;

    if ((list == NULL) || (*list == NULL)) {
        return false;
    }
    tmp = *list;
    // only one element
    if (tmp->next == NULL) {
        return c_list_pop_front(list, func);
    }
    while (tmp->next->next != NULL) {
        tmp = tmp->next;
    }
    if (func != NULL) {
        func(tmp->next);
    }
    tmp->next = NULL;
    return true;
}

/**
 * @brief Insert a new node after a given list node (reference)
 *
 * @param[in,out] ref reference node
 * @param[in,out] node new node to add
 * @return true on success, false on error
 */
bool c_list_insert_after(c_list_node_t *ref, c_list_node_t *node)
{
    if ((ref == NULL) || (node == NULL) || (node->next != NULL)) {
        return false;
    }
    node->next = ref->next;
    ref->next  = node;
    return true;
}

/**
 * @brief Insert a new node before a given list node (reference)
 *
 * @param[in,out] list pointer to the list
 * @param[in,out] ref reference node
 * @param[in,out] node new node to add
 * @return true on success, false on error
 */
bool c_list_insert_before(c_list_node_t **list, c_list_node_t *ref, c_list_node_t *node)
{
    c_list_node_t *it = NULL;

    if ((list == NULL) || (*list == NULL) || (ref == NULL) || (node == NULL)
        || (node->next != NULL)) {
        return false;
    }
    if (*list == ref) {
        // insert at front
        return c_list_push_front(list, node);
    }
    it = *list;
    while ((it->next != ref) && (it->next != NULL)) {
        it = it->next;
    }
    if (it->next != ref) {
        // ref not found in list
        return false;
    }
    node->next = ref;
    it->next   = node;
    return true;
}

/**
 * @brief Remove a given node from the list
 *
 * @param[in,out] list pointer to the list
 * @param[out] node node to remove
 * @param[in] func pointer to the node deletion function
 * @return true on success, false if node not found or invalid parameters
 */
bool c_list_remove(c_list_node_t **list, c_list_node_t *node, f_list_node_del func)
{
    c_list_node_t *it  = NULL;
    c_list_node_t *tmp = NULL;

    if ((list == NULL) || (node == NULL) || (*list == NULL)) {
        return false;
    }
    if (node == *list) {
        // first element
        return c_list_pop_front(list, func);
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
    if (func != NULL) {
        func(it->next);
    }
    it->next = tmp;
    return true;
}

/**
 * @brief Remove all nodes from the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node deletion function
 * @return true on success, false if list is NULL
 */
bool c_list_clear(c_list_node_t **list, f_list_node_del func)
{
    c_list_node_t *tmp  = NULL;
    c_list_node_t *next = NULL;

    if (list == NULL) {
        return false;
    }
    tmp = *list;
    while (tmp != NULL) {
        next = tmp->next;
        if (func != NULL) {
            func(tmp);
        }
        tmp = next;
    }
    *list = NULL;
    return true;
}

/**
 * @brief Sort the list
 *
 * @param[in,out] list pointer to the list
 * @param[in] func pointer to the node comparison function
 * @return true on success, false if invalid parameters
 */
bool c_list_sort(c_list_node_t **list, f_list_node_cmp func)
{
    c_list_node_t **tmp    = NULL;
    c_list_node_t  *a      = NULL;
    c_list_node_t  *b      = NULL;
    bool            sorted = false;

    if ((list == NULL) || (func == NULL)) {
        return false;
    }
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
    return true;
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
