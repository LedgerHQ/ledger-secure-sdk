#!/bin/bash -eu

# build fuzzers
pushd fuzzing
rm -rf build/*
echo "CC: $CC"
echo "CFALGS: $CFLAGS"
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS:bool=On
cat build/compile_commands.json
cmake --build build
mv ./build/fuzz_* "${OUT}"
popd
