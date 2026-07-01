#!/usr/bin/env python3
"""PoC LTO: aggregate per-build artifacts into size & build-time comparison tables.

Each artifact directory is named ``elf-<app>-<target>-<config>`` and contains the
uploaded ``build/<target>`` directory, i.e. ``bin/app.elf`` plus ``poc_time.txt`` and
``poc_rc.txt`` written by poc_lto_build_target.sh.

Usage: poc_lto_compare.py <artifacts_root> <output.md>
"""
import pathlib
import subprocess
import sys

CONFIGS = ["baseline", "lto-sp", "lto"]  # most-specific suffix first
TARGETS = ["nanos2", "nanox", "stax", "flex", "apex_p"]


def parse_name(dirname):
    name = dirname[len("elf-"):]
    cfg = next((c for c in CONFIGS if name.endswith("-" + c)), None)
    if cfg is None:
        return None
    name = name[: -(len(cfg) + 1)]
    tgt = next((t for t in TARGETS if name.endswith("-" + t)), None)
    if tgt is None:
        return None
    app = name[: -(len(tgt) + 1)]
    return app, tgt, cfg


def elf_sizes(elf):
    text = None
    out = subprocess.check_output(["size", "-A", str(elf)], text=True)
    for line in out.splitlines():
        cols = line.split()
        if len(cols) >= 2 and cols[0] == ".text":
            text = int(cols[1])
            break
    total = None
    berkeley = subprocess.check_output(["size", str(elf)], text=True).splitlines()
    if len(berkeley) >= 2:
        total = int(berkeley[-1].split()[3])  # text data bss dec hex filename
    return text, total


def read_int(path):
    try:
        return int(path.read_text().strip())
    except (OSError, ValueError):
        return None


def main():
    artifacts_root = pathlib.Path(sys.argv[1])
    out_path = pathlib.Path(sys.argv[2])

    rows = {}  # (app, target) -> {config: {...}}
    for d in sorted(artifacts_root.glob("elf-*-*-*")):
        if not d.is_dir():
            continue
        parsed = parse_name(d.name)
        if parsed is None:
            print(f"skip unrecognized artifact: {d.name}")
            continue
        app, target, cfg = parsed

        rc = read_int(d / "poc_rc.txt")
        secs = read_int(d / "poc_time.txt")
        elf = next(iter(sorted(d.rglob("app.elf"))), None)
        text = total = None
        if elf is not None:
            text, total = elf_sizes(elf)
        ok = rc == 0 and elf is not None
        rows.setdefault((app, target), {})[cfg] = {
            "text": text, "total": total, "secs": secs, "ok": ok,
        }

    def delta(base, val):
        if base is None or val is None or base == 0:
            return "N/A"
        return f"{val - base:+d} ({(val - base) / base * 100:+.1f}%)"

    def size_cell(d, key):
        if d is None:
            return "—"
        if not d["ok"]:
            return "⚠️ REGRESSION"
        v = d.get(key)
        return str(v) if v is not None else "N/A"

    out = ["# PoC LTO — comparison", ""]

    out += [
        "## ELF size (bytes)", "",
        "| App | Target | .text base | .text lto | .text lto-sp | Δ.text lto | Δ.text lto-sp | total base | total lto | total lto-sp |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for app, target in sorted(rows):
        c = rows[(app, target)]
        b, l, s = c.get("baseline"), c.get("lto"), c.get("lto-sp")
        bt = b["text"] if b and b["ok"] else None
        lt = l["text"] if l and l["ok"] else None
        st = s["text"] if s and s["ok"] else None
        out.append("| " + " | ".join([
            app, target,
            size_cell(b, "text"), size_cell(l, "text"), size_cell(s, "text"),
            delta(bt, lt), delta(bt, st),
            size_cell(b, "total"), size_cell(l, "total"), size_cell(s, "total"),
        ]) + " |")

    out += [
        "", "## Build time (s) — indicative, runners differ across jobs", "",
        "| App | Target | base | lto | lto-sp | Δ lto | × lto | Δ lto-sp | × lto-sp |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for app, target in sorted(rows):
        c = rows[(app, target)]
        b, l, s = c.get("baseline"), c.get("lto"), c.get("lto-sp")
        bs = b["secs"] if b else None

        def tcells(x):
            if x is None:
                return "—", "—"
            if not x["ok"]:
                return "⚠️ REGRESSION", "—"
            xs = x["secs"]
            if bs is None or xs is None or bs == 0:
                return "N/A", "N/A"
            return f"{xs - bs:+d}s", f"{xs / bs:.2f}×"

        dl, xl = tcells(l)
        ds, xs_ = tcells(s)
        out.append("| " + " | ".join([
            app, target,
            size_cell(b, "secs"), size_cell(l, "secs"), size_cell(s, "secs"),
            dl, xl, ds, xs_,
        ]) + " |")

    regressions = sum(1 for c in rows.values() for d in c.values() if not d["ok"])
    out += ["", f"**Regressions (failed builds): {regressions}**"]

    text = "\n".join(out) + "\n"
    out_path.write_text(text, encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
