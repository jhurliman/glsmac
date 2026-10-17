#!/usr/bin/env python3
"""Compare production C++ projections with retained original-x86 observations.

Reads hash-pinned private evidence; never imports or executes its research harness.
Counters, off-view propagation and complete chooser/queue behavior are excluded.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

FIXTURE_SHA = "1e824953c44c89db6509025bda4cef2cd2591588fd1980fc83e942aa8ba9bd96"
REPORT_SHA = "2c9f31239dcd528c9fe56ff3a6e0d340e8c671d1e81464891516909e93a62a73"
HARNESS_SHA = "fc3c9d36865ab24cbced1b4027ee78bc8f9287e79d9a62090536dbddd7047eb6"
EXE_SHA = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def pinned(path, expected):
    data = Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f"evidence hash mismatch: {path}")
    return data


def scenarios(fixtures):
    default = dict(fixtures[0]["inputs"])
    result = [(case["id"], case["inputs"]) for case in fixtures]
    items = (0, 1, -3, -4, -34, -38, -39, -40)

    def add(label, **changes):
        result.append((f"{label}-{len(result)}", {**default, **changes}))

    # Input schedules from the pinned experiment, not an arithmetic oracle.
    for strict, human, baseline, target, minerals in itertools.product(range(4), (False, True), items, items, (0, 10, 11, 31)):
        add("AUTO", strictness=strict, human=human, baseline_item=baseline, current_item=baseline,
            targets=[target], baseline_minerals=minerals, current_minerals=minerals)
    for strict, difficulty, baseline, target, minerals in itertools.product(range(4), (1, 2), items, items, (0, 10, 11, 31)):
        add("MANUAL", mode="manual", strictness=strict, difficulty=difficulty, baseline_item=baseline,
            current_item=baseline, targets=[target], baseline_minerals=minerals, current_minerals=minerals)
    for mode, strict, current, baseline, target in itertools.product(("automatic", "manual"), range(4), (-3, 0, -39), (-4, 0, -40), (-3, 1, -41)):
        add("PRESENT", mode=mode, strictness=strict, current_item=current, baseline_item=baseline,
            targets=[target], facilities=[3, 10] if strict == 1 else [3])
    for mode, percent, exemption, baseline in itertools.product(("automatic", "manual"), (0, 1, 25, 50, 75, 99, 100), (0, 10, 20), (0, 1, 10, 11, 21, 31)):
        add("PERCENT", mode=mode, loss_percent=percent, exemption=exemption,
            baseline_minerals=baseline, current_minerals=baseline)
    return result


def request(config, target, state):
    item, minerals, baseline, baseline_minerals = state
    facilities = config["facilities"]
    fields = [int(config["mode"] == "manual"), int(config["human"]), config["difficulty"],
              config["strictness"], config["loss_percent"], config["exemption"], baseline,
              baseline_minerals, item, minerals, int(target == "keep_current"),
              0 if target == "keep_current" else target, int(config["accept_penalty"]),
              int(10 in facilities), len(facilities), *facilities]
    return " ".join(map(str, fields))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--harness", type=Path, required=True, help="hash verification only; never executed")
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    fixture = json.loads(pinned(args.fixtures, FIXTURE_SHA))
    report = json.loads(pinned(args.report, REPORT_SHA))
    pinned(args.harness, HARNESS_SHA)
    if report["executable_sha256"] != EXE_SHA or report["fixture_sha256"] != FIXTURE_SHA or report["total"] != 4594 or report["passed"] != 4594:
        raise ValueError("unexpected evidence identity or completeness")
    configs = scenarios(fixture["cases"])
    if len(configs) != len(report["results"]):
        raise ValueError("scenario inventory mismatch")
    states = [(c["current_item"], c["current_minerals"], c["baseline_item"], c["baseline_minerals"]) for _, c in configs]
    steps = 0
    # Advance each multi-step sequence using actual C++ output, never expected state.
    for index in range(max(len(c["targets"]) for _, c in configs)):
        selected = [i for i, (_, c) in enumerate(configs) if index < len(c["targets"])]
        payload = "\n".join(request(configs[i][1], configs[i][1]["targets"][index], states[i]) for i in selected) + "\n"
        process = subprocess.run([str(args.probe.resolve())], input=payload, text=True, capture_output=True, timeout=60, check=True)
        lines = process.stdout.splitlines()
        if process.stderr or len(lines) != len(selected):
            raise ValueError("unexpected probe output or diagnostics")
        for i, line in zip(selected, lines):
            label, config = configs[i]
            observation = report["results"][i]
            if observation["id"] != label or not observation["passed"] or len(observation["actual"]) != len(config["targets"]):
                raise ValueError(f"scenario identity mismatch: {label}")
            parts = line.split()
            if len(parts) != 9 or parts[0] != "ok":
                raise ValueError(f"{label} step {index}: rejected or malformed output {line!r}")
            actual = tuple(map(int, parts[1:]))
            row = observation["actual"][index]
            expected = (row["item"], row["minerals"], row["baseline_item"], row["baseline_minerals"], row["reported_loss"],
                        int(row.get("refresh_requested", False)), int(row.get("penalty_prompt_required", False)), int(row.get("penalty_cancelled", False)))
            if actual != expected:
                raise ValueError(f"{label} step {index}: C++ {actual} != original {expected}")
            states[i] = actual[:4]
            steps += 1
    print(f"Matched {len(configs)} scenarios / {steps} steps: state, decision and baseline-loss projections; counters/propagation excluded.")


if __name__ == "__main__":
    main()
