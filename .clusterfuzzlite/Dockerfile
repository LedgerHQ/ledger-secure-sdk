# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1

RUN pip3 install --break-system-packages --no-cache-dir pillow>=3.4.0
RUN apt update && apt install -y ninja-build

# Copy the project's source code.
COPY . $SRC/ledger-secure-sdk

# Working directory for build.sh
WORKDIR $SRC/ledger-secure-sdk

# Copy build.sh into $SRC dir.
COPY ./.clusterfuzzlite/build.sh $SRC/
#CMD ["bash"]
