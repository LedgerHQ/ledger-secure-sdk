#!/usr/bin/env bash
# App configuration loader.
#
# Reads <APP_DIR>/<APP_FUZZ_SUBDIR>/fuzz-manifest.toml through fuzz_manifest.py
# and exports the values used by the campaign pipeline.
#
# Supports both single-target manifests ([target]) and multi-target manifests
# ([[targets]]).  For multi-target, only global settings are loaded at source
# time; per-target config is deferred to load_target_config().

# ── App directory (absolute) ─────────────────────────────────────────────────
if [[ -z "${APP_DIR:-}" ]]; then
  echo "error: APP_DIR is not set." >&2
  echo "hint: export APP_DIR=/path/to/app or use --app-dir." >&2
  exit 1
fi

# ── Scenario layout header location ─────────────────────────────────────────
APP_FUZZ_SUBDIR="${APP_FUZZ_SUBDIR:-fuzzing}"
export APP_FUZZ_SUBDIR
APP_FUZZ_DIR="${APP_DIR}/${APP_FUZZ_SUBDIR}"
SCENARIO_LAYOUT_HEADER="${APP_FUZZ_DIR}/mock/scenario_layout.h"
export SCENARIO_LAYOUT_HEADER

# ── App directory validation ─────────────────────────────────────────────────
if [[ ! -d "${APP_FUZZ_DIR}" ]]; then
  echo "error: app fuzzing directory not found at ${APP_FUZZ_DIR}" >&2
  echo "hint: set APP_DIR to the app root directory (e.g. /path/to/app-boilerplate)." >&2
  echo "      see \${BOLOS_SDK}/fuzzing/docs/APP_CONTRACT.md." >&2
  exit 1
fi
if [[ ! -f "${SCENARIO_LAYOUT_HEADER}" ]]; then
  echo "warning: scenario_layout.h not found at ${SCENARIO_LAYOUT_HEADER}" >&2
  echo "hint: copy from \${BOLOS_SDK}/fuzzing/template/mock/scenario_layout.h" >&2
fi

# ── Manifest path ────────────────────────────────────────────────────────────
_APP_MANIFEST="${APP_FUZZ_DIR}/fuzz-manifest.toml"
export _APP_MANIFEST

if [[ ! -f "${_APP_MANIFEST}" ]]; then
  echo "error: fuzz-manifest.toml not found at ${_APP_MANIFEST}" >&2
  echo "hint: create one from \${BOLOS_SDK}/fuzzing/template/fuzz-manifest.toml" >&2
  exit 1
fi

# ── Detect multi-target manifest ─────────────────────────────────────────────
_target_list=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --list-targets "${_APP_MANIFEST}" 2>&1) || {
  echo "error: failed to list targets: ${_target_list}" >&2
  exit 1
}
mapfile -t ALL_TARGETS <<< "${_target_list}"
export ALL_TARGETS

IS_MULTI_TARGET=0
if (( ${#ALL_TARGETS[@]} > 1 )); then
  IS_MULTI_TARGET=1
fi
export IS_MULTI_TARGET

# ── Framework defaults (overridden by manifest values) ───────────────────────
FUZZER="${FUZZER:-fuzz_globals}"
KEY_FILES_REL=()
LAYOUT_UPDATE_EXTRA_ARGS=()
COVERAGE_EXCLUDE_REGEXES=(
  '.*ledger-secure-sdk.*'
  '.*fuzz_dispatcher\.c'
  '.*fuzzer\.c'
  '.*fuzzing/mock/.*'
  '.*src/main\.c'
  '.*src/ui/menu_nbgl\.c'
)

# ── Load manifest values ─────────────────────────────────────────────────────
# For single-target manifests, load everything immediately (backward compat).
# For multi-target, load only global config here; per-target via load_target_config().
if [[ "${IS_MULTI_TARGET}" == "0" ]]; then
  _manifest_vars=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --shell "${_APP_MANIFEST}" 2>&1) || {
    echo "error: failed to read manifest: ${_manifest_vars}" >&2
    exit 1
  }
  eval "${_manifest_vars}"
  unset _manifest_vars
else
  # Multi-target: load global coverage excludes from the first target (they're shared).
  _manifest_vars=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --shell "${_APP_MANIFEST}" \
    --fuzzer "${ALL_TARGETS[0]}" 2>&1) || {
    echo "error: failed to read manifest: ${_manifest_vars}" >&2
    exit 1
  }
  # Only extract COVERAGE_EXCLUDE_REGEXES from the global config.
  eval "$(echo "${_manifest_vars}" | grep '^COVERAGE_EXCLUDE_REGEXES=')"
  unset _manifest_vars
fi

# ── load_target_config() ─────────────────────────────────────────────────────
# Load per-target config from the manifest.  Sets FUZZER, KEY_FILES_REL, and
# LAYOUT_UPDATE_EXTRA_ARGS for the given target name.
load_target_config() {
  local target_name="${1:?missing target name}"
  local _vars

  _vars=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --shell "${_APP_MANIFEST}" \
    --fuzzer "${target_name}" 2>&1) || {
    echo "error: failed to load config for target '${target_name}': ${_vars}" >&2
    return 1
  }
  eval "${_vars}"
}

# ── Promoted base corpus ──────────────────────────────────────────────────────
if [[ -z "${BASE_CORPUS_DIR+x}" ]]; then
  BASE_CORPUS_DIR="${APP_FUZZ_DIR}/base-corpus"
elif [[ -n "${BASE_CORPUS_DIR}" && ! -d "${BASE_CORPUS_DIR}" ]]; then
  echo "error: BASE_CORPUS_DIR does not exist at ${BASE_CORPUS_DIR}" >&2
  exit 1
fi
if [[ -n "${BASE_CORPUS_DIR}" && ! -d "${BASE_CORPUS_DIR}" ]]; then
  BASE_CORPUS_DIR=""
fi
export BASE_CORPUS_DIR

write_app_dictionary() {
  local manifest_path="${_APP_MANIFEST}"
  local fuzzer_flag=""
  if [[ "${IS_MULTI_TARGET}" == "1" && -n "${2:-}" ]]; then
    fuzzer_flag="--fuzzer ${2}"
  fi
  # shellcheck disable=SC2086
  python3 "${SCRIPT_DIR}/fuzz_manifest.py" --dict "${manifest_path}" "${1}" ${fuzzer_flag} >/dev/null
}

generate_app_seed_corpus() {
  local output_dir="${1:?missing output dir}"
  local fuzzer_flag=""
  if [[ "${IS_MULTI_TARGET}" == "1" && -n "${2:-}" ]]; then
    fuzzer_flag="--fuzzer ${2}"
  fi
  APP_DIR="${APP_DIR}" \
  SCENARIO_LAYOUT_HEADER="${SCENARIO_LAYOUT_HEADER}" \
  BUILD_DIR_FAST="${BUILD_DIR_FAST:-${APP_DIR}/build/fast}" \
  BUILD_DIR="${BUILD_DIR_FAST:-${BUILD_DIR:-}}" \
  BUILD_DIR_COV="${BUILD_DIR_COV:-${APP_DIR}/build/cov}" \
  FUZZER="${FUZZER}" \
  python3 "${SCRIPT_DIR}/generate-seeds.py" "${_APP_MANIFEST}" "${output_dir}" ${fuzzer_flag}
}
