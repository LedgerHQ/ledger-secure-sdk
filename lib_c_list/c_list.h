#pragma once

#include <stdlib.h>
#include <stdbool.h>

/*
 * Chained list generic node
 */
typedef struct c_list_node_s {
    struct c_list_node_s *next;
} c_list_node_t;

// Function pointer types
typedef void (*f_list_node_del)(c_list_node_t *node);
typedef bool (*f_list_node_cmp)(const c_list_node_t *a, const c_list_node_t *b);

void   c_list_push_front(c_list_node_t **list, c_list_node_t *node);
void   c_list_pop_front(c_list_node_t **list, f_list_node_del func);
void   c_list_push_back(c_list_node_t **list, c_list_node_t *node);
void   c_list_pop_back(c_list_node_t **list, f_list_node_del func);
void   c_list_insert_after(c_list_node_t *ref, c_list_node_t *node);
void   c_list_remove(c_list_node_t **list, c_list_node_t *node, f_list_node_del func);
void   c_list_clear(c_list_node_t **list, f_list_node_del func);
void   c_list_sort(c_list_node_t **list, f_list_node_cmp func);
size_t c_list_size(c_list_node_t *const *list);
