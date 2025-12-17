# Dynamic Allocator Unit Tests

## Prerequisites

- CMake >= 3.10
- CMocka >= 1.1.5
- lcov >= 1.14 (for code coverage)

## Overview

This test suite covers both low-level allocator functions (`mem_alloc.c`) and high-level memory utility wrappers (`app_mem_utils.c`).

### Test Coverage

**Low-level allocator tests (5 tests):**

- `test_alloc` - Basic allocation/deallocation and coalescing
- `test_corrupt_invalid` - Corruption detection with invalid chunk size
- `test_corrupt_overflow` - Corruption detection with buffer overflow
- `test_fragmentation` - Heap fragmentation scenarios
- `test_init` - Initialization edge cases

**High-level utility tests (11 tests):**

- `test_utils_init` - Initialization with valid/invalid parameters
- `test_utils_basic_alloc` - Basic allocation and free operations
- `test_utils_zero_size` - Zero-size allocation handling
- `test_utils_buffer_allocate` - Zero-initialized buffer allocation
- `test_utils_buffer_realloc` - Buffer reallocation
- `test_utils_buffer_zero_size` - Zero-size buffer cleanup
- `test_utils_strdup` - String duplication
- `test_utils_format_uint` - Integer formatting
- `test_utils_large_alloc` - Large allocation handling
- `test_utils_out_of_memory` - OOM condition handling
- `test_utils_stress` - Stress test with repeated alloc/free cycles

## Building and Running

### Compile tests

In `unit-tests/lib_alloc` folder, compile with:

```bash
cmake -Bbuild -H. && make -C build
```

### Run tests directly

```bash
./build/test_mem_alloc
```

### Run via ctest

```bash
cd build && ctest --verbose
```

Or with make:

```bash
CTEST_OUTPUT_ON_FAILURE=1 make -C build test
```

## Code Coverage

Generate coverage report in `unit-tests` folder:

```bash
../gen_coverage.sh
```

Output: `coverage.total` and `coverage/index.html`

## Memory Profiling

The SDK includes `tools/valground.py` to analyze memory allocations and detect leaks when running tests with Speculos.

### Usage with Speculos

Compile your app with `HAVE_MEMORY_PROFILING=1` and pipe the output:

```bash
pytest --device nanosp -s -k test_name 2>&1 | ./tools/valground.py
```

### Options

- `--quiet` / `-q` - Minimal output (only errors and global summary)
- `--colors` / `-c` - Enable colored output (requires `colorama`)

### Output

The tool detects:

- **Memory leaks** - Allocated blocks never freed
- **Double free errors** - Freeing the same pointer twice
- **Free without malloc** - Freeing unallocated pointers
- **Persistent allocations** - Blocks that persist across test boundaries
- **Memory usage statistics** - Total/max allocation, heap utilization percentage

### Example

```bash
# Install colorama for colored output (optional)
pip install colorama

# Run with colors enabled
pytest --device nanosp -s 2>&1 | ./tools/valground.py --colors
```

Exit code: `0` if no errors, `1` if leaks or free errors detected.
