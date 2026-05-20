#!/usr/bin/env python3
"""Generate generic seed corpus entries for any Ledger app fuzzer.

State-level model: each seed is a single APDU (prefix + tail).  Absolution
controls global state via the prefix; the tail carries one APDU payload.

Env contract: APP_DIR, SCENARIO_LAYOUT_HEADER, BUILD_DIR_FAST, FUZZER.
"""

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from fuzz_seed_utils import (
    parse_layout_header,
    resolve_prefix_size,
    resolve_seed_prefix,
    validate_prefix_size,
    make_prefix_with_ctrl,
    raw_ctrl_bytes,
    structured_ctrl_bytes,
    get_layout_header_path,
)

LAYOUT_HEADER = get_layout_header_path()
_LAYOUT_DEFS = parse_layout_header(LAYOUT_HEADER)

CTRL_OFF = _LAYOUT_DEFS.get("SCEN_CTRL_OFF", 0)
CTRL_LEN = _LAYOUT_DEFS.get("SCEN_CTRL_LEN", 16)


def resolve_ins_codes():
    env_ins = os.environ.get("FUZZ_APP_INS")
    if env_ins:
        return [int(x.strip(), 0) for x in env_ins.split(",") if x.strip()]
    return list(range(0, 16))


def generate_seeds(output_dir):
    os.makedirs(output_dir, exist_ok=True)

    prefix_size = resolve_prefix_size()
    validate_prefix_size(prefix_size, _LAYOUT_DEFS)

    base_prefix = resolve_seed_prefix(prefix_size)
    ins_codes = resolve_ins_codes()
    app_cla = int(os.environ.get("FUZZ_APP_CLA", "0xE0"), 0)
    seeds = []

    for i, ins in enumerate(ins_codes):
        ctrl = raw_ctrl_bytes(CTRL_LEN)
        prefix = make_prefix_with_ctrl(base_prefix, CTRL_OFF, ctrl)
        tail = bytearray(256)
        tail[0] = app_cla & 0xFF
        tail[1] = ins & 0xFF
        tail[2] = (i * 37) & 0xFF
        tail[3] = (i * 53 + 1) & 0xFF
        for j in range(4, len(tail)):
            tail[j] = (0x42 + j * 7 + i * 13) & 0xFF
        seeds.append((f"raw_ins_{ins:02x}", prefix + bytes(tail)))

    for i, ins in enumerate(ins_codes):
        ctrl = structured_ctrl_bytes(CTRL_LEN, i % max(len(ins_codes), 1))
        prefix = make_prefix_with_ctrl(base_prefix, CTRL_OFF, ctrl)
        tail = bytearray(512)
        for j in range(len(tail)):
            tail[j] = (0x55 + j * 11 + i * 19) & 0xFF
        seeds.append((f"structured_ins_{ins:02x}", prefix + bytes(tail)))

    for i, ins in enumerate(ins_codes):
        ctrl = raw_ctrl_bytes(CTRL_LEN)
        ctrl[0] = 5
        prefix = make_prefix_with_ctrl(base_prefix, CTRL_OFF, ctrl)
        tail = bytes([app_cla, ins, 0, 0])
        seeds.append((f"raw_minimal_{ins:02x}", prefix + tail))

    for i, ins in enumerate(ins_codes):
        ctrl = structured_ctrl_bytes(CTRL_LEN, i % max(len(ins_codes), 1))
        ctrl[0] = 0xC0
        prefix = make_prefix_with_ctrl(base_prefix, CTRL_OFF, ctrl)
        tail = bytes([app_cla, ins, 0, 0])
        seeds.append((f"structured_minimal_{ins:02x}", prefix + tail))

    written = 0
    for name, blob in seeds:
        path = os.path.join(output_dir, name)
        with open(path, "wb") as f:
            f.write(blob)
        written += 1

    print(f"Generated {written} seed corpus files in {output_dir} (prefix size {prefix_size})")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "base-corpus"
    generate_seeds(out)
