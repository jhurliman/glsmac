"""Compare a production C++ probe with retained RE-SMAC-MINERAL-COST-001 outputs.

Reads user-supplied reports only; never runs the research harness or writes its
fixtures. The hash-pinned v1 arithmetic grids reconstruct inputs absent from
individual GRID/ROUND/ADAPT/LOCAL report rows. Original dominance integration
outputs are checked separately with their recorded resolved dominance result.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

FIXTURE_SHA256 = "7da4f8ac592d5878de967ae51e19c9397ea1dd42046ba304064acdee3188fcd0"
REPORT_SHA256 = "db57119677ba60f36d189f1e5c01fd13abd635237b3723cf23d78775707755b2"
EXECUTABLE_SHA256 = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
WORLDS = {"tiny": 0, "small": 1, "standard": 2, "large": 2}
INDUSTRIES = [-10, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 10]
DEFAULT = dict(human=False, global_difficulty=5, leader_human=False,
               network_session=False, dominance=False, world_size="standard",
               mineral_multiplier=10, industry=0)


def load(path, expected):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError(f"Evidence hash mismatch: {path}")
    return json.loads(raw)


def factor_inputs(fixtures):
    cases = [(case["id"], case["inputs"]) for case in fixtures["cases"]]
    grids = [
        ("GRID", ["human", "global_difficulty", "world_size", "industry", "base_context"],
         itertools.product([False, True], range(6), ["tiny", "small", "standard"], INDUSTRIES, [False, True])),
        ("ROUND", ["human", "mineral_multiplier", "world_size", "industry"],
         itertools.product([False, True], [0, 1, 7, 11, 13], WORLDS, [-8, -2, 0, 1, 4, 5])),
        ("ADAPT", ["human", "global_difficulty", "leader_human", "network_session", "dominance"],
         itertools.product([False, True], range(6), [False, True], [False, True], [False, True])),
        ("LOCAL", ["growth", "creche", "golden_age", "base_context", "industry"],
         itertools.product([-10, 0, 10], [False, True], [False, True], [False, True], [-2, 0, 3])),
    ]
    for prefix, keys, rows in grids:
        for values in rows:
            cases.append((f"{prefix}-{len(cases)}", {**DEFAULT, **dict(zip(keys, values))}))
    if len(cases) != 1589:
        raise ValueError("Unexpected arithmetic evidence grid")
    return cases


def wire(inputs):
    return " ".join(map(str, [int(inputs["human"]), inputs["global_difficulty"],
                             int(inputs["leader_human"]), int(inputs["network_session"]),
                             int(inputs["dominance"]), WORLDS[inputs["world_size"]],
                             inputs["mineral_multiplier"], inputs["industry"]]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    fixtures = load(args.fixtures, FIXTURE_SHA256)
    report = load(args.report, REPORT_SHA256)
    if report["executable_sha256"] != EXECUTABLE_SHA256 or report["passed"] != 2669:
        raise ValueError("Unexpected original evidence profile")
    records = {row["id"]: row for row in report["results"]}
    cases = [(name, inputs, records[name]["actual"]) for name, inputs in factor_inputs(fixtures)]
    arithmetic_count = len(cases)
    for row in fixtures["integrations"]:
        for index, human in enumerate([True, False]):
            inputs = {**DEFAULT, **row["inputs"], "human": human, "leader_human": True}
            cases.append((f'{row["id"]}/{"human" if human else "ai"}', inputs,
                          records[row["id"]]["actual_human_ai"][index]))
    result = subprocess.run([str(args.probe.resolve())], input="\n".join(wire(c[1]) for c in cases) + "\n",
                            text=True, capture_output=True, check=True, timeout=60)
    if result.stderr:
        raise ValueError(f"Unexpected probe diagnostics: {result.stderr}")
    outputs = result.stdout.splitlines()
    if len(outputs) != len(cases):
        raise ValueError("Probe output count differs from input count")
    failures = [{"id": name, "expected": expected, "actual": actual}
                for (name, _, expected), actual in zip(cases, outputs) if actual != f"ok {expected}"]
    print(json.dumps({"contract": "RE-SMAC-MINERAL-COST-001/v1", "report_sha256": REPORT_SHA256,
                      "arithmetic_outputs": arithmetic_count, "integrated_factor_outputs": len(cases) - arithmetic_count,
                      "compared": len(cases), "matched": len(cases) - len(failures),
                      "omitted_production_comparisons": 864, "failures": failures[:10]}, indent=2))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
