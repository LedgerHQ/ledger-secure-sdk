# Generic Linked List Unit Tests

## Prerequisites

- CMake >= 3.10
- CMocka >= 1.1.5
- lcov >= 1.14 (for code coverage)

## Overview

This test suite covers all functions provided by the Generic Linked List library (`c_list.c`).

### Test Coverage

The test suite includes **16 comprehensive tests** covering:

**Insertion Operations (3 tests):**

- `test_push_front` - Add nodes at the beginning
- `test_push_front_errors` - Error handling for push_front
- `test_push_back` - Add nodes at the end

**Removal Operations (2 tests):**

- `test_pop_front` - Remove nodes from the beginning
- `test_pop_back` - Remove nodes from the end

**Advanced Insertion (3 tests):**

- `test_insert_after` - Insert after a reference node
- `test_insert_before` - Insert before a reference node
- `test_insert_before_errors` - Error handling for insert_before

**Node Removal (2 tests):**

- `test_remove` - Remove specific nodes
- `test_remove_not_found` - Handle removal of non-existent nodes

**List Operations (1 test):**

- `test_clear` - Clear entire list

**Sorting (3 tests):**

- `test_sort` - Sort list with comparison function
- `test_sort_empty` - Sort empty list
- `test_sort_single` - Sort single-element list

**Utilities (2 tests):**

- `test_size` - Count nodes in list
- `test_traversal` - Traverse and manipulate list

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
2. **Node state validation** - Insertion functions verify `node->next == NULL`
3. **Return value checking** - All mutating operations return bool for success/failure
4. **Node not found** - Functions handle missing nodes gracefully
5. **Empty list operations** - Safe handling of operations on empty lists

### Example Test Output

```bash
[==========] Running 16 test(s).
[ RUN      ] test_push_front
[       OK ] test_push_front
[ RUN      ] test_push_front_errors
[       OK ] test_push_front_errors
[ RUN      ] test_push_back
[       OK ] test_push_back
...
[==========] 16 test(s) run.
[  PASSED  ] 16 test(s).
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

1. Add test function following the pattern:

```c
static void test_new_feature(void **state)
{
    (void) state;
    // Test implementation
}
```

2. Register in main():

```c
cmocka_unit_test(test_new_feature),
```

3. Rebuild and run tests

## Known Limitations

- Tests use malloc/free for simplicity
- Real applications may use different memory allocators
- Performance tests are not included (focus on correctness)
