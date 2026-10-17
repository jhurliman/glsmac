#!/usr/bin/env python3
"""Compare the real C++ producer with pinned original outputs, without importing an oracle."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

BASIC_SHA = "1cf777e554bcdb7bb67fa51c63111764d0771ebbc5cabe9e0daeadd51518fc38"
ABILITY_SHA = "f7e7ba540baecdd4c0affdc4e9ebdea99442e04482ad711d6b0b602396e42235"
ALPHA_SHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"
EXE_SHA = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Evidence fingerprint changed: {path}")
    return json.loads(path.read_text())


def quote(value):
    # std::quoted syntax for the admitted ASCII source keys/field values.
    return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "alpha", "basic-report", "ability-report", "basic-fixtures", "ability-fixtures", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    basic = read(args.basic_report, BASIC_SHA)
    ability = read(args.ability_report, ABILITY_SHA)
    for report in (basic, ability):
        if report["alpha_sha256"] != ALPHA_SHA or report["executable_sha256"] != EXE_SHA or report["passed"] != report["total"]:
            raise ValueError("Original receipt identity or completion changed")
    if digest(args.alpha) != ALPHA_SHA:
        raise ValueError("Installed alpha fingerprint changed")
    basic_fixtures = read(args.basic_fixtures, basic["fixture_sha256"])
    ability_fixtures = read(args.ability_fixtures, ability["fixture_sha256"])

    def run(mode, request):
        result = subprocess.run([str(args.probe.resolve()), mode, str(args.alpha.resolve())],
                                input=request, text=True, capture_output=True, timeout=10)
        if result.returncode:
            raise RuntimeError(f"Probe {mode} rejected input: {result.stderr.strip()}")
        return json.loads(result.stdout)

    def catalog(changes, capacities):
        commands = [f"unit {quote(index)} {field} {quote(value)}"
                    for index, fields in changes.items() for field, value in fields.items()]
        commands += [f"capacity {quote(name)} cargo {quote(value)}" for name, value in capacities.items()]
        rows = run("catalog", str(len(commands)) + "\n" + "\n".join(commands) + "\n")
        if len(rows) != 14:
            raise ValueError("Installed producer did not return fourteen rows")
        return rows

    comparisons = []

    def record(label, row, original):
        # Read only actual executed output, never its expected/oracle sibling.
        args_original = original["builder_arguments"]
        if original["flags"] != 1:
            raise ValueError("Original initial active/unprototyped state changed")
        final = [args_original[0], *original["components"], args_original[4],
                 original["plan"], original["cost"], original["carry"], original["icon"],
                 original["prerequisite"], 1, 0, original["faction_exclusion"]]
        actual = row[:6] + row[9:]
        if actual != final:
            raise AssertionError(f"{label}: final record mismatch {actual} != {final}")
        before = original.get("all_before_override", original.get("before_override"))
        if before and row[6:9] != [before["plan"], before["cost"], before["carry"]]:
            raise AssertionError(f"{label}: builder record mismatch")
        comparisons.append({"id": label, "before_override": bool(before), "passed": True})

    basic_rows = {row["id"]: row for row in basic["results"]}
    for case in basic_fixtures["cases"]:
        rows = catalog(case["rule_changes"], {})
        for i, row in enumerate(rows):
            key = f"{case['id']}-DESIGN-{i}"
            record("basic/" + key, row, basic_rows[key]["actual"])
    basic_count = len(comparisons)
    basic_before = sum(row["before_override"] for row in comparisons)

    ability_rows = {row["id"]: row for row in ability["results"]}
    direct = 0
    for case in ability_fixtures["cases"]:
        inputs, result = case["inputs"], ability_rows[case["id"]]
        if case["kind"] == "full_predefined_loader":
            rows = catalog(inputs["unit_row_changes"], inputs["chassis_capacity_changes"])
            original = {row["id"]: row["actual"] for row in result["private_results"] if "-DESIGN-" in row["id"]}
            for i, row in enumerate(rows):
                key = f"{case['id']}-DESIGN-{i}"
                record("ability/" + key, row, original[key])
        else:
            selected = inputs.get("ability_table_indices", [1] if case["kind"] == "land_cost_flag" else [])
            mask = sum(1 << index for index in selected)
            request = f"{quote(inputs['chassis'])} {inputs['reactor']} {mask} "
            if case["kind"] == "capacity_helper":
                value = run("capacity", request + str(inputs["chassis_capacity"]) + "\n")
            else:
                override = f"1 -2147483648 {int(inputs['land_surcharge_flag'])}" if case["kind"] == "land_cost_flag" else "-1 0 -1"
                value = run("cost", request + f"{quote(inputs['weapon'])} {quote(inputs['armor'])} {override}\n")
            if value != result["actual"]:
                raise AssertionError(f"{case['id']}: helper mismatch {value} != {result['actual']}")
            direct += 1
    ability_before = sum(row["before_override"] for row in comparisons[basic_count:])
    if (basic_count, basic_before, len(comparisons) - basic_count, ability_before, direct) != (140, 30, 406, 406, 254):
        raise ValueError("Unexpected reference coverage")
    report = {"basic_report_sha256": BASIC_SHA, "ability_report_sha256": ABILITY_SHA,
              "alpha_sha256": ALPHA_SHA, "probe_sha256": digest(args.probe),
              "basic_final_records": basic_count, "basic_pre_override": basic_before,
              "ability_final_records": 406, "ability_pre_override": ability_before, "direct_helpers": direct,
              "excluded": "Original eligibility, cost consumers, founding scan, dirty-record replacement and runtime activation.",
              "results": comparisons}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print("Matched 140 basic finals/30 pre-overrides, 406 ability finals/pre-overrides and 254 direct helpers.")


if __name__ == "__main__":
    main()
