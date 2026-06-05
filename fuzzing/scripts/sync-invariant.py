#!/usr/bin/env python3
"""Sync app invariant from Absolution discovery; zeroed symbols get a fixed .whole_values domain so they leave the fuzzable prefix without consuming fuzzer bytes."""

import argparse
import re
import sys
from math import prod


def load_zero_symbols(path):
    """Load selectors from a zero-symbol list file."""
    symbols = []
    if not path:
        return symbols
    try:
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if "@" in line:
                    name, source = line.split("@", 1)
                    symbols.append((name.strip(), source.strip()))
                else:
                    symbols.append((line, None))
    except FileNotFoundError:
        pass
    return symbols


def matches_zero_list(entry_name, entry_source, zero_symbols):
    for sym_name, sym_source in zero_symbols:
        if entry_name == sym_name:
            if sym_source is None:
                return True
            if entry_source and sym_source in entry_source:
                return True
    return False


def compute_field_zero_bytes(bit_width, dim_lens):
    n = bit_width // 8
    if dim_lens:
        n *= prod(dim_lens)
    return n


def make_zero_hex(n_bytes):
    return "\\x00" * n_bytes


def process_zon(zon_text, zero_symbols):
    """Zero entries matching the symbol list; returns (output_text, zeroed_count, total_count)."""
    lines = zon_text.split("\n")
    result = []
    depth = 0

    entry_name = None
    entry_source = None
    entry_zeroed = False
    entry_decided = False

    field_bit_width = None
    field_dim_lens = []

    zeroed_count = 0
    total_count = 0

    for line in lines:
        start_depth = depth

        for ch in line:
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1

        if start_depth == 1 and depth >= 2:
            total_count += 1
            entry_name = None
            entry_source = None
            entry_zeroed = False
            entry_decided = False
            field_bit_width = None
            field_dim_lens = []

        if start_depth == 2:
            m = re.search(r'\.name\s*=\s*"([^"]*)"', line)
            if m:
                entry_name = m.group(1)
            m = re.search(r'\.source_file\s*=\s*"([^"]*)"', line)
            if m:
                entry_source = m.group(1)

        if not entry_decided and entry_name is not None:
            if entry_source is not None or ".fields" in line:
                entry_zeroed = matches_zero_list(
                    entry_name, entry_source, zero_symbols
                )
                entry_decided = True
                if entry_zeroed:
                    zeroed_count += 1

        if entry_zeroed and start_depth >= 3:
            bw = re.search(r"\.bit_width\s*=\s*(\d+)", line)
            if bw:
                field_bit_width = int(bw.group(1))
                field_dim_lens = []

            if field_bit_width is not None:
                for dm in re.finditer(r"\.len\s*=\s*(\d+)", line):
                    field_dim_lens.append(int(dm.group(1)))

            domain_match = re.match(r"^(\s*)\.domain\s*=\s*", line)
            if domain_match and field_bit_width is not None:
                indent = domain_match.group(1)
                n_bytes = compute_field_zero_bytes(field_bit_width, field_dim_lens)
                zero_hex = make_zero_hex(n_bytes)
                result.append(
                    f'{indent}.domain = .{{ .whole_values = .{{ "{zero_hex}" }} }},'
                )
                field_bit_width = None
                field_dim_lens = []
                continue

        if start_depth >= 2 and depth == 1:
            entry_zeroed = False

        result.append(line)

    return "\n".join(result), zeroed_count, total_count


def main():
    parser = argparse.ArgumentParser(
        description="Sync app invariant from Absolution discovery with zero-symbol policies"
    )
    parser.add_argument("generated_zon", help="Path to Absolution's generated fuzzer.c.zon")
    parser.add_argument("output_zon", help="Path to app's fuzz_globals.zon (overwritten)")
    parser.add_argument(
        "--framework-zeros",
        help="Framework SDK/NBGL zero-symbol list",
    )
    parser.add_argument(
        "--app-zeros",
        help="App-local zero-symbol list",
    )
    args = parser.parse_args()

    zero_symbols = []
    if args.framework_zeros:
        zero_symbols.extend(load_zero_symbols(args.framework_zeros))
    if args.app_zeros:
        zero_symbols.extend(load_zero_symbols(args.app_zeros))

    with open(args.generated_zon, "r", encoding="utf-8") as f:
        zon_text = f.read()

    output_text, zeroed_count, total_count = process_zon(zon_text, zero_symbols)

    with open(args.output_zon, "w", encoding="utf-8") as f:
        f.write(output_text)

    kept = total_count - zeroed_count
    print(
        f"Synced {args.output_zon}: {total_count} globals "
        f"({zeroed_count} zeroed, {kept} fuzzable)"
    )


if __name__ == "__main__":
    main()
