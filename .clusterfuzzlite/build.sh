#!/bin/bash -eu
# ClusterFuzzLite build for the SDK self-fuzz targets. Delegates to the shared
# fuzzing/scripts/cfl-build.sh; only the tree location differs from an app.
export BOLOS_SDK="${SRC:-/src}/ledger-secure-sdk"
export APP_DIR="${BOLOS_SDK}"
export APP_FUZZ_SUBDIR="fuzzing/sdk-fuzz"
exec "${BOLOS_SDK}/fuzzing/scripts/cfl-build.sh"
