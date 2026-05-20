#!/bin/bash -e

# SDK ClusterFuzzLite build. The SDK ships two parallel fuzz trees that
# CFL must exercise on every matrix entry:
#
#   - fuzzing/  (entry: fuzzing/CMakeLists.txt → extra/*.cmake)
#       Plain libFuzzer harnesses kept for backward-compat: fuzz_alloc,
#       fuzz_alloc_utils, fuzz_apdu_parser, fuzz_base58, fuzz_bip32,
#       fuzz_lists, fuzz_qrcodegen. They link the SDK libs directly via
#       add_executable() and never see Absolution.
#
#   - fuzzing/sdk-fuzz/  (entry: fuzzing/sdk-fuzz/CMakeLists.txt)
#       Absolution-driven counterparts of the same SDK libs plus extras
#       (fuzz_nfc_ndef, fuzz_tlv_dynamic_descriptor,
#       fuzz_tlv_trusted_name). Each goes through `absolution_add_fuzzer()`
#       so the framework regenerates the harness from the model.
#
# Seven binary names overlap (fuzz_alloc, …, fuzz_qrcodegen). The classic
# tree is published with a `_classic` suffix so both flavours land in
# ${OUT} and CFL runs them side by side — backward-compat coverage in the
# old tree, model-driven coverage in the new one.

SDK_DIR="${SRC:-/src}/ledger-secure-sdk"

# ── Classic libFuzzer tree (extra/*.cmake) ───────────────────────────────────
pushd "${SDK_DIR}/fuzzing"

cmake -S . -B build-classic \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_BUILD_TYPE=Debug \
      -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
      -DBOLOS_SDK="${SDK_DIR}" \
      -DTARGET=flex \
      -DAPP_BUILD_PATH="${SDK_DIR}"

cmake --build build-classic

# Publish under <name>_classic to avoid colliding with the Absolution tree.
for fuzzer in ./build-classic/fuzz_*; do
    [[ -f "${fuzzer}" && -x "${fuzzer}" ]] || continue
    name="$(basename "${fuzzer}")"
    cp "${fuzzer}" "${OUT}/${name}_classic"
done

popd

# ── Absolution-driven tree (fuzzing/sdk-fuzz) ────────────────────────────────
pushd "${SDK_DIR}/fuzzing/sdk-fuzz"

cmake -S . -B build-absolution \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_BUILD_TYPE=Debug \
      -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
      -DBOLOS_SDK="${SDK_DIR}" \
      -DTARGET=flex \
      -DAPP_BUILD_PATH="${SDK_DIR}"

cmake --build build-absolution

for fuzzer in ./build-absolution/fuzz_*; do
    [[ -f "${fuzzer}" && -x "${fuzzer}" ]] || continue
    cp "${fuzzer}" "${OUT}/"
done

popd

# ── Per-target seed corpora ──────────────────────────────────────────────────
# Match v1 corpus packaging: harness/<name>/ subdirs become
# <name>_seed_corpus.zip. SDK self-fuzz harness sources are currently flat
# .c files (no per-target corpora), so this loop is a no-op today; kept for
# parity in case per-target seed corpora are added later under
# harness/fuzz_*/ subdirs.
for dir in "${SDK_DIR}"/fuzzing/harness/*; do
    if [ -d "${dir}" ]; then
        fuzzer_name=$(basename "${dir}")
        zip_name="${fuzzer_name}_seed_corpus.zip"
        echo "Zipping corpus from ${dir} into ${zip_name}"
        (cd "${dir}" && zip -q -r "${zip_name}" .)
        mv "${dir}/${zip_name}" "${OUT}/"
    fi
done
