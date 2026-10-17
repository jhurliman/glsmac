#!/usr/bin/env python3
"""Compare actual C++ parser/yields to pinned original literals, without RE execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

FIXTURE_SHA256 = "65fe760ee3c87687663e4d3e297533342fe804e35001a6a294761ef62bddfea6"
ALPHA_SHA256 = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--installed", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    raw = args.fixture.read_bytes()
    if hashlib.sha256(raw).hexdigest() != FIXTURE_SHA256:
        raise ValueError("Unexpected resource text fixture")
    if args.installed and hashlib.sha256(args.installed.read_bytes()).hexdigest() != ALPHA_SHA256:
        raise ValueError("Unexpected installed alpha.txt")
    cases = json.loads(raw)["cases"]
    assert len(cases) == 38 and len({c["id"] for c in cases}) == 38
    results, failures = [], []
    with tempfile.TemporaryDirectory(prefix="resourceinfo-owned-") as folder:
        for case in cases:
            if "synthetic_text" in case:
                path = Path(folder) / (case["id"] + ".txt")
                path.write_bytes(case["synthetic_text"].encode("ascii"))
            else:
                assert case["id"] == "installed"
                if not args.installed:
                    continue
                path = args.installed
            run = subprocess.run([str(args.probe.resolve()), str(path.resolve())], text=True, capture_output=True, timeout=30)
            expected = [case["expected_rows"][i][:3] for i in (1, 2, 4)] + [c["expected"] for c in case["checks"]]
            actual = [[int(v) for v in line.split()] for line in run.stdout.splitlines()]
            passed = run.returncode == 0 and actual == expected
            results.append({"id": case["id"], "passed": passed, "exit": run.returncode})
            if not passed:
                failures.append({"id": case["id"], "stderr": run.stderr, "actual": actual, "expected": expected})
    report = {"fixture_sha256": FIXTURE_SHA256, "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
              "installed_sha256": ALPHA_SHA256 if args.installed else None,
              "cases": len(results), "yield_triples": len(results) * 16, "results": results, "failures": failures}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: report[k] for k in ("cases", "yield_triples", "failures")}))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
