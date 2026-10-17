#!/usr/bin/env python3
"""Copy pinned original collection outputs into the owned native fixture."""
import argparse
import hashlib
import json
from pathlib import Path

PINS = {
    "worked-square-collection-cases.json": "0ea35cb1492dcf611cc88e0c5689a3a699053d12bbf7a1074db8021ac3eb29da",
    "base-square-intake-updates-cases.json": "7764998ce1f9da001783a8bb202262a8a1d83e46fbc3bb0f7ee2664beefd4ff3",
    "worker-intake-integration-cases.json": "823718431f75ccddd028cf4fd29b3c17de86a76f0a5f0bf03bfff606d925febc",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def array(values):
    return "{ " + ", ".join(map(str, values)) + " }"


def mask(indices):
    if len(indices) != len(set(indices)) or any(not 0 <= i < 32 for i in indices):
        raise ValueError("invalid literal mask")
    return str(sum(1 << i for i in indices)) + "u"


def flag(value):
    return str(value).lower()


def project(directory):
    sources = {}
    for name, pin in PINS.items():
        raw = (directory / name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != pin:
            raise ValueError(f"source fingerprint mismatch: {name}")
        sources[name] = json.loads(raw)
    lines = ["#pragma once", "// Literal projections: tools/reference/project_smac_worked_collection.py.",
             "#include <array>", "#include <cstddef>", "#include <cstdint>", "namespace gse::tests::smac_worked_fixture {",
             "struct CollectionCase { const char* id; int32_t population; uint32_t selected, unavailable; std::array<int32_t,2> center; bool wrap; std::array<int32_t,3> seed; uint32_t sea_indices, cleaned; bool repair; std::array<int32_t,3> intake; int32_t remaining; uint32_t visited; std::array<std::array<int32_t,2>,21> coordinates; std::size_t coordinate_count; bool sea; };",
             "inline const CollectionCase collection[] = {"]
    source = sources["worked-square-collection-cases.json"]
    for case in source["cases"]:
        value, expected = source["defaults"] | case["overrides"], case["expected"]
        fields = [json.dumps(case["id"]), str(value["population"]), mask(value["selected_indices"]),
                  mask(value["unavailable_indices"]), array(value["center"]), flag(value["horizontal_wrap"]),
                  array(value["seed_intake"]), mask(value["sea_indices"]), mask(expected["cleaned_indices"]),
                  flag(expected["allocation_repair_required"]), array(expected["intake"]),
                  str(expected["remaining_collection_slots"]), mask(expected["visited_indices"]),
                  "{{ " + ", ".join(array(p) for p in expected["collected_squares"]) + " }}",
                  str(len(expected["collected_squares"])), flag(expected["sea_participation"])]
        lines.append("\t{ " + ", ".join(fields) + " },")
    lines += ["};", "struct WrapCase { const char* id; bool selected, sea; std::array<int32_t,3> seed, yield, intake; int32_t remaining; };",
              "inline const WrapCase wrapping[] = {"]
    count = 0
    for case in sources["base-square-intake-updates-cases.json"]["cases"]:
        if case["path"] != "collect":
            continue
        expected = case["expected"]
        require(expected["return_value"] is None and expected["fourth_intake"] == 123, "unsupported collection side effects")
        lines.append("\t{ " + ", ".join([json.dumps(case["id"]), flag(case["center_worker_bit"]),
                     flag(not case["land"]), array(case["seed_intake"]), array(case["expected_yield"]),
                     array(expected["intake"]), str(expected["remaining_slots"])]) + " },")
        count += 1
    require(count == 168, "unexpected collection boundary count")
    lines += ["};", "struct CompleteCase { const char* id; int32_t population, specialists; uint32_t selected; std::array<int32_t,3> old_yield, new_yield, intake; uint32_t visited; };",
              "inline const CompleteCase complete[] = {"]
    source = sources["worker-intake-integration-cases.json"]
    retained = {"WORKER-INTAKE-002", "WORKER-INTAKE-007", "WORKER-INTAKE-009"}
    for case in source["cases"]:
        if case["id"] not in retained:
            continue
        value, expected = source["defaults"] | case["overrides"], case["expected"]
        require(expected["specialists"] == value["retained_specialists"], "early completion changed specialists")
        lines.append("\t{ " + ", ".join([json.dumps(case["id"]), str(value["population"]),
                     str(value["retained_specialists"]), mask(value["selected_indices"]), array(value["old_yield"]),
                     array(value["new_yield"]), array(expected["intake"]), mask(expected["visited_indices"])]) + " },")
    return "\n".join(lines + ["};", "}", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="read-only behavior fixture directory")
    parser.add_argument("output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result = project(args.source)
    if args.check:
        if args.output.read_text() != result:
            raise SystemExit("owned projection differs")
        print("19 collection, 168 collection-wrap and 3 early-completion projections match")
    else:
        args.output.write_text(result, newline="\n")


if __name__ == "__main__":
    main()
