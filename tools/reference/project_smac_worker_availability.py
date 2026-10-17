#!/usr/bin/env python3
"""Project literal original unavailable/cleanup outputs; recreate only input schedules."""
import argparse
import hashlib
import json
import random
from pathlib import Path

FIXTURE = "51b646e776efef392aa1485c372a5c3c2a81df351d581f3c9904730b93e3460f"
REPORT = "877b9f9b93004adc047c813a71460281de161c74b596248636a70d98ec9e80c2"
EXECUTABLE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def read(path, pin):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError(f"Fingerprint mismatch: {path}")
    return json.loads(raw)


def schedules(fixture):
    # This is only the retained checker's deterministic input construction.
    # No original emulator, oracle or calculated expected-mask code is imported.
    result = [(row["id"], fixture["defaults"] | row["overrides"]) for row in fixture["cases"]]
    rng = random.Random(0x4FC140)
    for j in range(500):
        cx, cy = rng.choice([[64, 32], [0, 32], [64, 0], [126, 62]])
        wrap = bool(rng.getrandbits(1))
        dx, dy = rng.choice([(4, 0), (6, 0), (8, 0), (-2, 2), (0, 4)])
        bx, by = cx + dx, cy + dy
        if wrap:
            bx %= 128
        bases = [dict(center=[bx, by], owner=rng.choice([1, 2]),
                      selected_indices=rng.sample(range(21), rng.randrange(0, 12)))] if 0 <= bx < 128 and 0 <= by < 64 else []
        value = dict(center=[cx, cy], horizontal_wrap=wrap,
                     unexplored_indices=rng.sample(range(21), rng.randrange(0, 8)),
                     all_map_revealed=bool(rng.getrandbits(1)),
                     sea_indices=rng.sample(range(21), rng.randrange(0, 12)),
                     foreign_claim_indices=rng.sample(range(21), rng.randrange(0, 8)),
                     recognize_foreign_territory=bool(rng.getrandbits(1)),
                     relation=rng.choice(["none", "Pact", "Treaty", "Vendetta"]), other_bases=bases,
                     selected_indices=rng.sample(range(21), rng.randrange(0, 22)))
        result.append((f"WORKER-UNAVAILABLE-GRID-{j+1:04}", fixture["defaults"] | value))
    for j in range(200):
        owner, index = rng.choice([1, 2]), rng.randrange(1, 21)
        value = dict(unexplored_indices=[index] if rng.getrandbits(1) else [],
                     all_map_revealed=bool(rng.getrandbits(1)),
                     foreign_claim_indices=[index] if rng.getrandbits(1) else [],
                     recognize_foreign_territory=bool(rng.getrandbits(1)),
                     relation=rng.choice(["none", "Pact", "Treaty", "Vendetta"]),
                     units=[dict(radius_index=index, owner=owner, visible=bool(rng.getrandbits(1)),
                                 active_convoy=bool(rng.getrandbits(1))) for _ in range(rng.randrange(1, 4))])
        result.append((f"WORKER-UNAVAILABLE-UNIT-GRID-{j+1:04}", fixture["defaults"] | value))
    return result


def flag(value):
    return str(bool(value)).lower()


def array(values):
    return "{ " + ", ".join(map(str, values)) + " }"


def mask(indices):
    if len(indices) != len(set(indices)) or any(not 0 <= i < 32 for i in indices):
        raise ValueError("Invalid mask projection")
    return str(sum(1 << i for i in indices)) + "u"


def project(fixture_path, report_path):
    fixture, report = read(fixture_path, FIXTURE), read(report_path, REPORT)
    if report["executable_sha256"] != EXECUTABLE or report["fixture_sha256"] != FIXTURE:
        raise ValueError("Report provenance differs")
    rows = schedules(fixture)
    if len(rows) != 736 or report["total"] != 736 or report["passed"] != 736 or len(report["results"]) != 736:
        raise ValueError("Original report count differs")
    lines = ["#pragma once", "// Literal actual outputs: tools/reference/project_smac_worker_availability.py.",
             "#include <array>", "#include <cstdint>", "#include <vector>",
             "namespace gse::tests::smac_availability_fixture {",
             "struct Base { std::array<int32_t,2> center; int32_t owner; uint32_t selected; };",
             "struct Unit { int32_t index, owner; bool visible, convoy; };",
             "struct Case { const char* id; std::array<int32_t,2> center; bool wrap; uint32_t unexplored; bool reveal; uint32_t sea, foreign; bool recognized; int32_t relation; std::vector<Base> bases; std::vector<Unit> units; uint32_t selected, unavailable, cleaned; };",
             "inline const Case cases[] = {"]
    for (identifier, value), actual in zip(rows, report["results"]):
        if actual["id"] != identifier or not actual["passed"]:
            raise ValueError("Original report order/result differs")
        if not 0 <= actual["actual_mask"] < 1 << 21:
            raise ValueError("Invalid captured unavailable mask")
        fields = [json.dumps(identifier), array(value["center"]), flag(value["horizontal_wrap"]),
                  mask(value["unexplored_indices"]), flag(value["all_map_revealed"]), mask(value["sea_indices"]),
                  mask(value["foreign_claim_indices"]), flag(value["recognize_foreign_territory"]),
                  str({"none": 0, "Pact": 1, "Treaty": 2, "Vendetta": 3}[value["relation"]]),
                  "{ " + ", ".join("{ " + array(b["center"]) + ", " + str(b["owner"]) + ", " + mask(b["selected_indices"]) + " }" for b in value["other_bases"]) + " }",
                  "{ " + ", ".join("{ " + ", ".join([str(u["radius_index"]), str(u["owner"]), flag(u["visible"]), flag(u["active_convoy"])]) + " }" for u in value["units"]) + " }",
                  mask(value["selected_indices"]), str(actual["actual_mask"]) + "u", mask(actual["retained"])]
        lines.append("\t{ " + ", ".join(fields) + " },")
    return "\n".join(lines + ["};", "}", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("fixture", type=Path)
    parser.add_argument("report", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = project(args.fixture, args.report)
    if args.check:
        if args.output.read_text() != result:
            raise SystemExit("Owned projection differs")
        print("736 actual unavailable masks and 736 original cleanup outputs match the owned fixture")
    else:
        args.output.write_text(result, newline="\n")


if __name__ == "__main__":
    main()
