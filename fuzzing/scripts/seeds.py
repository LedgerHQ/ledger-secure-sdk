#!/usr/bin/env python3
"""Generate a target's seed corpus from its fuzz manifest.

    seeds.py <manifest.toml> <output_dir> [--fuzzer NAME]

Two sources, either of which a manifest may switch off:

  generic  built here, from the manifest's `seeds.ins` list
  custom   an app-supplied script named by `seeds.custom.script`

A seed is laid out exactly like any other fuzzer input (see fuzz_defs.h):

    [ Absolution prefix ][ lane, command, P1, P2 ][ payload ]

The prefix is left as the neutral template Absolution generated. Seeds
deliberately carry no hand-written global state: measurements showed
state-bearing prefixes and zero-filled ones reach the same coverage, and writing
values into domain-constrained fields is wrong anyway, since those bytes are
selectors into an allowed-value table rather than the values themselves.

What a seed is genuinely useful for is the payload and the command selection,
which is exactly what the control bytes steer.
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from fuzz_manifest import read_manifest, get_manifest_dir, get_target  # noqa: E402
from fuzz_seed_utils import resolve_prefix_size, resolve_seed_prefix, ctrl_bytes  # noqa: E402

#: INS codes assumed when a manifest declares none.
DEFAULT_INS_CODES = list(range(0, 16))


def generate_generic_seeds(output_dir, ins_codes, fuzzer_name):
    """Write three seeds per INS code: raw lane, structured lane, empty payload."""
    prefix_size = resolve_prefix_size(fuzzer_name)
    prefix = resolve_seed_prefix(prefix_size, fuzzer_name)
    n_ins = max(len(ins_codes), 1)
    written = 0

    for i, ins in enumerate(ins_codes):
        # Raw lane: the payload is the APDU body itself.
        raw = bytes((0x42 + j * 7 + i * 13) & 0xFF for j in range(256))
        # Structured lane: the payload feeds whatever builders the app installs.
        structured = bytes((0x55 + j * 11 + i * 19) & 0xFF for j in range(512))

        for name, blob in (
            (f"raw_ins_{ins:02x}", ctrl_bytes(False, i % n_ins, i * 37, i * 53 + 1) + raw),
            (f"structured_ins_{ins:02x}", ctrl_bytes(True, i % n_ins, i * 29, i * 41) + structured),
            # Control bytes and nothing else, to exercise the empty-payload paths.
            (f"minimal_ins_{ins:02x}", ctrl_bytes(i % 2 == 0, i % n_ins)),
        ):
            with open(os.path.join(output_dir, name), "wb") as fh:
                fh.write(prefix + blob)
            written += 1

    print(f"Generated {written} seed corpus files in {output_dir} (prefix size {prefix_size})")


def resolve_custom_script(script, manifest_path):
    """Locate a manifest-declared custom seed script, or return None."""
    for base in (get_manifest_dir(manifest_path), os.environ.get("APP_DIR"), SCRIPT_DIR):
        if base:
            candidate = os.path.join(base, script)
            if os.path.isfile(candidate):
                return candidate
    return None


def run_custom_script(script, manifest_path, output_dir, fuzzer_name):
    """Run an app's own seed generator as a subprocess."""
    script_path = resolve_custom_script(script, manifest_path)
    if script_path is None:
        print(f"warning: custom seed script not found at {script}", file=sys.stderr)
        return False

    # Point the child at *this* SDK's scripts rather than letting it guess from
    # BOLOS_SDK, and name the target so its resolve_prefix_size() reads the right
    # generated fuzzer instead of falling back to the "fuzz_globals" default.
    env = dict(os.environ, LEDGER_FUZZ_SCRIPTS=SCRIPT_DIR)
    if fuzzer_name:
        env["FUZZER"] = fuzzer_name

    # Output is inherited, not captured: a custom generator's progress and its
    # tracebacks then appear in real time. Flush first -- the child writes to the
    # same descriptor, so anything still sitting in our buffer would otherwise be
    # printed after it whenever stdout is a pipe rather than a terminal.
    sys.stdout.flush()
    code = subprocess.run([sys.executable, script_path, output_dir], env=env, check=False).returncode
    sys.stdout.flush()
    if code != 0:
        print(f"warning: custom seed script exited {code}", file=sys.stderr)
    return code == 0


def count_files(directory):
    return sum(1 for e in os.scandir(directory) if e.is_file()) if os.path.isdir(directory) else 0


def main():
    args = sys.argv[1:]
    fuzzer_name = None
    if "--fuzzer" in args:
        i = args.index("--fuzzer")
        fuzzer_name = args[i + 1] if i + 1 < len(args) else None
        del args[i : i + 2]

    if len(args) < 2:
        sys.exit(f"usage: {sys.argv[0]} <manifest.toml> <output_dir> [--fuzzer NAME]")

    manifest_path, output_dir = os.path.realpath(args[0]), args[1]
    os.makedirs(output_dir, exist_ok=True)

    view = get_target(read_manifest(manifest_path), fuzzer_name)
    seeds_cfg = view["seeds"]
    # The resolved target's fuzzer name works for single- and multi-target
    # manifests alike; the raw --fuzzer argument is only a fallback.
    resolved_fuzzer = view.get("target", {}).get("fuzzer") or fuzzer_name

    before = count_files(output_dir)

    if seeds_cfg.get("generic", {}).get("enabled", True):
        print("--- Generic seeds ---")
        generate_generic_seeds(
            output_dir, seeds_cfg.get("ins") or DEFAULT_INS_CODES, resolved_fuzzer
        )

    custom_cfg = seeds_cfg.get("custom", {})
    if custom_cfg.get("enabled", False):
        print("--- Custom seeds ---")
        script = custom_cfg.get("script")
        if script:
            run_custom_script(script, manifest_path, output_dir, resolved_fuzzer)
        else:
            print("warning: seeds.custom.enabled but no script configured", file=sys.stderr)

    after = count_files(output_dir)
    print(f"Total seeds in {output_dir}: {after} (+{after - before} new)")


if __name__ == "__main__":
    main()
