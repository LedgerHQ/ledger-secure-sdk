#!/usr/bin/env python3
"""App manifest reader for the Ledger fuzz framework.

Reads an app-local ``fuzz-manifest.toml`` and provides:
  - parsed manifest data with validation
  - shell-sourceable variable export
  - LibFuzzer dictionary writing
  - compatibility key computation
  - multi-target enumeration

Supports two manifest shapes:
  Single-target (app):  [target] + [coverage] + [seeds] ...
  Multi-target  (SDK):  [[targets]] + optional [coverage] ...

CLI:
  python3 fuzz_manifest.py --shell   <manifest> [--fuzzer NAME]
  python3 fuzz_manifest.py --dict    <manifest> <output.dict> [--fuzzer NAME]
  python3 fuzz_manifest.py --compat-key <manifest> [--prefix-size N] [--invariant PATH] [--fuzzer NAME]
  python3 fuzz_manifest.py --list-targets <manifest>
  python3 fuzz_manifest.py --validate <manifest>
"""

import hashlib
import os
import sys
import tomllib

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
    """Validate a traditional single-target manifest."""
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
    if "cla" not in seeds:
        raise ValueError("manifest [seeds] missing 'cla'")
    if "ins" not in seeds:
        raise ValueError("manifest [seeds] missing 'ins'")

    manifest.setdefault("dictionary", {})
    manifest["dictionary"].setdefault("tokens", [])
    seeds.setdefault("generic", {"enabled": True})
    seeds.setdefault("custom", {"enabled": False})

    mocks = manifest.setdefault("mocks", {})
    mocks.setdefault("override_sources", [])

    if not isinstance(mocks["override_sources"], list):
        raise ValueError("manifest [mocks].override_sources must be a list")


    return manifest


def _validate_multi(manifest):
    """Validate a multi-target manifest (``[[targets]]`` array)."""
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

        seeds = t.setdefault("seeds", {"cla": 0x00, "ins": [0x01]})
        seeds.setdefault("generic", {"enabled": True})
        seeds.setdefault("custom", {"enabled": False})

        mocks = t.setdefault("mocks", {})
        mocks.setdefault("override_sources", [])

    manifest.setdefault("coverage", {})
    manifest["coverage"].setdefault("exclude_regexes", [])

    return manifest


def list_targets(manifest):
    """Return list of fuzzer names."""
    if _is_multi_target(manifest):
        return [t["fuzzer"] for t in manifest["targets"]]
    return [manifest["target"]["fuzzer"]]


def get_target(manifest, fuzzer_name=None):
    """Return a normalised single-target dict for *fuzzer_name*.

    For single-target manifests, *fuzzer_name* is ignored.
    For multi-target manifests, *fuzzer_name* is required.
    The returned dict always has the same shape as a single-target manifest's
    [target]+[seeds]+[coverage]+... sections, so downstream code is unchanged.
    """
    if not _is_multi_target(manifest):
        return {
            "target": manifest["target"],
            "coverage": manifest["coverage"],
            "seeds": manifest["seeds"],
            "dictionary": manifest.get("dictionary", {"tokens": []}),
            "mocks": manifest.get("mocks", {"override_sources": []}),
            "layout": manifest.get("layout", {}),
        }

    if fuzzer_name is None:
        raise ValueError("multi-target manifest requires --fuzzer NAME")

    for t in manifest["targets"]:
        if t["fuzzer"] == fuzzer_name:
            return {
                "target": {
                    "fuzzer": t["fuzzer"],
                    "harness_version": t["harness_version"],
                },
                "coverage": {
                    "key_files": t.get("key_files", []),
                    "exclude_regexes": manifest["coverage"].get("exclude_regexes", []),
                },
                "seeds": t.get("seeds", {"cla": 0x00, "ins": [0x01],
                                          "generic": {"enabled": True},
                                          "custom": {"enabled": False}}),
                "dictionary": t.get("dictionary", {"tokens": []}),
                "mocks": t.get("mocks", {"override_sources": []}),
                "layout": t.get("layout", {}),
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

    key_files = [f'"{f}"' for f in coverage.get("key_files", [])]
    lines.append(f'KEY_FILES_REL=({" ".join(key_files)})')

    exclude_regexes = [f"'{r}'" for r in coverage.get("exclude_regexes", [])]
    lines.append(f'COVERAGE_EXCLUDE_REGEXES=({" ".join(exclude_regexes)})')

    layout_extra = view.get("layout", {}).get("extra_args", [])
    if layout_extra:
        extra_parts = [f'"{a}"' for a in layout_extra]
        lines.append(f'LAYOUT_UPDATE_EXTRA_ARGS=({" ".join(extra_parts)})')

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


def _parse_extra_args(argv, start=3):
    """Parse --fuzzer NAME and other extra flags from argv[start:]."""
    fuzzer_name = None
    prefix_size = None
    invariant_path = None
    rest = []
    i = start
    while i < len(argv):
        if argv[i] == "--fuzzer" and i + 1 < len(argv):
            fuzzer_name = argv[i + 1]
            i += 2
        elif argv[i] == "--prefix-size" and i + 1 < len(argv):
            prefix_size = int(argv[i + 1])
            i += 2
        elif argv[i] == "--invariant" and i + 1 < len(argv):
            invariant_path = argv[i + 1]
            i += 2
        else:
            rest.append(argv[i])
            i += 1
    return fuzzer_name, prefix_size, invariant_path, rest


def main():
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} --shell|--dict|--compat-key|--list-targets|--validate <manifest> [args...]",
              file=sys.stderr)
        sys.exit(1)

    mode = sys.argv[1]
    manifest_path = sys.argv[2]
    manifest = read_manifest(manifest_path)

    fuzzer_name, prefix_size, invariant_path, rest = _parse_extra_args(sys.argv)

    if mode == "--list-targets":
        for name in list_targets(manifest):
            print(name)

    elif mode == "--shell":
        view = get_target(manifest, fuzzer_name)
        print(shell_export(view))

    elif mode == "--dict":
        if not rest:
            print("error: --dict requires output path", file=sys.stderr)
            sys.exit(1)
        view = get_target(manifest, fuzzer_name)
        write_dictionary(view, rest[0])
        print(f"Wrote dictionary to {rest[0]}")

    elif mode == "--compat-key":
        view = get_target(manifest, fuzzer_name)

        if prefix_size is None:
            print("error: --compat-key requires --prefix-size", file=sys.stderr)
            sys.exit(1)
        if invariant_path is None:
            fuzz_dir = get_manifest_dir(manifest_path)
            fuzzer = view["target"]["fuzzer"]
            invariant_path = os.path.join(fuzz_dir, "invariants", f"{fuzzer}.zon")
            if not os.path.exists(invariant_path):
                invariant_path = os.path.join(fuzz_dir, "invariants", "fuzz_globals.zon")

        key = compute_compat_key(prefix_size, invariant_path, view)
        print(key)

    elif mode == "--validate":
        targets = list_targets(manifest)
        for tname in targets:
            view = get_target(manifest, tname)
        if _is_multi_target(manifest):
            print(f"Manifest OK: multi-target, {len(targets)} targets: {targets}")
        else:
            t = manifest["target"]
            print(f"Manifest OK: fuzzer={t['fuzzer']}, "
                  f"harness_version={t['harness_version']}, "
                  f"key_files={len(manifest['coverage']['key_files'])}, "
                  f"ins={manifest['seeds']['ins']}, ")

    else:
        print(f"error: unknown mode {mode}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
