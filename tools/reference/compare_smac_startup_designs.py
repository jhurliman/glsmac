#!/usr/bin/env python3
"""Compare the production startup-design stages with literal original outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

GATE_REPORT = "1d9c042254a5936e7dc8ba05f949140f25c4330f908f797f4fa42fa43124931a"
GATE_FIXTURE = "235404b572df4111f49e6600ccc32a049b40a5056169125ab9e3a5553d4a304a"
FIRST_REPORT = "cb4ab8c026f4f57ec2aa3bc1c4647ef40c6f5752c98119af9a1a5e19eda86dc5"
FIRST_FIXTURE = "d7d7b1aae73dda634850449b44fb53261656280a1c0f3f433b45c199913147e9"
ALPHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
PROFILES = {"gaians": "GAIANS", "hive": "HIVE", "univ": "UNIVERSITY", "morgan": "MORGANITES",
            "spartans": "SPARTANS", "believe": "BELIEVERS", "peace": "PEACEKEEPERS"}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Evidence fingerprint changed: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def installed(directory, name):
    matches = [p for p in directory.iterdir() if p.name.lower() == name.lower()]
    if len(matches) != 1:
        raise ValueError(f"Missing or ambiguous installed source: {name}")
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "installed", "gate-report", "gate-fixtures", "first-report", "first-fixtures", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    gate = read(args.gate_report, GATE_REPORT)
    gate_input = read(args.gate_fixtures, GATE_FIXTURE)
    first = read(args.first_report, FIRST_REPORT)
    first_input = read(args.first_fixtures, FIRST_FIXTURE)
    if digest(installed(args.installed, "alpha.txt")) != ALPHA:
        raise ValueError("Installed alpha identity differs")
    for report, fixture, count in ((gate, GATE_FIXTURE, 1176), (first, FIRST_FIXTURE, 9)):
        if (report["executable_sha256"] != EXE or report["alpha_sha256"] != ALPHA
                or report["fixture_sha256"] != fixture or report["total"] != count or report["passed"] != count):
            raise ValueError("Original report identity or completed count differs")
    selected = []
    if [c["id"] for c in gate_input["cases"]] != [r["id"] for r in gate["results"]]:
        raise ValueError("Original gate rows differ from ordered input")
    for case, row in zip(gate_input["cases"], gate["results"]):
        inputs = case["inputs"]
        if not inputs["actor_human"]:
            continue  # 168 AI controls do not define the production admission.
        if inputs["alternate_multiplayer_service"] or inputs["preexisting_custom_designs"] or not row["passed"]:
            raise ValueError("Unsupported generator context")
        request = [PROFILES[inputs["profile"]], inputs["owner"], inputs["global_difficulty_index"],
                   inputs["stored_difficulty_index"], int(inputs["network_session_active"])]
        selected.append((case["id"], request, row["actual"]))
    if len(selected) != 1008:
        raise ValueError("Supported human gate count changed")
    if [c["id"] for c in first_input["cases"]] != [r["id"] for r in first["results"]]:
        raise ValueError("Original first-base rows differ from ordered input")
    for case, row in zip(first_input["cases"], first["results"]):
        inputs = case["inputs"]
        if inputs["global_turn"] != 0 or inputs["global_difficulty"] != 3 or not row["passed"]:
            raise ValueError("Unexpected baseline context")
        actual = row["actual"]["first_base_startup"]
        projection = {"before": actual["generated_before_mark"], "after": actual["generated_after_mark"], "selected": actual["chosen_scout"]}
        selected.append((case["id"], [PROFILES[inputs["faction_file"]], 1, 3, 3, 0], projection))
    request = "".join(" ".join(map(str, values)) + "\n" for _, values, _ in selected)
    process = subprocess.run([str(args.probe.resolve()), str(args.installed.resolve())], input=request,
                             text=True, capture_output=True, timeout=120)
    if process.returncode:
        raise RuntimeError(f"Production rejected comparison stream: {process.stderr}")
    actual_rows = [json.loads(line) for line in process.stdout.splitlines()]
    if len(actual_rows) != len(selected):
        raise ValueError("Production returned a different row count")
    comparisons = []
    for (identity, _, original), actual in zip(selected, actual_rows):
        if actual["roundtrip"] is not True or actual["selected"] != original["selected"]:
            raise AssertionError(f"Selection/roundtrip differs: {identity}")
        for stage in ("before", "after"):
            if len(actual[stage]) != len(original[stage]):
                raise AssertionError(f"Generated count differs: {identity}/{stage}")
            for produced, captured in zip(actual[stage], original[stage]):
                # The earlier baseline omits carry; compare only captured fields.
                if {key: produced[key] for key in captured} != captured:
                    raise AssertionError(f"Original record differs: {identity}/{stage}")
        comparisons.append({"id": identity, "actual": actual})
    result = {"gate_report_sha256": GATE_REPORT, "gate_fixture_sha256": GATE_FIXTURE,
              "first_report_sha256": FIRST_REPORT, "first_fixture_sha256": FIRST_FIXTURE,
              "original_executable_sha256": EXE, "probe_sha256": digest(args.probe),
              "installed_sources": {name: digest(installed(args.installed, name)) for name in
                  ("alpha.txt", "gaians.txt", "hive.txt", "univ.txt", "morgan.txt", "spartans.txt", "believe.txt", "peace.txt")},
              "matched": len(comparisons), "human_gate": 1008, "baseline_projections": 9,
              "scope": "Human empty-block Generate/Mark/Scout selection only; no AI, founding, placement, earlier network grants or live units.",
              "comparisons": comparisons}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Matched 1008 human gate + 9 baseline original projections; 1017 exact producer codec roundtrips")


if __name__ == "__main__":
    main()
