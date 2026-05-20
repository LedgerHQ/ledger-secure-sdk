#!/usr/bin/env python3
"""Unified seed generation orchestrator for the Ledger fuzz engine.

Reads the app manifest and runs all configured seed sources:
  1. Generic seeds (generate-seed-corpus-generic.py)
  2. Custom seed script

CLI:
  python3 generate-seeds.py <manifest.toml> <output_dir>

Env contract: same as generate-seed-corpus-generic.py
  (APP_DIR, SCENARIO_LAYOUT_HEADER, BUILD_DIR_FAST, BUILD_DIR_COV, FUZZER)
"""

import os
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.insert(0, SCRIPT_DIR)

from fuzz_manifest import read_manifest, get_manifest_dir, get_target, _is_multi_target


def run_generic_seeds(manifest, output_dir):
    """Run the generic seed generator with CLA/INS from the manifest."""
    seeds_cfg = manifest["seeds"]
    env = dict(os.environ)
    env["FUZZ_APP_CLA"] = hex(seeds_cfg["cla"])

    ins_list = seeds_cfg["ins"]
    env["FUZZ_APP_INS"] = ",".join(
        hex(v) if isinstance(v, int) else str(v) for v in ins_list
    )

    script = os.path.join(SCRIPT_DIR, "generate-seed-corpus-generic.py")
    result = subprocess.run(
        [sys.executable, script, output_dir],
        env=env, check=False, capture_output=True, text=True,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        print(f"warning: generic seed generator exited {result.returncode}",
              file=sys.stderr)
    return result.returncode == 0


def run_custom_script(manifest, manifest_path, output_dir):
    """Run a custom seed generation script declared in the manifest."""
    custom_cfg = manifest["seeds"].get("custom", {})
    if not custom_cfg.get("enabled", False):
        return True

    script = custom_cfg.get("script")
    if not script:
        print("warning: seeds.custom.enabled but no script configured",
              file=sys.stderr)
        return False

    manifest_dir = get_manifest_dir(manifest_path)
    script_path = os.path.join(manifest_dir, script)
    if not os.path.isfile(script_path):
        app_dir = os.environ.get("APP_DIR", "")
        if app_dir:
            script_path = os.path.join(app_dir, script)
    if not os.path.isfile(script_path):
        script_path = os.path.join(SCRIPT_DIR, script)
    if not os.path.isfile(script_path):
        print(f"warning: custom seed script not found at {script}", file=sys.stderr)
        return False

    result = subprocess.run(
        [sys.executable, script_path, output_dir],
        check=False, capture_output=True, text=True,
    )
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        print(f"warning: custom seed script exited {result.returncode}",
              file=sys.stderr)
    return result.returncode == 0


def main():
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} <manifest.toml> <output_dir> [--fuzzer NAME]",
              file=sys.stderr)
        sys.exit(1)

    manifest_path = os.path.realpath(sys.argv[1])
    output_dir = sys.argv[2]
    os.makedirs(output_dir, exist_ok=True)

    fuzzer_name = None
    i = 3
    while i < len(sys.argv):
        if sys.argv[i] == "--fuzzer" and i + 1 < len(sys.argv):
            fuzzer_name = sys.argv[i + 1]
            i += 2
        else:
            i += 1

    manifest = read_manifest(manifest_path)
    if _is_multi_target(manifest):
        view = get_target(manifest, fuzzer_name)
    else:
        view = get_target(manifest)
    seeds_cfg = view["seeds"]

    total_before = len([f for f in os.listdir(output_dir)
                        if os.path.isfile(os.path.join(output_dir, f))]) \
        if os.path.isdir(output_dir) else 0

    if seeds_cfg.get("generic", {}).get("enabled", True):
        print("--- Generic seeds ---")
        run_generic_seeds(view, output_dir)

    if seeds_cfg.get("custom", {}).get("enabled", False):
        print("--- Custom seeds ---")
        run_custom_script(view, manifest_path, output_dir)

    total_after = len([f for f in os.listdir(output_dir)
                       if os.path.isfile(os.path.join(output_dir, f))]) \
        if os.path.isdir(output_dir) else 0

    print(f"Total seeds in {output_dir}: {total_after} "
          f"(+{total_after - total_before} new)")


if __name__ == "__main__":
    main()
