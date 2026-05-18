# SDK Fuzz Mock Layer

The SDK mock layer replaces hardware-dependent SDK code with host-side
implementations so fuzz builds link and run on Linux. Apps consume it
through `ledger_fuzz_secure_sdk`; nothing in this directory is copied into apps.

## Layout

| Path           | Replaces                                                            |
|----------------|---------------------------------------------------------------------|
| `cx/`          | `lib_cxng` crypto, big-number, and EC point helpers                 |
| `nbgl/`        | NBGL runtime state and use-case glue (auto-drives UI callbacks)     |
| `os/`          | OS, PIC, exception, libc, NVM, and I/O runtime shims                |
| `_generated/`  | Weak syscall stubs produced by `gen_mock.py` from `src/syscalls.c`  |
| `tlv_mutator.c`| Optional TLV grammar-aware mutator source (opt-in per app)          |
| `gen_mock.py`  | Generator that scans SDK syscalls and emits the weak stubs above    |
| `mock.cmake`   | Builds the `ledger_fuzz_mock` library and its include / link graph  |

## Strong vs weak overrides

`_generated/generated_syscalls.c` defines every SDK syscall as a weak
no-op. Hand-written files in `cx/`, `nbgl/`, and `os/` provide stronger
overrides for the surfaces the framework actively models. The linker
keeps the strong symbol when both exist, so:

- Symbols handled by hand-written mocks behave like the framework expects.
- Every other syscall still links cleanly through the weak stub, even if
  the app does not exercise it.

This keeps the mock surface explicit and avoids accidental drift when a
new syscall is added to the SDK.

## When to add a mock here vs in the app

Add a mock to the SDK mock layer when **all** of the following hold:

- The symbol is part of the SDK surface (cxng, NBGL, OS, syscalls).
- More than one app can reasonably need it.
- Behaviour does not depend on any single app's protocol.

Keep it inside the app's own `fuzzing/mock/` directory when:

- The mock implements an app-specific protocol (e.g. Bitcoin's PSBT
  semantic host, Ethereum's TLV preimages).
- The behaviour only makes sense alongside one app's harness, semantic
  host, or invariant.

App-local mocks are wired through that app's `CMakeLists.txt`. Examples
exist under `app-bitcoin-new/fuzzing/mock/` (semantic host, BIP32, SHA-256
streaming) and `app-ethereum/fuzzing/mock/`.

## CMake graph

`mock.cmake` is included once by `fuzzing/v2/libs/lib_*.cmake` aggregators.
It builds a single `ledger_fuzz_mock` static library from the lists above
plus `src/os.c`, `src/ledger_assert.c`, and `src/cx_wrappers.c` from the SDK
itself. Apps inherit the library through `target_link_libraries(...
ledger_fuzz_secure_sdk)`.

The library's include path is intentionally minimal:

- `fuzzing/v2/mock/_generated/` for the generated syscall declarations.
- `target/${TARGET}/` for device-specific headers used by mocks.

App or SDK headers needed to compile mocks are pulled in transitively
through `ledger_fuzz_cxng`, `ledger_fuzz_nbgl`, `ledger_fuzz_standard_app`,
and `ledger_fuzz_macros`.

## Adding a new strong override

1. Pick the right subdirectory (`cx/`, `nbgl/`, `os/`).
2. Add the source file there with a short header line stating which SDK
   surface it overrides.
3. List the new file in the `LIB_MOCK_SOURCES` set in `mock.cmake`.
4. Run a zero-second campaign on `app-boilerplate` and the SDK self-fuzz
   targets to confirm the build links and behaviour does not change.

`gen_mock.py` regenerates `_generated/generated_syscalls.c` automatically
during the build, so adding a new SDK syscall does not require manual
work in the mock directory unless behaviour matters.
