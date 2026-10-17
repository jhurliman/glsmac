#!/usr/bin/env python3
"""Project supplied original energy inputs/results, without an expected model."""
import argparse
import hashlib
import json
from pathlib import Path

PINS = {
    "energy-waste-and-headquarters-distance-cases.json": "e3824dc307f12edb3117e655167a77f61a22d1bbb0e0c75b70b39a3476f87c86",
    "psych-allocation-and-specialist-output-cases.json": "4e69ebcf121d085a39948b91cbac89620c645c6c423646c773696a757435e4c6",
    "energy-multipliers-and-allocation-penalty-cases.json": "d5a356cc87b1d0cf984c18b812c203d6d8a694e458a1eb43b8d59e190ca3e2ae",
}
FACILITIES = {"energy_bank": 7, "network_node": 8, "biology_lab": 9, "hologram_theatre": 10,
              "tree_farm": 13, "hybrid_forest": 14, "fusion_lab": 15, "quantum_lab": 16,
              "research_hospital": 17, "nanohospital": 18}

def cpp(value):
    if isinstance(value, bool): return str(value).lower()
    if isinstance(value, str): return json.dumps(value)
    if isinstance(value, (tuple, list)): return "{ " + ", ".join(map(cpp, value)) + " }"
    return str(value)

def categories(value): return [value[k] for k in ("economy", "psych", "labs")]
def facilities(value): return "{ " + cpp([i in [FACILITIES[n] for n in value] for i in range(34)]) + " }"

def project(directory):
    sources = []
    for name, pin in PINS.items():
        raw = (directory / name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != pin: raise ValueError("source fingerprint mismatch: " + name)
        sources.append(json.loads(raw))
    out = ["#pragma once", "// Literal original projections: tools/reference/project_smac_base_energy.py.",
           '#include "game/backend/base/SmacBaseEnergy.h"', "namespace gse::tests::smac_energy_fixture {",
           "using namespace game::backend::base;",
           "struct Waste { const char* id; int32_t width; bool wrap; SmacEnergyCoordinate base; std::vector<SmacEnergyCoordinate> hqs; int32_t energy, commerce, efficiency; bool creche; int32_t distance, waste_before, net_before, waste_after, net_after; };",
           "inline const Waste waste[] = {"]
    for row in sources[0]["cases"]:
        v = sources[0]["defaults"] | row["overrides"]; e = row["expected"]
        # Owner/queued filtering is caller admission, not part of the distance helper.
        hqs = [[x["x"], x["y"]] for x in v["headquarters"] if x.get("owner", 1) == 1]
        if v["current_headquarters"]: hqs.append([v["base_x"], v["base_y"]])
        out.append(cpp([row["id"], v["map_width"], v["horizontal_wrap"], [v["base_x"], v["base_y"]], hqs,
                        v["gross_energy"] + v["incoming_energy"] - v["outgoing_energy"], v["commerce_added"], v["efficiency"], v["creche"],
                        e["distances_evaluated"][0] if e["distances_evaluated"] else -1, e["waste_before_commerce"], e["net_before_commerce"], e["waste_after_commerce"], e["net_after_commerce"]]) + ",")
    out += ["};", "struct Allocation { const char* id; int32_t energy; SmacEnergyRates rates; std::vector<SmacEnergyCategories> specialists; SmacEnergyAllocation expected; std::array<bool,34> facilities; bool virtual_world; int32_t quarters, psych; };", "inline const Allocation allocation[] = {"]
    for row in sources[1]["cases"]:
        v = sources[1]["defaults"] | row["overrides"]; e = row["expected"]
        outputs = [categories(sources[1]["specialist_outputs"][name]) for name in v["specialists"]]
        prefix = cpp([row["id"], v["net_energy"], [v["psych_tenths"], v["labs_tenths"]], outputs,
                      [categories(e["allocation"]), categories(e["after_specialists"])]])[2:-2]
        out.append("{ " + prefix + ", " + facilities(v["facilities"]) + ", " + cpp(v["virtual_world"] in ("self_here", "self_elsewhere", "self")) + ", " + cpp(e["psych_multiplier_quarters"]) + ", " + cpp(e["final_psych"]) + " },")
    out += ["};", "struct Tail { const char* id; SmacEnergyTailInputs input; SmacEnergyTail expected; };", "inline const Tail tail[] = {"]
    for row in sources[2]["cases"]:
        v = sources[2]["defaults"] | row["overrides"]; e = row["expected"]
        commerce = [0 if v["current_commerce_blocked"] or (i > 0 and v["partner_commerce_blocked"][i-1]) else n for i, n in enumerate(v["incoming_commerce"])]
        fields = [cpp([v["economy_input"], v["psych_input"], v["labs_input"]]), cpp([v["psych_tenths"], v["labs_tenths"]]), cpp(v["efficiency"]), cpp(v["flat_faction_labs_adjustment"]), facilities(v["facilities"]),
                  cpp(v["virtual_world"] in ("self_here", "self_elsewhere", "self"))]
        fields += [cpp(v[k] == "self_here") for k in ("supercollider", "theory_of_everything", "space_elevator", "longevity_vaccine")]
        fields += [cpp(v["economics"] == "free_market"), cpp(v["network_backbone"] == "self_here"), cpp(v["other_network_nodes"] + ("network_node" in v["facilities"])), "{ " + cpp(commerce) + " }",
                   cpp(v["current_stockpile"]), cpp(v["previous_stockpile"]), cpp(v["mineral_surplus"])]
        out.append("{ " + cpp(row["id"]) + ", { " + ", ".join(fields) + " }, " + cpp([categories(e[k]) for k in ("multiplier_quarters", "before_allocation_penalty", "after_allocation_penalty", "final")]) + " },")
    return "\n".join(out + ["};", "}", ""])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(); parser.add_argument("input", type=Path); parser.add_argument("output", type=Path)
    args = parser.parse_args(); args.output.write_text(project(args.input), encoding="utf-8", newline="\n")
