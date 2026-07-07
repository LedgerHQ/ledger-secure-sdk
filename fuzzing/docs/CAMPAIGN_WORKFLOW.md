# Campaign Workflow Reference

Operational reference for running `app-campaign.sh`, understanding `.zon`
source paths, and driving the pipeline manually.

For app-side integration (which files an app must ship, what each one must
contain) see [APP_CONTRACT.md](APP_CONTRACT.md).

## `app-campaign.sh`

Invoke the campaign from the SDK. The **campaign name** is the last
positional argument (optional). It becomes the directory name under
`<app>/.fuzz-artifacts/<name>/`. If you omit it, the script uses a UTC
timestamp (`campaign-…`).

```bash
BOLOS_SDK=/path/to/ledger-secure-sdk \
  "$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir /path/to/app my-campaign
```

Use an absolute `--app-dir` (or `export APP_DIR=…`) so behaviour does not
depend on the shell’s current working directory.

### CLI flags

| Flag                   | Description                                                       |
|------------------------|-------------------------------------------------------------------|
| `--app-dir DIR`        | Path to the app root (required)                                   |
| `--fuzz-subdir DIR`    | Override the fuzzing subtree (default: `fuzzing`)                 |
| `--target NAME`        | Restrict to one fuzzer (repeatable; multi-target manifest only)   |
| `--clean`              | Delete build directories before configuring (full rebuild)        |

### Environment variables

| Variable               | Default | Meaning                                             |
|------------------------|---------|-----------------------------------------------------|
| `WARMUP_SEC`           | `30`    | Warmup duration **per worker** (seconds). Wide exploration from bootstrap seeds. |
| `MAIN_SEC`             | `60`    | Main phase **per worker** (seconds). Deeper mutations from merged warmup corpus. |
| `WORKERS`              | `min(2, nproc)` | LibFuzzer processes in parallel. Default caps at two for lighter local/CI use; set `WORKERS=8` (etc.) for long runs. On machines with one CPU, only one worker is started. Override the cap with `FUZZ_DEFAULT_WORKERS` (used only when `WORKERS` is unset). |
| `EXTRA_CORPUS`         | unset   | **Colon-separated** list of corpus directories copied into the bootstrap corpus after generated seeds. Use to feed a **prior merged corpus** (e.g. `.fuzz-artifacts/old-run/targets/fuzz_globals/corpus`). If a directory contains `.compat-key`, it must match the current build; otherwise the script errors (prevents corrupt state / wrong prefix size). |
| `BASE_CORPUS_DIR`      | `<app>/fuzzing/base-corpus` if that path exists | Additional promoted seeds (same compat-key rules when `.compat-key` is present). Set to empty to skip, e.g. when the checked-in corpus is stale: `BASE_CORPUS_DIR=`. |
| `BUILD_JOBS`           | derived from CPU count | Parallel `cmake --build` jobs. Lower to reduce compile CPU spikes. |
| `APP_TARGET`           | `flex`  | Device target for CMake (`flex`, `stax`, …).        |
| `OVERWRITE`            | unset   | Set to `1` to replace an existing `.fuzz-artifacts/<run>/` tree. |
| `SKIP_INVARIANT_SYNC`  | unset   | Skip `sync-invariant.py` (keep hand-tuned `.zon`)   |
| `BOOTSTRAP_INVARIANT`  | `auto`  | Reset every target's `invariants/<target>.zon` to `.{}` before configure so Absolution rediscovers the model from scratch (avoids `WholeValuesBlobMismatch` after sanitizer / SDK / toolchain / host changes). Values: `1` (always reset), `0` (never), `auto` (reset only with `--clean`). CI / ClusterFuzzLite builds should pass `BOOTSTRAP_INVARIANT=1` because each matrix entry has its own global layout. |
| `LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR` | unset | Path to a local Absolution install (`<dir>/bin/absolution` + `<dir>/lib/cmake/Absolution/`). When set, CMake skips the GitHub download. |
| `BOLOS_SDK`            | parent of `fuzzing/` when unset | Must point at this SDK checkout for paths and CMake. |
| `ARTIFACTS_ROOT`       | `<app>/.fuzz-artifacts` | Root directory for campaign output.          |
| `BUILD_DIR_FAST` / `BUILD_DIR_COV` | `<app>/build/fast` and `build/cov` | Sanitizer fuzz binary vs coverage replay binary. |

Short defaults (`30`/`60` s, two workers) keep the first successful run
fast. For meaningful coverage growth or regression hunting, increase
`WARMUP_SEC`, `MAIN_SEC`, and `WORKERS` (e.g. `WARMUP_SEC=300 MAIN_SEC=3300
WORKERS=4`). To cap machine load without editing the script, use
`WORKERS=1` and/or lower `BUILD_JOBS`.

### Absolution dependency

Absolution is a CMake `FetchContent` dependency of `ledger_fuzz_setup()`. On
first configure, CMake downloads the pinned `v1.1.0` Linux release zip from
GitHub into the build directory and sets `Absolution_DIR`,
`ABSOLUTION_EXECUTABLE`, and the build/install RPATHs accordingly. No script
flag, env var, or sibling checkout is involved.

To skip the download (offline machines, unreleased Absolution), set the
CMake variable or env var `LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR` to a directory
containing `bin/absolution` and `lib/cmake/Absolution/`:

```bash
export LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR=/absolute/path/to/absolution
```

### Campaign flow (single-target)

1. Configure and build the fast binary.
2. Sync the invariant (unless skipped).
3. Configure and build the coverage binary.
4. Update `scenario_layout.h`.
5. Write the LibFuzzer dictionary from the manifest.
6. Generate seeds.
7. Copy `<app>/fuzzing/base-corpus/` if present.
8. Add any `EXTRA_CORPUS`.
9. Fuzz (warmup → main), merge corpus, replay, write coverage reports.

### Campaign flow (multi-target)

1. Configure CMake once for all targets.
2. Build every target's fast binary.
3. Sync invariants per target, rebuild.
4. Configure and build every coverage binary.
5. Per target: seed, warmup, main fuzz, corpus merge, replay.
6. Merge all targets' profraw files.
7. Generate one combined HTML coverage report
   (`llvm-cov` with `--object` per binary).

The `--target` flag selects a subset. Without it, every `[[targets]]` entry
is fuzzed.

### SDK self-fuzz invocation

```bash
"$BOLOS_SDK"/fuzzing/scripts/app-campaign.sh \
  --app-dir "$BOLOS_SDK" \
  --fuzz-subdir fuzzing/sdk-fuzz
```

## Invariant `.zon` files and `/app/...` paths

Each fuzzer has an `invariants/<fuzzer>.zon` (Absolution model) that contains
`.source_file = "/path/..."` strings. Those paths are **Absolution metadata**
(copied from the generated `build/<fast>/_absolution/<fuzzer>/fuzzer.c.zon`
on the machine that last ran the pipeline).

Apps **must not** commit their per-target `fuzz_globals.zon`: the file is
a machine-local snapshot whose `.whole_values` blob sizes depend on the
build's global memory layout (sanitizer choice, SDK revision, toolchain).
A committed `.zon` from one machine/config will fail on another with
`WholeValuesBlobMismatch`. The framework regenerates the file on demand
(see "machine-local artefact" in [APP_CONTRACT.md](APP_CONTRACT.md)).
SDK self-fuzz models under `fuzzing/sdk-fuzz/invariants/*.zon` and the
generated `fuzzing/sdk-fuzz/mock/scenario_layout.h` follow the same rule:
they are gitignored and regenerated on every fresh configure. On the
first build, CMake bootstraps each missing `.zon` to `.{}` so Absolution
can discover the layout from scratch — the campaign then writes a real
model in place. Only `invariants/zero-symbols.txt` and
`domain-overrides.txt` (hand-tuned policy inputs) are tracked.

- Fuzzing does not require your checkout to live at `/app`. On a laptop, the
  first **configure + build** produces a fresh `fuzzer.c.zon` whose
  `.source_file` entries match **your** absolute paths.
- `sync-invariant.py` (run by the campaign or manually below) rewrites
  `fuzz_globals.zon` from that generated file. After a successful sync on
  your machine, paths in `fuzz_globals.zon` match your tree (until someone
  commits a different snapshot).
- `zero-symbols.txt` selectors that use `@file.c` match when that substring
  appears anywhere in the path, so `io_ext.c` still matches
  `/home/you/.../io_ext.c`.

Doc examples use `/path/to/ledger-secure-sdk` and `--app-dir`; only
committed snapshots from the devcontainer often show `/app/...`.

## Manual workflow (without `app-campaign.sh`)

Use this when you only want to **configure**, **sync the invariant**,
**refresh `scenario_layout.h`**, or **run LibFuzzer** without
warmup/merge/coverage replay. Adjust `fuzz_globals` if your manifest
defines another target name.

**Environment (typical):**

```bash
export APP_DIR="/absolute/path/to/your-app"      # e.g. app-ethereum
export BOLOS_SDK="/absolute/path/to/ledger-secure-sdk"
export APP_TARGET="${APP_TARGET:-flex}"
SCRIPT_DIR="${BOLOS_SDK}/fuzzing/scripts"
# Optional, only needed for offline / unreleased Absolution:
# export LEDGER_FUZZ_ABSOLUTION_LOCAL_DIR=/absolute/path/to/absolution
```

**1. One shell with helpers loaded**

```bash
set -euo pipefail
source "${SCRIPT_DIR}/app-common.sh"
source "${SCRIPT_DIR}/app-config.sh"   # requires APP_DIR already set
```

**2. Configure and build the sanitizer (“fast”) fuzzer**

```bash
BUILD_FAST="${APP_DIR}/build/fast"
configure_fuzz_build "${APP_DIR}" "${BUILD_FAST}" RelWithDebInfo 0
build_fuzzer_target "${BUILD_FAST}" fuzz_globals
```

**3. Sync `fuzz_globals.zon` and rebuild if the invariant changed**

```bash
INV="${APP_DIR}/fuzzing/invariants/fuzz_globals.zon"
sync_invariant "${BUILD_FAST}" fuzz_globals "${INV}"
if [[ "${INVARIANT_CHANGED:-0}" == 1 ]]; then
  build_fuzzer_target "${BUILD_FAST}" fuzz_globals
fi
```

To **force** a resync, delete
`"${BUILD_FAST}/.fuzz-invariant-hash-fuzz_globals"` and run step 3 again.

**4. Refresh `mock/scenario_layout.h`**

```bash
LAYOUT="${APP_DIR}/fuzzing/mock/scenario_layout.h"
update_scenario_layout "${BUILD_FAST}" fuzz_globals "${LAYOUT}"
```

**5. Optional: coverage instrumented binary** (separate directory; used for
`llvm-cov` replay, not for fast fuzzing)

```bash
BUILD_COV="${APP_DIR}/build/cov"
configure_fuzz_build "${APP_DIR}" "${BUILD_COV}" RelWithDebInfo 1
build_fuzzer_target "${BUILD_COV}" fuzz_globals
```

**6. Run LibFuzzer directly** (no dictionary / seeds unless you pass them)

```bash
mkdir -p /tmp/fuzz-corpus
"${BUILD_FAST}/fuzz_globals" /tmp/fuzz-corpus \
  -max_total_time=120 \
  -timeout=2
```

Optional dictionary from the manifest:

```bash
DICT=/tmp/fuzz.dict
python3 "${SCRIPT_DIR}/fuzz_manifest.py" --dict "${APP_DIR}/fuzzing/fuzz-manifest.toml" "${DICT}"
# single-target; for multi-target add: --fuzzer fuzz_globals
"${BUILD_FAST}/fuzz_globals" /tmp/fuzz-corpus -dict="${DICT}" -max_total_time=120
```

**7. Seeds** without the full campaign: each app’s `fuzzing/scripts/` may
ship a generator (e.g. `generate-seed-corpus.py`); run it with `APP_DIR`
and `BOLOS_SDK` set per that script’s help. The campaign wires the same
generators automatically.

Clang, the matching `llvm-*` tools, and Ninja come from the standard
`ledger-app-dev-tools` image (`pick_clang` in `app-common.sh` resolves
them); no extra install is needed.

## Compatibility key

Corpora are versioned by:

```text
sha256(prefix_size || sha256(fuzz_globals.zon) || fuzzer || harness_version)
```

If a promoted corpus carries a `.compat-key`, `app-campaign.sh` rejects it
when it does not match the current build.
