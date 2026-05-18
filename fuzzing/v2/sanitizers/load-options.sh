#!/usr/bin/env bash
# Source this from app-campaign.sh, replay scripts, or .clusterfuzzlite/build.sh.
#
# Reads ubsan-options.env and asan-options.env and exports them as the
# colon-separated strings the sanitizer runtimes expect.
# Format of each .env file:
#   key=value   — one per line
#   # …         — comments (ignored)
#   <blank>     — ignored

_sanitizers_dir="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

_join_kv() {
  awk '/^#/{next} /^[[:space:]]*$/{next} {print}' "$1" | paste -sd: -
}

if [[ -f "${_sanitizers_dir}/ubsan-options.env" ]]; then
  UBSAN_OPTIONS="$(_join_kv "${_sanitizers_dir}/ubsan-options.env")"
  if [[ -f "${_sanitizers_dir}/ubsan-suppressions.txt" ]]; then
    UBSAN_OPTIONS="${UBSAN_OPTIONS}:suppressions=${_sanitizers_dir}/ubsan-suppressions.txt"
  fi
  export UBSAN_OPTIONS
fi

if [[ -f "${_sanitizers_dir}/asan-options.env" ]]; then
  ASAN_OPTIONS="$(_join_kv "${_sanitizers_dir}/asan-options.env")"
  export ASAN_OPTIONS
fi

unset _sanitizers_dir _join_kv
