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

# when using nightly build of llvm, output of clang --version looks like this:
# Debian clang version 21.1.8 (++20251221033036+2078da43e25a-1~exp1~20251221153213.50)
# when using version from apt repositories, output of clang --version looks like this:
# Debian clang version 14.0.6
LLVM_VERSION=$(clang --version | head -n 1)
if [[ "$LLVM_VERSION" =~ Debian\ clang\ version\ ([0-9]+)\. ]]
then
    LLVM_MAJOR_VERSION="${BASH_REMATCH[1]}"
else
    echo "Unable to determine LLVM major version from clang --version output: $LLVM_VERSION"
    exit 1
fi

cd /tmp

apt source "llvm-toolchain-$LLVM_MAJOR_VERSION"

LLVM_DIR=$(find /tmp/ -type d -name "llvm-toolchain-$LLVM_MAJOR_VERSION*")

if [ -z "$LLVM_DIR" ]
then
    echo "Unable to find any llvm-toolchain-$LLVM_MAJOR_VERSION source directory in /tmp"
    exit 1
fi

cd "$LLVM_DIR"
rm -rf build
mkdir build
cd build

TARGET=arm-none-eabi
SYSROOT=/usr/lib/arm-none-eabi

cmake ../compiler-rt \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCOMPILER_RT_OS_DIR="baremetal" \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_BUILD_CRT=OFF \
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
    -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DCOMPILER_RT_BUILD_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_ORC=OFF \
    -DCOMPILER_RT_BUILD_CTX_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_SCUDO_STANDALONE=OFF \
    -DCOMPILER_RT_BUILD_SCUDO_STANDALONE_WITH_LLVM_LIBC=OFF \
    -DCMAKE_C_COMPILER="$(which clang)" \
    -DCMAKE_C_COMPILER_TARGET="${TARGET}" \
    -DCMAKE_ASM_COMPILER_TARGET="${TARGET}" \
    -DCMAKE_AR="$(which llvm-ar)" \
    -DCMAKE_NM="$(which llvm-nm)" \
    -DCMAKE_RANLIB="$(which llvm-ranlib)" \
    -DCOMPILER_RT_BAREMETAL_BUILD=ON \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DLLVM_CONFIG_PATH="$(which llvm-config)" \
    -DCOMPILER_RT_HAS_FPIC_FLAG=OFF \
    -DCMAKE_C_FLAGS="-mcpu=${TARGET_CPU} -mlittle-endian -mthumb -Oz -g0 -fropi -frwpi" \
    -DCMAKE_ASM_FLAGS="-mcpu=${TARGET_CPU} -mlittle-endian -mthumb" \
    -DCMAKE_SYSROOT="$SYSROOT"
make -j
mkdir -p "$OUTPUT_DIR"
cp lib/baremetal/libclang_rt.builtins-arm.a "$OUTPUT_DIR/libclang_rt.builtins.a"
