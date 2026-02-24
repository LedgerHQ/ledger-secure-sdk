#!/usr/bin/env bash

set -e

TARGET_CPU=""
OUTPUT_DIR=""

while getopts "t:o:" opt
do
    case "$opt" in
        t)
            TARGET_CPU="$OPTARG"
            ;;
        o)
            OUTPUT_DIR="$OPTARG"
            ;;
        ?)
            exit 1
            ;;
    esac
done
shift "$((OPTIND - 1))"

if [ -z "$TARGET_CPU" ] || [ -z "$OUTPUT_DIR" ]
then
    echo "Usage: $0 -t TARGET_CPU -o OUTPUT_FILE" >&2
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(realpath "$OUTPUT_DIR")

# enable source repository
sed -i 's/^\(Types: deb\)$/\1 deb-src/g' /etc/apt/sources.list.d/debian.sources

apt update

apt install -y --no-install-recommends \
    dpkg-dev

LLVM_VERSION="21.1.8~++20251221033036+2078da43e25a"
LLVM_MAJOR_VERSION=$(echo "$LLVM_VERSION" | cut -d. -f1)

cd /tmp

LLVM_DIR="llvm-toolchain-$LLVM_MAJOR_VERSION-$LLVM_VERSION"
if [ ! -d "$LLVM_DIR" ]
then
    # install Debian source package
    apt source "llvm-toolchain-$LLVM_MAJOR_VERSION"
fi

cd "$LLVM_DIR"
rm -rf build
mkdir build
cd build

TARGET=arm-none-eabi
SYSROOT=/usr/lib/arm-none-eabi

cmake ../compiler-rt/lib/builtins \
     -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
     -DCOMPILER_RT_OS_DIR="baremetal" \
     -DCOMPILER_RT_BUILD_BUILTINS=ON \
     -DCMAKE_C_COMPILER="$(which clang)" \
     -DCMAKE_C_COMPILER_TARGET="${TARGET}" \
     -DCMAKE_ASM_COMPILER_TARGET="${TARGET}" \
     -DCMAKE_AR="/usr/bin/llvm-ar" \
     -DCMAKE_NM="/usr/bin/llvm-nm" \
     -DCMAKE_RANLIB="/usr/bin/llvm-ranlib" \
     -DCOMPILER_RT_BAREMETAL_BUILD=ON \
     -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
     -DLLVM_CMAKE_DIR="/usr/lib/llvm-21/lib/cmake/llvm" \
     -DCOMPILER_RT_HAS_FPIC_FLAG=OFF \
     -DCMAKE_C_FLAGS="-mcpu=${TARGET_CPU} -mlittle-endian -mthumb -Oz -g0 -fropi -frwpi -isystem ${SYSROOT}/include" \
     -DCMAKE_ASM_FLAGS="-mcpu=${TARGET_CPU} -mlittle-endian -mthumb" \
     -DCMAKE_SYSROOT="$SYSROOT"

make -j
mkdir -p "$OUTPUT_DIR"
cp lib/baremetal/libclang_rt.builtins-arm.a "$OUTPUT_DIR/libclang_rt.builtins.a"
