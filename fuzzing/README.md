# Ledger SDK Fuzzing Framework

Coverage-guided fuzzing for the Ledger Secure SDK and Ledger apps. The
framework wraps LibFuzzer and clang sanitizers, and uses
[Absolution](https://github.com/Ledger-Donjon/absolution) to drive global
state through declarative invariants.

This is the SDK fuzzing framework. CMake targets keep the original
v1-compatible names (`secure_sdk`, `standard_app`, `cxng`, `nbgl`, ...) so
apps with existing `add_subdirectory(${BOLOS_SDK}/fuzzing)` integrations
build unchanged. Apps only add their own `fuzzing/` subtree; nothing is
copied out of the SDK.

## Concepts in plain English

- **Campaign**: one fuzzing run. `scripts/app-campaign.sh` builds the fuzzer,
  generates seeds, runs a short **warmup** phase (wide coverage fast), then a
  longer **main** phase (depth-first from the warmup corpus), then replays
  the surviving corpus against a coverage build to produce an HTML report.

- **Corpus**: the set of inputs LibFuzzer keeps around because each one
  triggered a new code path. The fuzzer refines it continuously — adding
  interesting inputs, dropping redundant ones. A campaign grows its corpus
  from an initial set of seeds.

- **Seed**: a starter input used before the fuzzer begins mutating. Seeds
  come from the dictionary and templates declared in `fuzz-manifest.toml` and
  from any `base-corpus/` directory the manifest points at. Good seeds
  shorten the time to first coverage.

- **Base corpus**: a promoted, on-disk corpus checked into the app (by
  convention at `<app>/fuzzing/base-corpus/`). The pipeline picks it up as
  additional seeds for the next campaign. Promote a corpus once it covers
  the features you care about so future runs start from that state.

- **Invariant** (`.zon` file): an Absolution description of the fuzz target's
  global state — which fields exist, their widths, their value domains
  (enums, BIP32 paths, swap flags, etc.). Absolution uses it to interpret
  the start of each fuzzer input as a description of the initial state.

- **Prefix** / **tail**: every fuzzer input is split by Absolution into two
  halves. The *prefix* (first N bytes) sets up global state via the
  invariant. The *tail* (remaining bytes) is the APDU / payload the harness
  actually processes. The split offset is written to `scenario_layout.h` by
  the pipeline after each build.

- **Zero-symbols** (`invariants/zero-symbols.txt`): globals the prefix must
  forcibly zero instead of driving — typically large buffers, display state,
  or framework bookkeeping the app does not need to fuzz. Keeps the prefix
  small and focused.

- **Domain overrides** (`invariants/domain-overrides.txt`): per-symbol
  constraints on what values Absolution may place in the prefix (e.g.
  restrict a `uint8_t` enum to `{0,1,2}`). Improves convergence on the
  states the app actually reaches.

## Quickstart

Fuzz an existing app (it must provide a `fuzzing/` folder that follows
[docs/APP_CONTRACT.md](docs/APP_CONTRACT.md)):

```bash
BOLOS_SDK=/path/to/ledger-secure-sdk \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir /path/to/app my-campaign
```

- **`my-campaign`** is the run name (optional). It becomes the directory name
  under `.fuzz-artifacts/`. If you omit it, the script uses a UTC timestamp.
- Prefer **`--app-dir /absolute/path`** (or **`export APP_DIR=...`**) so the
  script does not depend on the current working directory.

Fuzz the SDK's own targets (10 built-in fuzzers under `sdk-fuzz/`):

```bash
"$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir "$BOLOS_SDK" \
  --fuzz-subdir fuzzing/sdk-fuzz sdk-sanity
```

Each run writes artefacts to `<app-dir>/.fuzz-artifacts/<campaign-name>/`:

- `targets/<fuzzer>/base-corpus/` — starting seeds
- `targets/<fuzzer>/warmup/`, `warmup-merged/`, `main/` — per-worker corpora
- `targets/<fuzzer>/meta.env`, `<fuzzer>.dict` — run metadata
- `report/index.html` — combined LLVM source-level coverage

Crashes, if any, appear as `crash-*` files under the worker directories and
are summarised at the end of the run.

Common environment variables (defaults favour quick local sanity runs; raise
times and `WORKERS` for overnight or farm jobs):

| Variable         | Default              | Meaning |
|------------------|----------------------|---------|
| `WARMUP_SEC`     | `30`                 | Per-worker warmup duration (seconds). Explores from the bootstrap corpus. |
| `MAIN_SEC`       | `60`                 | Per-worker main phase (seconds). Mutates from the merged warmup corpus. |
| `WORKERS`        | `min(2, nproc)`      | Parallel LibFuzzer processes. Use `1` to minimise CPU; increase for throughput. Cap is `FUZZ_DEFAULT_WORKERS` (default `2`). |
| `EXTRA_CORPUS`   | unset                | Colon-separated list of **extra corpus directories** merged into bootstrap (after seeds). Each tree may carry a `.compat-key`; it must match the current build or the script aborts. Use this to chain campaigns (e.g. prior run’s `targets/<fuzzer>/corpus`). |
| `BASE_CORPUS_DIR`| app’s `fuzzing/base-corpus` if present | Promoted on-disk seeds. Set to empty (`BASE_CORPUS_DIR=`) to skip when the directory is incompatible with the current `compat-key`. |
| `BUILD_JOBS`     | CPU-based            | Parallel compile jobs during `cmake --build`. Lower to reduce peak CPU. |
| `OVERWRITE`      | unset                | Set to `1` to replace an existing `.fuzz-artifacts/<run-name>/` directory. |
| `APP_TARGET`     | `flex`               | BOLOS target passed to CMake (`flex`, `stax`, …). |

Full CLI flags, compatibility keys, why `.zon` files contain `/app/...` paths,
and how to configure / sync / run LibFuzzer without `app-campaign.sh`:

[docs/CAMPAIGN_WORKFLOW.md](docs/CAMPAIGN_WORKFLOW.md).

## Prerequisites

- Clang ≥ 14 with `llvm-profdata` and `llvm-cov` (for coverage reports).
- Network access on first configure: CMake fetches the pinned Absolution
  `v1.1.0` Linux release automatically. Set `LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR` (CMake
  variable or env var) to a local Absolution install to skip the download
  (offline / unreleased Absolution).
- `BOLOS_SDK` pointing at a checkout of this SDK.

## Directory layout

Paths are relative to `${BOLOS_SDK}/fuzzing/`.

| Path               | Purpose                                                                                   |
|--------------------|-------------------------------------------------------------------------------------------|
| `cmake/`           | `LedgerAppFuzz.cmake` — the CMake module apps include                                     |
| `docs/`            | App contract, campaign workflow, and SDK-specific fuzz target notes                       |
| `template/`        | Minimal app fuzzing scaffold; copy into a new app as `fuzzing/`                           |
| `scripts/`         | Campaign pipeline (`app-campaign.sh`, seed generators, invariant / layout sync)           |
| `sdk-fuzz/`        | Self-fuzz targets exercising the framework with the same app contract                     |
| `harness/`         | Plain libFuzzer harness sources reused by `extra/*.cmake`                                 |
| `extra/`           | Per-library standalone fuzz targets (classic libFuzzer, no Absolution)                    |
| `include/`         | Framework headers + optional TLV grammar-aware mutator                                    |
| `libs/`            | Per-library CMake modules aggregated into the `secure_sdk` INTERFACE target               |
| `macros/`          | Build macro extraction (`make list-defines`) and add / exclude lists                      |
| `mock/cx/`         | Strong crypto, big-number, and EC point mocks                                             |
| `mock/nbgl/`       | NBGL runtime and use-case mocks                                                           |
| `mock/os/`         | OS, PIC, exception, libc, NVM, and I/O runtime shims                                      |
| `mock/_generated/` | Generated weak syscall stubs                                                              |
| `mock/gen_mock.py` | Generator producing weak syscall stubs into `_generated/`                                 |
| `invariants/`      | SDK-level zero-symbol policy (applied to every app)                                       |
| `sanitizers/`      | UBSan / ASan runtime config and ignorelists                                               |

## For app developers

Start from [`template/`](template/README.md) and follow
[`docs/APP_CONTRACT.md`](docs/APP_CONTRACT.md). The app owns its `fuzzing/`
folder: manifest, CMake file, harness, app-local mocks, invariants, macros,
and optional seeds. Shared SDK mocks and libraries come from
`include(${BOLOS_SDK}/fuzzing/cmake/LedgerAppFuzz.cmake)`.
