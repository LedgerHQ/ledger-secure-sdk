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
# Three knobs. Everything else is fixed, so two runs differ only by these.
WORKERS="${WORKERS:-$(pick_default_workers)}"
FUZZ_TIME="${FUZZ_TIME:-90}"      # seconds of fuzzing per target
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"

# Fixed, deliberately: a campaign is a measurement, and these decide what it
# measures. Changing one changes results, so change it here where it is reviewed.
readonly LIBFUZZER_SEED=13371337  # reproducible corpus growth
# Matches ClusterFuzzLite's FUZZER_ARGS so local and CI runs behave the same.
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
RSS_LIMIT_MB="${RSS_LIMIT_MB:-2560}"
readonly VALUE_PROFILE=1          # -use_value_profile
# -max_len is the prefix plus [target].tail_budget from the app manifest; see
# manifest.dox for why it is declared per app rather than defaulted.

# shellcheck disable=SC1091
source "${BOLOS_SDK}/fuzzing/sanitizers/load-options.sh"

SKIP_INVARIANT_SYNC="${SKIP_INVARIANT_SYNC:-0}"

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
        -print_final_stats=1
        -rss_limit_mb="${RSS_LIMIT_MB}"
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
check_llvm_versions "$(pick_clang)" "${LLVM_PROFDATA_BIN}" "${LLVM_COV_BIN}"

if [[ "${BUILD_DIR_FAST}" == "${BUILD_DIR_COV}" ]]; then
  echo "error: BUILD_DIR_FAST and BUILD_DIR_COV must be different directories" >&2
  exit 1
fi

if [[ "${_CLI_CLEAN}" == "1" ]]; then
  echo "=== Cleaning build directories ==="
  rm -rf "${BUILD_DIR_FAST}" "${BUILD_DIR_COV}"
fi

# invariants/<target>.zon snapshots are machine-local (blob sizes depend on the build's memory layout); resetting to .{} forces Absolution to re-discover a model matching the current build.
BOOTSTRAP_INVARIANT="${BOOTSTRAP_INVARIANT:-auto}"
case "${BOOTSTRAP_INVARIANT}" in
  auto) _should_bootstrap="${_CLI_CLEAN}" ;;  # only --clean can change the layout
  1)    _should_bootstrap=1 ;;
  0)    _should_bootstrap=0 ;;
  *)    echo "error: BOOTSTRAP_INVARIANT must be 1, 0, or auto (got: ${BOOTSTRAP_INVARIANT})" >&2
        exit 1 ;;
esac

if [[ "${_should_bootstrap}" == "1" ]]; then
  echo "=== Bootstrapping invariants to .{} (BOOTSTRAP_INVARIANT=${BOOTSTRAP_INVARIANT}) ==="
  for _target in "${CAMPAIGN_TARGETS[@]}"; do
    _inv="$(resolve_invariant_path "${_target}")"
    if [[ -f "${_inv}" ]]; then
      echo "  Resetting ${_inv} to .{}"
      printf '.{}\n' > "${_inv}"
    fi
  done
fi

echo "=== Building fuzzers (fast) ==="
configure_fuzz_build "${APP_DIR}" "${BUILD_DIR_FAST}" "${BUILD_TYPE}" 0

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "  Building ${_target}..."
  build_fuzzer_target "${BUILD_DIR_FAST}" "${_target}"
done

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


echo "=== Building fuzzers (coverage) ==="
configure_fuzz_build "${APP_DIR}" "${BUILD_DIR_COV}" "${BUILD_TYPE}" 1

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "  Building ${_target} (cov)..."
  build_fuzzer_target "${BUILD_DIR_COV}" "${_target}"
done


run_single_target() {
  local target_name="${1}"
  local target_dir="${RUN_DIR}/targets/${target_name}"
  local fuzzer_path="${BUILD_DIR_FAST}/${target_name}"
  local fuzzer_cov_path="${BUILD_DIR_COV}/${target_name}"
  local bootstrap_dir="${target_dir}/bootstrap-base"
  local fuzz_dir="${target_dir}/fuzz"
  local final_corpus_dir="${target_dir}/corpus"
  local replay_profraw_dir="${target_dir}/replay-profraw"
  local dict_file="${target_dir}/${target_name}.dict"
  local replay_log="${target_dir}/replay.log"

  mkdir -p "${target_dir}" "${bootstrap_dir}"

  if [[ "${IS_MULTI_TARGET}" == "1" ]]; then
    load_target_config "${target_name}"
  fi

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

  # Derived from the target's own prefix size, not configurable: a hand-set
  # -max_len that disagrees with the prefix silently changes what fraction of each
  # input is state versus payload.
  local target_max_len
  target_max_len="$(default_max_len_for_prefix "${prefix_size}")"
  if (( target_max_len <= prefix_size )); then
    echo "error: computed max_len (${target_max_len}) must exceed the prefix size (${prefix_size}) for ${target_name}" >&2
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

  # errexit is off inside this function, so this failure would otherwise be silent.
  if ! stage_base_corpus "${bootstrap_dir}" "${compat_key}"; then
    echo "  [${target_name}] WARNING: base corpus not staged -- starting from generated" \
         "seeds only. Re-promote after this run:" >&2
    echo "    python3 \${BOLOS_SDK}/fuzzing/scripts/corpus.py promote \\" >&2
    echo "      ${target_dir}/corpus ${BASE_CORPUS_ZIP}" >&2
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


  # One stage. Warmup and main were the same code with a different duration: run
  # T1, merge the whole corpus, run T2, merge again. The intermediate merge was
  # never shown to help -- libFuzzer prunes its own corpus -- so it cost a full
  # merge pass per target and gave two ways to describe one budget.
  local fuzz_status=0
  if (( FUZZ_TIME > 0 )); then
    echo "  [${target_name}] Fuzzing: ${WORKERS} worker(s) for ${FUZZ_TIME}s"
    if ! run_stage "${FUZZ_TIME}" "${bootstrap_dir}" "${fuzz_dir}" \
        "${fuzzer_path}" "${dict_file}" "${target_max_len}"; then
      fuzz_status=1
    fi
    shopt -s nullglob
    local fuzz_corpora=("${fuzz_dir}"/worker-*/corpus)
    shopt -u nullglob
    merge_corpus_dirs "${final_corpus_dir}" "${fuzzer_path}" "${dict_file}" "${target_max_len}" \
      "${bootstrap_dir}" "${fuzz_corpora[@]}"
  else
    mkdir -p "${final_corpus_dir}"
    cp -a "${bootstrap_dir}/." "${final_corpus_dir}/"
  fi

  local crash_dir="${target_dir}/crashes"
  mkdir -p "${crash_dir}"
  # libFuzzer names artifacts by what happened; only crash- is a finding.
  local crash_count=0 oom_count=0 timeout_count=0 leak_count=0 other_count=0
  shopt -s nullglob
  for crash_file in "${fuzz_dir}"/worker-*/crash/*; do
    [[ -f "${crash_file}" ]] || continue
    local _base; _base="$(basename "${crash_file}")"
    cp "${crash_file}" "${crash_dir}/${_base}"
    case "${_base}" in
      crash-*)   crash_count=$((crash_count + 1)) ;;
      oom-*)     oom_count=$((oom_count + 1)) ;;
      timeout-*) timeout_count=$((timeout_count + 1)) ;;
      leak-*)    leak_count=$((leak_count + 1)) ;;
      *)         other_count=$((other_count + 1)) ;;
    esac
  done
  shopt -u nullglob
  echo "  [${target_name}] Collected ${crash_count} crash file(s)"
  if (( oom_count || timeout_count || leak_count || other_count )); then
    echo "  [${target_name}] Also, NOT crashes -- the run hit a limit:" \
         "${oom_count} oom, ${timeout_count} timeout, ${leak_count} leak," \
         "${other_count} other -- see running.dox." >&2
  fi

  # Recoverable sanitizer diagnostics never abort, so libFuzzer writes no crash
  # artifact for them and they would otherwise stay buried in the worker logs
  # while the campaign reported a clean run. Surface them with their sites.
  local san_report="${target_dir}/sanitizer-reports.txt"
  shopt -s nullglob
  local worker_logs=("${fuzz_dir}"/worker-*/fuzz.log)
  shopt -u nullglob
  if (( ${#worker_logs[@]} > 0 )); then
    # Group by site and check, dropping the per-input values so the same defect
    # does not appear once per hit. grep is line-based, so `.*` is safe here --
    # `[^\n]*` would be a bracket expression meaning "not n", truncating the text.
    grep -hoE '[^ ]+\.(c|h):[0-9]+:[0-9]+: runtime error: .*' "${worker_logs[@]}" 2>/dev/null \
      | sed -e 's/ of value .*//' -e 's/ (aka [^)]*)//g' -e 's/: [0-9-]* \* [0-9-]* cannot/: N * N cannot/' \
      | sort | uniq -c | sort -rn > "${san_report}" || true
    if [[ -s "${san_report}" ]]; then
      local san_sites
      san_sites=$(wc -l < "${san_report}")
      echo "  [${target_name}] ${san_sites} recoverable sanitizer report site(s) — no crash artifact is written for these:"
      sed 's/^/      /' "${san_report}"
      echo "      full list: ${san_report}"
    else
      rm -f "${san_report}"
    fi
  fi

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

  if (( fuzz_status != 0 )); then
    return 1
  fi
}

overall_status=0

echo "=== Running campaign: ${#CAMPAIGN_TARGETS[@]} target(s) ==="

for _target in "${CAMPAIGN_TARGETS[@]}"; do
  echo "--- Target: ${_target} ---"
  if ! run_single_target "${_target}"; then
    echo "warning: target ${_target} had failures" >&2
    overall_status=1
  fi
done

echo "=== Generating coverage report ==="

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
