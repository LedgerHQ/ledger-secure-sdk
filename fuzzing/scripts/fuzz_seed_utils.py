"""Shared utilities for Ledger fuzz seed corpus generators.

Provides layout parsing, prefix size resolution, and seed prefix
materialization used by both the generic and app-specific seed generators.

Apps with custom seed generators import this module instead of duplicating
the layout/prefix resolution logic.  See docs/APP_CONTRACT.md for
the env var interface.
"""

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


def get_layout_header_path():
    return os.environ.get(
        "SCENARIO_LAYOUT_HEADER",
        os.path.join(get_app_dir(), "fuzzing", "mock", "scenario_layout.h"),
    )


def parse_layout_header(path=None):
    """Parse SCEN_* defines from scenario_layout.h into a dict."""
    if path is None:
        path = get_layout_header_path()
    defs = {}
    try:
        with open(path, "r", encoding="utf-8") as fh:
            for line in fh:
                m = re.match(r"#define\s+(SCEN_\w+)\s+(\d+)", line)
                if m:
                    defs[m.group(1)] = int(m.group(2))
    except OSError:
        pass
    return defs


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


def validate_prefix_size(prefix_size, layout_defs):
    """Check that resolved prefix matches SCEN_PREFIX_SIZE if defined."""
    header_size = layout_defs.get("SCEN_PREFIX_SIZE")
    if header_size is not None and header_size != prefix_size:
        raise SystemExit(
            f"error: SCEN_PREFIX_SIZE={header_size} "
            f"!= generated prefix {prefix_size}"
        )


def make_prefix_with_ctrl(base_prefix, ctrl_off, ctrl_bytes):
    """Overlay control bytes onto a base prefix at ctrl_off."""
    buf = bytearray(base_prefix)
    end = min(ctrl_off + len(ctrl_bytes), len(buf))
    buf[ctrl_off : end] = ctrl_bytes[: end - ctrl_off]
    return bytes(buf)


def raw_ctrl_bytes(ctrl_len):
    """Build control bytes for raw lane (below threshold)."""
    ctrl = bytearray(ctrl_len)
    ctrl[0] = 10
    return ctrl


def structured_ctrl_bytes(ctrl_len, cmd_idx=0):
    """Build control bytes for structured lane (above threshold).

    ctrl[0] = lane byte (>threshold)
    ctrl[1] = command index
    """
    ctrl = bytearray(ctrl_len)
    ctrl[0] = 0xFF
    ctrl[1] = cmd_idx
    return ctrl
