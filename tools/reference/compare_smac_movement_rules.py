#!/usr/bin/env python3
"""Compare actual C++ parser/profile/basic outputs with retained original fixtures."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

FIXTURE = "c5a1be815524453228356ac7257e0a596737b632b268ae61031e1f6d79462359"
REPORT = "142357caba479c41eea0dcee9d7a7f5bbd10b6fd876222216180fb547f8e53ba"
SOURCE = "b48fb64a0f62f214cbe90f6c6f4fe4ede7a3c8b262bcee4041eb35179f3e91d3"
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
ALPHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pinned(path, sha):
    if digest(path) != sha:
        raise ValueError(f"Evidence fingerprint differs: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "fixture", "report", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--alpha", type=Path)
    args = parser.parse_args()
    fixture = pinned(args.fixture, FIXTURE)
    report = pinned(args.report, REPORT)
    if (report["fixture_sha256"], report["source_sha256"], report["executable_sha256"],
            report["alpha_sha256"], report["total"], report["passed"]) != (FIXTURE, SOURCE, EXE, ALPHA, 220, 220):
        raise ValueError("Original report identity/count differs")
    cases = fixture["cases"]
    if len(cases) != 220 or [c["id"] for c in cases] != [f"MOVE-Q-{i:04}" for i in range(1, 221)]:
        raise ValueError("Ordered fixture IDs differ")
    requests, expected = [], []
    for case in cases:
        i, e = case["inputs"], case["expected"]
        chassis = {"Infantry": 0, "Speeder": 1}[i["chassis"]]
        requests.append(f'{i["road_movement_rule"]} {chassis} {i["chassis_move_field"]} {i["chassis_cost_field"]}\n')
        expected.append(f'ok {i["road_movement_rule"]} {e["quantum"]} {i["chassis_move_field"]} {i["chassis_cost_field"]} {e["basic_allowance"]}')
    run = subprocess.run([str(args.probe.resolve())], input="".join(requests), text=True, capture_output=True, timeout=30, check=True)
    actual = run.stdout.splitlines()
    if len(actual) != len(expected):
        raise ValueError("Production result count differs")
    failures = [{"id": c["id"], "expected": e, "actual": a}
                for c, e, a in zip(cases, expected, actual) if e != a]
    installed = 0
    if args.alpha:
        if digest(args.alpha) != ALPHA:
            raise ValueError("Installed source differs from original report")
        run = subprocess.run([str(args.probe.resolve()), str(args.alpha.resolve())], text=True, capture_output=True, timeout=30, check=True)
        # Cost is checked separately by synthetic Cost17 rows; installed receipt
        # records only Q, Moves, allowance, not a claim about cost production.
        rows = run.stdout.splitlines()
        if len(rows) != 2:
            raise ValueError("Installed result count differs")
        for row, expected_row in zip(rows, report["installed_cases"]):
            parts = row.split()
            if (len(parts) != 6 or parts[0] != "ok" or list(map(int, parts[1:4])) != [3, expected_row["quantum"], expected_row["chassis_moves"]]
                    or int(parts[5]) != expected_row["basic_allowance"]):
                failures.append({"installed": expected_row["chassis"], "actual": row})
            installed += 1
    receipt = {"fixture_sha256": FIXTURE, "report_sha256": REPORT, "source_sha256": SOURCE,
               "executable_sha256": EXE, "probe_sha256": digest(args.probe), "compared": 220,
               "installed_compared": installed, "failures": failures,
               "scope": "Literal original parsed-integer fixtures and optional installed source; C++ uses real port text parsers and codec. No original lexer rerun, host/world binding or full unit allowance."}
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
