#!/usr/bin/env python3
"""Compare an opt-in installed-asset probe with original native badge hashes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    parser.add_argument("assets", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    receipt = json.loads((Path(__file__).resolve().parents[1] / "docs/behavior/native-badge-raster-cases.json").read_text())
    for name, expected in receipt["assets"].items():
        if hashlib.sha256((args.assets / name).read_bytes()).hexdigest() != expected:
            raise SystemExit("Installed asset differs from pinned original: " + name)
    subprocess.run([str(args.probe.resolve()), str(args.assets.resolve()), str(args.output.resolve())], check=True)
    checked = 0
    for case in receipt["cases"]:
        stem = "native-{}-{}".format(case["level"], case["variant"])
        for suffix, field, size in [("indices", "indexed_sha256", 690), ("rgba", "canonical_RGBA_sha256", 2760)]:
            path = args.output / (stem + "." + suffix)
            data = path.read_bytes()
            if len(data) != size or hashlib.sha256(data).hexdigest() != case[field]:
                raise SystemExit("Original badge mismatch: " + str(path))
            checked += 1
    print("Matched all {} original indexed/RGBA badge hashes".format(checked))


if __name__ == "__main__":
    main()
