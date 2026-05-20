#!/bin/bash -e

# SDK ClusterFuzzLite build. Builds the SDK self-fuzz targets defined under
# fuzzing/extra/*.cmake (fuzz_bip32, fuzz_base58, fuzz_alloc,
# fuzz_alloc_utils, fuzz_lists, fuzz_qrcodegen, fuzz_apdu_parser) via the
# unified fuzzing/ tree.

SDK_DIR="${SRC:-/src}/ledger-secure-sdk"

pushd fuzzing

cmake -S . -B build \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_BUILD_TYPE=Debug \
      -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
      -DBOLOS_SDK="${SDK_DIR}" \
      -DTARGET=flex \
      -DAPP_BUILD_PATH="${SDK_DIR}"

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

cmake --build build

mv ./build/fuzz_* "${OUT}/"

popd
