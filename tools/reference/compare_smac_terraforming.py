#!/usr/bin/env python3
"""Compare actual parser/codec output to retained original-loader integers.

No original executable or reference checker is imported/executed. Inputs remain
private; only numeric projections are printed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPORT = "d13aba8981ec971c523dfd5360bd42e130a94978107640234913bd0fdc4a042e"
ALPHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"
EXECUTABLE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"

def pinned(path, digest):
    data = Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != digest:
        raise ValueError(f"Unexpected input digest: {path}")
    return data

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("probe")
    parser.add_argument("alpha")
    parser.add_argument("report")
    args = parser.parse_args()
    report = json.loads(pinned(args.report, REPORT))
    pinned(args.alpha, ALPHA)
    if report["alpha_sha256"] != ALPHA or report["executable_sha256"] != EXECUTABLE or report["loader_return"] != 0:
        raise ValueError("Original capture identity or return differs")
    words = report["words"]
    expected = [[i, *[words[str(0x6afd00 + 32 * i + offset)] for offset in (8, 12, 24)]] for i in range(20)]
    fixture = (Path(__file__).parents[2] / "src/gse/tests/fixtures/SmacTerraformingRules.h").read_text()
    for name, column in (("land", 1), ("sea", 2), ("times", 3)):
        found = re.search(r"\b" + name + r"\{([^}]+)\}", fixture)
        if not found or [int(v) for v in found[1].split(",")] != [row[column] for row in expected]:
            raise ValueError(f"Committed literal projection differs: {name}")
    run = subprocess.run([args.probe, args.alpha], text=True, capture_output=True, check=True, timeout=30)
    actual = [[int(v) for v in line.split()] for line in run.stdout.splitlines()]
    if actual != expected:
        raise ValueError("Port parser/codec/resolver differs from original active TERRAIN records")
    print("20 original active TERRAIN rows matched; committed literal fixture matched")

if __name__ == "__main__":
    main()
