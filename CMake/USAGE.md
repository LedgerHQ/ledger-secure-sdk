# CMake modules — Usage

This directory hosts the reusable **CMake modules** shipped with the Ledger
Secure SDK. Each module is self-contained and documented in its own section
below. New modules are expected to be added over time; please keep this document
in sync (see [Conventions](#conventions)).

## Modules

| Module             | Entry point      | Purpose                                   |
| ------------------ | ---------------- | ----------------------------------------- |
| [Unit-Test framework](#1-unit-test-framework-ledgerut) | `LedgerUT.cmake` | Unity/CMock unit tests for SDK-based apps |

---

## 1. Unit-Test framework (`LedgerUT`)

A small framework, based on [Unity](https://github.com/ThrowTheSwitch/Unity) and
[CMock](https://github.com/ThrowTheSwitch/CMock), to write and run unit tests for
embedded apps built on top of the SDK.

It takes care of:

- fetching Unity and CMock (via `FetchContent`, pinned to a fixed commit),
- generating mocks from SDK headers,
- compiling test executables with the right SDK include paths and `-D` defines,
- registering tests with CTest,
- producing an LCOV/Cobertura coverage report.

### 1.1. Files

| File                       | Role                                                                 |
| -------------------------- | -------------------------------------------------------------------- |
| `LedgerUT.cmake`           | Entry point. Fetches Unity/CMock and defines the public macros.      |
| `ut_include_dirs.cmake`    | `UT_INCLUDE_DIRS_SDK`: SDK include paths needed to compile tests.    |
| `ut_compile_defines.cmake` | `UT_COMPILE_DEFS_SDK`: SDK `-D` defines (target Flex by default).    |
| `cmock_config.yml.in`      | CMock configuration template (mock prefix, plugins, strippables).   |
| `gen_coverage.sh`          | Generates `coverage.info` / `coverage.xml` and an HTML report.      |

### 1.2. Requirements

- CMake ≥ 3.10
- A C11 host compiler (GCC recommended — coverage uses `gcov`)
- Ruby (CMock's mock generator runs on Ruby — found via `find_program`)
- `lcov` / `genhtml` for coverage, and optionally `lcov_cobertura` for `coverage.xml`

### 1.3. Quick start

Create a `CMakeLists.txt` for your app's unit tests (e.g. in `tests/unit/`),
pointing `SDK_DIR` at the SDK checkout and including `LedgerUT.cmake`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app_unit_tests LANGUAGES C)

# Path to the ledger-secure-sdk checkout.
set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../sdk)

include(${SDK_DIR}/CMake/LedgerUT.cmake)

ledger_unit_tests_init()

# Discover all app sources (used to build the `app` coverage library).
ledger_unit_tests_find_sources()      # populates APP_SOURCES

ledger_unit_tests_build_app_sources_as_lib(
    SOURCES      ${APP_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    COMPILE_DEFS APPNAME="MyApp")

# A test executable. `<NAME>.c` is expected next to this CMakeLists.txt.
ledger_unit_tests_add_test(
    NAME         test_foo
    SOURCES      ../../src/foo.c          # real sources under test
    MOCK_HEADERS ${SDK_DIR}/include/os.h  # headers to mock (optional)
    INCLUDE_DIRS ../../src                # extra -I (optional)
    COMPILE_DEFS APPNAME="MyApp")         # extra -D (optional)
```

Then configure, build and run:

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

### 1.4. Public API

#### `ledger_unit_tests_init()`

Call once, before declaring any test. It:

- enables CTest (`enable_testing()`),
- defaults `CMAKE_BUILD_TYPE` to `Debug`,
- sets C11 and the coverage flags (`--coverage`, `-O0 -g`, `-Wall -pedantic`),
- forbids in-source builds,
- creates the mock output directory and renders `cmock_config.yml` from the
  template,
- adds the global `TEST` define and silences `PRINTF(...)`.

#### `ledger_unit_tests_find_sources()`

Globs every `*.c` under `../src` (relative to the current source dir) into
`APP_SOURCES`, excluding `main.c` / `app_main.c`. Feed the result to
`ledger_unit_tests_build_app_sources_as_lib`.

#### `ledger_unit_tests_build_app_sources_as_lib(SOURCES … INCLUDE_DIRS … COMPILE_DEFS …)`

Builds all app sources into a static library named `app`. This library is **not**
linked into the test executables — it exists so that coverage reflects the whole
app codebase (every line is compiled), not only the files a given test happens to
pull in. Each test stays isolated and links just its own sources.

#### `ledger_unit_tests_add_test(NAME … SOURCES … MOCK_HEADERS … INCLUDE_DIRS … COMPILE_DEFS …)`

Declares one Unity/CMock test executable and registers it with CTest.

| Argument       | Required | Meaning                                                          |
| -------------- | -------- | ---------------------------------------------------------------- |
| `NAME`         | yes      | Test name. The test entry file must be `<NAME>.c`.               |
| `SOURCES`      | no       | Real source files under test (compiled into this test).          |
| `MOCK_HEADERS` | no       | Headers from which CMock generates `Mock<name>.c` stubs.         |
| `INCLUDE_DIRS` | no       | Extra include directories (added *before* the defaults).         |
| `COMPILE_DEFS` | no       | Extra preprocessor defines.                                      |

Mocks are generated at most once per header, even if several tests mock it
(deduplicated via the `CMOCK_MOCKED_HEADERS` global property).

### 1.5. Coverage

A `generate_coverage` target is created automatically. After building and running
the tests:

```bash
cmake --build build --target generate_coverage
```

`gen_coverage.sh` captures gcov data, drops the test and `_deps/` files, and emits:

- `coverage.info`  — LCOV tracefile
- `coverage/`      — HTML report (`genhtml`)
- `coverage.xml`   — Cobertura report for CI (only if `lcov_cobertura` is installed)

The script auto-detects whether it runs against the `app` library
(`CMakeFiles/app.dir`) or plain SDK tests and scopes the report accordingly.

### 1.6. Tuning the SDK defines / includes

The default `UT_COMPILE_DEFS_SDK` targets **Flex** (`TARGET_FLEX`, `TARGET="flex"`,
`API_LEVEL=26`, full crypto/NBGL feature set). To test against another target or a
different feature set, override or append values via the `COMPILE_DEFS` /
`INCLUDE_DIRS` arguments of `ledger_unit_tests_build_app_sources_as_lib` and
`ledger_unit_tests_add_test`, or edit `ut_compile_defines.cmake` /
`ut_include_dirs.cmake` for project-wide changes.

---

## Conventions

Guidelines for adding a new module to this directory:

- **One module = one entry point** (`<Name>.cmake`), included by consumers via
  `include(${SDK_DIR}/CMake/<Name>.cmake)`.
- Prefix public macros/functions with the module name (e.g. `ledger_unit_tests_*`)
  to avoid clashing in the caller's scope.
- Keep large data sets (include paths, defines, …) in dedicated, well-named
  `*.cmake` files included by the entry point, as `ut_include_dirs.cmake` and
  `ut_compile_defines.cmake` do.
- Document the module by adding a new top-level section here and a row in the
  [Modules](#modules) table.
