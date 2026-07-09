#!/usr/bin/env bash
# Promote a corpus directory into the tracked base-corpus.zip + compat-key sidecar.
#
#   promote-corpus.sh <corpus-dir> <base-corpus.zip>
#
# <corpus-dir> is typically a campaign's merged corpus
# (.fuzz-artifacts/<run>/targets/<target>/corpus), which carries a .compat-key.
# The zip stores the inputs (the .compat-key is written next to it as a sidecar).
set -euo pipefail
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

corpus_dir="${1:?usage: promote-corpus.sh <corpus-dir> <base-corpus.zip>}"
dest_zip="${2:?usage: promote-corpus.sh <corpus-dir> <base-corpus.zip>}"

if [[ ! -d "${corpus_dir}" ]]; then
  echo "error: corpus directory not found: ${corpus_dir}" >&2
  exit 1
fi

python3 "${SCRIPT_DIR}/corpus.py" pack "${corpus_dir}" "${dest_zip}"

key_src="${corpus_dir}/.compat-key"
key_dst="${dest_zip%.zip}.compat-key"
if [[ -f "${key_src}" ]]; then
  tr -d '[:space:]' < "${key_src}" > "${key_dst}"
  printf '\n' >> "${key_dst}"
  echo "promote-corpus: wrote ${key_dst}"
else
  echo "warning: ${key_src} not found; leaving ${key_dst} unchanged" >&2
  echo "hint: run a campaign first so the corpus carries a .compat-key." >&2
fi
