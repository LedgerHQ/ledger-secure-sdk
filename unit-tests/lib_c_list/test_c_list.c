#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>

#include "c_list.h"

// Test node structure
typedef struct test_node_s {
    c_list_node_t node;
    int           value;
} test_node_t;

// Helper function to create a test node
static test_node_t *create_node(int value)
{
    test_node_t *node = malloc(sizeof(test_node_t));
    assert_non_null(node);
    node->node.next = NULL;
    node->value     = value;
    return node;
}

// Deletion callback for tests
static void delete_node(c_list_node_t *node)
{
    if (node != NULL) {
        free(node);
    }
}

// Test: push_front basic functionality
static void test_push_front(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node1, *node2, *node3;

    // Add first node
    node1 = create_node(1);
    assert_true(c_list_push_front(&list, &node1->node));
    assert_ptr_equal(list, &node1->node);
    assert_int_equal(c_list_size(&list), 1);

    // Add second node
    node2 = create_node(2);
    assert_true(c_list_push_front(&list, &node2->node));
    assert_ptr_equal(list, &node2->node);
    assert_int_equal(c_list_size(&list), 2);

    // Add third node
    node3 = create_node(3);
    assert_true(c_list_push_front(&list, &node3->node));
    assert_ptr_equal(list, &node3->node);
    assert_int_equal(c_list_size(&list), 3);

    // Verify order: 3 -> 2 -> 1
    test_node_t *current = (test_node_t *) list;
    assert_int_equal(current->value, 3);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 1);

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: push_front error cases
static void test_push_front_errors(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node;

    // NULL list pointer
    node = create_node(1);
    assert_false(c_list_push_front(NULL, &node->node));
    free(node);

    // NULL node pointer
    assert_false(c_list_push_front(&list, NULL));

    // Node already linked
    node             = create_node(1);
    test_node_t *tmp = create_node(2);
    node->node.next  = &tmp->node;
    assert_false(c_list_push_front(&list, &node->node));
    free(node);
    free(tmp);
}

// Test: push_back basic functionality
static void test_push_back(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node1, *node2, *node3;

    // Add first node
    node1 = create_node(1);
    assert_true(c_list_push_back(&list, &node1->node));
    assert_int_equal(c_list_size(&list), 1);

    // Add second node
    node2 = create_node(2);
    assert_true(c_list_push_back(&list, &node2->node));
    assert_int_equal(c_list_size(&list), 2);

    // Add third node
    node3 = create_node(3);
    assert_true(c_list_push_back(&list, &node3->node));
    assert_int_equal(c_list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_node_t *current = (test_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: push_back error cases
static void test_push_back_errors(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node;

    // NULL list pointer
    node = create_node(1);
    assert_false(c_list_push_back(NULL, &node->node));
    free(node);

    // NULL node pointer
    assert_false(c_list_push_back(&list, NULL));

    // Node already linked
    node             = create_node(1);
    test_node_t *tmp = create_node(2);
    node->node.next  = &tmp->node;
    assert_false(c_list_push_back(&list, &node->node));
    free(node);
    free(tmp);
}

// Test: pop_front basic functionality
static void test_pop_front(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Add three nodes
    test_node_t *node1 = create_node(1);
    test_node_t *node2 = create_node(2);
    test_node_t *node3 = create_node(3);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node2->node);
    c_list_push_back(&list, &node3->node);

    // Pop first node
    assert_true(c_list_pop_front(&list, delete_node));
    assert_int_equal(c_list_size(&list), 2);
    assert_int_equal(((test_node_t *) list)->value, 2);

    // Pop second node
    assert_true(c_list_pop_front(&list, delete_node));
    assert_int_equal(c_list_size(&list), 1);
    assert_int_equal(((test_node_t *) list)->value, 3);

    // Pop last node
    assert_true(c_list_pop_front(&list, delete_node));
    assert_int_equal(c_list_size(&list), 0);
    assert_null(list);

    // Pop from empty list
    assert_false(c_list_pop_front(&list, delete_node));
}

// Test: pop_back basic functionality
static void test_pop_back(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Add three nodes
    test_node_t *node1 = create_node(1);
    test_node_t *node2 = create_node(2);
    test_node_t *node3 = create_node(3);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node2->node);
    c_list_push_back(&list, &node3->node);

    // Pop last node
    assert_true(c_list_pop_back(&list, delete_node));
    assert_int_equal(c_list_size(&list), 2);

    // Pop second node
    assert_true(c_list_pop_back(&list, delete_node));
    assert_int_equal(c_list_size(&list), 1);
    assert_int_equal(((test_node_t *) list)->value, 1);

    // Pop first node
    assert_true(c_list_pop_back(&list, delete_node));
    assert_int_equal(c_list_size(&list), 0);
    assert_null(list);

    // Pop from empty list
    assert_false(c_list_pop_back(&list, delete_node));
}

// Test: insert_after basic functionality
static void test_insert_after(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create initial list: 1 -> 3
    test_node_t *node1 = create_node(1);
    test_node_t *node3 = create_node(3);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node3->node);

    // Insert 2 after 1
    test_node_t *node2 = create_node(2);
    assert_true(c_list_insert_after(&node1->node, &node2->node));
    assert_int_equal(c_list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_node_t *current = (test_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: insert_after error cases
static void test_insert_after_errors(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node1, *node2;

    node1 = create_node(1);
    c_list_push_back(&list, &node1->node);

    // NULL prev pointer
    node2 = create_node(2);
    assert_false(c_list_insert_after(NULL, &node2->node));
    free(node2);

    // NULL node pointer
    assert_false(c_list_insert_after(&node1->node, NULL));

    // Node already linked
    node2            = create_node(2);
    test_node_t *tmp = create_node(3);
    node2->node.next = &tmp->node;
    assert_false(c_list_insert_after(&node1->node, &node2->node));
    free(node2);
    free(tmp);

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: insert_before basic functionality
static void test_insert_before(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create initial list: 1 -> 3
    test_node_t *node1 = create_node(1);
    test_node_t *node3 = create_node(3);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node3->node);

    // Insert 2 before 3
    test_node_t *node2 = create_node(2);
    assert_true(c_list_insert_before(&list, &node3->node, &node2->node));
    assert_int_equal(c_list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_node_t *current = (test_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 2);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    // Insert 0 before 1 (at front)
    test_node_t *node0 = create_node(0);
    assert_true(c_list_insert_before(&list, &node1->node, &node0->node));
    assert_ptr_equal(list, &node0->node);

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: insert_before error cases
static void test_insert_before_errors(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;
    test_node_t   *node1, *node2, *node3;

    node1 = create_node(1);
    c_list_push_back(&list, &node1->node);

    // Empty list with non-null ref
    node2                     = create_node(2);
    node3                     = create_node(3);
    c_list_node_t *empty_list = NULL;
    assert_false(c_list_insert_before(&empty_list, &node2->node, &node3->node));

    // Ref not in list
    assert_false(c_list_insert_before(&list, &node2->node, &node3->node));

    // Cleanup
    free(node2);
    free(node3);
    c_list_clear(&list, delete_node);
}

// Test: remove basic functionality
static void test_remove(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create list: 1 -> 2 -> 3
    test_node_t *node1 = create_node(1);
    test_node_t *node2 = create_node(2);
    test_node_t *node3 = create_node(3);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node2->node);
    c_list_push_back(&list, &node3->node);

    // Remove middle node
    assert_true(c_list_remove(&list, &node2->node, delete_node));
    assert_int_equal(c_list_size(&list), 2);

    // Verify order: 1 -> 3
    test_node_t *current = (test_node_t *) list;
    assert_int_equal(current->value, 1);
    current = (test_node_t *) current->node.next;
    assert_int_equal(current->value, 3);

    // Remove first node
    assert_true(c_list_remove(&list, &node1->node, delete_node));
    assert_int_equal(c_list_size(&list), 1);
    assert_int_equal(((test_node_t *) list)->value, 3);

    // Remove last node
    assert_true(c_list_remove(&list, &node3->node, delete_node));
    assert_int_equal(c_list_size(&list), 0);
    assert_null(list);
}

// Test: remove node not in list
static void test_remove_not_found(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create list: 1 -> 2
    test_node_t *node1 = create_node(1);
    test_node_t *node2 = create_node(2);
    c_list_push_back(&list, &node1->node);
    c_list_push_back(&list, &node2->node);

    // Try to remove node not in list
    test_node_t *node3 = create_node(3);
    assert_false(c_list_remove(&list, &node3->node, delete_node));
    assert_int_equal(c_list_size(&list), 2);

    // Cleanup
    free(node3);
    c_list_clear(&list, delete_node);
}

// Test: clear
static void test_clear(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create list with multiple nodes
    for (int i = 0; i < 10; i++) {
        test_node_t *node = create_node(i);
        c_list_push_back(&list, &node->node);
    }
    assert_int_equal(c_list_size(&list), 10);

    // Clear all nodes
    assert_true(c_list_clear(&list, delete_node));
    assert_int_equal(c_list_size(&list), 0);
    assert_null(list);

    // Clear already empty list
    assert_true(c_list_clear(&list, delete_node));
}

// Test: sort
static bool compare_ascending(const c_list_node_t *a, const c_list_node_t *b)
{
    const test_node_t *node_a = (const test_node_t *) a;
    const test_node_t *node_b = (const test_node_t *) b;
    return node_a->value <= node_b->value;
}

static void test_sort(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create unsorted list: 3 -> 1 -> 4 -> 2
    int values[] = {3, 1, 4, 2};
    for (int i = 0; i < 4; i++) {
        test_node_t *node = create_node(values[i]);
        c_list_push_back(&list, &node->node);
    }

    // Sort the list
    assert_true(c_list_sort(&list, compare_ascending));

    // Verify sorted order: 1 -> 2 -> 3 -> 4
    test_node_t *current = (test_node_t *) list;
    for (int i = 1; i <= 4; i++) {
        assert_non_null(current);
        assert_int_equal(current->value, i);
        current = (test_node_t *) current->node.next;
    }

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: sort empty list
static void test_sort_empty(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Sort empty list (should succeed)
    assert_true(c_list_sort(&list, compare_ascending));
    assert_null(list);
}

// Test: sort single element
static void test_sort_single(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    test_node_t *node = create_node(42);
    c_list_push_back(&list, &node->node);

    assert_true(c_list_sort(&list, compare_ascending));
    assert_int_equal(((test_node_t *) list)->value, 42);

    c_list_clear(&list, delete_node);
}

// Test: sort error cases
static void test_sort_errors(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create a list
    test_node_t *node = create_node(1);
    c_list_push_back(&list, &node->node);

    // NULL list pointer
    assert_false(c_list_sort(NULL, compare_ascending));

    // NULL compare function
    assert_false(c_list_sort(&list, NULL));

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: size
static void test_size(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Empty list
    assert_int_equal(c_list_size(&list), 0);

    // Add nodes and check size
    for (int i = 1; i <= 5; i++) {
        test_node_t *node = create_node(i);
        c_list_push_back(&list, &node->node);
        assert_int_equal(c_list_size(&list), i);
    }

    // Cleanup
    c_list_clear(&list, delete_node);
}

// Test: size with NULL pointer
static void test_size_null(void **state)
{
    (void) state;

    // NULL list pointer should return 0
    assert_int_equal(c_list_size(NULL), 0);
}

// Test: list traversal and manipulation
static void test_traversal(void **state)
{
    (void) state;
    c_list_node_t *list = NULL;

    // Create list: 1 -> 2 -> 3 -> 4 -> 5
    for (int i = 1; i <= 5; i++) {
        test_node_t *node = create_node(i);
        c_list_push_back(&list, &node->node);
    }

    // Traverse and verify
    int            count   = 0;
    int            sum     = 0;
    c_list_node_t *current = list;
    while (current != NULL) {
        test_node_t *node = (test_node_t *) current;
        count++;
        sum += node->value;
        current = current->next;
    }

    assert_int_equal(count, 5);
    assert_int_equal(sum, 15);  // 1+2+3+4+5

    // Cleanup
    c_list_clear(&list, delete_node);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_push_front),    cmocka_unit_test(test_push_front_errors),
        cmocka_unit_test(test_push_back),     cmocka_unit_test(test_push_back_errors),
        cmocka_unit_test(test_pop_front),     cmocka_unit_test(test_pop_back),
        cmocka_unit_test(test_insert_after),  cmocka_unit_test(test_insert_after_errors),
        cmocka_unit_test(test_insert_before), cmocka_unit_test(test_insert_before_errors),
        cmocka_unit_test(test_remove),        cmocka_unit_test(test_remove_not_found),
        cmocka_unit_test(test_clear),         cmocka_unit_test(test_sort),
        cmocka_unit_test(test_sort_empty),    cmocka_unit_test(test_sort_single),
        cmocka_unit_test(test_sort_errors),   cmocka_unit_test(test_size),
        cmocka_unit_test(test_size_null),     cmocka_unit_test(test_traversal),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
