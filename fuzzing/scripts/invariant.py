#!/usr/bin/env python3
"""Build the Absolution invariant model for one target.

Two steps, always run together, so they live in one place:

  1. sync   -- rewrite the model from what Absolution just discovered, giving any
               symbol named in a zero list a fixed single-value domain so it
               leaves the fuzzable prefix without consuming input bytes.
  2. tune   -- apply the app's domain-overrides.txt on top.

by app-common.sh with the second silently skipped when the overrides file was
absent. One entry point makes the pipeline visible and removes a file.

  invariant.py <generated.zon> <output.zon> [--framework-zeros F] [--app-zeros F]
               [--domain-overrides F]
"""

import argparse
import re
import sys
from math import prod

# ── step 1: sync from discovery ──
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


def matches_zero_list(entry_name, entry_source, zero_symbols, matched=None):
    """True when a zero-list selector covers this entry.

    `matched` collects the selectors that hit something, so the caller can report
    the ones that never did. Without that report a selector naming a symbol which
    does not exist -- misspelled, renamed, or in a TU absolution never parsed --
    is silently ignored, and the global it was meant to pin stays fuzzable. Domain
    overrides have been validated this way all along; zero symbols were not, which
    is how two dead selectors survived in an app list and how a set of new ones
    appeared to do nothing.
    """
    hit = False
    for sym_name, sym_source in zero_symbols:
        if entry_name != sym_name:
            continue
        if sym_source is None or (entry_source and sym_source in entry_source):
            if matched is not None:
                matched.add((sym_name, sym_source))
            hit = True
    return hit


def compute_field_zero_bytes(bit_width, dim_lens):
    n = bit_width // 8
    if dim_lens:
        n *= prod(dim_lens)
    return n


def make_zero_hex(n_bytes):
    return "\\x00" * n_bytes


def process_zon(zon_text, zero_symbols, matched=None):
    """Zero entries matching the symbol list.

    Returns (output_text, zeroed_count, total_count). Selectors that matched an
    entry are added to `matched` when given.
    """
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
                    entry_name, entry_source, zero_symbols, matched
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


# ── step 2: apply the app's domain overrides ──
def parse_overrides(path):
    """Parse domain overrides into {(global, field): hex-list for "values", or None for "top"}.

    The selector splits on the FIRST dot, because the model names a nested field
    by its whole flattened path: `g_ctx.metadata.op_type` is the field
    ".metadata.op_type" of the global "g_ctx". Splitting on the last dot instead
    looks for a global named "g_ctx.metadata", which never exists, so every
    nested-field override was silently dropped.
    """
    overrides = {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r"(\S+?)\.(\S*)\s*=\s*top\s*$", line)
            if m:
                overrides[(m.group(1), m.group(2))] = None
                continue
            m = re.match(r"(\S+?)\.(\S*)\s*=\s*values\s+(.*)", line)
            if m:
                values = m.group(3).strip().split()
                overrides[(m.group(1), m.group(2))] = values
                continue
            print(f"warning: skipping unparsable line: {line}", file=sys.stderr)
    return overrides


def apply_overrides(zon_text, overrides):
    """Rewrite domains in place. Returns (text, changes, applied_keys)."""
    lines = zon_text.split("\n")
    result = []
    applied = set()

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
                applied.add(key)
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

    return "\n".join(result), changes, applied


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("generated_zon")
    ap.add_argument("output_zon")
    ap.add_argument("--framework-zeros")
    ap.add_argument("--app-zeros")
    ap.add_argument("--domain-overrides",
                    help="applied after the sync; absent means no overrides")
    args = ap.parse_args()

    zero_symbols = []
    if args.framework_zeros:
        zero_symbols.extend(load_zero_symbols(args.framework_zeros))
    if args.app_zeros:
        zero_symbols.extend(load_zero_symbols(args.app_zeros))

    with open(args.generated_zon, "r", encoding="utf-8") as f:
        zon_text = f.read()

    matched = set()
    zon_text, zeroed, total = process_zon(zon_text, zero_symbols, matched)
    print(f"Synced {args.output_zon}: {total} globals "
          f"({zeroed} zeroed, {total - zeroed} fuzzable)")

    # Same check the domain overrides get below, but scoped to the app's own list.
    # A selector that matches nothing leaves the global it was meant to pin fuzzable,
    # silently spending prefix bytes -- and unlike an override, that failure was
    # reported nowhere. In the app list a non-match means the symbol was misspelled or
    # renamed, which is a bug worth naming. The framework list is shared across apps
    # and deliberately covers translation units a given app never parses, so a
    # non-match there is normal and would drown the signal.
    if args.app_zeros:
        stale = [s for s in load_zero_symbols(args.app_zeros) if s not in matched]
        if stale:
            print(f"warning: {len(stale)} app zero symbol(s) in {args.app_zeros} "
                  f"matched no global:", file=sys.stderr)
            for name, source in stale:
                print(f"  {name}" + (f" @ {source}" if source else ""), file=sys.stderr)

    if args.domain_overrides:
        overrides = parse_overrides(args.domain_overrides)
        if overrides:
            zon_text, changes, applied = apply_overrides(zon_text, overrides)
            unmatched = [k for k in overrides if k not in applied]
            if unmatched:
                # An override that matches nothing is a typo or a symbol that
                # moved. Reporting it as applied is how nested-field selectors
                # stayed broken.
                print(f"error: {len(unmatched)} domain override(s) matched no field:",
                      file=sys.stderr)
                for g, f_name in unmatched:
                    print(f"  {g}.{f_name}" if f_name else f"  {g}.", file=sys.stderr)
                sys.exit(1)
            print(f"Applied {changes} domain override(s)")

    with open(args.output_zon, "w", encoding="utf-8") as f:
        f.write(zon_text)


if __name__ == "__main__":
    main()
