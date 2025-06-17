#!/bin/bash -eu

# build fuzzers
pushd fuzzing
cmake -S . -B build -G Ninja
cmake --build build
mv ./build/fuzz_* "${OUT}"
popd
