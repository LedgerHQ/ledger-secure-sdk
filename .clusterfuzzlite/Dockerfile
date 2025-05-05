# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1

# Copy the project's source code.
COPY . $SRC/ledger-secure-sdk

# Working directory for build.sh
WORKDIR $SRC/ledger-secure-sdk

# Copy build.sh into $SRC dir.
COPY ./.clusterfuzzlite/build.sh $SRC/
