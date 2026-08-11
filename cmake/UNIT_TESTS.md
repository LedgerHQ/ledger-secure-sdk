# Unit Tests

## Prerequisites

- CMake >= 3.28
- **Ruby** — required by CMock to generate mock source files at configure time
- **Unity + CMock** — fetched automatically from GitHub when CMake runs, so the build machine needs network access
- lcov >= 1.14 (for code coverage)

All prerequisites are available in the `ledger-app-builder-lite` Docker image used by CI.

## Building and Running

Each component keeps its tests in its own `unit-tests/` directory. From the SDK root:

```bash
cmake --preset unit-tests
cmake --build --preset unit-tests
ctest --preset unit-tests
```

## Code Coverage

```bash
cmake --build --preset unit-tests --target generate_coverage
```

Produces `coverage.info` (lcov tracefile), an HTML report in `coverage/index.html`,
and optionally `coverage.xml` (Cobertura) if `lcov_cobertura` is installed.

## Test Suites

| Directory                           | Module under test                                     |
|-------------------------------------|-------------------------------------------------------|
| `app_features/address_book/unit-tests/` | Address Book APDU handlers (TLV parsing and UI flows) |
| `io_legacy/unit-tests/`             | Legacy IO: one pending command at a time, CTAP cancel |
| `lib_alloc/unit-tests/`             | Dynamic memory allocator and utility wrappers         |
| `lib_bagl/unit-tests/`              | BAGL label animations (classic and circular tickers)  |
| `lib_lists/unit-tests/`             | Generic singly- and doubly-linked list library        |
| `lib_nbgl/unit-tests/`              | NBGL use cases (Nano page navigation, touch value aliases), NBGL pages |
| `lib_standard_app/unit-tests/`      | Standard app helpers (APDU parsing, buffers, formats) and app storage |
| `lib_tlv/unit-tests/`               | TLV (tag-length-value) encoding/decoding              |
| `lib_u2f/unit-tests/`               | CTAPHID channel handling (U2F transport, HID U2F)     |
| `lib_ux/unit-tests/`                | BAGL flows: button gestures across screen changes     |
| `app_features/transaction_check/unit-tests/` | Transaction Check APDU, report binding and opt-in UI |
| `src/unit-tests/`                   | `PRINTF` / `snprintf`, hex helpers, `os_longjmp`, `TRY` / `CATCH` / `THROW` host stubs |

## Memory Profiling (lib_alloc)

`tools/valground.py` analyses allocations when tests run under Speculos. Build the app
with `HAVE_MEMORY_PROFILING=1` and pipe output:

```bash
pytest --device nanosp -s -k test_name 2>&1 | ./tools/valground.py
```

Options: `--quiet` / `-q` (minimal output), `--colors` / `-c` (requires `colorama`).

Detects leaks, double-free, free-without-malloc, and persistent allocations across
test boundaries. Exits `0` on success, `1` on errors.
