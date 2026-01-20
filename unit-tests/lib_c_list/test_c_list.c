#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>

#include "c_list.h"

// ============================================================================
// Forward list test structures and helpers
// ============================================================================

typedef struct test_flist_node_s {
    c_flist_node_t node;
    int            value;
} test_flist_node_t;

static test_flist_node_t *create_flist_node(int value)
{
    test_flist_node_t *node = malloc(sizeof(test_flist_node_t));
    assert_non_null(node);
    node->node.next = NULL;
    node->value     = value;
    return node;
}

static void delete_flist_node(c_flist_node_t *node)
{
    if (node != NULL) {
        free(node);
    }
}

// ============================================================================
// Doubly-linked list test structures and helpers
// ============================================================================

typedef struct test_dlist_node_s {
    c_dlist_node_t node;
    int            value;
} test_dlist_node_t;

static test_dlist_node_t *create_dlist_node(int value)
{
    test_dlist_node_t *node = malloc(sizeof(test_dlist_node_t));
    assert_non_null(node);
    node->node._list.next = NULL;
    node->node.prev       = NULL;
    node->value           = value;
    return node;
}

static void delete_dlist_node(c_flist_node_t *node)
{
    if (node != NULL) {
        free(node);
    }
}

// ============================================================================
// Comparison and predicate functions
// ============================================================================

static bool compare_ascending_flist(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const test_flist_node_t *node_a = (const test_flist_node_t *) a;
    const test_flist_node_t *node_b = (const test_flist_node_t *) b;
    return node_a->value <= node_b->value;
}

static bool are_equal_flist(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const test_flist_node_t *node_a = (const test_flist_node_t *) a;
    const test_flist_node_t *node_b = (const test_flist_node_t *) b;
    return node_a->value == node_b->value;
}

static bool is_negative_flist(const c_flist_node_t *node)
{
    const test_flist_node_t *test_node = (const test_flist_node_t *) node;
    return test_node->value < 0;
}

// Doubly-linked list comparison and predicate functions
static bool compare_ascending_dlist(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const test_dlist_node_t *node_a = (const test_dlist_node_t *) a;
    const test_dlist_node_t *node_b = (const test_dlist_node_t *) b;
    return node_a->value <= node_b->value;
}

static bool are_equal_dlist(const c_flist_node_t *a, const c_flist_node_t *b)
{
    const test_dlist_node_t *node_a = (const test_dlist_node_t *) a;
    const test_dlist_node_t *node_b = (const test_dlist_node_t *) b;
    return node_a->value == node_b->value;
}

static bool is_even_dlist(const c_flist_node_t *node)
{
    const test_dlist_node_t *test_node = (const test_dlist_node_t *) node;
    return (test_node->value % 2) == 0;
}

// ============================================================================
// Forward list tests
// ============================================================================

// Test: flist push_front
static void test_flist_push_front(void **state)
{
    (void) state;
    c_flist_node_t    *list = NULL;
    test_flist_node_t *node1, *node2, *node3;

    node1 = create_flist_node(1);
    assert_true(c_flist_push_front(&list, &node1->node));
    assert_ptr_equal(list, &node1->node);
    assert_int_equal(c_flist_size(&list), 1);

    node2 = create_flist_node(2);
    assert_true(c_flist_push_front(&list, &node2->node));
    assert_ptr_equal(list, &node2->node);
    assert_int_equal(c_flist_size(&list), 2);

    node3 = create_flist_node(3);
    assert_true(c_flist_push_front(&list, &node3->node));
    assert_ptr_equal(list, &node3->node);
    assert_int_equal(c_flist_size(&list), 3);

    // Verify order: 3 -> 2 -> 1
    test_flist_node_t *current = (test_flist_node_t *) list;
    assert_int_equal(current->value, 3);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 1);

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist push_back
static void test_flist_push_back(void **state)
{
    (void) state;
    c_flist_node_t    *list = NULL;
    test_flist_node_t *node1, *node2, *node3;

    node1 = create_flist_node(1);
    assert_true(c_flist_push_back(&list, &node1->node));
    assert_int_equal(c_flist_size(&list), 1);

    node2 = create_flist_node(2);
    assert_true(c_flist_push_back(&list, &node2->node));
    assert_int_equal(c_flist_size(&list), 2);

    node3 = create_flist_node(3);
    assert_true(c_flist_push_back(&list, &node3->node));
    assert_int_equal(c_flist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist pop_front
static void test_flist_pop_front(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    c_flist_push_back(&list, &node1->node);
    c_flist_push_back(&list, &node2->node);
    c_flist_push_back(&list, &node3->node);

    assert_true(c_flist_pop_front(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 2);
    assert_int_equal(((test_flist_node_t *) list)->value, 2);

    assert_true(c_flist_pop_front(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 1);
    assert_int_equal(((test_flist_node_t *) list)->value, 3);

    assert_true(c_flist_pop_front(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 0);
    assert_null(list);

    assert_false(c_flist_pop_front(&list, delete_flist_node));
}

// Test: flist pop_back
static void test_flist_pop_back(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    c_flist_push_back(&list, &node1->node);
    c_flist_push_back(&list, &node2->node);
    c_flist_push_back(&list, &node3->node);

    assert_true(c_flist_pop_back(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 2);

    assert_true(c_flist_pop_back(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 1);
    assert_int_equal(((test_flist_node_t *) list)->value, 1);

    assert_true(c_flist_pop_back(&list, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 0);
    assert_null(list);

    assert_false(c_flist_pop_back(&list, delete_flist_node));
}

// Test: flist insert_after
static void test_flist_insert_after(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node3 = create_flist_node(3);
    c_flist_push_back(&list, &node1->node);
    c_flist_push_back(&list, &node3->node);

    test_flist_node_t *node2 = create_flist_node(2);
    assert_true(c_flist_insert_after(&list, &node1->node, &node2->node));
    assert_int_equal(c_flist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist remove
static void test_flist_remove(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    c_flist_push_back(&list, &node1->node);
    c_flist_push_back(&list, &node2->node);
    c_flist_push_back(&list, &node3->node);

    assert_true(c_flist_remove(&list, &node2->node, delete_flist_node));
    assert_int_equal(c_flist_size(&list), 2);

    // Verify order: 1 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist remove_if
static void test_flist_remove_if(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    // Create list: -2, -1, 0, 1, 2, 3
    int values[] = {-2, -1, 0, 1, 2, 3};
    for (int i = 0; i < 6; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        c_flist_push_back(&list, &node->node);
    }

    // Remove all negative values
    size_t removed = c_flist_remove_if(&list, is_negative_flist, delete_flist_node);
    assert_int_equal(removed, 2);
    assert_int_equal(c_flist_size(&list), 4);

    // Verify remaining: 0, 1, 2, 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 0; i < 4; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist unique
static void test_flist_unique(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    // Create list with duplicates: 1, 1, 2, 2, 2, 3, 3
    int values[] = {1, 1, 2, 2, 2, 3, 3};
    for (int i = 0; i < 7; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        c_flist_push_back(&list, &node->node);
    }

    size_t removed = c_flist_unique(&list, are_equal_flist, delete_flist_node);
    assert_int_equal(removed, 4);  // Removed 4 duplicates
    assert_int_equal(c_flist_size(&list), 3);

    // Verify remaining: 1, 2, 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 1; i <= 3; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist reverse
static void test_flist_reverse(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_flist_node_t *node = create_flist_node(i);
        c_flist_push_back(&list, &node->node);
    }

    assert_true(c_flist_reverse(&list));

    // Verify reversed: 5, 4, 3, 2, 1
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 5; i >= 1; i--) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    c_flist_clear(&list, delete_flist_node);
}

// Test: flist empty
static void test_flist_empty(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    assert_true(c_flist_empty(&list));

    test_flist_node_t *node = create_flist_node(1);
    c_flist_push_front(&list, &node->node);
    assert_false(c_flist_empty(&list));

    c_flist_clear(&list, delete_flist_node);
    assert_true(c_flist_empty(&list));
}

// Test: flist sort
static void test_flist_sort(void **state)
{
    (void) state;
    c_flist_node_t *list = NULL;

    // Create unsorted list: 3, 1, 4, 2
    int values[] = {3, 1, 4, 2};
    for (int i = 0; i < 4; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        c_flist_push_back(&list, &node->node);
    }

    assert_true(c_flist_sort(&list, compare_ascending_flist));

    // Verify sorted: 1, 2, 3, 4
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 1; i <= 4; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    c_flist_clear(&list, delete_flist_node);
}

// ============================================================================
// Doubly-linked list tests
// ============================================================================

// Test: dlist push_front
static void test_dlist_push_front(void **state)
{
    (void) state;
    c_dlist_node_t    *list = NULL;
    test_dlist_node_t *node1, *node2, *node3;

    node1 = create_dlist_node(1);
    assert_true(c_dlist_push_front(&list, &node1->node));
    assert_ptr_equal(list, &node1->node);
    assert_int_equal(c_dlist_size(&list), 1);

    node2 = create_dlist_node(2);
    assert_true(c_dlist_push_front(&list, &node2->node));
    assert_ptr_equal(list, &node2->node);
    assert_int_equal(c_dlist_size(&list), 2);

    node3 = create_dlist_node(3);
    assert_true(c_dlist_push_front(&list, &node3->node));
    assert_ptr_equal(list, &node3->node);
    assert_int_equal(c_dlist_size(&list), 3);

    // Verify order: 3 -> 2 -> 1
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    assert_int_equal(current->value, 3);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 2);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 1);

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist push_back (O(1) - fast!)
static void test_dlist_push_back(void **state)
{
    (void) state;
    c_dlist_node_t    *list = NULL;
    test_dlist_node_t *node1, *node2, *node3;

    node1 = create_dlist_node(1);
    assert_true(c_dlist_push_back(&list, &node1->node));
    assert_int_equal(c_dlist_size(&list), 1);

    node2 = create_dlist_node(2);
    assert_true(c_dlist_push_back(&list, &node2->node));
    assert_int_equal(c_dlist_size(&list), 2);

    node3 = create_dlist_node(3);
    assert_true(c_dlist_push_back(&list, &node3->node));
    assert_int_equal(c_dlist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 2);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 3);

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist pop_front
static void test_dlist_pop_front(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    test_dlist_node_t *node1 = create_dlist_node(1);
    test_dlist_node_t *node2 = create_dlist_node(2);
    test_dlist_node_t *node3 = create_dlist_node(3);
    c_dlist_push_back(&list, &node1->node);
    c_dlist_push_back(&list, &node2->node);
    c_dlist_push_back(&list, &node3->node);

    assert_true(c_dlist_pop_front(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 2);
    assert_int_equal(((test_dlist_node_t *) list)->value, 2);

    assert_true(c_dlist_pop_front(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 1);
    assert_int_equal(((test_dlist_node_t *) list)->value, 3);

    assert_true(c_dlist_pop_front(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 0);
    assert_null(list);

    assert_false(c_dlist_pop_front(&list, delete_dlist_node));
}

// Test: dlist pop_back (O(1) - fast!)
static void test_dlist_pop_back(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    test_dlist_node_t *node1 = create_dlist_node(1);
    test_dlist_node_t *node2 = create_dlist_node(2);
    test_dlist_node_t *node3 = create_dlist_node(3);
    c_dlist_push_back(&list, &node1->node);
    c_dlist_push_back(&list, &node2->node);
    c_dlist_push_back(&list, &node3->node);

    assert_true(c_dlist_pop_back(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 2);

    assert_true(c_dlist_pop_back(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 1);
    assert_int_equal(((test_dlist_node_t *) list)->value, 1);

    assert_true(c_dlist_pop_back(&list, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 0);
    assert_null(list);

    assert_false(c_dlist_pop_back(&list, delete_dlist_node));
}

// Test: dlist insert_before (O(1) - unique to doubly-linked!)
static void test_dlist_insert_before(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    test_dlist_node_t *node1 = create_dlist_node(1);
    test_dlist_node_t *node3 = create_dlist_node(3);
    c_dlist_push_back(&list, &node1->node);
    c_dlist_push_back(&list, &node3->node);

    test_dlist_node_t *node2 = create_dlist_node(2);
    assert_true(c_dlist_insert_before(&list, &node3->node, &node2->node));
    assert_int_equal(c_dlist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 2);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 3);

    // Insert before head
    test_dlist_node_t *node0 = create_dlist_node(0);
    assert_true(c_dlist_insert_before(&list, &node1->node, &node0->node));
    assert_ptr_equal(list, &node0->node);

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist insert_after
static void test_dlist_insert_after(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    test_dlist_node_t *node1 = create_dlist_node(1);
    test_dlist_node_t *node3 = create_dlist_node(3);
    c_dlist_push_back(&list, &node1->node);
    c_dlist_push_back(&list, &node3->node);

    test_dlist_node_t *node2 = create_dlist_node(2);
    assert_true(c_dlist_insert_after(&list, &node1->node, &node2->node));
    assert_int_equal(c_dlist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 2);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 3);

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist remove (O(1) - fast!)
static void test_dlist_remove(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    test_dlist_node_t *node1 = create_dlist_node(1);
    test_dlist_node_t *node2 = create_dlist_node(2);
    test_dlist_node_t *node3 = create_dlist_node(3);
    c_dlist_push_back(&list, &node1->node);
    c_dlist_push_back(&list, &node2->node);
    c_dlist_push_back(&list, &node3->node);

    // Remove middle node (O(1))
    assert_true(c_dlist_remove(&list, &node2->node, delete_dlist_node));
    assert_int_equal(c_dlist_size(&list), 2);

    // Verify order: 1 -> 3
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_dlist_node_t *) current->node._list.next;
    assert_int_equal(current->value, 3);

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist remove_if
static void test_dlist_remove_if(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5, 6
    for (int i = 1; i <= 6; i++) {
        test_dlist_node_t *node = create_dlist_node(i);
        c_dlist_push_back(&list, &node->node);
    }

    // Remove all even values
    size_t removed = c_dlist_remove_if(&list, is_even_dlist, delete_dlist_node);
    assert_int_equal(removed, 3);  // Removed 2, 4, 6
    assert_int_equal(c_dlist_size(&list), 3);

    // Verify remaining: 1, 3, 5
    test_dlist_node_t *current    = (test_dlist_node_t *) list;
    int                expected[] = {1, 3, 5};
    for (int i = 0; i < 3; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, expected[i]);
        current = (test_dlist_node_t *) current->node._list.next;
    }

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist unique
static void test_dlist_unique(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    // Create list with duplicates: 1, 1, 2, 3, 3, 3, 4
    int values[] = {1, 1, 2, 3, 3, 3, 4};
    for (int i = 0; i < 7; i++) {
        test_dlist_node_t *node = create_dlist_node(values[i]);
        c_dlist_push_back(&list, &node->node);
    }

    size_t removed = c_dlist_unique(&list, are_equal_dlist, delete_dlist_node);
    assert_int_equal(removed, 3);  // Removed 3 duplicates
    assert_int_equal(c_dlist_size(&list), 4);

    // Verify remaining: 1, 2, 3, 4
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    for (int i = 1; i <= 4; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_dlist_node_t *) current->node._list.next;
    }

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist reverse
static void test_dlist_reverse(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_dlist_node_t *node = create_dlist_node(i);
        c_dlist_push_back(&list, &node->node);
    }

    assert_true(c_dlist_reverse(&list));

    // Verify reversed: 5, 4, 3, 2, 1
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    for (int i = 5; i >= 1; i--) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_dlist_node_t *) current->node._list.next;
    }

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist empty
static void test_dlist_empty(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    assert_true(c_dlist_empty(&list));

    test_dlist_node_t *node = create_dlist_node(1);
    c_dlist_push_front(&list, &node->node);
    assert_false(c_dlist_empty(&list));

    c_dlist_clear(&list, delete_dlist_node);
    assert_true(c_dlist_empty(&list));
}

// Test: dlist sort
static void test_dlist_sort(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    // Create unsorted list: 4, 1, 3, 2, 5
    int values[] = {4, 1, 3, 2, 5};
    for (int i = 0; i < 5; i++) {
        test_dlist_node_t *node = create_dlist_node(values[i]);
        c_dlist_push_back(&list, &node->node);
    }

    assert_true(c_dlist_sort(&list, compare_ascending_dlist));

    // Verify sorted: 1, 2, 3, 4, 5
    test_dlist_node_t *current = (test_dlist_node_t *) list;
    for (int i = 1; i <= 5; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_dlist_node_t *) current->node._list.next;
    }

    c_dlist_clear(&list, delete_dlist_node);
}

// Test: dlist backward traversal (unique to doubly-linked!)
static void test_dlist_backward_traversal(void **state)
{
    (void) state;
    c_dlist_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_dlist_node_t *node = create_dlist_node(i);
        c_dlist_push_back(&list, &node->node);
    }

    // Find tail
    c_dlist_node_t *tail = list;
    while (tail && tail->_list.next) {
        tail = (c_dlist_node_t *) tail->_list.next;
    }

    // Traverse backward from tail
    test_dlist_node_t *current = (test_dlist_node_t *) tail;
    for (int i = 5; i >= 1; i--) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_dlist_node_t *) current->node.prev;
    }
    assert_null(current);  // Should reach NULL after first node

    c_dlist_clear(&list, delete_dlist_node);
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void)
{
    const struct CMUnitTest tests[] = {
        // Forward list tests
        cmocka_unit_test(test_flist_push_front),
        cmocka_unit_test(test_flist_push_back),
        cmocka_unit_test(test_flist_pop_front),
        cmocka_unit_test(test_flist_pop_back),
        cmocka_unit_test(test_flist_insert_after),
        cmocka_unit_test(test_flist_remove),
        cmocka_unit_test(test_flist_remove_if),
        cmocka_unit_test(test_flist_unique),
        cmocka_unit_test(test_flist_reverse),
        cmocka_unit_test(test_flist_empty),
        cmocka_unit_test(test_flist_sort),

        // Doubly-linked list tests
        cmocka_unit_test(test_dlist_push_front),
        cmocka_unit_test(test_dlist_push_back),
        cmocka_unit_test(test_dlist_pop_front),
        cmocka_unit_test(test_dlist_pop_back),
        cmocka_unit_test(test_dlist_insert_before),
        cmocka_unit_test(test_dlist_insert_after),
        cmocka_unit_test(test_dlist_remove),
        cmocka_unit_test(test_dlist_remove_if),
        cmocka_unit_test(test_dlist_unique),
        cmocka_unit_test(test_dlist_reverse),
        cmocka_unit_test(test_dlist_empty),
        cmocka_unit_test(test_dlist_sort),
        cmocka_unit_test(test_dlist_backward_traversal),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
