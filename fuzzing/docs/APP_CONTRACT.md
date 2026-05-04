# App Fuzzing Contract

Defines what an app must ship under `<app>/fuzzing/` to plug into the SDK
fuzz framework. Code and this file must agree; if they don't, fix the code
or fix this document immediately.

- `app-boilerplate` is the standard filled example.
- `app-bitcoin-new` is the advanced custom-harness example.
- For campaign mechanics (CLI flags, environment, manual workflow,
  `.zon` paths), see [CAMPAIGN_WORKFLOW.md](CAMPAIGN_WORKFLOW.md).

## Mental model

Every fuzzer input is two parts:

```text
[ prefix | tail ]
```

- The **prefix** is Absolution-managed global state restored before each
  iteration. The invariant (`fuzz_globals.zon`) describes which bytes map
  to which globals and which domains they can take.
- The **tail** is the remaining bytes the harness interprets — typically
  one APDU reconstructed from `fuzz_ctrl` + `fuzz_tail_ptr / fuzz_tail_len`.

For standard apps, one iteration means:

1. Restore globals from the prefix (Absolution does this).
2. Shape one APDU from the tail (the harness does this via
   `fuzz_harness_entry()`).
3. Dispatch it through the real app dispatcher.

State that would normally be accumulated over several host messages can be
restored directly into globals by Absolution, so one APDU per iteration is
enough for coverage-guided fuzzing.

### Data flow

```text
LibFuzzer
  └─> LLVMFuzzerTestOneInput   (Absolution-generated)
        └─> fuzz_entry         (app, in harness/fuzz_dispatcher.c)
              └─> fuzz_harness_entry   (SDK, fuzzing/include/fuzz_harness.h)
                    ├─ pick a fuzz_command_spec_t
                    ├─ build a command_t from the tail
                    └─> fuzz_app_dispatch  (app)
                          └─> apdu_dispatcher  (real app code)
```

Advanced apps may bypass `fuzz_harness_entry()` but must keep the same
prefix/tail ownership, mutator wiring, and required symbols.

## Minimal directory layout

Every integrated app must provide a `fuzzing/` subtree at the app root:

```text
<app>/fuzzing/
  fuzz-manifest.toml
  CMakeLists.txt
  base-corpus/                       # optional, promoted seeds
  harness/
    fuzz_dispatcher.c
  mock/
    mocks.h
    mocks.c
    scenario_layout.h
  invariants/
    fuzz_globals.zon
    zero-symbols.txt
    domain-overrides.txt             # optional
  macros/
    add_macros.txt                   # optional
    exclude_macros.txt               # optional
```

## File ownership

| Path                              | Owner    | Notes                                                              |
|-----------------------------------|----------|--------------------------------------------------------------------|
| `fuzz-manifest.toml`              | app      | declarative app config (CLA, INS, key files, seeds, dictionary)    |
| `CMakeLists.txt`                  | app      | project + sources + include/define wiring                          |
| `harness/fuzz_dispatcher.c`       | app      | entry point, mutator wiring, dispatcher adapter                    |
| `mock/mocks.h` / `mock/mocks.c`   | app      | required globals and any app-specific mocks                        |
| `mock/scenario_layout.h`          | pipeline | overwritten by `scripts/update-scenario-layout.py` after each build|
| `invariants/fuzz_globals.zon`     | pipeline | overwritten by `scripts/sync-invariant.py` unless skipped          |
| `invariants/zero-symbols.txt`     | app      | globals removed from the prefix                                    |
| `invariants/domain-overrides.txt` | app      | per-field constraints applied after sync                           |
| `macros/add_macros.txt`           | app      | extra `-D` defines appended to the app Makefile defines            |
| `macros/exclude_macros.txt`       | app      | `-D` defines removed from the app Makefile defines                 |
| `base-corpus/`                    | app      | promoted seeds auto-imported by `app-campaign.sh`                  |

Anything that is _not_ app-specific (cxng, NBGL, system, BN/EC, syscalls)
lives in the SDK mock library; see
[`../mock/README.md`](../mock/README.md).

## `fuzz-manifest.toml`

Two shapes are supported. The campaign script detects which one is in use.

### Single-target (one fuzzer per app)

```toml
[target]
fuzzer = "fuzz_globals"
harness_version = "6"

[coverage]
key_files = ["src/handler/my_handler.c"]

[seeds]
cla = 0xE0
ins = [0x01]

[seeds.generic]
enabled = true

[seeds.custom]
enabled = false

[mocks]
override_sources = []
```

Required fields: `[target].fuzzer`, `[target].harness_version`,
`[coverage].key_files`, `[seeds].cla`, `[seeds].ins`.

### Multi-target (SDK self-fuzz or apps with several fuzzers)

```toml
[sdk]
harness_version = "1"

[coverage]
exclude_regexes = [".*fuzzing/mock/.*"]

[[targets]]
fuzzer = "fuzz_alloc"
key_files = ["lib_alloc/mem_alloc.c"]
seeds = { cla = 0x00, ins = [0x01] }

[[targets]]
fuzzer = "fuzz_base58"
key_files = ["lib_standard_app/base58.c"]
seeds = { cla = 0x00, ins = [0x01] }
```

Each `[[targets]]` entry inherits `harness_version` from `[sdk]` and may
override it. Coverage exclude regexes are shared across all targets. The
campaign runs all targets (or a `--target`-filtered subset) and produces
a single combined coverage report.

### Common optional sections

- `[layout].extra_args`: extra globals for `update-scenario-layout.py`.
- `[dictionary].tokens`: LibFuzzer dictionary entries (written to
  `<fuzzer>.dict`).
- `[seeds.custom]`: app-specific seed generator hook.

`harness_version` is part of the corpus compatibility key; bump it whenever
the input ABI changes in a way that invalidates older corpora.

## `CMakeLists.txt`

Apps include the SDK CMake module, call `ledger_fuzz_setup()`, then add
their fuzzer target with `ledger_fuzz_add_app_target()`.
`ledger_fuzz_setup()` fetches the pinned Absolution `v1.1.0` Linux release zip
from GitHub on first configure; set `LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR` (CMake
variable or env var) to skip the download for offline / unreleased
Absolution. Recommended shape:

```cmake
include_guard()
cmake_minimum_required(VERSION 3.14)

project(MyAppFuzzer VERSION 1.0 LANGUAGES C)

if(NOT DEFINED BOLOS_SDK)
  message(FATAL_ERROR "BOLOS_SDK must be defined")
endif()

include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)
ledger_fuzz_setup()

set(APP_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)
file(GLOB_RECURSE C_SOURCES
  "${APP_SOURCE_DIR}/src/*.c"
  "${CMAKE_SOURCE_DIR}/mock/*.c"
)
list(REMOVE_ITEM C_SOURCES "${APP_SOURCE_DIR}/src/app_main.c")

ledger_fuzz_add_app_target(
    SOURCES             ${C_SOURCES}
    INCLUDE_DIRECTORIES ${APP_SOURCE_DIR}/src/
                        ${CMAKE_SOURCE_DIR}/mock/
                        ${CMAKE_SOURCE_DIR}
    COMPILE_DEFINITIONS HAVE_SWAP   # app-specific flags only
)
```

`ledger_fuzz_add_app_target()` defaults:

- `NAME      fuzz_globals`
- `HARNESS   ${CMAKE_SOURCE_DIR}/harness/fuzz_dispatcher.c`
- `ENTRY     fuzz_entry`
- `INVARIANT ${CMAKE_SOURCE_DIR}/invariants/fuzz_globals.zon`
- Links `secure_sdk` and any `EXTRA_TARGETS` the app requests.

Apps with non-standard layouts can pass any of the above explicitly, or
bypass the helper and call `absolution_add_fuzzer()` directly.

## `harness/fuzz_dispatcher.c`

The standard harness must provide:

- `LLVMFuzzerCustomMutator(...)` — wire Absolution + optional structured lane
- `fuzz_commands[]` / `fuzz_n_commands` — APDU command table
- `fuzz_app_reset()` — per-iteration reset hook
- `fuzz_app_dispatch()` — adapter to the real app dispatcher
- `fuzz_entry(...)` — LibFuzzer entry, normally `fuzz_harness_entry(data, size)`

Minimal standard pattern:

```c
#include "mocks.h"
#include "scenario_layout.h"

#define FUZZ_PREFIX_SIZE_FALLBACK SCEN_PREFIX_SIZE
#define FUZZ_CTRL_OFF             SCEN_CTRL_OFF
#define FUZZ_CTRL_LEN             SCEN_CTRL_LEN
#define fuzz_lane_is_structured(data, ps) \
    ((ps) > FUZZ_CTRL_OFF && (data)[FUZZ_CTRL_OFF] > FUZZ_STRUCTURED_LANE_THRESHOLD)

#include "fuzz_mutator.h"
#include "fuzz_layout_check.h"
#include "fuzz_harness.h"

size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size,
                               size_t max_size, unsigned int seed) {
    return fuzz_custom_mutator(data, size, max_size, seed);
}

const fuzz_command_spec_t fuzz_commands[] = {
    { .cla = CLA, .ins = INS_GET_VERSION },
};
const size_t fuzz_n_commands = sizeof(fuzz_commands) / sizeof(fuzz_commands[0]);

void fuzz_app_reset(void) {}
void fuzz_app_dispatch(void *cmd) { apdu_dispatcher((const command_t *) cmd); }

int fuzz_entry(const uint8_t *data, size_t size) {
    return fuzz_harness_entry(data, size);
}
```

For the standard lane split, byte 0 of the control region selects the lane:

- `<= 102`: raw lane (one APDU from the tail)
- `> 102`: structured lane (app-defined, e.g. swap callback replay)

Apps that need a lane-specific command distribution (different commands
and/or weights in each lane) can override `FUZZ_PICK_COMMAND_RAW(data, size)`
and `FUZZ_PICK_COMMAND_STRUCTURED(data, size)` before including
`fuzz_harness.h`. Each macro must expand to a `const fuzz_command_spec_t *`
drawn from an app-owned table; the defaults pick uniformly from
`fuzz_commands[]` using `data[1]` / `fuzz_ctrl[1]`.

## `mock/mocks.h` and `mock/mocks.c`

Every app must expose these symbols:

```c
extern try_context_t fuzz_exit_jump_ctx;

#define FUZZ_CTRL_SIZE 16
extern uint8_t fuzz_ctrl[FUZZ_CTRL_SIZE];
extern const uint8_t *fuzz_tail_ptr;
extern size_t fuzz_tail_len;
```

And define them in `mocks.c`:

```c
uint8_t fuzz_ctrl[FUZZ_CTRL_SIZE];
const uint8_t *fuzz_tail_ptr = NULL;
size_t fuzz_tail_len = 0;
```

Recommended additions:

```c
/* PRINTF is stripped from the macro set by the SDK exclude list so app
 * sources compile it as a function call. This stub keeps it a no-op. */
#include <stdarg.h>
int PRINTF(const char *format, ...) { (void) format; return 0; }

void os_explicit_zero_BSS_segment(void) {
    /* No-op: zeroing BSS would erase the Absolution prefix state. */
}
```

Do not reimplement SDK-level semantic mocks here; those live under
`ledger-secure-sdk/fuzzing/mock/` and are linked via `secure_sdk`.

## `mock/scenario_layout.h`

Pipeline-owned. The first build can use bootstrap values:

```c
#define SCEN_PREFIX_SIZE 64
#define SCEN_CTRL_OFF    0
#define SCEN_CTRL_LEN    16
```

After each build, `scripts/update-scenario-layout.py` rewrites the values
from the generated Absolution layout. Advanced apps may append extra
`SCEN_*` offsets for app-specific globals reused from both C and Python.

## Invariants

`invariants/fuzz_globals.zon` starts as:

```text
.{}
```

During a campaign the framework:

1. Builds a discovery binary.
2. Reads the generated `fuzzer.c.zon`.
3. Applies SDK + app zero-symbol policies.
4. Rewrites `fuzz_globals.zon`.
5. Optionally applies `domain-overrides.txt`.

Use `SKIP_INVARIANT_SYNC=1` to keep a hand-tuned invariant file across
runs.

### `invariants/zero-symbols.txt`

Globals the prefix must not consume bytes for — forcibly zeroed before
each iteration. Good candidates:

- Large static buffers used as scratch (`G_io_apdu_buffer`, APDU staging).
- UI / NBGL bookkeeping globals not relevant to protocol fuzzing.
- `G_ux`, display descriptors, and similar framework state.
- Any global the SDK already resets deterministically.

Format: one symbol per line. File-local symbols use `symbol@file.c`:

```text
G_io_apdu_buffer
ram_buffer@nbgl_runtime.c
```

Adding a global here makes the prefix shorter and focuses coverage on the
bytes that drive app logic.

### `invariants/domain-overrides.txt`

Per-field constraints applied after the invariant is generated. Use these
when Absolution's default domain is too wide to converge — typically
enums, state machines, or bits that only have a handful of meaningful
values.

```text
G_context.state = values \x00 \x01 \x02      # state enum
G_called_from_swap. = values \x00 \x01       # boolean flag
G_swap_response_ready. = values \x00 \x01    # boolean flag
some_counter. = top                          # use full domain (opt in)
```

Trailing `.` means "the value of this symbol" (not its address); `values
\x00 \x01` means "restrict to these byte patterns". The sync step writes
these constraints back into `fuzz_globals.zon` on every run.

## Macros

### `macros/add_macros.txt`

Extra `-D` defines appended to the set `make list-defines` reports from
the app Makefile. Use this when the fuzz build needs a flag the release
build doesn't.

```text
FUZZ_APP_EXTRA_FEATURE=1
```

Lines starting with `#` are comments.

### `macros/exclude_macros.txt`

`-D` defines to remove from the extracted Makefile set. Use this to strip
release-mode shims that break in fuzz builds. At SDK level, for example,
`macros/exclude_macros.txt` removes `PRINTF(...)=` so fuzz builds see real
`PRINTF` calls (apps must link a stub; see `mocks.c`).

App-level example (rarely needed):

```text
SOME_RELEASE_ONLY_FLAG
```

## `base-corpus/`

An on-disk corpus (one file per input) promoted from previous campaigns.
The campaign script copies it under
`<artifacts>/targets/<fuzzer>/base-corpus/` as additional seeds for the
next run. Promote a corpus when:

- It covers the features you care about.
- You want future campaigns to start from that coverage (faster
  convergence, regression guard for cherry-picked fixes).

One promoted corpus snapshot per app is the preferred release shape. If
the corpus carries a `.compat-key`, `app-campaign.sh` rejects it when the
build key changes.
