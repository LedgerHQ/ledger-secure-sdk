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

# Working in a temporary folder
mkdir tmp && cd tmp

# Enabling source repository
sed -i 's/^\(Types: deb\)$/\1 deb-src/g' /etc/apt/sources.list.d/debian.sources

# Installing newlib build dependencies
apt update
apt install -y --no-install-recommends texinfo dpkg-dev

# Get the latest fixed version for this OS version
apt source newlib

# Entering the source directory
cd */

# ENV SETUP
WORKDIR="$(pwd)"
#SRC="${WORKDIR}/newlib"
BUILD="${WORKDIR}/arm_none_eabi_build"
INSTALL="${WORKDIR}/arm_none_eabi_install"

# Build configuration
mkdir -p "${BUILD}" "${INSTALL}"
cd "${BUILD}"

CC_FOR_TARGET="clang" \
CFLAGS_FOR_TARGET="--target=arm-none-eabi -ffunction-sections -fdata-sections -fshort-wchar -DPREFER_SIZE_OVER_SPEED -mcpu=${TARGET_CPU} -mthumb -mlittle-endian -fropi -frwpi" \
    ../configure  \
    `# Setup` \
    --host=x86_64-linux-gnu `# Host` \
    --target=arm-none-eabi ` # Target` \
    --disable-multilib `# Building only one library` \
    --prefix="${INSTALL}" `#  Installation prefix` \
    `# Options` \
    --disable-libgloss `# unused library in our sdk` \
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

# Removing the .ARM.exidx section
arm-none-eabi-objcopy -R .ARM.exidx "${INSTALL}/arm-none-eabi/lib/libc.a"

# Stripping debug symbols
arm-none-eabi-strip --strip-debug "${INSTALL}/arm-none-eabi/lib/libc.a"
arm-none-eabi-strip --strip-debug "${INSTALL}/arm-none-eabi/lib/libm.a"

# Copy back
cp "${INSTALL}/arm-none-eabi/lib/libc.a" "$OUTPUT_DIR"
cp "${INSTALL}/arm-none-eabi/lib/libm.a" "$OUTPUT_DIR"
