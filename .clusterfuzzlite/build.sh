#!/bin/bash -e

# SDK self-fuzz targets under fuzzing/sdk-fuzz/ are Absolution-driven: absolution_add_fuzzer() regenerates each harness from the model, using the same app contract as real apps.

SDK_DIR="${SRC:-/src}/ledger-secure-sdk"

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
