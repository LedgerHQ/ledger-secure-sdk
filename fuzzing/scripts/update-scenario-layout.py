#!/usr/bin/env python3
"""Extract Absolution prefix layout from generated fuzzer.c and update scenario_layout.h.

Parses the generated sample_invariant() function to determine byte offsets
of named globals within the Absolution prefix, then patches the corresponding
#define macros in the app's scenario_layout.h.

Usage:
    python3 update-scenario-layout.py <fuzzer.c> <scenario_layout.h> [--global NAME:MACRO ...]

Examples:
    # Minimal app: only fuzz_ctrl
    python3 update-scenario-layout.py build/fast/_absolution/fuzz_globals/fuzzer.c \
        fuzzing/mock/scenario_layout.h

"""

import argparse
import re
import sys


def extract_prefix_size(content):
    m = re.search(r"#define ABSOLUTION_GLOBALS_SIZE (\d+)", content)
    return int(m.group(1)) if m else None


def extract_function_body(content, func_name):
    match = re.search(rf"\b{re.escape(func_name)}\s*\(", content)
    if match is None:
        return None

    brace_pos = content.find("{", match.end())
    if brace_pos < 0:
        return None

    depth = 1
    i = brace_pos + 1
    while i < len(content) and depth > 0:
        if content[i] == "{":
            depth += 1
        elif content[i] == "}":
            depth -= 1
        i += 1

    if depth != 0:
        return None

    return content[brace_pos + 1 : i - 1]


def extract_global_offsets(content, target_names):
    """Track the `off` variable through sample_invariant() to find global offsets."""
    body = extract_function_body(content, "sample_invariant")
    if body is None:
        return {}

    offsets = {}
    off = 0
    loop_stack = []
    lines = body.split("\n")
    for line in lines:
        line = line.strip()

        for name in target_names:
            if name in offsets:
                continue
            if re.match(rf"memset\({re.escape(name)}\b", line) or re.match(
                rf"memcpy\(&{re.escape(name)}(?:\[|\b)", line
            ):
                offsets[name] = off

        loop_m = re.match(r"for\s*\(.*;\s*\w+\s*<\s*(\d+)\s*;", line)
        if loop_m and "{" in line:
            loop_stack.append(int(loop_m.group(1)))
            continue

        om = re.match(r"off\s*\+=\s*(\d+);", line)
        if om:
            multiplier = 1
            for count in loop_stack:
                multiplier *= count
            off += int(om.group(1)) * multiplier

        for _ in range(line.count("}")):
            if loop_stack:
                loop_stack.pop()

    return offsets


def update_layout_header(path, replacements):
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()

    for macro, value in replacements.items():
        content = re.sub(
            rf"(#define\s+{re.escape(macro)}\s+)\d+",
            rf"\g<1>{value}",
            content,
        )

    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def main():
    parser = argparse.ArgumentParser(
        description="Extract Absolution prefix layout and update scenario_layout.h"
    )
    parser.add_argument("fuzzer_c", help="Path to generated fuzzer.c")
    parser.add_argument("layout_h", help="Path to scenario_layout.h to update")
    parser.add_argument(
        "--global",
        dest="globals",
        action="append",
        default=[],
        metavar="NAME:MACRO",
        help="Additional global to extract (e.g. psbt_entropy:SCEN_ENTROPY_OFF)",
    )
    parser.add_argument(
        "--ctrl-name",
        default="fuzz_ctrl",
        help="Name of the control header global (default: fuzz_ctrl)",
    )
    args = parser.parse_args()

    with open(args.fuzzer_c, "r", encoding="utf-8") as f:
        content = f.read()

    prefix_size = extract_prefix_size(content)
    if prefix_size is None:
        print("error: ABSOLUTION_GLOBALS_SIZE not found", file=sys.stderr)
        sys.exit(1)

    target_names = {args.ctrl_name}
    extra_map = {}
    for spec in args.globals:
        if ":" not in spec:
            print(f"error: --global must be NAME:MACRO, got '{spec}'", file=sys.stderr)
            sys.exit(1)
        name, macro = spec.split(":", 1)
        target_names.add(name)
        extra_map[name] = macro

    offsets = extract_global_offsets(content, target_names)

    replacements = {"SCEN_PREFIX_SIZE": prefix_size}

    ctrl_off = offsets.get(args.ctrl_name)
    if ctrl_off is not None:
        replacements["SCEN_CTRL_OFF"] = ctrl_off
    else:
        print(
            f"warning: could not find offset for {args.ctrl_name}",
            file=sys.stderr,
        )

    for name, macro in extra_map.items():
        if name in offsets:
            replacements[macro] = offsets[name]
        else:
            print(f"warning: could not find offset for {name}", file=sys.stderr)

    update_layout_header(args.layout_h, replacements)

    parts = [f"SCEN_PREFIX_SIZE={prefix_size}"]
    if ctrl_off is not None:
        parts.append(f"SCEN_CTRL_OFF={ctrl_off}")
    for name, macro in extra_map.items():
        if name in offsets:
            parts.append(f"{macro}={offsets[name]}")
    print(f"Updated {args.layout_h}: {', '.join(parts)}")


if __name__ == "__main__":
    main()
