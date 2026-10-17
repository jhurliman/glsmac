#!/usr/bin/env python3
"""Compare the production specialist parser/codec with literal original loader output."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess

REPORT = "9a0e4203d9bb96de02adf3519ec68bd03cd5cab4b49cbbf0a12ff150648486d1"
FIXTURE = "ef6a3611670cf27280afb99c637be9d57b70df7c835d0bae3c308f92f08c9797"
SOURCE = "4a0e02937cc2cef8efaedbeb5e80ce248f3422f2bc22a9626fbe76ce366eb405"
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
ALPHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pinned(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Evidence fingerprint differs: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "alpha", "fixture", "report", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    report = pinned(args.report, REPORT)
    fixture = pinned(args.fixture, FIXTURE)
    if digest(args.alpha) != ALPHA:
        raise ValueError("Installed alpha differs from original capture")
    if (report["executable_sha256"], report["source_sha256"], report["fixture_sha256"],
            report["input_sha256"]["alpha.txt"], report["loader_return"], report["total"], report["passed"]) != (EXE, SOURCE, FIXTURE, ALPHA, 0, 8, 8):
        raise ValueError("Original capture provenance or completion differs")
    if report["loader_reads"] != {"names": 20, "technology_references": 14, "outputs": 21}:
        raise ValueError("Original section read boundary differs")
    rows = report["records"]
    if rows != fixture["specialists"] or [row["id"] for row in rows] != list(range(7)):
        raise ValueError("Literal output rows or ordering differ")
    if report["minimum_specialist_size"] != fixture["minimum_specialist_size"]["value"]:
        raise ValueError("Literal minimum-size output differs")
    run = subprocess.run([str(args.probe.resolve()), str(args.alpha.resolve())], capture_output=True,
                         text=True, check=True, timeout=30)
    lines = run.stdout.splitlines()
    if len(lines) != 8:
        raise ValueError("Production row count differs")
    failures = []
    if lines[0] != f'minimum {report["minimum_specialist_size"]}':
        failures.append({"field": "minimum", "actual": lines[0]})
    fields = ("id", "name", "plural", "prerequisite", "obsolete", "economy", "psych", "labs",
              "prerequisite_key", "obsolete_key", "source_trailing_token_metadata")
    for row, line in zip(rows, lines[1:]):
        expected = [str(row[field]) for field in fields]
        actual = shlex.split(line)
        if actual != expected:
            failures.append({"id": row["id"], "expected": expected, "actual": actual})
    receipt = {"report_sha256": REPORT, "fixture_sha256": FIXTURE, "source_sha256": SOURCE,
               "original_executable_sha256": EXE, "alpha_sha256": ALPHA,
               "production_probe_sha256": digest(args.probe), "compared_records": 7,
               "compared_minimum": 1, "codec_roundtrips": 1, "failures": failures,
               "scope": "Literal original stored names/reference IDs/EPL/minimum plus supplied source-key/tail metadata. No original rerun, selection, worker allocation or arbitrary malformed-input equivalence."}
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
