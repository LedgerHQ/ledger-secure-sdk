# CMake modules — Usage

This directory hosts the reusable **CMake modules** shipped with the Ledger
Secure SDK. Each module is self-contained and documented in its own section
below. New modules are expected to be added over time; please keep this document
in sync (see [Conventions](#conventions)).

## Modules

| Module             | Entry point      | Purpose                                   |
| ------------------ | ---------------- | ----------------------------------------- |
| [Unit-Test framework](#1-unit-test-framework-ledgerut) | `LedgerUT.cmake` | Unity/CMock unit tests for the SDK and SDK-based apps |
| [App build](#2-app-build-ledgerapp) | `LedgerApp.cmake` | `app` library and device binary of an SDK-based app |

---

## 1. Unit-Test framework (`LedgerUT`)

A small framework, based on [Unity](https://github.com/ThrowTheSwitch/Unity) and
[CMock](https://github.com/ThrowTheSwitch/CMock), to write and run unit tests.

It takes care of:

- fetching Unity and CMock (via `FetchContent`, pinned to a fixed commit),
- generating mocks from SDK headers,
- registering tests with CTest,
- producing an LCOV/Cobertura coverage report.

`LedgerUT.cmake` is never included directly: the SDK root `CMakeLists.txt`
includes it when `BUILD_UNIT_TESTS` is on, which the `unit-tests` preset sets.
Two builds use it:

- **SDK unit tests**, configured from the SDK root. Each component declares its
  tests in `<component>/unit-tests/`, built only when the SDK is the top-level
  project.
- **App unit tests**, configured from the app with its `unit-tests` preset.
  `ledger_app()` adds the SDK, calls `ledger_unit_tests_init()` and builds the
  `app` library (see [App build](#2-app-build-ledgerapp)); the SDK tests are
  not built.

### 1.1. Files

| File                       | Role                                                                 |
| -------------------------- | -------------------------------------------------------------------- |
| `LedgerUT.cmake`           | Fetches Unity/CMock and defines the public macros.                   |
| `ut/include_dirs.cmake`    | `UT_INCLUDE_DIRS_SDK`: SDK include paths needed to compile tests.    |
| `ut/compile_defines.cmake` | `UT_COMPILE_DEFS_SDK`: host-only `-D` defines.                       |
| `ut/cmock_config.yml.in`   | CMock configuration template (mock prefix, plugins, strippables).   |
| `ut/gen_coverage.sh`       | Generates `coverage.info` / `coverage.xml` and an HTML report.      |

### 1.2. Requirements

- A C11 host compiler (GCC recommended — coverage uses `gcov`)
- Ruby (CMock's mock generator runs on Ruby — found via `find_program`)
- `lcov` / `genhtml` for coverage, and optionally `lcov_cobertura` for `coverage.xml`

### 1.3. Quick start

SDK unit tests, from the SDK root:

```bash
cmake --preset unit-tests
cmake --build --preset unit-tests
ctest --preset unit-tests
```

App unit tests use the same commands from the app root. The app declares its
tests after `ledger_app()`:

```cmake
ledger_unit_tests_add_test(
    NAME         test_foo
    SOURCES      src/foo.c                       # real sources under test
    MOCK_HEADERS ${LEDGER_SDK_ROOT}/include/os.h # headers to mock (optional)
    INCLUDE_DIRS ${LEDGER_APP_INCLUDE_DIRS} ${UT_INCLUDE_DIRS_SDK}
    COMPILE_DEFS ${UT_COMPILE_DEFS_SDK})
```

### 1.4. Public API

#### `ledger_unit_tests_init()`

Called by the SDK root (SDK unit tests) or by `ledger_app()` (app unit tests).
It:

- enables CTest (`enable_testing()`),
- defaults `CMAKE_BUILD_TYPE` to `Debug`,
- sets C11 and the coverage flags (`--coverage`, `-O0 -g`, `-Wall -pedantic`),
- forbids in-source builds,
- creates the mock output directory and renders `cmock_config.yml` from the
  template,
- adds the global `TEST` define and silences `PRINTF(...)`.

#### `ledger_unit_tests_add_test(NAME … SOURCES … MOCK_HEADERS … INCLUDE_DIRS … COMPILE_DEFS … COMPILE_OPTIONS … LINK_OPTIONS …)`

Declares one Unity/CMock test executable and registers it with CTest. The test
gets the target and feature defines from `ledger::target-profile`.

| Argument          | Required | Meaning                                                          |
| ----------------- | -------- | ---------------------------------------------------------------- |
| `NAME`            | yes      | Test name. The test entry file must be `<NAME>.c`.               |
| `SOURCES`         | no       | Real source files under test (compiled into this test).          |
| `MOCK_HEADERS`    | no       | Headers from which CMock generates `Mock<name>.c` stubs.         |
| `INCLUDE_DIRS`    | no       | Extra include directories (added *before* the defaults).         |
| `COMPILE_DEFS`    | no       | Extra preprocessor defines.                                      |
| `COMPILE_OPTIONS` | no       | Extra compiler flags.                                            |
| `LINK_OPTIONS`    | no       | Extra linker flags.                                              |

Mocks are generated at most once per header, even if several tests mock it
(deduplicated via the `CMOCK_MOCKED_HEADERS` global property).

### 1.5. Coverage

A `generate_coverage` target is created automatically. After building and running
the tests:

```bash
cmake --build --preset unit-tests --target generate_coverage
```

`gen_coverage.sh` captures gcov data, keeps the project sources, drops the build
directory (mocks, `_deps/`) and the `unit-tests/` directories, and emits:

- `coverage.info`  — LCOV tracefile
- `coverage/`      — HTML report (`genhtml`)
- `coverage.xml`   — Cobertura report for CI (only if `lcov_cobertura` is installed)

The script auto-detects whether it runs against the `app` library
(`CMakeFiles/app.dir`) or plain SDK tests and scopes the report accordingly.

### 1.6. Target and features

Tests are built for the target and features of the `unit-tests` preset, through
`ledger::target-profile`, the same profile as the device build. Change them in
the preset, or with `ledger_app()` `FEATURES` / `DISABLE` for an app.

## 2. App build (`LedgerApp`)

Included by the app, which then calls `ledger_app()`; `ledger_app()` adds the SDK.

### 2.1. App files

The app `CMakePresets.json` only includes the SDK presets, which declare one preset per
device (`nanox`, `nanos2`, `stax`, `flex`, `apex_p`, `apex_m`) and `unit-tests`:

```json
{
  "version": 7,
  "include": ["$penv{BOLOS_SDK}/cmake/presets/LedgerAppPresets.json"]
}
```

The app `CMakeLists.txt` starts with:

```cmake
cmake_minimum_required(VERSION ${LEDGER_CMAKE_MINIMUM_VERSION})
project(my-app VERSION 1.0.0 LANGUAGES C)
include($ENV{BOLOS_SDK}/cmake/LedgerApp.cmake)
```

`LEDGER_CMAKE_MINIMUM_VERSION` comes from the SDK presets, so raising the CMake minimum
version is done in the SDK only.

The version comes from the app `project(... VERSION x.y.z)` and gives
`APPVERSION`, `MAJOR_VERSION`, `MINOR_VERSION` and `PATCH_VERSION`.

### 2.2. App manifest

`ledger_app.toml` at the app root is required. Its `[metadata]` `author` gives
`APP_METADATA_AUTHOR`, and the `COPYRIGHT` argument of `ledger_app()` gives
`APP_METADATA_COPYRIGHT`; a missing one fails the
configure step. Editing the manifest re-runs the configure step.

### 2.3. `ledger_app()`

```cmake
ledger_app(
    NAME            "MyApp"
    COPYRIGHT       "(c) 2026 My Company"
    ICON_NANOX      icons/app_14px.gif
    ICON_NANOS2     icons/app_14px.gif
    ICON_STAX       icons/app_32px.gif
    ICON_FLEX       icons/app_40px.gif
    ICON_APEX_P     icons/app_32px_apex.png
    GLYPHS_DIR      glyphs                # optional
    FEATURES        BLUETOOTH NBGL_QRCODE # optional
    DISABLE         STANDARD_U2F          # optional
    EXCLUDE_SOURCES src/unused.c          # optional
    CURVES          secp256k1
    PATHS           "44'/1'"
)
```

| Argument          | Meaning                                                              |
| ----------------- | -------------------------------------------------------------------- |
| `NAME`            | Application name (`APPNAME`), required                               |
| `COPYRIGHT`       | Copyright notice (`APP_METADATA_COPYRIGHT`), required                |
| `ICON_<TARGET>`   | App icon, required for the target being built                        |
| `GLYPHS_DIR`      | App glyphs directory                                                 |
| `ICON_HOME_NANO`  | Nano home screen icon, generated from the Nano app icon; its file name gives the glyph name |
| `FEATURES`        | Sets `ENABLE_<feature>`, see `sdk/target_profile.cmake`              |
| `DISABLE`         | Sets `DISABLE_<feature>`, see `sdk/target_profile.cmake` and `sdk/compile_options.cmake` |
| `PERMISSIONS`     | Sets `HAVE_APPLICATION_FLAG_<permission>`, see `include/appflags.h`  |
| `CUSTOM_APP_FLAGS`| Extra application flags, added to the computed ones                  |
| `EXCLUDE_SOURCES` | Sources left out of `app`                                            |
| `INCLUDE_DIRS`    | Extra include directories                                            |
| `CURVES`, `PATHS` | Derivation curves and paths, required for a device build             |

Paths are relative to the app root. A `PERMISSIONS`, `FEATURES` or `DISABLE` name with
no matching SDK option fails the configure step.
The stack protector and LTO are on unless disabled with `DISABLE STACK_PROTECTOR`
or `DISABLE LINK_TIME_OPTIMIZATION`.

The application flags are computed as in `Makefile.standard_app`, from `PERMISSIONS`,
`CUSTOM_APP_FLAGS` and the features (BLE or NFC give `BOLOS_SETTINGS`,
`SWAP` gives `LIBRARY`).

`ledger_app()` adds the SDK, then creates the `app` static library from every `src/**/*.c` of the app, minus
`EXCLUDE_SOURCES` (paths relative to the app root). Every `src/` directory
holding a header is an include directory, listed in `LEDGER_APP_INCLUDE_DIRS`
afterwards; `INCLUDE_DIRS` (optional) adds others. Adding a source or header
file re-runs the configure step. For a device build, it also creates
`bin/app.elf`, `bin/app.hex`, `bin/app.apdu` and `app.map`. For a unit-test build, it calls
`ledger_unit_tests_init()` and leaves out `main.c` / `app_main.c`.

As `make load` and `make delete`, a device build installs or removes the app on a
connected device:

```bash
cmake --build --preset flex --target load
cmake --build --preset flex --target delete
```

---

## Conventions

Guidelines for adding a new module to this directory:

- **One module = one entry point** (`<Name>.cmake`), the only kind of file at the
  top level of `cmake/`, included by consumers via
  `include($ENV{BOLOS_SDK}/cmake/<Name>.cmake)`.
- Everything else goes in a subdirectory: `sdk/` for the SDK build itself, `app/`
  and `ut/` for the internals of `LedgerApp` and `LedgerUT`, plus `presets/` and
  `toolchains/`.
- Prefix public macros/functions with the module name (e.g. `ledger_unit_tests_*`)
  to avoid clashing in the caller's scope.
- Document the module by adding a new top-level section here and a row in the
  [Modules](#modules) table.
