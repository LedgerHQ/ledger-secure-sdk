/**
 * @file
 * @brief Generic linked list implementation (singly and doubly-linked)
 *
 * This file provides both forward lists (singly-linked) and doubly-linked lists.
 * Based on app-ethereum implementation.
 */

#pragma once

#include <stdlib.h>
#include <stdbool.h>

/**
 * @struct flist_node_t
 * @brief Forward list node structure (singly-linked)
 *
 * This structure represents a node in a forward list (singly-linked list).
 * It contains only a pointer to the next node, making it memory-efficient (4-8 bytes per node).
 *
 * @note Memory footprint: 4 bytes (32-bit) or 8 bytes (64-bit)
 * @see list_node_t for doubly-linked list with backward traversal support
 */
typedef struct flist_node_t {
    struct flist_node_t *next; /**< Pointer to the next node (NULL if last) */
} flist_node_t;

/**
 * @struct list_node_t
 * @brief Doubly-linked list node structure
 *
 * This structure represents a node in a doubly-linked list.
 * It embeds a flist_node_t and adds a previous pointer for bidirectional traversal.
 *
 * @note Memory footprint: 8 bytes (32-bit) or 16 bytes (64-bit)
 * @note The _list member provides forward traversal compatibility
 * @see flist_node_t for lighter singly-linked alternative
 */
typedef struct list_node_t {
    flist_node_t        _list; /**< Forward list node (contains next pointer) */
    struct list_node_t *prev;  /**< Pointer to the previous node (NULL if first) */
} list_node_t;

/**
 * @typedef f_list_node_del
 * @brief Callback function to delete/free a node
 *
 * This function is called when a node needs to be deleted from the list.
 * It should free any resources associated with the node and the node itself.
 *
 * @param[in] node The node to delete (never NULL when called by list functions)
 *
 * @note If NULL is passed to list functions, nodes are removed but not freed
 * @note This function should handle the complete node lifecycle cleanup
 */
typedef void (*f_list_node_del)(flist_node_t *node);

/**
 * @typedef f_list_node_cmp
 * @brief Callback function to compare two nodes for sorting
 *
 * This function is used by flist_sort() and list_sort() to determine node order.
 * It should return true if node 'a' should come before node 'b' in the sorted list.
 *
 * @param[in] a First node to compare
 * @param[in] b Second node to compare
 * @return true if a < b (a should come before b), false otherwise
 *
 * @note Both parameters are never NULL when called by list functions
 * @note Use consistent comparison logic for stable sorting
 */
typedef bool (*f_list_node_cmp)(const flist_node_t *a, const flist_node_t *b);

/**
 * @typedef f_list_node_pred
 * @brief Callback function to test a single node (unary predicate)
 *
 * This function is used by flist_remove_if() and list_remove_if() to determine
 * which nodes should be removed from the list.
 *
 * @param[in] node The node to test
 * @return true if the node matches the condition (should be removed), false otherwise
 *
 * @note The parameter is never NULL when called by list functions
 */
typedef bool (*f_list_node_pred)(const flist_node_t *node);

/**
 * @typedef f_list_node_bin_pred
 * @brief Callback function to test two nodes for equality (binary predicate)
 *
 * This function is used by flist_unique() and list_unique() to determine
 * if two consecutive nodes are considered equal and should be deduplicated.
 *
 * @param[in] a First node to compare
 * @param[in] b Second node to compare
 * @return true if nodes are equal (b should be removed), false otherwise
 *
 * @note Both parameters are never NULL when called by list functions
 * @note Typically used after sorting to remove consecutive duplicates
 */
typedef bool (*f_list_node_bin_pred)(const flist_node_t *a, const flist_node_t *b);

// ============================================================================
// Forward list (singly-linked) functions
// ============================================================================

/**
 * @brief Add a node at the beginning of the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to add (must have node->next == NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void flist_push_front(flist_node_t **list, flist_node_t *node);

/**
 * @brief Remove and delete the first node from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void flist_pop_front(flist_node_t **list, f_list_node_del del_func);

/**
 * @brief Add a node at the end of the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to add (must have node->next == NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void flist_push_back(flist_node_t **list, flist_node_t *node);

/**
 * @brief Remove and delete the last node from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void flist_pop_back(flist_node_t **list, f_list_node_del del_func);

/**
 * @brief Insert a node after a reference node in the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] ref Reference node (must be in list)
 * @param[in] node Node to insert (must have node->next == NULL)
 * @note If @p ref or @p node is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void flist_insert_after(flist_node_t **list, flist_node_t *ref, flist_node_t *node);

/**
 * @brief Remove and delete a specific node from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to remove (must be in list)
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void flist_remove(flist_node_t **list, flist_node_t *node, f_list_node_del del_func);

/**
 * @brief Remove all nodes matching a predicate from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] pred_func Predicate function to test each node
 * @param[in] del_func Function to delete removed nodes (can be NULL)
 * @return Number of nodes removed
 * @note Time complexity: O(n)
 */
size_t flist_remove_if(flist_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func);

/**
 * @brief Remove and delete all nodes from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete each node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void flist_clear(flist_node_t **list, f_list_node_del del_func);

/**
 * @brief Get the number of nodes in the forward list
 * @param[in] list Pointer to the list head
 * @return Number of nodes (0 if list is NULL or empty)
 * @note Time complexity: O(n)
 */
size_t flist_size(flist_node_t *const *list);

/**
 * @brief Check if the forward list is empty
 * @param[in] list Pointer to the list head
 * @return true if empty or NULL, false otherwise
 * @note Time complexity: O(1)
 */
bool flist_empty(flist_node_t *const *list);

/**
 * @brief Sort the forward list using a comparison function
 * @param[in,out] list Pointer to the list head
 * @param[in] cmp_func Comparison function (returns true if a < b)
 * @note If @p list or @p cmp_func is NULL, the function performs no action.
 * @note Time complexity: O(n²) - bubble sort algorithm
 */
void flist_sort(flist_node_t **list, f_list_node_cmp cmp_func);

/**
 * @brief Remove consecutive duplicate nodes from the forward list
 * @param[in,out] list Pointer to the list head
 * @param[in] pred_func Binary predicate to test equality (returns true if a == b)
 * @param[in] del_func Function to delete removed nodes (can be NULL)
 * @return Number of nodes removed
 * @note Time complexity: O(n)
 * @note List should be sorted first for best results
 */
size_t flist_unique(flist_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func);

/**
 * @brief Reverse the order of nodes in the forward list
 * @param[in,out] list Pointer to the list head
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void flist_reverse(flist_node_t **list);

// ============================================================================
// Doubly-linked list functions
// ============================================================================

/**
 * @brief Add a node at the beginning of the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to add (must have node->_list.next == NULL and node->prev == NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void list_push_front(list_node_t **list, list_node_t *node);

/**
 * @brief Remove and delete the first node from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void list_pop_front(list_node_t **list, f_list_node_del del_func);

/**
 * @brief Add a node at the end of the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to add (must have node->_list.next == NULL and node->prev == NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void list_push_back(list_node_t **list, list_node_t *node);

/**
 * @brief Remove and delete the last node from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void list_pop_back(list_node_t **list, f_list_node_del del_func);

/**
 * @brief Insert a node before a reference node in the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] ref Reference node (must be in list)
 * @param[in] node Node to insert (must have node->_list.next == NULL and node->prev == NULL)
 * @note If @p list or @p ref or @p node is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void list_insert_before(list_node_t **list, list_node_t *ref, list_node_t *node);

/**
 * @brief Insert a node after a reference node in the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] ref Reference node (must be in list)
 * @param[in] node Node to insert (must have node->_list.next == NULL and node->prev == NULL)
 * @note If @p ref or @p node is NULL, the function performs no action.
 * @note Time complexity: O(1)
 */
void list_insert_after(list_node_t **list, list_node_t *ref, list_node_t *node);

/**
 * @brief Remove and delete a specific node from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] node Node to remove (must be in list)
 * @param[in] del_func Function to delete the node (can be NULL)
 * @note If @p list or @p node is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void list_remove(list_node_t **list, list_node_t *node, f_list_node_del del_func);

/**
 * @brief Remove all nodes matching a predicate from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] pred_func Predicate function to test each node
 * @param[in] del_func Function to delete removed nodes (can be NULL)
 * @return Number of nodes removed
 * @note Time complexity: O(n)
 */
size_t list_remove_if(list_node_t **list, f_list_node_pred pred_func, f_list_node_del del_func);

/**
 * @brief Remove and delete all nodes from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] del_func Function to delete each node (can be NULL)
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void list_clear(list_node_t **list, f_list_node_del del_func);

/**
 * @brief Get the number of nodes in the doubly-linked list
 * @param[in] list Pointer to the list head
 * @return Number of nodes (0 if list is NULL or empty)
 * @note Time complexity: O(n)
 */
size_t list_size(list_node_t *const *list);

/**
 * @brief Check if the doubly-linked list is empty
 * @param[in] list Pointer to the list head
 * @return true if empty or NULL, false otherwise
 * @note Time complexity: O(1)
 */
bool list_empty(list_node_t *const *list);

/**
 * @brief Sort the doubly-linked list using a comparison function
 * @param[in,out] list Pointer to the list head
 * @param[in] cmp_func Comparison function (returns true if a < b)
 * @note If @p list or @p cmp_func is NULL, the function performs no action.
 * @note Time complexity: O(n²) - bubble sort algorithm
 */
void list_sort(list_node_t **list, f_list_node_cmp cmp_func);

/**
 * @brief Remove consecutive duplicate nodes from the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @param[in] pred_func Binary predicate to test equality (returns true if a == b)
 * @param[in] del_func Function to delete removed nodes (can be NULL)
 * @return Number of nodes removed
 * @note Time complexity: O(n)
 * @note List should be sorted first for best results
 */
size_t list_unique(list_node_t **list, f_list_node_bin_pred pred_func, f_list_node_del del_func);

/**
 * @brief Reverse the order of nodes in the doubly-linked list
 * @param[in,out] list Pointer to the list head
 * @note If @p list is NULL, the function performs no action.
 * @note Time complexity: O(n)
 */
void list_reverse(list_node_t **list);
