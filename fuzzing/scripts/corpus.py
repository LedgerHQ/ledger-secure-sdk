#!/usr/bin/env python3
"""Pack, unpack, and promote a fuzzing corpus, using only the standard library.

No external ``zip`` / ``unzip`` binary is required. ``.compat-key`` is corpus
metadata (it ties the corpus to a build), not a fuzz input, so it is never stored
inside the archive; ``promote`` writes it as a sidecar next to the zip.

Usage:
    corpus.py pack    <src_dir> <dest.zip>
    corpus.py unpack  <src.zip> <dest_dir>
    corpus.py promote <corpus_dir> <base-corpus.zip>

``promote`` is the one to reach for by hand: point it at a campaign's merged
corpus (``.fuzz-artifacts/<run>/targets/<target>/corpus``) and it produces the
tracked ``base-corpus.zip`` plus its ``base-corpus.compat-key``.
"""
import os
import sys
import zipfile

# Metadata files that live next to the inputs but must not enter the archive.
_SKIP = {".compat-key"}


def _iter_inputs(root):
    for dirpath, _dirs, files in os.walk(root):
        for name in files:
            if name in _SKIP:
                continue
            full = os.path.join(dirpath, name)
            yield full, os.path.relpath(full, root)


def pack(src_dir, dest_zip):
    entries = sorted(_iter_inputs(src_dir), key=lambda p: p[1])
    parent = os.path.dirname(os.path.abspath(dest_zip))
    os.makedirs(parent, exist_ok=True)
    with zipfile.ZipFile(dest_zip, "w", zipfile.ZIP_DEFLATED) as zf:
        for full, arc in entries:
            # Fixed metadata keeps the archive byte-reproducible across machines.
            info = zipfile.ZipInfo(arc, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            with open(full, "rb") as f:
                zf.writestr(info, f.read())
    return len(entries)


def unpack(src_zip, dest_dir):
    os.makedirs(dest_dir, exist_ok=True)
    with zipfile.ZipFile(src_zip) as zf:
        zf.extractall(dest_dir)


def promote(corpus_dir, dest_zip):
    """Pack a corpus and copy its compat key to the archive's sidecar."""
    if not os.path.isdir(corpus_dir):
        raise SystemExit(f"error: corpus directory not found: {corpus_dir}")

    count = pack(corpus_dir, dest_zip)
    print(f"corpus: packed {count} file(s) into {dest_zip}", flush=True)

    key_src = os.path.join(corpus_dir, ".compat-key")
    key_dst = f"{dest_zip[:-4] if dest_zip.endswith('.zip') else dest_zip}.compat-key"
    if not os.path.isfile(key_src):
        print(f"warning: {key_src} not found; leaving {key_dst} unchanged", file=sys.stderr)
        print("hint: run a campaign first so the corpus carries a .compat-key.", file=sys.stderr)
        return

    with open(key_src, encoding="utf-8") as f:
        key = "".join(f.read().split())
    with open(key_dst, "w", encoding="utf-8") as f:
        f.write(f"{key}\n")
    print(f"corpus: wrote {key_dst}")


def main(argv):
    actions = {"pack": pack, "unpack": unpack, "promote": promote}
    if len(argv) != 4 or argv[1] not in actions:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    result = actions[argv[1]](argv[2], argv[3])
    if argv[1] == "pack":
        print(f"corpus: packed {result} file(s) into {argv[3]}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
