# Unit Tests

## Prerequisites

- CMake >= 3.10
- **Ruby** — required by CMock to generate mock source files at configure time
- **Unity + CMock** — fetched automatically from GitHub when CMake runs, so the build machine needs network access
- lcov >= 1.14 (for code coverage)

All prerequisites are available in the `ledger-app-builder-lite` Docker image used by CI.

## Building and Running

Each test suite lives in its own subdirectory. From the suite's directory:

```bash
cmake -Bbuild -H. && cmake --build build
ctest --test-dir build --output-on-failure
```

## Code Coverage

From any suite's build directory:

```bash
cmake --build build --target generate_coverage
```

Produces `coverage.info` (lcov tracefile), an HTML report in `coverage/index.html`,
and optionally `coverage.xml` (Cobertura) if `lcov_cobertura` is installed.

## Test Suites

| Directory          | Module under test                                     |
|--------------------|-------------------------------------------------------|
| `address_book/`    | Address Book APDU handlers (TLV parsing and UI flows) |
| `app_storage/`     | Application persistent storage                        |
| `lib_alloc/`       | Dynamic memory allocator and utility wrappers         |
| `lib_lists/`       | Generic singly- and doubly-linked list library        |
| `lib_standard_app/`| Standard app boilerplate (APDU dispatch, IO helpers)  |
| `lib_tlv/`         | TLV (tag-length-value) encoding/decoding              |
| `print/`           | `PRINTF` and `snprintf` formatting                    |

## Memory Profiling (lib_alloc)

`tools/valground.py` analyses allocations when tests run under Speculos. Build the app
with `HAVE_MEMORY_PROFILING=1` and pipe output:

```bash
pytest --device nanosp -s -k test_name 2>&1 | ./tools/valground.py
```

Options: `--quiet` / `-q` (minimal output), `--colors` / `-c` (requires `colorama`).

Detects leaks, double-free, free-without-malloc, and persistent allocations across
test boundaries. Exits `0` on success, `1` on errors.
