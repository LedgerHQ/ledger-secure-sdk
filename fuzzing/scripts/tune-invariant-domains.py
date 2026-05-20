#!/usr/bin/env python3
"""Post-process a synced fuzz_globals.zon to apply domain constraints.

Reads a domain-overrides file (simple key=value format) and rewrites
matching field domains in the .zon.  This runs AFTER sync-invariant.py
and applies app-specific semantic constraints.

Usage:
    python3 tune-invariant-domains.py <fuzz_globals.zon> <overrides.txt>

Overrides file format (one per line, # comments, blank lines ignored):
    G_context.state = values \x00 \x01 \x02
    G_context.req_type = values \x00 \x01
    G_swap_validated.amount = top
    G_called_from_swap. = values \x00 \x01

The selector is: GLOBAL_NAME.FIELD_NAME = values HEX_BYTE [HEX_BYTE ...]
                  GLOBAL_NAME.FIELD_NAME = top
For flat globals (single unnamed field "."), use a trailing dot:
                  GLOBAL_NAME. = values ...
For multi-byte fields: HEX_BYTES (e.g. \\x00\\x00\\x01\\xFE)
"""

import re
import sys


def parse_overrides(path):
    """Parse domain overrides.

    Returns dict of (global_name, field_name) -> override, where override is
    a list of hex strings for "values" type, or None for "top" type.
    For flat globals (ZON field .name = "."), use trailing dot: GLOBAL. = ...
    """
    overrides = {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r"(\S+)\.(\S*)\s*=\s*top\s*$", line)
            if m:
                overrides[(m.group(1), m.group(2))] = None
                continue
            m = re.match(r"(\S+)\.(\S*)\s*=\s*values\s+(.*)", line)
            if m:
                values = m.group(3).strip().split()
                overrides[(m.group(1), m.group(2))] = values
                continue
            print(f"warning: skipping unparsable line: {line}", file=sys.stderr)
    return overrides


def apply_overrides(zon_text, overrides):
    lines = zon_text.split("\n")
    result = []

    current_global = None
    current_field = None
    depth = 0
    changes = 0
    skip_until_close = False
    skip_target_depth = 0

    for line in lines:
        start_depth = depth
        for ch in line:
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1

        if skip_until_close:
            if depth <= skip_target_depth:
                skip_until_close = False
            continue

        if start_depth == 1 and depth >= 2:
            current_global = None
            current_field = None

        if start_depth == 2:
            m = re.search(r'\.name\s*=\s*"([^"]*)"', line)
            if m:
                name = m.group(1)
                if not name.startswith("."):
                    current_global = name
                    current_field = None

        if start_depth >= 3:
            m = re.search(r'\.name\s*=\s*"(\.[^"]*)"', line)
            if m:
                current_field = m.group(1)

        if start_depth >= 3 and current_global and current_field:
            key = (current_global, current_field.lstrip("."))
            if key in overrides:
                domain_match = re.match(r"^(\s*)\.domain\s*=\s*", line)
                if domain_match:
                    indent = domain_match.group(1)
                    override_val = overrides[key]
                    if override_val is None:
                        result.append(f"{indent}.domain = .top,")
                    else:
                        val_strs = ", ".join(f'"{v}"' for v in override_val)
                        result.append(f"{indent}.domain = .{{ .values = .{{ {val_strs} }} }},")
                    changes += 1
                    if depth > start_depth:
                        skip_until_close = True
                        skip_target_depth = start_depth
                    continue

        result.append(line)

    return "\n".join(result), changes


def main():
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} <fuzz_globals.zon> <overrides.txt>",
              file=sys.stderr)
        sys.exit(1)

    zon_path = sys.argv[1]
    overrides_path = sys.argv[2]

    overrides = parse_overrides(overrides_path)
    if not overrides:
        print("No overrides to apply.")
        return

    with open(zon_path, "r", encoding="utf-8") as f:
        zon_text = f.read()

    output_text, changes = apply_overrides(zon_text, overrides)

    with open(zon_path, "w", encoding="utf-8") as f:
        f.write(output_text)

    print(f"Applied {changes} domain overrides to {zon_path}")
    for (g, f_name), val in overrides.items():
        label = f"{g}.{f_name}" if f_name else f"{g}."
        if val is None:
            print(f"  {label} -> top")
        else:
            print(f"  {label} -> {len(val)} values")


if __name__ == "__main__":
    main()
