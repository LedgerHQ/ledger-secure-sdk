"""Shared utilities for Ledger fuzz seed corpus generators."""

import os
import re

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
FUZZ_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, ".."))

STRUCTURED_LANE_THRESHOLD = 102


def get_app_dir():
    app_dir = os.environ.get("APP_DIR")
    if not app_dir:
        raise SystemExit(
            "error: APP_DIR is not set. "
            "hint: export APP_DIR=/path/to/app-boilerplate."
        )
    return app_dir




def get_fuzzer_name():
    return os.environ.get("FUZZER", "fuzz_globals")


def candidate_build_dirs():
    """Return ordered list of candidate build directories for prefix resolution."""
    app_dir = get_app_dir()
    dirs = []
    for build_dir in (
        os.environ.get("BUILD_DIR_FAST"),
        os.environ.get("BUILD_DIR"),
        os.environ.get("BUILD_DIR_COV"),
        os.path.join(app_dir, "build", "fast"),
        os.path.join(app_dir, "build", "cov"),
    ):
        if build_dir and build_dir not in dirs:
            dirs.append(build_dir)
    return dirs


def resolve_prefix_size(fuzzer_name=None):
    """Resolve the Absolution prefix size from env or generated fuzzer.c."""
    env_value = os.environ.get("ABSOLUTION_GLOBALS_SIZE") or os.environ.get(
        "PREFIX_SIZE"
    )
    if env_value:
        return int(env_value, 0)

    if fuzzer_name is None:
        fuzzer_name = get_fuzzer_name()

    for build_dir in candidate_build_dirs():
        generated_fuzzer = os.path.join(
            build_dir, "_absolution", fuzzer_name, "fuzzer.c"
        )
        try:
            with open(generated_fuzzer, "r", encoding="utf-8") as f:
                match = re.search(
                    r"#define ABSOLUTION_GLOBALS_SIZE (\d+)", f.read()
                )
                if match:
                    return int(match.group(1))
        except OSError:
            pass

    raise SystemExit(
        "error: could not resolve prefix size; build the fuzzer first "
        "or set ABSOLUTION_GLOBALS_SIZE/PREFIX_SIZE explicitly"
    )


def resolve_seed_prefix(prefix_size, fuzzer_name=None):
    """Read the Absolution-generated seed prefix from build output."""
    if fuzzer_name is None:
        fuzzer_name = get_fuzzer_name()

    for build_dir in candidate_build_dirs():
        seed_file = os.path.join(
            build_dir, "_absolution", fuzzer_name, "fuzzer.seed"
        )
        try:
            with open(seed_file, "rb") as f:
                seed = f.read()
            if len(seed) >= prefix_size:
                return seed[:prefix_size]
            return seed + (b"\x00" * (prefix_size - len(seed)))
        except OSError:
            continue

    return b"\x00" * prefix_size


# Framework control bytes, at the start of the harness input (see fuzz_defs.h):
#   [0] lane selector, [1] command index, [2] P1, [3] P2
CTRL_LEN = 4


def ctrl_bytes(structured, cmd_idx=0, p1=0, p2=0):
    """Build the framework control header for a seed input."""
    lane = STRUCTURED_LANE_THRESHOLD + 1 if structured else 0
    return bytes([lane & 0xFF, cmd_idx & 0xFF, p1 & 0xFF, p2 & 0xFF])
