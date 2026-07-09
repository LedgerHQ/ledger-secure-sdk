# SDK ClusterFuzzLite Dockerfile. Builds the Absolution-driven SDK self-fuzz
# targets defined under fuzzing/sdk-fuzz/ (see .clusterfuzzlite/build.sh).
FROM gcr.io/oss-fuzz-base/base-builder:v1

RUN pip3 install --break-system-packages --no-cache-dir pillow>=3.4.0
RUN apt update && apt install -y ninja-build

COPY . $SRC/ledger-secure-sdk

WORKDIR $SRC/ledger-secure-sdk

COPY ./.clusterfuzzlite/build.sh $SRC/
