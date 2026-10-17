#!/usr/bin/env python3
"""Copy pinned original specialist bookkeeping outputs; no expected-result model."""
import argparse
import hashlib
import json
from pathlib import Path

PINS = {
    "existing-specialist-cleanup-cases.json": "df13b06445ad7aca682f8add174ca60f0a452ee8b19030853a22b26d4cbc39b5",
    "specialist-tile-competition-cases.json": "5b01f88275c599e1d926e10203c74c3cc44d3756adc65a399735f21917f2d076",
    "specialist-competition-finalization-cases.json": "40314a624b6e37fab59274eb7c00db16c6fa18450342976412805eb15a5689fb",
}
NAMES = ["Technician", "Doctor", "Librarian", "Engineer", "Empath", "Thinker", "Transcend"]
OUTPUTS = [[3, 0, 0], [0, 2, 0], [0, 0, 3], [3, 0, 2], [2, 2, 0], [0, 1, 3], [2, 2, 4]]


def cpp(v):
    if isinstance(v, bool):
        return str(v).lower()
    if isinstance(v, str):
        return json.dumps(v)
    if isinstance(v, (list, tuple)):
        return "{ " + ", ".join(map(cpp, v)) + " }"
    return str(v)


def types(names):
    return [NAMES.index(name) for name in names]


def catalog(minimum=5, available=None, obsolete=None, overrides=None, psych=None):
    rows = []
    for i, name in enumerate(NAMES):
        output = list((overrides or {}).get(name, OUTPUTS[i]))
        if psych and name in psych:
            output[1] = psych[name]
        rows.append(output + [(available or [True] * 7)[i], (obsolete or [False] * 7)[i]])
    return "{ " + str(minimum) + ", { " + cpp(rows) + " } }"


def project(directory):
    sources = {}
    for name, pin in PINS.items():
        data = (directory / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != pin:
            raise ValueError("source fingerprint mismatch: " + name)
        sources[name] = json.loads(data)
    out = ["#pragma once", "// Literal projections: tools/reference/project_smac_specialist_allocation.py.",
           '#include "game/backend/base/SmacSpecialistAllocation.h"',
           "namespace gse::tests::smac_specialist_allocation_fixture {", "using namespace game::backend::base;",
           "struct Cleanup { const char* id; SmacSpecialistSlots input; SmacSpecialistAllocationCatalog catalog; bool managed, upkeep; uint8_t overflow; int32_t split_r, split_q, final_r, final_q; std::array<uint8_t,16> prefix; size_t prefix_count; };",
           "inline const Cleanup cleanup[] = {"]
    source = sources["existing-specialist-cleanup-cases.json"]
    assert len(source["cases"]) == 17
    for case in source["cases"]:
        v, e = source["defaults"] | case["overrides"], case["expected"]
        initial = types(v["stored_types"])
        initial += [NAMES.index(v["unused_stored_type"])] * (16 - len(initial))
        prefix = types(e["active_stored_types_after"])
        out.append("\t{ " + ", ".join([cpp(case["id"]), cpp([initial, v["specialist_count"]]), catalog(psych=v["psych_overrides"]),
            cpp(v["managed_allocation"]), cpp(v["upkeep_context"]), str(NAMES.index(v["automatic_overflow_type"])),
            str(e["counts_after_split"]["retained_count"]), str(e["counts_after_split"]["provisional_count"]),
            str(e["retained_count_after"]), str(e["provisional_count_after"]), cpp(prefix), str(len(prefix))]) + " },")
    out += ["};", "struct Competition { const char* id; SmacSpecialistAllocationCatalog catalog; SmacSpecialistCompetitionContext context; SmacSpecialistBookkeeping input; SmacWorkerStepState workers; int32_t winner, score; bool inserted, rescan; int32_t final_q; std::array<uint8_t,16> expected; };", "inline const Competition competition[] = {"]
    source = sources["specialist-tile-competition-cases.json"]
    assert len(source["cases"]) == 28 and source["stock_outputs_economy_psych_labs"] == OUTPUTS
    for case in source["cases"]:
        v, e = source["defaults"] | case["overrides"], case["expected"]
        context = [v[k] for k in ["managed_allocation", "population", "turn", "nutrients_per_citizen", "current_nutrients", "current_energy", "new_specialists", "maximum_candidate_nutrients", "economy_priority", "labs_priority", "prior_tile_score"]]
        out.append("\t{ " + ", ".join([cpp(case["id"]), catalog(v["minimum_specialist_size"], v["available"], v["obsolete"], v["output_overrides"]),
            cpp(context), cpp([[3] * 16, v["retained_specialists"], v["new_specialists"]]), cpp([0, [v["current_nutrients"], 0, v["current_energy"]], v["remaining_slots"], False]),
            str(-1 if e["chosen_specialist"] is None else NAMES.index(e["chosen_specialist"])), str(e["best_score"]), cpp(e["inserted"]), cpp(e["next_boundary"] == "rescan"),
            str(e["new_specialists_after"]), cpp(types(e["stored_types_after"]))]) + " },")
    out += ["};", "struct Finalization { const char* id; int32_t population, turn, retained, nutrients; bool managed; int32_t insertions, before_r, before_q, final_count; std::array<uint8_t,16> prefix; size_t prefix_count; };", "inline const Finalization finalization[] = {"]
    source = sources["specialist-competition-finalization-cases.json"]
    assert len(source["cases"]) == 7
    for case in source["cases"]:
        v, e = source["defaults"] | case["overrides"], case["expected"]
        prefix = types(e["stored_types_after"])
        out.append("\t{ " + ", ".join(map(cpp, [case["id"], v["population"], v["turn"], v["retained_doctors"], v["center_nutrients"], v["managed_allocation"],
            e["provisional_insertion_count"], e["retained_before_fill"], e["provisional_before_fill"], e["final_specialist_count"], prefix, len(prefix)])) + " },")
    return "\n".join(out + ["};", "}", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    target = Path(__file__).resolve().parents[2] / "src/gse/tests/fixtures/SmacSpecialistAllocation.h"
    result = project(args.evidence)
    if args.check:
        if target.read_text() != result:
            raise SystemExit("owned fixture differs from pinned projection")
    else:
        target.write_text(result, newline="\n")
    print("17 cleanup + 28 competition + 7 finalization projections verified")


if __name__ == "__main__":
    main()
