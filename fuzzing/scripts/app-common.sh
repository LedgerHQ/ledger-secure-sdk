#!/usr/bin/env bash

# This file locates its own siblings rather than trusting the caller to have set
# SCRIPT_DIR: it is used in eight places here, including at source time now that
# the manifest configuration lives at the bottom, and a caller that forgot it got
# "python3: can't open file '/fuzz_manifest.py'" instead of a named cause.
SCRIPT_DIR="${SCRIPT_DIR:-$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)}"

pick_cmd() {
  local env_name="$1"
  local env_value="$2"
  shift 2

  if [[ -n "${env_value}" ]]; then
    echo "${env_value}"
    return 0
  fi

  for cmd in "$@"; do
    if command -v "${cmd}" >/dev/null 2>&1; then
      echo "${cmd}"
      return 0
    fi
  done

  echo "error: none of [$*] found; set ${env_name}" >&2
  return 1
}

# Prefer unversioned names, falling back to the image's clang-21 LLVM release; override with CC / LLVM_PROFDATA / LLVM_COV.
pick_clang() { pick_cmd CC "${CC:-}" clang clang-21; }
pick_llvm_profdata() { pick_cmd LLVM_PROFDATA "${LLVM_PROFDATA:-}" llvm-profdata llvm-profdata-21; }
pick_llvm_cov() { pick_cmd LLVM_COV "${LLVM_COV:-}" llvm-cov llvm-cov-21; }

# LLVM's raw profile format is locked to the release that produced it: reading one
# with a different llvm-profdata fails outright ("raw profile version mismatch:
# Profile uses raw profile format version = 13; expected version = 10"). That used
# to surface at the merge -- after a full build and a full campaign -- so the three
# tools are compared up front instead.
llvm_major() { "${1}" --version 2>/dev/null | sed -n 's/.*version \([0-9][0-9]*\).*/\1/p' | head -1; }

check_llvm_versions() {
  local cc="${1}" profdata="${2}" cov="${3}"
  local cc_v profdata_v cov_v
  cc_v="$(llvm_major "${cc}")"
  profdata_v="$(llvm_major "${profdata}")"
  cov_v="$(llvm_major "${cov}")"

  if [[ -z "${cc_v}" || -z "${profdata_v}" || -z "${cov_v}" ]]; then
    echo "warning: no version readable from ${cc} / ${profdata} / ${cov}; skipping the check" >&2
    return 0
  fi
  if [[ "${profdata_v}" != "${cc_v}" || "${cov_v}" != "${cc_v}" ]]; then
    echo "error: LLVM tools come from different releases -- coverage cannot work." >&2
    echo "  ${cc}: ${cc_v}    ${profdata}: ${profdata_v}    ${cov}: ${cov_v}" >&2
    echo "hint: set CC / LLVM_PROFDATA / LLVM_COV to one matching release." >&2
    return 1
  fi
}

cmake_bool() {
  local value="${1:-}"
  case "${value,,}" in
    1|on|true|yes)
      echo "ON"
      ;;
    0|off|false|no|'')
      echo "OFF"
      ;;
    *)
      echo "error: expected a boolean value, got '${value}'" >&2
      return 1
      ;;
  esac
}

pick_default_build_jobs() {
  local cpus
  cpus=$(nproc 2>/dev/null || echo 1)
  if (( cpus > 8 )); then cpus=8; fi
  echo "${cpus}"
}

pick_default_workers() {
  local cap="${FUZZ_DEFAULT_WORKERS:-2}"
  local cpus
  cpus=$(nproc 2>/dev/null || echo 1)
  if (( cpus < cap )); then
    echo "${cpus}"
  else
    echo "${cap}"
  fi
}

resolve_invariant_path() {
  local target_name="${1:?missing target name}"
  local app_dir="${APP_DIR:?APP_DIR must be set}"
  local fuzz_subdir="${APP_FUZZ_SUBDIR:-fuzzing}"
  local path="${app_dir}/${fuzz_subdir}/invariants/${target_name}.zon"
  if [[ ! -f "${path}" ]]; then
    path="${app_dir}/${fuzz_subdir}/invariants/fuzz_globals.zon"
  fi
  echo "${path}"
}

configure_fuzz_build() {
  local app_dir="${1:?missing app dir}"
  local build_dir="${2:?missing build dir}"
  local build_type="${3:-RelWithDebInfo}"
  local llvm_coverage
  local sdk_dir="${BOLOS_SDK:?BOLOS_SDK must be set}"
  local clang
  local target="${APP_TARGET:-flex}"
  local sanitizer="${APP_SANITIZER:-address}"
  local fuzz_subdir="${APP_FUZZ_SUBDIR:-fuzzing}"
  local -a generator=()

  if [[ ! -d "${app_dir}/${fuzz_subdir}" ]]; then
    echo "error: ${app_dir}/${fuzz_subdir} does not exist" >&2
    echo "hint: set APP_DIR to the app directory, or APP_FUZZ_SUBDIR for non-standard layouts." >&2
    return 1
  fi

  check_build_dir_app_match "${build_dir}" "${app_dir}/${fuzz_subdir}"

  clang="$(pick_clang)"
  llvm_coverage="$(cmake_bool "${4:-${APP_FUZZ_SOURCE_COVERAGE:-0}}")"

  if command -v ninja >/dev/null 2>&1; then
    generator=(-G Ninja)
  fi

  # The generated prefix size is compiled in, so it is part of the configure identity.
  local _config_key="${app_dir}|${fuzz_subdir}|${build_type}|${sdk_dir}|${target}|${sanitizer}|${llvm_coverage}"
  local _config_hash
  _config_hash=$(printf '%s' "${_config_key}" | sha256sum | cut -d' ' -f1)
  local _hash_file="${build_dir}/.fuzz-configure-hash"

  if [[ -f "${_hash_file}" && -f "${build_dir}/CMakeCache.txt" ]]; then
    local _cached_hash
    _cached_hash=$(cat "${_hash_file}" 2>/dev/null || echo "")
    if [[ "${_cached_hash}" == "${_config_hash}" ]]; then
      echo "  (configure cache hit — skipping cmake)"
      return 0
    fi
  fi

  (
    cd "${app_dir}"
    CC="${clang}" cmake \
      -S "${fuzz_subdir}" \
      -B "${build_dir}" \
      -D CMAKE_BUILD_TYPE="${build_type}" \
      -D CMAKE_EXPORT_COMPILE_COMMANDS=On \
      -D BOLOS_SDK="${sdk_dir}" \
      -D TARGET="${target}" \
      -D SANITIZER="${sanitizer}" \
      -D FUZZ_ENABLE_SOURCE_COVERAGE="${llvm_coverage}" \
      -D APP_BUILD_PATH="$(pwd)" \
      "${generator[@]}"
  )

  echo "${_config_hash}" > "${_hash_file}"
}

build_fuzzer_target() {
  local build_dir="${1:?missing build dir}"
  local fuzzer_name="${2:?missing fuzzer name}"
  local jobs="${BUILD_JOBS:-$(pick_default_build_jobs)}"

  cmake --build "${build_dir}" --target "${fuzzer_name}" -j "${jobs}"
}

prefix_size_from_generated_fuzzer() {
  local build_dir="${1:?missing build dir}"
  local fuzzer_name="${2:?missing fuzzer name}"
  local fuzzer_c="${build_dir}/_absolution/${fuzzer_name}/fuzzer.c"

  if [[ ! -f "${fuzzer_c}" ]]; then
    echo "error: generated fuzzer.c not found at ${fuzzer_c}" >&2
    return 1
  fi

  grep -oP '#define ABSOLUTION_GLOBALS_SIZE \K[0-9]+' "${fuzzer_c}"
}

# -max_len = prefix + the app's declared tail budget. TAIL_BUDGET comes from the
# manifest via fuzz_manifest.py --shell; the fallback matches its DEFAULT_TAIL_BUDGET
# and is what the APDU protocol allows (4 control bytes + a 255-byte Lc + slack).
default_max_len_for_prefix() {
  local prefix_size="${1:?missing prefix size}"
  echo "$(( prefix_size + ${TAIL_BUDGET:-288} ))"
}

sync_invariant() {
  local build_dir="${1:?missing build dir}"
  local fuzzer_name="${2:?missing fuzzer name}"
  local app_invariant="${3:?missing app invariant path}"
  local generated_zon="${build_dir}/_absolution/${fuzzer_name}/fuzzer.c.zon"

  if [[ ! -f "${generated_zon}" ]]; then
    echo "warning: generated .zon not found at ${generated_zon}, skipping invariant sync" >&2
    return 0
  fi

  local sdk_dir="${BOLOS_SDK:?BOLOS_SDK must be set}"

  local -a extra_args=()
  local fw_zeros="${sdk_dir}/fuzzing/invariants/sdk-zero-symbols.txt"
  if [[ -f "${fw_zeros}" ]]; then
    extra_args+=(--framework-zeros "${fw_zeros}")
  fi

  local app_zeros
  app_zeros="$(dirname "${app_invariant}")/zero-symbols.txt"
  if [[ -f "${app_zeros}" ]]; then
    extra_args+=(--app-zeros "${app_zeros}")
  fi

  local _inv_hash_file="${build_dir}/.fuzz-invariant-hash-${fuzzer_name}"
  local _inv_hash=""
  for _f in "${generated_zon}" "${app_invariant}" "${fw_zeros}" "${app_zeros}"; do
    if [[ -f "${_f}" ]]; then
      _inv_hash="${_inv_hash}$(sha256sum "${_f}" | cut -d' ' -f1)"
    fi
  done
  _inv_hash=$(printf '%s' "${_inv_hash}" | sha256sum | cut -d' ' -f1)

  if [[ -f "${_inv_hash_file}" ]]; then
    local _cached_inv_hash
    _cached_inv_hash=$(cat "${_inv_hash_file}" 2>/dev/null || echo "")
    if [[ "${_cached_inv_hash}" == "${_inv_hash}" ]]; then
      echo "  (invariant unchanged for ${fuzzer_name} — skipping sync)"
      INVARIANT_CHANGED=0
      return 0
    fi
  fi

  local domain_overrides
  domain_overrides="$(dirname "${app_invariant}")/domain-overrides.txt"
  if [[ -f "${domain_overrides}" ]]; then
    extra_args+=(--domain-overrides "${domain_overrides}")
  fi

  python3 "${SCRIPT_DIR}/invariant.py" \
    "${generated_zon}" "${app_invariant}" "${extra_args[@]}"

  mkdir -p "$(dirname "${_inv_hash_file}")"
  echo "${_inv_hash}" > "${_inv_hash_file}"
  INVARIANT_CHANGED=1
}

check_build_dir_app_match() {
  local build_dir="${1:?missing build dir}"
  local expected_source="${2:?missing expected source dir}"

  if [[ ! -f "${build_dir}/CMakeCache.txt" ]]; then
    return 0
  fi

  local cached_source
  cached_source=$(grep -oP 'CMAKE_HOME_DIRECTORY:INTERNAL=\K.*' "${build_dir}/CMakeCache.txt" 2>/dev/null || true)

  if [[ -z "${cached_source}" ]]; then
    return 0
  fi

  local expected_real
  expected_real=$(realpath -m "${expected_source}" 2>/dev/null || echo "${expected_source}")
  local cached_real
  cached_real=$(realpath -m "${cached_source}" 2>/dev/null || echo "${cached_source}")

  if [[ "${cached_real}" != "${expected_real}" ]]; then
    echo "error: build directory ${build_dir} was configured for a different app" >&2
    echo "  cached source:   ${cached_source}" >&2
    echo "  expected source:  ${expected_source}" >&2
    echo "hint: use app-scoped build directories (default) or delete ${build_dir}" >&2
    return 1
  fi
}

# Unpacks BASE_CORPUS_ZIP into dest_dir when its sidecar compat-key matches the
# current build. No-op when no base corpus is configured. Returns 1 (incompatible)
# when the keys disagree so callers can decide whether that is fatal.
stage_base_corpus() {
  local dest_dir="${1:?missing dest dir}"
  local compat_key="${2:-}"
  local zip="${BASE_CORPUS_ZIP:-}"
  local key_file="${BASE_CORPUS_KEY:-}"

  [[ -n "${zip}" && -f "${zip}" ]] || return 0

  if [[ -n "${compat_key}" ]]; then
    if [[ ! -f "${key_file}" ]]; then
      echo "error: base corpus ${zip} has no compat-key sidecar at ${key_file}" >&2
      return 1
    fi
    local source_key
    source_key=$(tr -d '[:space:]' < "${key_file}")
    if [[ "${source_key}" != "${compat_key}" ]]; then
      echo "error: base corpus ${zip} is incompatible with the current build" >&2
      echo "  source compat_key:  ${source_key}" >&2
      echo "  current compat_key: ${compat_key}" >&2
      return 1
    fi
  fi

  python3 "${SCRIPT_DIR}/corpus.py" unpack "${zip}" "${dest_dir}"
}

# ── Manifest-derived campaign configuration ───────────────────────────────────
# Everything below ran from a separate app-config.sh that both entry points
# sourced on the line after this file, and that neither could work without.
#
# fuzz_manifest.py parses TOML with tomllib, so python3 must be 3.11 or newer;
# checked here so a stale interpreter is named rather than arriving as a
# ModuleNotFoundError traceback wrapped in "failed to list targets".
if ! python3 -c 'import sys; sys.exit(0 if sys.version_info >= (3, 11) else 1)' 2>/dev/null; then
  echo "error: python3 >= 3.11 is required (tomllib); found $(python3 --version 2>&1)" >&2
  exit 1
fi

if [[ -z "${APP_DIR:-}" ]]; then
  echo "error: APP_DIR is not set." >&2
  echo "hint: export APP_DIR=/path/to/app or use --app-dir." >&2
  exit 1
fi

APP_FUZZ_SUBDIR="${APP_FUZZ_SUBDIR:-fuzzing}"
export APP_FUZZ_SUBDIR
APP_FUZZ_DIR="${APP_DIR}/${APP_FUZZ_SUBDIR}"
if [[ ! -d "${APP_FUZZ_DIR}" ]]; then
  echo "error: app fuzzing directory not found at ${APP_FUZZ_DIR}" >&2
  echo "hint: set APP_DIR to the app root directory (e.g. /path/to/app-boilerplate)." >&2
  echo "      see the Fuzzing Framework page in the SDK documentation." >&2
  exit 1
fi

_APP_MANIFEST="${APP_FUZZ_DIR}/fuzz-manifest.toml"
export _APP_MANIFEST

if [[ ! -f "${_APP_MANIFEST}" ]]; then
  echo "error: fuzz-manifest.toml not found at ${_APP_MANIFEST}" >&2
  echo "hint: create one from the app-boilerplate reference (app-boilerplate/fuzzing/fuzz-manifest.toml)" >&2
  exit 1
fi

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

FUZZER="${FUZZER:-fuzz_globals}"
KEY_FILES_REL=()
COVERAGE_EXCLUDE_REGEXES=(
  '.*ledger-secure-sdk.*'
  '.*fuzz_dispatcher\.c'
  '.*fuzzer\.c'
  '.*fuzzing/mock/.*'
  '.*src/main\.c'
  '.*src/ui/menu_nbgl\.c'
)

if [[ "${IS_MULTI_TARGET}" == "0" ]]; then
  _manifest_vars=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --shell "${_APP_MANIFEST}" 2>&1) || {
    echo "error: failed to read manifest: ${_manifest_vars}" >&2
    exit 1
  }
  eval "${_manifest_vars}"
  unset _manifest_vars
else
  # Coverage excludes are shared, so read them from the first target.
  _manifest_vars=$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --shell "${_APP_MANIFEST}" \
    --fuzzer "${ALL_TARGETS[0]}" 2>&1) || {
    echo "error: failed to read manifest: ${_manifest_vars}" >&2
    exit 1
  }
  eval "$(echo "${_manifest_vars}" | grep '^COVERAGE_EXCLUDE_REGEXES=')"
  unset _manifest_vars
fi

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

# Promoted base corpus: a zip of inputs plus a tracked compat-key sidecar.
# Set BASE_CORPUS_ZIP= (empty) to skip it for a run.
BASE_CORPUS_ZIP="${BASE_CORPUS_ZIP-${APP_FUZZ_DIR}/base-corpus.zip}"
BASE_CORPUS_KEY="${BASE_CORPUS_KEY:-${APP_FUZZ_DIR}/base-corpus.compat-key}"
if [[ -n "${BASE_CORPUS_ZIP}" && ! -f "${BASE_CORPUS_ZIP}" ]]; then
  BASE_CORPUS_ZIP=""
fi
export BASE_CORPUS_ZIP BASE_CORPUS_KEY

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
  local build_fast="${BUILD_DIR_FAST:-${APP_DIR}/build/fast}"
  APP_DIR="${APP_DIR}" \
  BUILD_DIR_FAST="${build_fast}" \
  BUILD_DIR="${build_fast}" \
  BUILD_DIR_COV="${BUILD_DIR_COV:-${APP_DIR}/build/cov}" \
  FUZZER="${FUZZER}" \
  python3 "${SCRIPT_DIR}/seeds.py" "${_APP_MANIFEST}" "${output_dir}" ${fuzzer_flag}
}
