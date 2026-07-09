#!/usr/bin/env python3
"""Pack/unpack a fuzzing corpus directory using only the standard library.

No external ``zip`` / ``unzip`` binary is required. ``.compat-key`` is corpus
metadata (it ties the corpus to a build), not a fuzz input, so it is never
stored inside the archive.

Usage:
    corpus.py pack   <src_dir> <dest.zip>
    corpus.py unpack <src.zip> <dest_dir>
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


def main(argv):
    if len(argv) != 4 or argv[1] not in ("pack", "unpack"):
        print(__doc__.strip(), file=sys.stderr)
        return 2
    if argv[1] == "pack":
        count = pack(argv[2], argv[3])
        print(f"corpus: packed {count} file(s) into {argv[3]}")
    else:
        unpack(argv[2], argv[3])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
