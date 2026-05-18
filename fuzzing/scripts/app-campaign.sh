#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
FUZZ_DIR=$(realpath -m "${SCRIPT_DIR}/..")
BOLOS_SDK="${BOLOS_SDK:-$(realpath -m "${FUZZ_DIR}/..")}"
export BOLOS_SDK

APP_DIR="${APP_DIR:-}"
_CLI_FUZZ_SUBDIR=""
_CLI_TARGETS=()
_CLI_CLEAN=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --app-dir) APP_DIR="$2"; shift 2 ;;
    --app-dir=*) APP_DIR="${1#*=}"; shift ;;
    --fuzz-subdir) _CLI_FUZZ_SUBDIR="$2"; shift 2 ;;
    --fuzz-subdir=*) _CLI_FUZZ_SUBDIR="${1#*=}"; shift ;;
    --target) _CLI_TARGETS+=("$2"); shift 2 ;;
    --target=*) _CLI_TARGETS+=("${1#*=}"); shift ;;
    --clean) _CLI_CLEAN=1; shift ;;
    *) RUN_NAME="$1"; shift ;;
  esac
done
if [[ -z "${APP_DIR}" ]]; then
  echo "error: APP_DIR is not set. Use --app-dir or export APP_DIR." >&2
  exit 1
fi
APP_DIR=$(realpath -m "${APP_DIR}")
export APP_DIR
if [[ -n "${_CLI_FUZZ_SUBDIR}" ]]; then
  export APP_FUZZ_SUBDIR="${_CLI_FUZZ_SUBDIR}"
fi

source "${SCRIPT_DIR}/app-common.sh"
source "${SCRIPT_DIR}/app-config.sh"

# ── Resolve target list ──────────────────────────────────────────────────────
CAMPAIGN_TARGETS=()
if (( ${#_CLI_TARGETS[@]} > 0 )); then
  for _t in "${_CLI_TARGETS[@]}"; do
    _found=0
    for _a in "${ALL_TARGETS[@]}"; do
      if [[ "${_t}" == "${_a}" ]]; then _found=1; break; fi
    done
    if [[ "${_found}" == "0" ]]; then
      echo "error: target '${_t}' not found in manifest. Available: ${ALL_TARGETS[*]}" >&2
      exit 1
    fi
    CAMPAIGN_TARGETS+=("${_t}")
  done
else
  CAMPAIGN_TARGETS=("${ALL_TARGETS[@]}")
fi

RUN_NAME="${RUN_NAME:-campaign-$(date -u +"%Y%m%dT%H%M%SZ")}"
BUILD_DIR_FAST=$(realpath -m "${BUILD_DIR_FAST:-${BUILD_DIR:-${APP_DIR}/build/fast}}")
BUILD_DIR_COV=$(realpath -m "${BUILD_DIR_COV:-${APP_DIR}/build/cov}")
ARTIFACTS_ROOT=$(realpath -m "${ARTIFACTS_ROOT:-${APP_DIR}/.fuzz-artifacts}")
RUN_DIR="${ARTIFACTS_ROOT}/${RUN_NAME}"
WORKERS="${WORKERS:-$(pick_default_workers)}"
WARMUP_SEC="${WARMUP_SEC:-30}"
MAIN_SEC="${MAIN_SEC:-60}"
LIBFUZZER_SEED="${LIBFUZZER_SEED:-13371337}"
TIMEOUT_SEC="${TIMEOUT_SEC:-2}"
VALUE_PROFILE="${VALUE_PROFILE:-1}"
TAIL_BUDGET="${TAIL_BUDGET:-24576}"
MIN_MAX_LEN="${MIN_MAX_LEN:-65536}"
MAX_LEN="${MAX_LEN:-}"
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
EXTRA_CORPUS="${EXTRA_CORPUS:-}"

# shellcheck disable=SC1091
source "${BOLOS_SDK}/fuzzing/v2/sanitizers/load-options.sh"

SKIP_INVARIANT_SYNC="${SKIP_INVARIANT_SYNC:-0}"

# ── Per-target helper functions ──────────────────────────────────────────────

run_stage() {
  local duration_sec="${1}"
  local base_corpus_dir="${2}"
  local stage_dir="${3}"
  local fuzzer_path="${4}"
  local dict_file="${5}"
  local max_len="${6}"
  local status=0
  local worker worker_id worker_dir worker_corpus worker_crash worker_log
  local -a pids=()

  mkdir -p "${stage_dir}"

  for (( worker = 0; worker < WORKERS; worker++ )); do
    printf -v worker_id "%02d" "${worker}"
    worker_dir="${stage_dir}/worker-${worker_id}"
    worker_corpus="${worker_dir}/corpus"
    worker_crash="${worker_dir}/crash"
    worker_log="${worker_dir}/fuzz.log"

    rm -rf "${worker_dir}"
    mkdir -p "${worker_corpus}" "${worker_crash}"

    (
      local -a _fuzz_args=(
        -seed="$((LIBFUZZER_SEED + worker))"
        -max_total_time="${duration_sec}"
        -max_len="${max_len}"
        -timeout="${TIMEOUT_SEC}"
        -len_control=0
        -use_value_profile="${VALUE_PROFILE}"
        -artifact_prefix="${worker_crash}/"
      )
      [[ -s "${dict_file}" ]] && _fuzz_args+=(-dict="${dict_file}")

      "${fuzzer_path}" "${_fuzz_args[@]}" \
        "${worker_corpus}" "${base_corpus_dir}" \
        > "${worker_log}" 2>&1
    ) &
    pids+=("$!")
  done

  for pid in "${pids[@]}"; do
    if ! wait "${pid}"; then
      status=1
    fi
  done

  return "${status}"
}

merge_corpus_dirs() {
  local output_dir="${1}"
  local fuzzer_path="${2}"
  local dict_file="${3}"
  local max_len="${4}"
  shift 4

  rm -rf "${output_dir}"
  mkdir -p "${output_dir}"

  local -a _merge_args=(-merge=1 -max_len="${max_len}" -timeout="${TIMEOUT_SEC}"
                        -artifact_prefix="${output_dir}/")
  [[ -s "${dict_file}" ]] && _merge_args+=(-dict="${dict_file}")

  "${fuzzer_path}" "${_merge_args[@]}" "${output_dir}" "$@"
}

copy_bootstrap_corpus() {
  local source_dir="${1:?missing source corpus dir}"
  local dest_dir="${2:?missing dest dir}"
  local label="${3:-corpus}"
  local compat_key="${4:-}"

  if [[ ! -d "${source_dir}" ]]; then
    echo "error: ${label} directory does not exist at ${source_dir}" >&2
    return 1
  fi

  if [[ -n "${compat_key}" && -f "${source_dir}/.compat-key" ]]; then
    local source_key
    source_key=$(tr -d '[:space:]' < "${source_dir}/.compat-key")
    if [[ -n "${source_key}" && "${source_key}" != "${compat_key}" ]]; then
      echo "error: ${label} at ${source_dir} is incompatible with the current build" >&2
      echo "  source compat_key: ${source_key}" >&2
      echo "  current compat_key: ${compat_key}" >&2
      return 1
    fi
  fi

  cp -a "${source_dir}/." "${dest_dir}/"
}

# ── Run directory setup ──────────────────────────────────────────────────────

if [[ -d "${RUN_DIR}" ]]; then
  if [[ "${OVERWRITE:-0}" == "1" ]]; then
    rm -rf "${RUN_DIR}"
  else
    echo "error: run directory already exists at ${RUN_DIR}" >&2
    exit 1
  fi
fi

mkdir -p "${ARTIFACTS_ROOT}" "${RUN_DIR}"

LLVM_PROFDATA_BIN="$(pick_llvm_profdata)"
LLVM_COV_BIN="$(pick_llvm_cov)"

if [[ "${BUILD_DIR_FAST}" == "${BUILD_DIR_COV}" ]]; then
  echo "error: BUILD_DIR_FAST and BUILD_DIR_COV must be different directories" >&2
  exit 1
fi

# ── Clean build dirs if requested ─────────────────────────────────────────────

if [[ "${_CLI_CLEAN}" == "1" ]]; then
  echo "=== Cleaning build directories ==="
  rm -rf "${BUILD_DIR_FAST}" "${BUILD_DIR_COV}"
fi

# ── Build all targets ────────────────────────────────────────────────────────

echo "=== Building fuzzers (fast) ==="
configure_fuzz_build "${APP_DIR}" "${BUILD_DIR_FAST}" "${BUILD_TYPE}" 0

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "  Building ${_target}..."
  build_fuzzer_target "${BUILD_DIR_FAST}" "${_target}"
done

# ── Invariant sync ───────────────────────────────────────────────────────────

if [[ "${SKIP_INVARIANT_SYNC}" != "1" ]]; then
  _any_invariant_changed=0
  for _target in "${CAMPAIGN_TARGETS[@]}"; do
    _app_invariant="$(resolve_invariant_path "${_target}")"
    if [[ -f "${_app_invariant}" ]]; then
      INVARIANT_CHANGED=0
      echo "Syncing invariant for ${_target}..."
      sync_invariant "${BUILD_DIR_FAST}" "${_target}" "${_app_invariant}"
      if [[ "${INVARIANT_CHANGED}" == "1" ]]; then
        _any_invariant_changed=1
      fi
    fi
  done

  if [[ "${_any_invariant_changed}" == "1" ]]; then
    echo "Rebuilding with reduced invariants..."
    for _target in "${CAMPAIGN_TARGETS[@]}"; do
      build_fuzzer_target "${BUILD_DIR_FAST}" "${_target}"
    done
  else
    echo "  (all invariants unchanged — skipping rebuild)"
  fi
fi

# ── Coverage build ───────────────────────────────────────────────────────────

echo "=== Building fuzzers (coverage) ==="
configure_fuzz_build "${APP_DIR}" "${BUILD_DIR_COV}" "${BUILD_TYPE}" 1

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "  Building ${_target} (cov)..."
  build_fuzzer_target "${BUILD_DIR_COV}" "${_target}"
done

# ── Update scenario layout ───────────────────────────────────────────────────

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  update_scenario_layout "${BUILD_DIR_FAST}" "${_target}" "${SCENARIO_LAYOUT_HEADER}"
done

# ── Per-target fuzz campaign ─────────────────────────────────────────────────

run_single_target() {
  local target_name="${1}"
  local target_dir="${RUN_DIR}/targets/${target_name}"
  local fuzzer_path="${BUILD_DIR_FAST}/${target_name}"
  local fuzzer_cov_path="${BUILD_DIR_COV}/${target_name}"
  local bootstrap_dir="${target_dir}/bootstrap-base"
  local warmup_dir="${target_dir}/warmup"
  local warmup_merged_dir="${target_dir}/warmup-merged"
  local main_dir="${target_dir}/main"
  local final_corpus_dir="${target_dir}/corpus"
  local replay_profraw_dir="${target_dir}/replay-profraw"
  local dict_file="${target_dir}/${target_name}.dict"
  local meta_file="${target_dir}/meta.env"
  local replay_log="${target_dir}/replay.log"

  mkdir -p "${target_dir}" "${bootstrap_dir}"

  if [[ "${IS_MULTI_TARGET}" == "1" ]]; then
    load_target_config "${target_name}"
  fi

  local key_files=()
  for _rel in "${KEY_FILES_REL[@]}"; do
    key_files+=("${APP_DIR}/${_rel}")
  done

  if [[ ! -x "${fuzzer_path}" ]]; then
    echo "error: missing fuzzer binary at ${fuzzer_path}" >&2
    return 1
  fi
  if [[ ! -x "${fuzzer_cov_path}" ]]; then
    echo "error: missing coverage binary at ${fuzzer_cov_path}" >&2
    return 1
  fi

  local prefix_size prefix_size_cov
  prefix_size="$(prefix_size_from_generated_fuzzer "${BUILD_DIR_FAST}" "${target_name}")"
  prefix_size_cov="$(prefix_size_from_generated_fuzzer "${BUILD_DIR_COV}" "${target_name}")"
  if [[ "${prefix_size}" != "${prefix_size_cov}" ]]; then
    echo "error: fast/coverage prefix sizes differ for ${target_name} (${prefix_size} vs ${prefix_size_cov})" >&2
    return 1
  fi

  local target_max_len="${MAX_LEN}"
  if [[ -z "${target_max_len}" ]]; then
    target_max_len="$(default_max_len_for_prefix "${prefix_size}")"
  fi
  if (( target_max_len <= prefix_size )); then
    echo "error: MAX_LEN (${target_max_len}) must be greater than prefix size (${prefix_size}) for ${target_name}" >&2
    return 1
  fi

  local compat_key=""
  local _invariant_path
  _invariant_path="$(resolve_invariant_path "${target_name}")"
  if [[ -f "${_APP_MANIFEST}" ]]; then
    compat_key=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --compat-key "${_APP_MANIFEST}" \
      --prefix-size "${prefix_size}" \
      --invariant "${_invariant_path}" \
      ${IS_MULTI_TARGET:+--fuzzer "${target_name}"} 2>/dev/null || echo "")
  fi

  write_app_dictionary "${dict_file}" "${target_name}"

  echo "  Preparing bootstrap corpus for ${target_name}..."
  generate_app_seed_corpus "${bootstrap_dir}" "${target_name}"

  if [[ -n "${BASE_CORPUS_DIR:-}" && -d "${BASE_CORPUS_DIR}" ]]; then
    copy_bootstrap_corpus "${BASE_CORPUS_DIR}" "${bootstrap_dir}" "base corpus" "${compat_key}"
  fi

  if [[ -n "${EXTRA_CORPUS}" ]]; then
    local old_ifs="${IFS}"
    IFS=':'
    for corpus_dir in ${EXTRA_CORPUS}; do
      [[ -n "${corpus_dir}" ]] || continue
      corpus_dir=$(realpath -m "${corpus_dir}")
      if [[ ! -d "${corpus_dir}" ]]; then
        echo "error: EXTRA_CORPUS entry does not exist at ${corpus_dir}" >&2
        return 1
      fi
      copy_bootstrap_corpus "${corpus_dir}" "${bootstrap_dir}" "EXTRA_CORPUS entry" "${compat_key}"
    done
    IFS="${old_ifs}"
  fi

  local min_input_size=$((prefix_size + 4))
  local removed_count=0
  shopt -s nullglob
  for f in "${bootstrap_dir}"/*; do
    local fsize
    fsize=$(wc -c < "${f}")
    if (( fsize < min_input_size )); then
      rm -f "${f}"
      removed_count=$((removed_count + 1))
    fi
  done
  shopt -u nullglob
  if (( removed_count > 0 )); then
    echo "  Filtered ${removed_count} corpus files smaller than ${min_input_size} bytes"
  fi

  shopt -s nullglob
  local bootstrap_inputs=("${bootstrap_dir}"/*)
  shopt -u nullglob
  if [[ ${#bootstrap_inputs[@]} -eq 0 ]]; then
    echo "error: bootstrap corpus is empty for ${target_name} at ${bootstrap_dir}" >&2
    echo "hint: generate_app_seed_corpus() must produce semantic seeds for the current build." >&2
    return 1
  fi

  {
    echo "run_name=${RUN_NAME}"
    echo "fuzzer=${target_name}"
    echo "prefix_size=${prefix_size}"
    echo "max_len=${target_max_len}"
    echo "build_dir_fast=${BUILD_DIR_FAST}"
    echo "build_dir_cov=${BUILD_DIR_COV}"
    echo "build_type=${BUILD_TYPE}"
    echo "fast_llvm_coverage=OFF"
    echo "cov_llvm_coverage=ON"
    echo "workers=${WORKERS}"
    echo "warmup_sec=${WARMUP_SEC}"
    echo "main_sec=${MAIN_SEC}"
    echo "timeout_sec=${TIMEOUT_SEC}"
    echo "value_profile=${VALUE_PROFILE}"
    echo "libfuzzer_seed=${LIBFUZZER_SEED}"
    echo "extra_corpus=${EXTRA_CORPUS}"
    echo "compat_key=${compat_key}"
    echo "timestamp_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  } > "${meta_file}"

  local warmup_status=0
  local main_status=0

  if (( WARMUP_SEC > 0 )); then
    echo "  [${target_name}] Warmup: ${WORKERS} workers for ${WARMUP_SEC}s"
    if ! run_stage "${WARMUP_SEC}" "${bootstrap_dir}" "${warmup_dir}" \
        "${fuzzer_path}" "${dict_file}" "${target_max_len}"; then
      warmup_status=1
    fi
    shopt -s nullglob
    local warmup_corpora=("${warmup_dir}"/worker-*/corpus)
    shopt -u nullglob
    merge_corpus_dirs "${warmup_merged_dir}" "${fuzzer_path}" "${dict_file}" "${target_max_len}" \
      "${bootstrap_dir}" "${warmup_corpora[@]}"
  else
    mkdir -p "${warmup_merged_dir}"
    cp -a "${bootstrap_dir}/." "${warmup_merged_dir}/"
  fi

  if (( MAIN_SEC > 0 )); then
    echo "  [${target_name}] Main: ${WORKERS} workers for ${MAIN_SEC}s"
    if ! run_stage "${MAIN_SEC}" "${warmup_merged_dir}" "${main_dir}" \
        "${fuzzer_path}" "${dict_file}" "${target_max_len}"; then
      main_status=1
    fi
    shopt -s nullglob
    local main_corpora=("${main_dir}"/worker-*/corpus)
    shopt -u nullglob
    merge_corpus_dirs "${final_corpus_dir}" "${fuzzer_path}" "${dict_file}" "${target_max_len}" \
      "${warmup_merged_dir}" "${main_corpora[@]}"
  else
    mkdir -p "${final_corpus_dir}"
    cp -a "${warmup_merged_dir}/." "${final_corpus_dir}/"
  fi

  local crash_dir="${target_dir}/crashes"
  mkdir -p "${crash_dir}"
  local crash_count=0
  shopt -s nullglob
  for crash_file in "${warmup_dir}"/worker-*/crash/* "${main_dir}"/worker-*/crash/*; do
    [[ -f "${crash_file}" ]] || continue
    cp "${crash_file}" "${crash_dir}/$(basename "${crash_file}")"
    crash_count=$((crash_count + 1))
  done
  shopt -u nullglob
  echo "  [${target_name}] Collected ${crash_count} crash file(s)"

  mkdir -p "${replay_profraw_dir}"
  : > "${replay_log}"

  shopt -s nullglob
  local corpus_inputs=("${final_corpus_dir}"/*)
  shopt -u nullglob

  if [[ ${#corpus_inputs[@]} -eq 0 ]]; then
    echo "error: final merged corpus is empty for ${target_name}" >&2
    return 1
  fi

  echo "  [${target_name}] Replaying for coverage..."
  mapfile -t sorted_inputs < <(printf '%s\n' "${corpus_inputs[@]}" | sort)
  local replay_total=0
  local replay_fail=0
  for input_path in "${sorted_inputs[@]}"; do
    local input_name
    input_name=$(basename "${input_path}")
    export LLVM_PROFILE_FILE="${replay_profraw_dir}/${input_name}.profraw"
    replay_total=$((replay_total + 1))
    if ! "${fuzzer_cov_path}" "${input_path}" >> "${replay_log}" 2>&1; then
      replay_fail=$((replay_fail + 1))
    fi
  done
  echo "  [${target_name}] Replay: ${replay_total} inputs, ${replay_fail} failures"

  if [[ -n "${compat_key}" ]]; then
    echo "${compat_key}" > "${final_corpus_dir}/.compat-key"
  fi

  if (( warmup_status != 0 || main_status != 0 )); then
    return 1
  fi
}

# ── Execute campaign for all targets ─────────────────────────────────────────

overall_status=0

echo "=== Running campaign: ${#CAMPAIGN_TARGETS[@]} target(s) ==="

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "--- Target: ${_target} ---"
  if ! run_single_target "${_target}"; then
    echo "warning: target ${_target} had failures" >&2
    overall_status=1
  fi
done

# ── Combined coverage report ─────────────────────────────────────────────────

echo "=== Generating coverage report ==="

# Collect all profraw files from all targets
_all_profraws=()
shopt -s nullglob
for _target in "${CAMPAIGN_TARGETS[@]}"; do
  _all_profraws+=("${RUN_DIR}/targets/${_target}/replay-profraw"/*.profraw)
done
shopt -u nullglob

if [[ ${#_all_profraws[@]} -eq 0 ]]; then
  echo "error: no profraw files found from any target" >&2
  exit 1
fi

"${LLVM_PROFDATA_BIN}" merge -sparse "${_all_profraws[@]}" -o "${RUN_DIR}/coverage.profdata"

COV_EXCLUDE_FLAGS=()
for regex in "${COVERAGE_EXCLUDE_REGEXES[@]}"; do
  COV_EXCLUDE_FLAGS+=(--ignore-filename-regex="${regex}")
done

# Build the --object flags for multi-binary coverage
_first_cov_bin=""
_object_flags=()
for _target in "${CAMPAIGN_TARGETS[@]}"; do
  _cov_path="${BUILD_DIR_COV}/${_target}"
  if [[ -z "${_first_cov_bin}" ]]; then
    _first_cov_bin="${_cov_path}"
  else
    _object_flags+=(--object "${_cov_path}")
  fi
done

"${LLVM_COV_BIN}" show "${_first_cov_bin}" "${_object_flags[@]}" \
  -instr-profile="${RUN_DIR}/coverage.profdata" \
  -format=html \
  "${COV_EXCLUDE_FLAGS[@]}" \
  -output-dir="${RUN_DIR}/report"

"${LLVM_COV_BIN}" report "${_first_cov_bin}" "${_object_flags[@]}" \
  -instr-profile="${RUN_DIR}/coverage.profdata" \
  "${COV_EXCLUDE_FLAGS[@]}" \
  > "${RUN_DIR}/global-coverage.txt"

# Key-files report: collect key_files from all targets
_all_key_files=()
for _target in "${CAMPAIGN_TARGETS[@]}"; do
  if [[ "${IS_MULTI_TARGET}" == "1" ]]; then
    load_target_config "${_target}"
  fi
  for _rel in "${KEY_FILES_REL[@]}"; do
    _all_key_files+=("${APP_DIR}/${_rel}")
  done
done

if (( ${#_all_key_files[@]} > 0 )); then
  "${LLVM_COV_BIN}" report "${_first_cov_bin}" "${_object_flags[@]}" \
    -instr-profile="${RUN_DIR}/coverage.profdata" \
    "${_all_key_files[@]}" \
    > "${RUN_DIR}/key-files-coverage.txt"
fi

echo ""
echo "Campaign artifacts written to ${RUN_DIR}"
echo "Coverage report: ${RUN_DIR}/report/index.html"
echo "Targets fuzzed: ${CAMPAIGN_TARGETS[*]}"

if (( overall_status != 0 )); then
  exit 1
fi
