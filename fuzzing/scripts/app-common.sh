#!/usr/bin/env bash

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

pick_clang() { pick_cmd CC "${CC:-}" clang-19 clang; }
pick_llvm_profdata() { pick_cmd LLVM_PROFDATA "${LLVM_PROFDATA:-}" llvm-profdata-19 llvm-profdata; }
pick_llvm_cov() { pick_cmd LLVM_COV "${LLVM_COV:-}" llvm-cov-19 llvm-cov; }

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
  # Default campaign profile: at most two workers (friendly for laptops/CI).
  # Override with WORKERS=N, or raise the cap with FUZZ_DEFAULT_WORKERS=N.
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
  local build_type="${3:-${APP_FUZZ_BUILD_TYPE:-RelWithDebInfo}}"
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

default_max_len_for_prefix() {
  local prefix_size="${1:?missing prefix size}"
  local tail_budget="${TAIL_BUDGET:-24576}"
  local min_max_len="${MIN_MAX_LEN:-65536}"
  local max_len=$((prefix_size + tail_budget))

  if (( max_len < min_max_len )); then
    max_len="${min_max_len}"
  fi

  echo "${max_len}"
}

update_scenario_layout() {
  local build_dir="${1:?missing build dir}"
  local fuzzer_name="${2:?missing fuzzer name}"
  local layout_header="${3:?missing scenario_layout.h path}"
  local fuzzer_c="${build_dir}/_absolution/${fuzzer_name}/fuzzer.c"
  shift 3

  if [[ ! -f "${fuzzer_c}" ]]; then
    echo "warning: generated fuzzer.c not found at ${fuzzer_c}, skipping layout update" >&2
    return 0
  fi

  if [[ ! -f "${layout_header}" ]]; then
    echo "warning: scenario_layout.h not found at ${layout_header}, skipping layout update" >&2
    return 0
  fi

  python3 "${SCRIPT_DIR}/update-scenario-layout.py" "${fuzzer_c}" "${layout_header}" "$@" \
    "${LAYOUT_UPDATE_EXTRA_ARGS[@]}"
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
      # Signal to caller that no rebuild is needed
      INVARIANT_CHANGED=0
      return 0
    fi
  fi

  python3 "${SCRIPT_DIR}/sync-invariant.py" \
    "${generated_zon}" "${app_invariant}" "${extra_args[@]}"

  local domain_overrides
  domain_overrides="$(dirname "${app_invariant}")/domain-overrides.txt"
  if [[ -f "${domain_overrides}" ]]; then
    python3 "${SCRIPT_DIR}/tune-invariant-domains.py" \
      "${app_invariant}" "${domain_overrides}"
  fi

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
