# Fuzzing lib_c_list

## Overview

This fuzzer tests the generic linked list library (`lib_c_list`) for memory safety issues, edge cases, and potential crashes. It exercises all list operations including insertion, removal, sorting, and traversal.

## Operations Tested

The fuzzer uses the lower 4 bits of each input byte to select operations:

| Op Code | Operation         | Description                                    |
|---------|-------------------|------------------------------------------------|
| 0x00    | `push_front`      | Add node at the beginning                      |
| 0x01    | `push_back`       | Add node at the end                            |
| 0x02    | `pop_front`       | Remove first node                              |
| 0x03    | `pop_back`        | Remove last node                               |
| 0x04    | `insert_after`    | Insert node after reference node               |
| 0x05    | `insert_before`   | Insert node before reference node              |
| 0x06    | `remove`          | Remove specific node                           |
| 0x07    | `clear`           | Remove all nodes                               |
| 0x08    | `sort`            | Sort list by node ID                           |
| 0x09    | `size`            | Get list size                                  |
| 0x0A    | `traverse`        | Traverse and verify integrity (cycle detection)|

## Features

### Safety Checks

- **Cycle detection**: Prevents infinite loops in traversal
- **Node tracking**: All allocated nodes are tracked for cleanup
- **Memory leak prevention**: Automatic cleanup at end of fuzzing iteration
- **Integrity verification**: Validates list structure during traversal

### Test Data

Each node contains:

- Unique ID (auto-incremented)
- 16 bytes of fuzzer-provided data
- Standard list node structure

### Limits

- `MAX_NODES`: 1000 (prevents excessive memory usage)
- `MAX_TRACKERS`: 100 (limits number of tracked nodes)
- Maximum input length: 256 bytes (configurable)

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

- Simple operations: `\x00\x00...` (push_front operations)
- Mixed operations: `\x00...\x01...\x02` (push_front, push_back, pop_front)
- Complex sequences: Various operation combinations

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
