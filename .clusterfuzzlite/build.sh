#!/bin/bash -eu

# build fuzzers

pushd fuzzing
cmake -B build
make -C build
mv ./build/fuzz_bip32 "${OUT}"
popd
