#!/usr/bin/env python3
"""App manifest reader for the Ledger fuzz framework."""

import hashlib
import argparse
import os
import tomllib

#: Harness input bytes above the Absolution prefix that the campaign should offer.
#:
#: An APDU's Lc is a single byte, and fuzz_harness.h clamps cmd.lc to 255, so the
#: default entry path can never dispatch more than 255 payload bytes however large
#: -max_len is. 4 control bytes + 255 payload + slack covers it. An app whose harness
#: builds its payload from a wider tail (per-scenario slots, for instance) declares
#: what it actually reads as [target].tail_budget; bytes beyond that are inert, and
#: inert bytes are where generic mutations go to die.
DEFAULT_TAIL_BUDGET = 288

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
FUZZ_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, ".."))


def _is_multi_target(manifest):
    return "targets" in manifest and isinstance(manifest["targets"], list)


def read_manifest(path):
    """Parse and validate a fuzz-manifest.toml file (single or multi-target)."""
    with open(path, "rb") as fh:
        manifest = tomllib.load(fh)

    if _is_multi_target(manifest):
        return _validate_multi(manifest)
    return _validate_single(manifest)


def _validate_single(manifest):
    for section in ("target", "coverage", "seeds"):
        if section not in manifest:
            raise ValueError(f"manifest missing required section: [{section}]")

    target = manifest["target"]
    if "fuzzer" not in target:
        raise ValueError("manifest [target] missing 'fuzzer'")
    if "harness_version" not in target:
        raise ValueError("manifest [target] missing 'harness_version'")

    coverage = manifest["coverage"]
    if "key_files" not in coverage:
        raise ValueError("manifest [coverage] missing 'key_files'")
    if not isinstance(coverage["key_files"], list):
        raise ValueError("manifest [coverage].key_files must be a list")

    seeds = manifest["seeds"]
    if "ins" not in seeds:
        raise ValueError("manifest [seeds] missing 'ins'")
    if not isinstance(seeds["ins"], list) or not seeds["ins"]:
        raise ValueError("manifest [seeds].ins must be a non-empty list")

    target.setdefault("tail_budget", DEFAULT_TAIL_BUDGET)
    if not isinstance(target["tail_budget"], int) or target["tail_budget"] < 1:
        raise ValueError("manifest [target].tail_budget must be a positive integer")

    manifest.setdefault("dictionary", {})
    manifest["dictionary"].setdefault("tokens", [])
    seeds.setdefault("generic", {"enabled": True})
    seeds.setdefault("custom", {"enabled": False})

    return manifest


def _validate_multi(manifest):
    targets = manifest["targets"]
    if not targets:
        raise ValueError("manifest [[targets]] array is empty")

    harness_version = manifest.get("harness_version",
                                   manifest.get("sdk", {}).get("harness_version"))
    if harness_version is None:
        raise ValueError("multi-target manifest missing top-level 'harness_version' or [sdk].harness_version")

    seen = set()
    for i, t in enumerate(targets):
        if "fuzzer" not in t:
            raise ValueError(f"[[targets]][{i}] missing 'fuzzer'")
        name = t["fuzzer"]
        if name in seen:
            raise ValueError(f"duplicate fuzzer name '{name}' in [[targets]]")
        seen.add(name)

        t.setdefault("harness_version", str(harness_version))
        t.setdefault("key_files", [])
        t.setdefault("dictionary", {})
        t["dictionary"].setdefault("tokens", [])

        seeds = t.setdefault("seeds", {"ins": [0x01]})
        seeds.setdefault("generic", {"enabled": True})
        seeds.setdefault("custom", {"enabled": False})


    manifest.setdefault("coverage", {})
    manifest["coverage"].setdefault("exclude_regexes", [])

    return manifest


def list_targets(manifest):
    """Return list of fuzzer names."""
    if _is_multi_target(manifest):
        return [t["fuzzer"] for t in manifest["targets"]]
    return [manifest["target"]["fuzzer"]]


def get_target(manifest, fuzzer_name=None):
    """Return a normalised single-target view (fuzzer_name required for multi-target)."""
    if not _is_multi_target(manifest):
        return {
            "target": manifest["target"],
            "coverage": manifest["coverage"],
            "seeds": manifest["seeds"],
            "dictionary": manifest.get("dictionary", {"tokens": []}),
        }

    if fuzzer_name is None:
        raise ValueError("multi-target manifest requires --fuzzer NAME")

    for t in manifest["targets"]:
        if t["fuzzer"] == fuzzer_name:
            return {
                "target": {
                    "fuzzer": t["fuzzer"],
                    "harness_version": t["harness_version"],
                    "tail_budget": t.get("tail_budget", DEFAULT_TAIL_BUDGET),
                },
                "coverage": {
                    "key_files": t.get("key_files", []),
                    "exclude_regexes": manifest["coverage"].get("exclude_regexes", []),
                },
                "seeds": t.get("seeds", {"ins": [0x01],
                                          "generic": {"enabled": True},
                                          "custom": {"enabled": False}}),
                "dictionary": t.get("dictionary", {"tokens": []}),
            }

    available = [t["fuzzer"] for t in manifest["targets"]]
    raise ValueError(f"fuzzer '{fuzzer_name}' not found in [[targets]]; available: {available}")


def get_manifest_dir(manifest_path):
    """Return the directory containing the manifest (the fuzzing subdir)."""
    return os.path.dirname(os.path.realpath(manifest_path))


def shell_export(view):
    """Return shell-sourceable variable assignments from a target view."""
    target = view["target"]
    coverage = view["coverage"]

    lines = []
    lines.append(f'FUZZER="{target["fuzzer"]}"')
    lines.append(f'TAIL_BUDGET={target.get("tail_budget", DEFAULT_TAIL_BUDGET)}')

    key_files = [f'"{f}"' for f in coverage.get("key_files", [])]
    lines.append(f'KEY_FILES_REL=({" ".join(key_files)})')

    exclude_regexes = [f"'{r}'" for r in coverage.get("exclude_regexes", [])]
    lines.append(f'COVERAGE_EXCLUDE_REGEXES=({" ".join(exclude_regexes)})')


    return "\n".join(lines)


def write_dictionary(view, output_path):
    """Write a LibFuzzer dictionary file from manifest tokens."""
    tokens = view.get("dictionary", {}).get("tokens", [])
    with open(output_path, "w", encoding="utf-8") as fh:
        for tok in tokens:
            name = tok.get("name", "token")
            value = tok.get("value", "")
            fh.write(f'{name}="{value}"\n')



def compute_compat_key(prefix_size, invariant_path, view):
    """Compute the compatibility key for corpus versioning."""
    hasher = hashlib.sha256()
    hasher.update(str(prefix_size).encode("utf-8"))

    inv_hash = hashlib.sha256()
    try:
        with open(invariant_path, "rb") as fh:
            inv_hash.update(fh.read())
    except OSError:
        inv_hash.update(b"<missing>")
    hasher.update(inv_hash.hexdigest().encode("utf-8"))

    hasher.update(view["target"]["fuzzer"].encode("utf-8"))
    hasher.update(str(view["target"]["harness_version"]).encode("utf-8"))

    return hasher.hexdigest()


def main():
    # argparse rather than a hand-rolled loop: the loop dropped anything it did not
    # recognise into a "rest" list, so a misspelled --fuzzer was silently ignored and
    # a multi-target manifest happily answered for the default target with exit 0.
    ap = argparse.ArgumentParser(description=__doc__)
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--list-targets", action="store_true", help="print every target name")
    mode.add_argument("--shell", action="store_true", help="print shell assignments to eval")
    mode.add_argument("--dict", action="store_true", help="write the LibFuzzer dictionary to OUTPUT")
    mode.add_argument("--compat-key", action="store_true", help="print the corpus compatibility key")
    ap.add_argument("manifest")
    ap.add_argument("output", nargs="?", help="destination path, for --dict")
    ap.add_argument("--fuzzer", help="target name; required for a multi-target manifest")
    ap.add_argument("--prefix-size", type=int, help="Absolution prefix size (--compat-key)")
    ap.add_argument("--invariant", help="path to the .zon model (--compat-key)")
    args = ap.parse_args()

    manifest = read_manifest(args.manifest)

    if args.list_targets:
        for name in list_targets(manifest):
            print(name)
        return

    view = get_target(manifest, args.fuzzer)

    if args.shell:
        print(shell_export(view))
    elif args.dict:
        if args.output is None:
            ap.error("--dict requires an output path")
        write_dictionary(view, args.output)
        print(f"Wrote dictionary to {args.output}")
    else:
        if args.prefix_size is None:
            ap.error("--compat-key requires --prefix-size")
        invariant_path = args.invariant
        if invariant_path is None:
            inv_dir = os.path.join(get_manifest_dir(args.manifest), "invariants")
            invariant_path = os.path.join(inv_dir, f"{view['target']['fuzzer']}.zon")
            if not os.path.exists(invariant_path):
                invariant_path = os.path.join(inv_dir, "fuzz_globals.zon")
        print(compute_compat_key(args.prefix_size, invariant_path, view))


if __name__ == "__main__":
    main()
