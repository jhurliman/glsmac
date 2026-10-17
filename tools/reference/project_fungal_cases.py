#!/usr/bin/env python3
"""Project pinned original semantic outputs; does not execute or model the original."""
import argparse
import hashlib
import json
from pathlib import Path

FUNGAL_SHA = "b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e"
WORKER_SHA = "84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab"
EXCLUDED = {"FUNGAL-009", "FUNGAL-011", "FUNGAL-014", "FUNGAL-023", "FUNGAL-024"}
PORT_IDS = dict(gaians="GAIANS", hive="HIVE", univ="UNIVERSITY", morgan="MORGANITES",
                spartans="SPARTANS", believe="BELIEVERS", peace="PEACEKEEPERS")


def read(path, expected):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError(f"fixture fingerprint mismatch: {path}")
    return json.loads(raw)


def array(values):
    return "{ " + ", ".join(str(x).lower() for x in values) + " }"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fungal", type=Path, required=True)
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    fungal = read(args.fungal, FUNGAL_SHA)
    worker = read(args.worker, WORKER_SHA)
    lines = ["#pragma once", "", "#include <array>", "#include <cstdint>", "#include <vector>", "",
             "// Generated from pinned original outputs by tools/reference/project_fungal_cases.py.",
             f"// FUNGAL SHA256 {FUNGAL_SHA}", f"// FIRST-WORKER SHA256 {WORKER_SHA}",
             "// Excluded: 009/011/023/024 require unadmitted mineral/energy faction tags; 014 is native faction0.",
             "namespace gse::tests::smac_fungal_fixture {",
             "struct Technology { int32_t id; uint16_t flags; bool owned; std::array< int32_t, 2 > prerequisites; };",
             "struct Modifier { bool nutrient; int32_t amount; };",
             "struct Case { const char* id; int32_t simulation; std::vector< Technology > technologies; std::vector< Modifier > modifiers;",
             "\tint32_t planet, economy; bool increase, decrease, dust, solar; std::array< int32_t, 3 > capabilities, yields; };",
             "inline const std::array< Case, 20 >& Cases() {", "\tstatic const std::array< Case, 20 > values = {{"]
    count = 0
    for case in fungal["cases"]:
        if case["id"] in EXCLUDED:
            continue
        p = case["inputs"]
        assert p["faction"] in range(1, 8)
        assert all(m["resource"] in ("nutrients", "unrelated") for m in p["faction_modifiers"])
        technologies = ["{ " + f'{t["index"]}, {sum(bit for bit, present in zip((256, 128, 64), t["fungal_flags"]) if present)}, {str(t["owned"]).lower()}, ' + array(t["prerequisites"]) + " }" for t in p["technologies"]]
        modifiers = ["{ " + f'{str(m["resource"] == "nutrients").lower()}, {m["amount"]}' + " }" for m in p["faction_modifiers"]]
        flags = ", ".join(str(p[k]).lower() for k in ("increase_events", "decrease_events", "atmospheric_dust", "solar_event"))
        lines.append(f'\t\t{{ "{case["id"]}", {p["faction"]}, {{ ' + ", ".join(technologies) + " }, { " + ", ".join(modifiers)
                     + f' }}, {p["planet"]}, {p["economy"]}, {flags}, ' + array(case["expected_capabilities"]) + ", " + array(case["expected_tile_yields"]) + " },")
        count += 1
    assert count == 20 and len(fungal["cases"]) == 25
    lines += ["\t}}; return values;", "}", "struct Worker { const char* faction; std::vector< int32_t > owned; std::array< int32_t, 3 > capabilities; };",
              "inline const std::array< Worker, 7 >& Workers() {", "\tstatic const std::array< Worker, 7 > values = {{"]
    for case in worker["cases"]:
        assert case["inputs"]["human_slot"] == 1 and case["expected"]["native_upkeep_builder_reached"]
        lines.append(f'\t\t{{ "{PORT_IDS[case["inputs"]["faction_file"]]}", ' + array(case["expected"]["owned_technology_ids"]) + ", " + array(case["expected"]["fungal_capabilities"]) + " },")
    assert len(worker["cases"]) == 7
    lines += ["\t}}; return values;", "}", "}", ""]
    args.output.write_text("\n".join(lines), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
