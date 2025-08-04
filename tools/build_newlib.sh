#!/usr/bin/env bash

set -exu

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

if [ -z "${TARGET_CPU}" ] || [ -z "${OUTPUT_DIR}" ]
then
    echo "Usage: $0 -t TARGET_CPU -o OUTPUT_FILE" >&2
    exit 1
fi


mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR=$(realpath "$OUTPUT_DIR")


# ENV SETUP
WORKDIR="$(pwd)"
SRC="${WORKDIR}/newlib"
BUILD="${SRC}/arm_none_eabi_build"
INSTALL="${SRC}/arm_none_eabi_install"
cd "${WORKDIR}"

# Installing newlib build dependencies
apt update
apt install -y --no-install-recommends texinfo

# Get the latest fixed version
git clone --depth=1 --single-branch --branch newlib-4.5.0 https://sourceware.org/git/newlib-cygwin.git "${SRC}"


# Build configuration
mkdir -p "${BUILD}" "${INSTALL}"
cd "${BUILD}"

CFLAGS_FOR_TARGET="-ffunction-sections -fdata-sections -fshort-wchar -DPREFER_SIZE_OVER_SPEED -mcpu=${TARGET_CPU} -mthumb -mlittle-endian" \
    ../configure  \
    `# Setup` \
    --host=x86_64-linux-gnu `# Host` \
    --target=arm-none-eabi ` # Target` \
    --disable-multilib `# Building only one library` \
    --prefix="${INSTALL}" `#  Installation prefix` \
    `# Options` \
    --disable-silent-rules `# verbose build output (undo: "make V=0")` \
    --disable-dependency-tracking `# speeds up one-time build` \
    --enable-newlib-reent-small `# enable small reentrant struct support` \
    --disable-newlib-fvwrite-in-streamio `# disable iov in streamio ` \
    --disable-newlib-fseek-optimization `# disable fseek optimization` \
    --disable-newlib-wide-orient `# Turn off wide orientation in streamio`\
    --enable-newlib-nano-malloc `# use small-footprint nano-malloc implementation`\
    --disable-newlib-unbuf-stream-opt `# disable unbuffered stream optimization in streamio` \
    --enable-lite-exit `# enable light weight exit` \
    --enable-newlib-global-atexit `# enable atexit data structure as global` \
    --enable-newlib-nano-formatted-io `# Use small-footprint nano-formatted-IO implementation`\
    --disable-newlib-supplied-syscalls `# disable newlib from supplying syscalls`\
    --disable-nls `# do not use Native Language Support` \
    --enable-target-optspace `# optimize for space (emits -g -Os)`


# Compilation
make "-j$(nproc)"
make install


# Copy back
mkdir -p "$OUTPUT_DIR"
cp "${INSTALL}/arm-none-eabi/lib/libc.a" "$OUTPUT_DIR"
cp "${INSTALL}/arm-none-eabi/lib/libm.a" "$OUTPUT_DIR"

