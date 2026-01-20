# Fuzzing lib_c_list

## Overview

This fuzzer tests the generic linked list library (`lib_c_list`) for memory safety issues, edge cases, and potential crashes. It exercises both **forward lists** (`c_flist_*`) and **doubly-linked lists** (`c_dlist_*`) with all available operations including insertion, removal, sorting, reversing, and traversal.

## List Type Selection

The **first byte** determines which type of list to test:

- **Bit 7 = 0**: Forward list (singly-linked)
- **Bit 7 = 1**: Doubly-linked list

This allows the fuzzer to test both implementations independently within a single harness.

## Operations Tested

### Forward List Operations (c_flist_*)

The fuzzer uses the lower 4 bits of each input byte to select operations:

| Op Code | Operation         | Description                                    |
|---------|-------------------|------------------------------------------------|
| 0x00    | `push_front`      | Add node at the beginning (O(1))               |
| 0x01    | `push_back`       | Add node at the end (O(n))                     |
| 0x02    | `pop_front`       | Remove first node (O(1))                       |
| 0x03    | `pop_back`        | Remove last node (O(n))                        |
| 0x04    | `insert_after`    | Insert node after reference node (O(1))        |
| 0x05    | `remove`          | Remove specific node (O(n))                    |
| 0x06    | `clear`           | Remove all nodes (O(n))                        |
| 0x07    | `sort`            | Sort list by node ID (O(n²))                   |
| 0x08    | `size`            | Get list size (O(n))                           |
| 0x09    | `reverse`         | Reverse list order (O(n))                      |
| 0x0A    | `empty`           | Check if list is empty (O(1))                  |

### Doubly-Linked List Operations (c_dlist_*)

| Op Code | Operation         | Description                                    |
|---------|-------------------|------------------------------------------------|
| 0x00    | `push_front`      | Add node at the beginning (O(1))               |
| 0x01    | `push_back`       | Add node at the end (O(1) ⚡)                   |
| 0x02    | `pop_front`       | Remove first node (O(1) ⚡)                     |
| 0x03    | `pop_back`        | Remove last node (O(1) ⚡)                      |
| 0x04    | `insert_after`    | Insert node after reference (O(1))             |
| 0x05    | `insert_before`   | Insert node before reference (O(1) ⚡)          |
| 0x06    | `remove`          | Remove specific node (O(1) ⚡)                  |
| 0x07    | `clear`           | Remove all nodes (O(n))                        |
| 0x08    | `sort`            | Sort list by node ID (O(n²))                   |
| 0x09    | `size`            | Get list size (O(n))                           |
| 0x0A    | `reverse`         | Reverse list order (O(n))                      |
| 0x0B    | `empty`           | Check if list is empty (O(1))                  |

## Features

### Dual List Testing

The fuzzer tests both list implementations:

- **Forward Lists** (`c_flist_node_t`): Singly-linked, minimal memory overhead
- **Doubly-Linked Lists** (`c_dlist_node_t`): Bidirectional traversal, O(1) operations

### Safety Checks

- **Type safety**: Separate tracking for forward and doubly-linked nodes
- **Node tracking**: All allocated nodes are tracked for cleanup
- **Memory leak prevention**: Automatic cleanup at end of fuzzing iteration
- **Size limits**: Prevents excessive memory usage

### Test Data

Each node contains:

- Unique ID (auto-incremented)
- 16 bytes of fuzzer-provided data
- Standard list node structure (with or without prev pointer)

### Limits

- `MAX_NODES`: 1000 (prevents excessive memory usage)
- `MAX_TRACKERS`: 100 (limits number of tracked nodes)
- Minimum input length: 2 bytes (1 for type selection, 1+ for operations)

## Building

From the SDK root:

```bash
cd fuzzing
mkdir -p build && cd build
cmake -S .. -B . -DCMAKE_C_COMPILER=clang -DSANITIZER=address -DBOLOS_SDK=/path/to/sdk
cmake --build . --target fuzz_c_list
```

## Running

### Basic run

```bash
./fuzz_c_list
```

### With specific options

```bash
# Run for 10000 iterations
./fuzz_c_list -runs=10000

# Limit input size to 128 bytes
./fuzz_c_list -max_len=128

# Use corpus directory
./fuzz_c_list corpus/

# Timeout per input (in seconds)
./fuzz_c_list -timeout=10
```

### Using the helper script

```bash
cd /path/to/sdk/fuzzing
./local_run.sh --build=1 --fuzzer=build/fuzz_c_list --j=4 --run-fuzzer=1 --BOLOS_SDK=/path/to/sdk
```

## Corpus

Initial corpus files can be placed in:

```bash
fuzzing/harness/fuzz_c_list/
```

Example corpus files:

**Forward list operations** (first byte < 0x80):

```bash
# Simple forward list operations
00 00 <16 bytes>  # push_front
00 01 <16 bytes>  # push_back
00 02             # pop_front

# Complex forward list sequence
00 00 <16 bytes> 01 <16 bytes> 07 09  # push_front, push_back, sort, reverse
```

**Doubly-linked list operations** (first byte >= 0x80):

```bash
# Simple doubly-linked operations
80 00 <16 bytes>  # push_front
80 01 <16 bytes>  # push_back (O(1) - fast!)
80 05 00 <16 bytes>  # insert_before (unique to doubly-linked)

# Complex doubly-linked sequence
80 00 <16 bytes> 01 <16 bytes> 08 0A  # push_front, push_back, sort, reverse
```

## Input Format

```bash
Byte 0:    [1 bit: list type] [7 bits: unused]
           - Bit 7 = 0: Forward list
           - Bit 7 = 1: Doubly-linked list

Byte 1+:   [Operation code] [Optional parameters]
           - Lower 4 bits: operation type (0x00-0x0F)
           - Upper 4 bits: unused

Parameters:
- Node creation: 16 bytes of data
- Node reference: 1 byte index (for insert/remove operations)
```

## Debugging

Enable debug output by uncommenting `#define DEBUG_CRASH` in `fuzzer_c_list.c`:

```c
#define DEBUG_CRASH
```

This will print:

- Node creation/deletion
- Operation execution
- List size changes
- Cleanup operations

## Crash Analysis

If a crash is found:

1. The fuzzer will save the crashing input to `crash-*` or `leak-*` files
2. Reproduce the crash:

   ```bash
   ./fuzz_c_list crash-HASH
   ```

3. Debug with AddressSanitizer output
4. Fix the issue in `lib_c_list/c_list.c`
5. Verify fix by re-running fuzzer

## Expected Behavior

The fuzzer should:

- ✅ Handle all operations safely
- ✅ Prevent memory leaks (all nodes cleaned up)
- ✅ Detect invalid operations (return false)
- ✅ Handle edge cases (empty list, single node, etc.)
- ✅ Maintain list integrity (no cycles, no corruption)

## Known Issues

None currently. If you find a crash, please report it!

## Coverage

To generate coverage report:

```bash
./local_run.sh --build=1 --fuzzer=build/fuzz_c_list --compute-coverage=1 --BOLOS_SDK=/path/to/sdk
```

Coverage files will be in `fuzzing/out/coverage/`.
