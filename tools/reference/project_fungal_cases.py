#!/usr/bin/env python3
"""Project pinned original semantic outputs; does not execute or model the original."""
import argparse
import hashlib
import json
from pathlib import Path

FUNGAL_SHA = "b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e"
WORKER_SHA = "84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab"
PARSER_SHA = "7387c3656e1decedabf8a14c7e72d19f27289df404595d62bf7e46b6751b023a"
EXCLUDED = {"FUNGAL-014"}
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
    parser.add_argument("--parser", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    fungal = read(args.fungal, FUNGAL_SHA)
    worker = read(args.worker, WORKER_SHA)
    parsed = read(args.parser, PARSER_SHA)
    lines = ["#pragma once", "", "#include <array>", "#include <cstdint>", "#include <utility>", "#include <vector>", "",
             "// Generated from pinned original outputs by tools/reference/project_fungal_cases.py.",
             f"// FUNGAL SHA256 {FUNGAL_SHA}", f"// FIRST-WORKER SHA256 {WORKER_SHA}",
             f"// FUNGAL-PARSER SHA256 {PARSER_SHA}", "// Excluded: FUNGAL-014 is native faction0.",
             "namespace gse::tests::smac_fungal_fixture {",
             "struct Technology { int32_t id; uint16_t flags; bool owned; std::array< int32_t, 2 > prerequisites; };",
             "struct Modifier { int32_t resource, amount; };",
             "struct Case { const char* id; int32_t simulation; std::vector< Technology > technologies; std::vector< Modifier > modifiers;",
             "\tint32_t planet, economy; bool increase, decrease, dust, solar; std::array< int32_t, 3 > capabilities, yields; };",
             "inline const std::array< Case, 24 >& Cases() {", "\tstatic const std::array< Case, 24 > values = {{"]
    count = 0
    for case in fungal["cases"]:
        if case["id"] in EXCLUDED:
            continue
        p = case["inputs"]
        assert p["faction"] in range(1, 8)
        technologies = ["{ " + f'{t["index"]}, {sum(bit for bit, present in zip((256, 128, 64), t["fungal_flags"]) if present)}, {str(t["owned"]).lower()}, ' + array(t["prerequisites"]) + " }" for t in p["technologies"]]
        modifiers = ["{ " + f'{("nutrients", "minerals", "energy", "unrelated").index(m["resource"])}, {m["amount"]}' + " }" for m in p["faction_modifiers"]]
        flags = ", ".join(str(p[k]).lower() for k in ("increase_events", "decrease_events", "atmospheric_dust", "solar_event"))
        lines.append(f'\t\t{{ "{case["id"]}", {p["faction"]}, {{ ' + ", ".join(technologies) + " }, { " + ", ".join(modifiers)
                     + f' }}, {p["planet"]}, {p["economy"]}, {flags}, ' + array(case["expected_capabilities"]) + ", " + array(case["expected_tile_yields"]) + " },")
        count += 1
    assert count == 24 and len(fungal["cases"]) == 25
    lines += ["\t}}; return values;", "}", "struct Worker { const char* faction; std::vector< int32_t > owned; std::array< int32_t, 3 > capabilities; };",
              "inline const std::array< Worker, 7 >& Workers() {", "\tstatic const std::array< Worker, 7 > values = {{"]
    for case in worker["cases"]:
        assert case["inputs"]["human_slot"] == 1 and case["expected"]["native_upkeep_builder_reached"]
        lines.append(f'\t\t{{ "{PORT_IDS[case["inputs"]["faction_file"]]}", ' + array(case["expected"]["owned_technology_ids"]) + ", " + array(case["expected"]["fungal_capabilities"]) + " },")
    assert len(worker["cases"]) == 7
    lines += ["\t}}; return values;", "}",
              "struct Record { int32_t tag, parameter, amount; };",
              "struct ParserCase { const char* id; std::vector< std::pair< const char*, const char* > > pairs; std::vector< Record > records; int32_t extra; std::array< int32_t, 3 > capabilities, yields; };",
              "inline const std::array< ParserCase, 12 >& ParserCases() {", "\tstatic const std::array< ParserCase, 12 > values = {{"]
    for case in parsed["cases"]:
        assert case["inputs"]["initial_technology_ownership"] == "none" and case["inputs"]["current_planet"] == 0
        pairs = ["{ " + json.dumps(k) + ", " + json.dumps(v) + " }" for k, v in case["inputs"]["bonus_pairs"]]
        records = []
        for record in case["expected"]["records"]:
            tag = {"TECH": 0, "SOCIAL": 2, "FUNGNUTRIENT": 5, "FUNGMINERALS": 14, "FUNGENERGY": 15}[record["kind"]]
            parameter = record["technology_id"] if tag == 0 else {"PLANET": 7, "ECONOMY": 0}[record["rating"]] if tag == 2 else record["amount"]
            records.append(array([tag, parameter, record["amount"] if tag == 2 else 0]))
        assert len(records) == case["expected"]["record_count"]
        lines.append(f'\t\t{{ "{case["id"]}", {{ ' + ", ".join(pairs) + " }, { " + ", ".join(records) + f' }}, {case["expected"]["extra_technology_count"]}, ' + array(case["expected"]["capabilities"]) + ", " + array(case["expected"]["fungal_tile_yields"]) + " },")
    assert len(parsed["cases"]) == 12
    lines += ["\t}}; return values;", "}", "}", ""]
    args.output.write_text("\n".join(lines), encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
