# SDK ClusterFuzzLite Dockerfile. Builds the SDK self-fuzz targets defined
# under fuzzing/extra/*.cmake (fuzz_bip32, fuzz_base58, fuzz_alloc,
# fuzz_alloc_utils, fuzz_lists, fuzz_qrcodegen, fuzz_apdu_parser).
FROM gcr.io/oss-fuzz-base/base-builder:v1

RUN pip3 install --break-system-packages --no-cache-dir pillow>=3.4.0
RUN apt update && apt install -y ninja-build zip

COPY . $SRC/ledger-secure-sdk

WORKDIR $SRC/ledger-secure-sdk

COPY ./.clusterfuzzlite/build.sh $SRC/
