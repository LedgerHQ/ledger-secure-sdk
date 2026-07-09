#!/usr/bin/env bash
# Shared ClusterFuzzLite build for every SDK fuzz tree (apps and SDK self-fuzz).
#
# A caller exports BOLOS_SDK and APP_DIR (plus APP_FUZZ_SUBDIR for non-standard
# layouts) and then execs this script. ClusterFuzzLite provides OUT and,
# per matrix entry, SANITIZER.
#
# invariants/<target>.zon is reset to `.{}` on every build: each CFL sanitizer
# (address/undefined/memory) shifts the global layout, so a committed model
# would mismatch. Absolution rediscovers it, then sync_invariant materialises a
# matching model before the final link.
set -euo pipefail

: "${BOLOS_SDK:?BOLOS_SDK must be set}"
: "${APP_DIR:?APP_DIR must be set}"
: "${OUT:?OUT must be set (ClusterFuzzLite output directory)}"

export APP_FUZZ_SUBDIR="${APP_FUZZ_SUBDIR:-fuzzing}"
export APP_TARGET="${APP_TARGET:-flex}"
export APP_SANITIZER="${APP_SANITIZER:-${SANITIZER:-address}}"

SCRIPT_DIR="${BOLOS_SDK}/fuzzing/scripts"
# shellcheck source=/dev/null
source "${SCRIPT_DIR}/app-common.sh"
# shellcheck source=/dev/null
source "${SCRIPT_DIR}/app-config.sh"

BUILD_DIR="${APP_FUZZ_DIR}/build"
export BUILD_DIR_FAST="${BUILD_DIR}"

ship_target() {
  local target="${1}"
  local inv seed_dir prefix_size compat_key min_size fuzzer_flag=""
  [[ "${IS_MULTI_TARGET}" == "1" ]] && fuzzer_flag="--fuzzer ${target}"

  update_scenario_layout "${BUILD_DIR}" "${target}" "${SCENARIO_LAYOUT_HEADER}"

  inv="$(resolve_invariant_path "${target}")"
  prefix_size="$(prefix_size_from_generated_fuzzer "${BUILD_DIR}" "${target}")"
  # shellcheck disable=SC2086
  compat_key="$(python3 "${SCRIPT_DIR}/fuzz_manifest.py" --compat-key "${_APP_MANIFEST}" \
      --prefix-size "${prefix_size}" --invariant "${inv}" ${fuzzer_flag})"

  seed_dir="${BUILD_DIR}/cfl-seed/${target}"
  rm -rf "${seed_dir}"
  mkdir -p "${seed_dir}"
  if ! generate_app_seed_corpus "${seed_dir}" "${target}"; then
    echo "  warning: seed generation failed for ${target}; shipping without seeds" >&2
  fi

  merge_base_corpus "${seed_dir}" "${compat_key}"

  # Inputs smaller than the prefix plus a byte of tail cannot drive the harness.
  min_size=$((prefix_size + 4))
  find "${seed_dir}" -type f -size "-${min_size}c" -delete 2>/dev/null || true

  cp "${BUILD_DIR}/${target}" "${OUT}/"
  python3 "${SCRIPT_DIR}/corpus.py" pack "${seed_dir}" "${OUT}/${target}_seed_corpus.zip"
}

merge_base_corpus() {
  local dest="${1}" compat_key="${2}"
  # Incompatible base corpus is skipped (not fatal) for CFL builds.
  if ! stage_base_corpus "${dest}" "${compat_key}"; then
    echo "  skipping incompatible base-corpus"
  fi
}

for target in "${ALL_TARGETS[@]}"; do
  printf '.{}\n' > "$(resolve_invariant_path "${target}")"
done

rm -rf "${BUILD_DIR}"
configure_fuzz_build "${APP_DIR}" "${BUILD_DIR}" RelWithDebInfo 0

for target in "${ALL_TARGETS[@]}"; do
  build_fuzzer_target "${BUILD_DIR}" "${target}"
done

any_changed=0
for target in "${ALL_TARGETS[@]}"; do
  INVARIANT_CHANGED=0
  sync_invariant "${BUILD_DIR}" "${target}" "$(resolve_invariant_path "${target}")"
  [[ "${INVARIANT_CHANGED}" == "1" ]] && any_changed=1
done
if [[ "${any_changed}" == "1" ]]; then
  for target in "${ALL_TARGETS[@]}"; do
    build_fuzzer_target "${BUILD_DIR}" "${target}"
  done
fi

for target in "${ALL_TARGETS[@]}"; do
  echo "--- Shipping ${target} ---"
  ship_target "${target}"
done

echo "cfl-build: shipped ${#ALL_TARGETS[@]} target(s) to ${OUT}"
