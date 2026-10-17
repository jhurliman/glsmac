#!/usr/bin/env python3
"""Compare the owned runtime producer to literal original START-TECH outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

REPORT_SHA = "c086ce624d47077f2985128c451b20f7f6cb701fc670437f12bb5471e8d76669"
FIXTURE_SHA = "f1b22047c99a1f0d672e2cd029b8bd57800ac764c9117b97b5e3541775a7208e"
ALPHA_SHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"
EXE_SHA = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Evidence fingerprint changed: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def quoted(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "alpha", "report", "fixtures", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    report = read(args.report, REPORT_SHA)
    fixtures = read(args.fixtures, FIXTURE_SHA)
    if (report["executable_sha256"] != EXE_SHA or report["fixture_sha256"] != FIXTURE_SHA
            or report["total"] != 128 or report["passed"] != 128 or digest(args.alpha) != ALPHA_SHA):
        raise ValueError("Original identity or completed subset changed")
    executed = [case for case in fixtures["cases"] if case.get("kind") != "initial_research_through_first_command"]
    if [case["id"] for case in executed] != [row["id"] for row in report["results"]]:
        raise ValueError("Original report no longer matches its ordered input subset")
    selected = [(case, row) for case, row in zip(executed, report["results"])
                if case.get("kind") != "technology_reset_guard_only"]
    if len(selected) != 107 or len(fixtures["cases"]) != 135:
        raise ValueError("Supported case count changed")
    comparisons = []
    for case, row in selected:
        inputs = case["inputs"]
        if inputs["global_turn"] != 0 or not row["passed"]:
            raise ValueError("Unsupported turn or failed original case")
        research = inputs["initial_research"]
        values = [inputs["faction_slot"], inputs["additional_named_owner"] or 0,
                  inputs["other_owner_seeded_after_global_clear"] or 0,
                  *(research[key] for key in ("score", "future_count", "labs", "target", "cached_threshold")),
                  len(inputs["bonus_pairs"])]
        request = " ".join(map(str, values)) + "\n"
        request += "\n".join(quoted(key) + " " + quoted(value) for key, value in inputs["bonus_pairs"]) + "\n"
        process = subprocess.run([str(args.probe.resolve()), str(args.alpha.resolve())], input=request,
                                 text=True, capture_output=True, timeout=10)
        if process.returncode:
            raise RuntimeError(f"Production rejected {case['id']}: {process.stderr}")
        actual = json.loads(process.stdout)
        original = row["actual"]  # never use the report's expected/oracle sibling
        expected = {"cleared": original["all_catalog_ownership_cleared"], "roundtrip": True,
                    "owners": original["owners"],
                    "research": [original["after_named"][key] for key in
                                 ("score", "future_count", "labs", "target", "cached_threshold")]}
        if actual != expected:
            raise AssertionError(f"Production mismatch {case['id']}: {actual} != {expected}")
        comparisons.append({"id": case["id"], "actual": actual})
    result = {"report_sha256": REPORT_SHA, "fixture_sha256": FIXTURE_SHA, "alpha_sha256": ALPHA_SHA,
              "executable_sha256": EXE_SHA, "probe_sha256": digest(args.probe), "matched": len(comparisons),
              "scope": "Playable simulation IDs1..7 ownership clear, final turn-zero research/named possession, and production codec roundtrip. Excludes Planet0, isolated turn guards, controller/ordinary availability/private provenance.",
              "comparisons": comparisons}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Matched {len(comparisons)}/107 original reset/grant projections plus exact production roundtrips")


if __name__ == "__main__":
    main()
