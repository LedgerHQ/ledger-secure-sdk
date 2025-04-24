#!/bin/bash -eu

# build fuzzers

pushd fuzzing
rm -rf build/*
cmake -B build -S . -DCMAKE_C_COMPILER=/usr/bin/clang -DSANITIZER=address
make -C build
mv ./build/fuzz_alloc "${OUT}"
popd
