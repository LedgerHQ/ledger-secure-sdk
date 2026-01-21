# Generic Linked List Unit Tests

## Prerequisites

- CMake >= 3.10
- CMocka >= 1.1.5
- lcov >= 1.14 (for code coverage)

## Overview

This test suite covers all functions provided by the Generic Linked List library (`c_list.c`), which supports both:

- **Forward lists** (`c_flist_*`) - Singly-linked lists (4-8 bytes/node)
- **Doubly-linked lists** (`c_dlist_*`) - Doubly-linked lists (8-16 bytes/node)

### Test Coverage

The test suite includes **24 comprehensive tests** covering:

**Forward Lists (c_flist_*) - 11 tests:**

- `test_c_flist_push_front` - Add nodes at the beginning
- `test_c_flist_push_back` - Add nodes at the end
- `test_c_flist_pop_front` - Remove nodes from the beginning
- `test_c_flist_pop_back` - Remove nodes from the end
- `test_c_flist_insert_after` - Insert after a reference node
- `test_c_flist_remove` - Remove specific nodes
- `test_c_flist_remove_if` - Remove nodes matching predicate
- `test_c_flist_clear` - Clear entire list
- `test_c_flist_sort` - Sort list with comparison function
- `test_c_flist_unique` - Remove duplicate consecutive nodes
- `test_c_flist_reverse` - Reverse list order

**Doubly-Linked Lists (c_dlist_*) - 13 tests:**

- `test_c_dlist_push_front` - Add nodes at the beginning
- `test_c_dlist_push_back` - Add nodes at the end
- `test_c_dlist_pop_front` - Remove nodes from the beginning
- `test_c_dlist_pop_back` - Remove nodes from the end
- `test_c_dlist_insert_after` - Insert after a reference node
- `test_c_dlist_insert_before` - Insert before a reference node
- `test_c_dlist_remove` - Remove specific nodes
- `test_c_dlist_remove_if` - Remove nodes matching predicate
- `test_c_dlist_clear` - Clear entire list
- `test_c_dlist_sort` - Sort list with comparison function
- `test_c_dlist_unique` - Remove duplicate consecutive nodes
- `test_c_dlist_reverse` - Reverse list order
- `test_c_dlist_backward_traversal` - Traverse list backward using prev pointers

## Building and Running Tests

### Quick Start

```bash
cd unit-tests/lib_c_list
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=/usr/bin/gcc ..
make
```

### Run Tests

```bash
# Run all tests
make test

# Or run directly with verbose output
./test_c_list
```

### Generate Code Coverage

```bash
# Generate coverage data
lcov --capture --directory . --output-file coverage.info

# Filter out system headers
lcov --remove coverage.info '/usr/*' --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_html

# View report
xdg-open coverage_html/index.html
```

## Test Details

### Safety Features Tested

1. **NULL pointer validation** - All functions check for NULL parameters
2. **Node state validation** - Insertion functions verify `node->next == NULL` (flist) or `node->_list.next == NULL && node->prev == NULL` (dlist)
3. **Return value checking** - All mutating operations return bool for success/failure
4. **Empty list operations** - Safe handling of operations on empty lists
5. **Predicate filtering** - remove_if tests validate callback-based filtering
6. **Duplicate removal** - unique tests validate consecutive duplicate handling
7. **Bidirectional traversal** - dlist tests validate both forward and backward traversal

### Example Test Output

```bash
[==========] Running 24 test(s).
[ RUN      ] test_c_flist_push_front
[       OK ] test_c_flist_push_front
[ RUN      ] test_c_flist_push_back
[       OK ] test_c_flist_push_back
[ RUN      ] test_c_flist_pop_front
[       OK ] test_c_flist_pop_front
...
[ RUN      ] test_c_dlist_backward_traversal
[       OK ] test_c_dlist_backward_traversal
[==========] 24 test(s) run.
[  PASSED  ] 24 test(s).
```

## Memory Safety

All tests use cmocka's memory leak detection to ensure:

- No memory leaks in list operations
- Proper cleanup with deletion callbacks
- Safe handling of node allocation/deallocation

## Continuous Integration

These tests should be run:

- Before committing changes to lib_c_list
- As part of the CI/CD pipeline
- When modifying list-related code in applications

## Adding New Tests

To add new tests:

1. Define test node structures for the appropriate list type:

```c
// For forward lists
typedef struct test_flist_node_s {
    c_flist_node_t _list;
    int value;
} test_flist_node_t;

// For doubly-linked lists
typedef struct test_dlist_node_s {
    c_dlist_node_t _list;
    int value;
} test_dlist_node_t;
```

2. Add test function following the pattern:

```c
static void test_new_feature(void **state)
{
    (void) state;
    // Test implementation
}
```

3. Register in main():

```c
cmocka_unit_test(test_new_feature),
```

4. Rebuild and run tests

## Known Limitations

- Tests use malloc/free for simplicity
- Real applications may use different memory allocators
- Performance tests are not included (focus on correctness)
