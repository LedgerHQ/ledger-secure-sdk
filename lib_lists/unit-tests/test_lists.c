#include <string.h>
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

#include "lists.h"

// ============================================================================
// Forward list test structures and helpers
// ============================================================================

typedef struct test_flist_node_t {
    flist_node_t node;
    int          value;
} test_flist_node_t;

static test_flist_node_t *create_flist_node(int value)
{
    test_flist_node_t *node = malloc(sizeof(test_flist_node_t));
    TEST_ASSERT_NOT_NULL(node);
    node->node.next = NULL;
    node->value     = value;
    return node;
}

static void delete_flist_node(flist_node_t *node)
{
    if (node != NULL) {
        free(node);
    }
}

// ============================================================================
// Doubly-linked list test structures and helpers
// ============================================================================

typedef struct test_list_node_t {
    list_node_t node;
    int         value;
} test_list_node_t;

static test_list_node_t *create_list_node(int value)
{
    test_list_node_t *node = malloc(sizeof(test_list_node_t));
    TEST_ASSERT_NOT_NULL(node);
    node->node._list.next = NULL;
    node->node.prev       = NULL;
    node->value           = value;
    return node;
}

static void delete_list_node(flist_node_t *node)
{
    if (node != NULL) {
        free(node);
    }
}

// ============================================================================
// Comparison and predicate functions
// ============================================================================

static bool compare_ascending_flist(const flist_node_t *a, const flist_node_t *b)
{
    const test_flist_node_t *node_a = (const test_flist_node_t *) a;
    const test_flist_node_t *node_b = (const test_flist_node_t *) b;
    return node_a->value <= node_b->value;
}

static bool are_equal_flist(const flist_node_t *a, const flist_node_t *b)
{
    const test_flist_node_t *node_a = (const test_flist_node_t *) a;
    const test_flist_node_t *node_b = (const test_flist_node_t *) b;
    return node_a->value == node_b->value;
}

static bool is_negative_flist(const flist_node_t *node)
{
    const test_flist_node_t *test_node = (const test_flist_node_t *) node;
    return test_node->value < 0;
}

// Doubly-linked list comparison and predicate functions
static bool compare_ascending_list(const flist_node_t *a, const flist_node_t *b)
{
    const test_list_node_t *node_a = (const test_list_node_t *) a;
    const test_list_node_t *node_b = (const test_list_node_t *) b;
    return node_a->value <= node_b->value;
}

static bool are_equal_list(const flist_node_t *a, const flist_node_t *b)
{
    const test_list_node_t *node_a = (const test_list_node_t *) a;
    const test_list_node_t *node_b = (const test_list_node_t *) b;
    return node_a->value == node_b->value;
}

static bool is_even_list(const flist_node_t *node)
{
    const test_list_node_t *test_node = (const test_list_node_t *) node;
    return (test_node->value % 2) == 0;
}

// ============================================================================
// Forward list tests
// ============================================================================

// Test: flist push_front
void test_flist_push_front(void)
{
    flist_node_t      *list = NULL;
    test_flist_node_t *node1, *node2, *node3;

    node1 = create_flist_node(1);
    TEST_ASSERT_TRUE(flist_push_front(&list, &node1->node));
    TEST_ASSERT_EQUAL_PTR(list, &node1->node);
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 1);

    node2 = create_flist_node(2);
    TEST_ASSERT_TRUE(flist_push_front(&list, &node2->node));
    TEST_ASSERT_EQUAL_PTR(list, &node2->node);
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 2);

    node3 = create_flist_node(3);
    TEST_ASSERT_TRUE(flist_push_front(&list, &node3->node));
    TEST_ASSERT_EQUAL_PTR(list, &node3->node);
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 3);

    // Verify order: 3 -> 2 -> 1
    test_flist_node_t *current = (test_flist_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 3);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 1);

    flist_clear(&list, delete_flist_node);
}

// Test: flist push_back
void test_flist_push_back(void)
{
    flist_node_t      *list = NULL;
    test_flist_node_t *node1, *node2, *node3;

    node1 = create_flist_node(1);
    TEST_ASSERT_TRUE(flist_push_back(&list, &node1->node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 1);

    node2 = create_flist_node(2);
    TEST_ASSERT_TRUE(flist_push_back(&list, &node2->node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 2);

    node3 = create_flist_node(3);
    TEST_ASSERT_TRUE(flist_push_back(&list, &node3->node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    flist_clear(&list, delete_flist_node);
}

// Test: flist pop_front
void test_flist_pop_front(void)
{
    flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    flist_push_back(&list, &node1->node);
    flist_push_back(&list, &node2->node);
    flist_push_back(&list, &node3->node);

    TEST_ASSERT_TRUE(flist_pop_front(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 2);
    TEST_ASSERT_EQUAL_INT(((test_flist_node_t *) list)->value, 2);

    TEST_ASSERT_TRUE(flist_pop_front(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 1);
    TEST_ASSERT_EQUAL_INT(((test_flist_node_t *) list)->value, 3);

    TEST_ASSERT_TRUE(flist_pop_front(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 0);
    TEST_ASSERT_NULL(list);

    TEST_ASSERT_FALSE(flist_pop_front(&list, delete_flist_node));
}

// Test: flist pop_back
void test_flist_pop_back(void)
{
    flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    flist_push_back(&list, &node1->node);
    flist_push_back(&list, &node2->node);
    flist_push_back(&list, &node3->node);

    TEST_ASSERT_TRUE(flist_pop_back(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 2);

    TEST_ASSERT_TRUE(flist_pop_back(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 1);
    TEST_ASSERT_EQUAL_INT(((test_flist_node_t *) list)->value, 1);

    TEST_ASSERT_TRUE(flist_pop_back(&list, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 0);
    TEST_ASSERT_NULL(list);

    TEST_ASSERT_FALSE(flist_pop_back(&list, delete_flist_node));
}

// Test: flist insert_after
void test_flist_insert_after(void)
{
    flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node3 = create_flist_node(3);
    flist_push_back(&list, &node1->node);
    flist_push_back(&list, &node3->node);

    test_flist_node_t *node2 = create_flist_node(2);
    TEST_ASSERT_TRUE(flist_insert_after(&list, &node1->node, &node2->node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    flist_clear(&list, delete_flist_node);
}

// Test: flist remove
void test_flist_remove(void)
{
    flist_node_t *list = NULL;

    test_flist_node_t *node1 = create_flist_node(1);
    test_flist_node_t *node2 = create_flist_node(2);
    test_flist_node_t *node3 = create_flist_node(3);
    flist_push_back(&list, &node1->node);
    flist_push_back(&list, &node2->node);
    flist_push_back(&list, &node3->node);

    TEST_ASSERT_TRUE(flist_remove(&list, &node2->node, delete_flist_node));
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 2);

    // Verify order: 1 -> 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_flist_node_t *) current->node.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    flist_clear(&list, delete_flist_node);
}

// Test: flist remove_if
void test_flist_remove_if(void)
{
    flist_node_t *list = NULL;

    // Create list: -2, -1, 0, 1, 2, 3
    int values[] = {-2, -1, 0, 1, 2, 3};
    for (int i = 0; i < 6; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        flist_push_back(&list, &node->node);
    }

    // Remove all negative values
    size_t removed = flist_remove_if(&list, is_negative_flist, delete_flist_node);
    TEST_ASSERT_EQUAL_INT(removed, 2);
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 4);

    // Verify remaining: 0, 1, 2, 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    flist_clear(&list, delete_flist_node);
}

// Test: flist unique
void test_flist_unique(void)
{
    flist_node_t *list = NULL;

    // Create list with duplicates: 1, 1, 2, 2, 2, 3, 3
    int values[] = {1, 1, 2, 2, 2, 3, 3};
    for (int i = 0; i < 7; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        flist_push_back(&list, &node->node);
    }

    size_t removed = flist_unique(&list, are_equal_flist, delete_flist_node);
    TEST_ASSERT_EQUAL_INT(removed, 4);  // Removed 4 duplicates
    TEST_ASSERT_EQUAL_INT(flist_size(&list), 3);

    // Verify remaining: 1, 2, 3
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 1; i <= 3; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    flist_clear(&list, delete_flist_node);
}

// Test: flist reverse
void test_flist_reverse(void)
{
    flist_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_flist_node_t *node = create_flist_node(i);
        flist_push_back(&list, &node->node);
    }

    TEST_ASSERT_TRUE(flist_reverse(&list));

    // Verify reversed: 5, 4, 3, 2, 1
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 5; i >= 1; i--) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    flist_clear(&list, delete_flist_node);
}

// Test: flist empty
void test_flist_empty(void)
{
    flist_node_t *list = NULL;

    TEST_ASSERT_TRUE(flist_empty(&list));

    test_flist_node_t *node = create_flist_node(1);
    flist_push_front(&list, &node->node);
    TEST_ASSERT_FALSE(flist_empty(&list));

    flist_clear(&list, delete_flist_node);
    TEST_ASSERT_TRUE(flist_empty(&list));
}

// Test: flist sort
void test_flist_sort(void)
{
    flist_node_t *list = NULL;

    // Create unsorted list: 3, 1, 4, 2
    int values[] = {3, 1, 4, 2};
    for (int i = 0; i < 4; i++) {
        test_flist_node_t *node = create_flist_node(values[i]);
        flist_push_back(&list, &node->node);
    }

    TEST_ASSERT_TRUE(flist_sort(&list, compare_ascending_flist));

    // Verify sorted: 1, 2, 3, 4
    test_flist_node_t *current = (test_flist_node_t *) list;
    for (int i = 1; i <= 4; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_flist_node_t *) current->node.next;
    }

    flist_clear(&list, delete_flist_node);
}

// ============================================================================
// Doubly-linked list tests
// ============================================================================

// Test: list push_front
void test_list_push_front(void)
{
    list_node_t      *list = NULL;
    test_list_node_t *node1, *node2, *node3;

    node1 = create_list_node(1);
    TEST_ASSERT_TRUE(list_push_front(&list, &node1->node));
    TEST_ASSERT_EQUAL_PTR(list, &node1->node);
    TEST_ASSERT_EQUAL_INT(list_size(&list), 1);

    node2 = create_list_node(2);
    TEST_ASSERT_TRUE(list_push_front(&list, &node2->node));
    TEST_ASSERT_EQUAL_PTR(list, &node2->node);
    TEST_ASSERT_EQUAL_INT(list_size(&list), 2);

    node3 = create_list_node(3);
    TEST_ASSERT_TRUE(list_push_front(&list, &node3->node));
    TEST_ASSERT_EQUAL_PTR(list, &node3->node);
    TEST_ASSERT_EQUAL_INT(list_size(&list), 3);

    // Verify order: 3 -> 2 -> 1
    test_list_node_t *current = (test_list_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 3);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 1);

    list_clear(&list, delete_list_node);
}

// Test: list push_back (O(1) - fast!)
void test_list_push_back(void)
{
    list_node_t      *list = NULL;
    test_list_node_t *node1, *node2, *node3;

    node1 = create_list_node(1);
    TEST_ASSERT_TRUE(list_push_back(&list, &node1->node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 1);

    node2 = create_list_node(2);
    TEST_ASSERT_TRUE(list_push_back(&list, &node2->node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 2);

    node3 = create_list_node(3);
    TEST_ASSERT_TRUE(list_push_back(&list, &node3->node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_list_node_t *current = (test_list_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    list_clear(&list, delete_list_node);
}

// Test: list pop_front
void test_list_pop_front(void)
{
    list_node_t *list = NULL;

    test_list_node_t *node1 = create_list_node(1);
    test_list_node_t *node2 = create_list_node(2);
    test_list_node_t *node3 = create_list_node(3);
    list_push_back(&list, &node1->node);
    list_push_back(&list, &node2->node);
    list_push_back(&list, &node3->node);

    TEST_ASSERT_TRUE(list_pop_front(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 2);
    TEST_ASSERT_EQUAL_INT(((test_list_node_t *) list)->value, 2);

    TEST_ASSERT_TRUE(list_pop_front(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 1);
    TEST_ASSERT_EQUAL_INT(((test_list_node_t *) list)->value, 3);

    TEST_ASSERT_TRUE(list_pop_front(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 0);
    TEST_ASSERT_NULL(list);

    TEST_ASSERT_FALSE(list_pop_front(&list, delete_list_node));
}

// Test: list pop_back (O(1) - fast!)
void test_list_pop_back(void)
{
    list_node_t *list = NULL;

    test_list_node_t *node1 = create_list_node(1);
    test_list_node_t *node2 = create_list_node(2);
    test_list_node_t *node3 = create_list_node(3);
    list_push_back(&list, &node1->node);
    list_push_back(&list, &node2->node);
    list_push_back(&list, &node3->node);

    TEST_ASSERT_TRUE(list_pop_back(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 2);

    TEST_ASSERT_TRUE(list_pop_back(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 1);
    TEST_ASSERT_EQUAL_INT(((test_list_node_t *) list)->value, 1);

    TEST_ASSERT_TRUE(list_pop_back(&list, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 0);
    TEST_ASSERT_NULL(list);

    TEST_ASSERT_FALSE(list_pop_back(&list, delete_list_node));
}

// Test: list insert_before (O(1) - unique to doubly-linked!)
void test_list_insert_before(void)
{
    list_node_t *list = NULL;

    test_list_node_t *node1 = create_list_node(1);
    test_list_node_t *node3 = create_list_node(3);
    list_push_back(&list, &node1->node);
    list_push_back(&list, &node3->node);

    test_list_node_t *node2 = create_list_node(2);
    TEST_ASSERT_TRUE(list_insert_before(&list, &node3->node, &node2->node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_list_node_t *current = (test_list_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    // Insert before head
    test_list_node_t *node0 = create_list_node(0);
    TEST_ASSERT_TRUE(list_insert_before(&list, &node1->node, &node0->node));
    TEST_ASSERT_EQUAL_PTR(list, &node0->node);

    list_clear(&list, delete_list_node);
}

// Test: list insert_after
void test_list_insert_after(void)
{
    list_node_t *list = NULL;

    test_list_node_t *node1 = create_list_node(1);
    test_list_node_t *node3 = create_list_node(3);
    list_push_back(&list, &node1->node);
    list_push_back(&list, &node3->node);

    test_list_node_t *node2 = create_list_node(2);
    TEST_ASSERT_TRUE(list_insert_after(&list, &node1->node, &node2->node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 3);

    // Verify order: 1 -> 2 -> 3
    test_list_node_t *current = (test_list_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 2);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    list_clear(&list, delete_list_node);
}

// Test: list remove (O(1) - fast!)
void test_list_remove(void)
{
    list_node_t *list = NULL;

    test_list_node_t *node1 = create_list_node(1);
    test_list_node_t *node2 = create_list_node(2);
    test_list_node_t *node3 = create_list_node(3);
    list_push_back(&list, &node1->node);
    list_push_back(&list, &node2->node);
    list_push_back(&list, &node3->node);

    // Remove middle node (O(1))
    TEST_ASSERT_TRUE(list_remove(&list, &node2->node, delete_list_node));
    TEST_ASSERT_EQUAL_INT(list_size(&list), 2);

    // Verify order: 1 -> 3
    test_list_node_t *current = (test_list_node_t *) list;
    TEST_ASSERT_EQUAL_INT(current->value, 1);
    current = (test_list_node_t *) current->node._list.next;
    TEST_ASSERT_EQUAL_INT(current->value, 3);

    list_clear(&list, delete_list_node);
}

// Test: list remove_if
void test_list_remove_if(void)
{
    list_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5, 6
    for (int i = 1; i <= 6; i++) {
        test_list_node_t *node = create_list_node(i);
        list_push_back(&list, &node->node);
    }

    // Remove all even values
    size_t removed = list_remove_if(&list, is_even_list, delete_list_node);
    TEST_ASSERT_EQUAL_INT(removed, 3);  // Removed 2, 4, 6
    TEST_ASSERT_EQUAL_INT(list_size(&list), 3);

    // Verify remaining: 1, 3, 5
    test_list_node_t *current    = (test_list_node_t *) list;
    int               expected[] = {1, 3, 5};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, expected[i]);
        current = (test_list_node_t *) current->node._list.next;
    }

    list_clear(&list, delete_list_node);
}

// Test: list unique
void test_list_unique(void)
{
    list_node_t *list = NULL;

    // Create list with duplicates: 1, 1, 2, 3, 3, 3, 4
    int values[] = {1, 1, 2, 3, 3, 3, 4};
    for (int i = 0; i < 7; i++) {
        test_list_node_t *node = create_list_node(values[i]);
        list_push_back(&list, &node->node);
    }

    size_t removed = list_unique(&list, are_equal_list, delete_list_node);
    TEST_ASSERT_EQUAL_INT(removed, 3);  // Removed 3 duplicates
    TEST_ASSERT_EQUAL_INT(list_size(&list), 4);

    // Verify remaining: 1, 2, 3, 4
    test_list_node_t *current = (test_list_node_t *) list;
    for (int i = 1; i <= 4; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_list_node_t *) current->node._list.next;
    }

    list_clear(&list, delete_list_node);
}

// Test: list reverse
void test_list_reverse(void)
{
    list_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_list_node_t *node = create_list_node(i);
        list_push_back(&list, &node->node);
    }

    TEST_ASSERT_TRUE(list_reverse(&list));

    // Verify reversed: 5, 4, 3, 2, 1
    test_list_node_t *current = (test_list_node_t *) list;
    for (int i = 5; i >= 1; i--) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_list_node_t *) current->node._list.next;
    }

    list_clear(&list, delete_list_node);
}

// Test: list empty
void test_list_empty(void)
{
    list_node_t *list = NULL;

    TEST_ASSERT_TRUE(list_empty(&list));

    test_list_node_t *node = create_list_node(1);
    list_push_front(&list, &node->node);
    TEST_ASSERT_FALSE(list_empty(&list));

    list_clear(&list, delete_list_node);
    TEST_ASSERT_TRUE(list_empty(&list));
}

// Test: list sort
void test_list_sort(void)
{
    list_node_t *list = NULL;

    // Create unsorted list: 4, 1, 3, 2, 5
    int values[] = {4, 1, 3, 2, 5};
    for (int i = 0; i < 5; i++) {
        test_list_node_t *node = create_list_node(values[i]);
        list_push_back(&list, &node->node);
    }

    TEST_ASSERT_TRUE(list_sort(&list, compare_ascending_list));

    // Verify sorted: 1, 2, 3, 4, 5
    test_list_node_t *current = (test_list_node_t *) list;
    for (int i = 1; i <= 5; i++) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_list_node_t *) current->node._list.next;
    }

    list_clear(&list, delete_list_node);
}

// Test: list backward traversal (unique to doubly-linked!)
void test_list_backward_traversal(void)
{
    list_node_t *list = NULL;

    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; i++) {
        test_list_node_t *node = create_list_node(i);
        list_push_back(&list, &node->node);
    }

    // Find tail
    list_node_t *tail = list;
    while (tail && tail->_list.next) {
        tail = (list_node_t *) tail->_list.next;
    }

    // Traverse backward from tail
    test_list_node_t *current = (test_list_node_t *) tail;
    for (int i = 5; i >= 1; i--) {
        TEST_ASSERT_NOT_NULL(current);
        TEST_ASSERT_EQUAL_INT(current->value, i);
        current = (test_list_node_t *) current->node.prev;
    }
    TEST_ASSERT_NULL(current);  // Should reach NULL after first node

    list_clear(&list, delete_list_node);
}

// ============================================================================
// Main test runner
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    // Forward list tests
    RUN_TEST(test_flist_push_front);
    RUN_TEST(test_flist_push_back);
    RUN_TEST(test_flist_pop_front);
    RUN_TEST(test_flist_pop_back);
    RUN_TEST(test_flist_insert_after);
    RUN_TEST(test_flist_remove);
    RUN_TEST(test_flist_remove_if);
    RUN_TEST(test_flist_unique);
    RUN_TEST(test_flist_reverse);
    RUN_TEST(test_flist_empty);
    RUN_TEST(test_flist_sort);
    // Doubly-linked list tests
    RUN_TEST(test_list_push_front);
    RUN_TEST(test_list_push_back);
    RUN_TEST(test_list_pop_front);
    RUN_TEST(test_list_pop_back);
    RUN_TEST(test_list_insert_before);
    RUN_TEST(test_list_insert_after);
    RUN_TEST(test_list_remove);
    RUN_TEST(test_list_remove_if);
    RUN_TEST(test_list_unique);
    RUN_TEST(test_list_reverse);
    RUN_TEST(test_list_empty);
    RUN_TEST(test_list_sort);
    RUN_TEST(test_list_backward_traversal);
    return UNITY_END();
}
