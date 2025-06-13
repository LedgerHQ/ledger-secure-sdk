#!/bin/bash -eu

# build fuzzers
BOLOS_SDK=$(pwd)
ls
pushd fuzzing
rm -rf build/*
cmake -S . -B build -DCMAKE_C_COMPILER=clang -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS:bool=On
cat build/compile_commands.json
cmake --build build
mv ./build/fuzz_* "${OUT}"
popd
