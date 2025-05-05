#!/bin/bash -eu

# build fuzzers

pushd fuzzing
rm -rf build/*
cmake -B build -S .
make -C build
mv ./build/fuzz_* "${OUT}"
popd
