#!/usr/bin/env bash
# PoC LTO: build one target for the current config, recording build time and the
# make return code so the compare job can report timings and regressions.
#
# Usage (run from the app build directory):
#   poc_lto_build_target.sh <TARGET> "<make_flags>"
#
# Never fails the step on a build error: a failed build is surfaced as a
# regression by the compare job (missing ELF / non-zero rc).
set -u

T="$1"
FLAGS="${2:-}"

echo "=== Building TARGET=$T  flags: [$FLAGS] ==="
make clean >/dev/null 2>&1 || true

start=$SECONDS
set +e
# shellcheck disable=SC2086  # FLAGS must word-split into separate make arguments
make TARGET="$T" BOLOS_SDK="$GITHUB_WORKSPACE/sdk" $FLAGS
rc=$?
set -e
elapsed=$((SECONDS - start))

# Collect outputs in a clean, build-directory-independent location for upload.
outdir="$GITHUB_WORKSPACE/poc_out/$T"
mkdir -p "$outdir"
echo "$elapsed" > "$outdir/poc_time.txt"
echo "$rc"      > "$outdir/poc_rc.txt"
elf=$(find build -path "*/bin/app.elf" 2>/dev/null | head -1)
if [ -n "$elf" ] && [ -f "$elf" ]; then
  cp "$elf" "$outdir/app.elf"
fi

echo "TARGET=$T flags=[$FLAGS] rc=$rc time=${elapsed}s"
exit 0
