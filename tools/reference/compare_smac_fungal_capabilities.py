#!/usr/bin/env python3
"""Compare the real owned producer to seven pinned original first-worker outputs."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

from project_fungal_cases import PORT_IDS, WORKER_SHA, read

BOUND_SHA = "751139dbb2cedb5774ba38e08b0637fdd2ee403443e8263d16ee7bf1a874d910"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--bound", type=Path, required=True)
    parser.add_argument("--installed", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    fixture = read(args.fixture, WORKER_SHA)
    bound = read(args.bound, BOUND_SHA)
    if bound["fixture_sha256"] != WORKER_SHA or bound["total"] != 7 or bound["passed"] != 7:
        raise ValueError("original bound receipt mismatch")
    files = ["alpha.txt", "gaians.txt", "hive.txt", "univ.txt", "morgan.txt", "spartans.txt", "believe.txt", "peace.txt"]
    for name in files:
        if hashlib.sha256((args.installed / name).read_bytes()).hexdigest() != bound["input_sha256"][name]:
            raise ValueError(f"installed source fingerprint mismatch: {name}")
    results = []
    for case in fixture["cases"]:
        command = [str(args.probe), str(args.installed), PORT_IDS[case["inputs"]["faction_file"]], *map(str, case["expected"]["owned_technology_ids"])]
        run = subprocess.run(command, capture_output=True, text=True, timeout=30, check=True)
        expected = " ".join(map(str, case["expected"]["fungal_capabilities"])) + "\n"
        if run.stdout != expected:
            raise AssertionError((case["id"], run.stdout, expected, run.stderr))
        results.append({"id": case["id"], "capabilities": case["expected"]["fungal_capabilities"]})
    controls = []
    for tokens in (["GAIANS", "89"], ["GAIANS", "-1"], ["GAIANS", "6x"], ["GAIANS", "6", "6"], ["unknown"]):
        run = subprocess.run([str(args.probe), str(args.installed), *tokens], capture_output=True, text=True, timeout=30)
        if run.returncode != 1 or run.stdout:
            raise AssertionError((tokens, run.returncode, run.stdout, run.stderr))
        controls.append({"tokens": tokens, "exit": run.returncode})
    report = {"fixture_sha256": WORKER_SHA, "bound_sha256": BOUND_SHA,
              "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(), "passed": len(results), "results": results, "controls": controls,
              "scope": "Installed actual C++ parsers, explicitly supplied original possession, owned producer and codec restore; no original execution, startup replay, worker assignment or live activation."}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{len(results)}/7 original capability outputs; {len(controls)} probe admission controls")


if __name__ == "__main__":
    main()
