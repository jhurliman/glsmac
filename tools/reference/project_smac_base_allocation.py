#!/usr/bin/env python3
"""Project pinned original allocation authority; no expected-result model."""
import argparse
import hashlib
import json
from pathlib import Path

PINS = {
    "docs/behavior/base-allocation-authority-cases.json": "301c4ed8fca338ee01d3570e491537ca390e1a52c85efd4858d722819fbe6381",
    "tmp/reference/base-allocation-authority-capture-original-x86-results-20261017.json": "2a2963821a41ff81ff19ef6ca60b9353b32ce69ee2527d82e7e763a840497b3b",
    "tmp/reference/base-allocation-authority-controls-original-x86-results-20261017.json": "64e7da2c73d28aca78931ebaa5149e536f154914e418719f2c0c564020d1d56c",
}

def array(values):
    return "{ " + ", ".join(map(str, values)) + " }"

def state(record):
    return "{ " + str(record["selected_mask32"]) + "u, { " + array(record["stored_slot_ids16"]) + ", " + str(record["specialist_count"]) + " } }"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    inputs = []
    for name, fingerprint in PINS.items():
        raw = (args.reference_root / name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != fingerprint:
            raise ValueError(f"Fingerprint changed: {name}")
        inputs.append(json.loads(raw))
    public, capture, controls = inputs
    assert capture["passed"] == capture["total"] == 7
    assert controls["normal_returns"] == controls["total"] == 6
    raw_frames = {(row["faction"], frame["label"]): frame["bases"] for row in capture["results"] for frame in row["allocation_authority_frames"]}
    raw_controls = {row["inputs"]["id"]: row for row in controls["results"]}
    lines = ['#pragma once', '#include "game/backend/base/SmacBaseAllocationState.h"', '',
             'namespace gse::tests::smac_base_allocation_fixture {',
             '// Literal original logical values; private packed bytes are not exported.',
             '// Fixture SHA256: ' + next(iter(PINS.values())),
             'using game::backend::base::SmacBaseAllocationState;',
             'struct Capture { const char* id; SmacBaseAllocationState state; };',
             'inline const Capture captures[] = {']
    count = 0
    for frame in public["captures"]:
        raw_bases = raw_frames[(frame["faction"], frame["stage"])]
        assert len(raw_bases) == len(frame["bases"])
        for base, raw in zip(frame["bases"], raw_bases):
            for key in ("base_id", "coordinate", "owner", "specialist_count"):
                assert base[key] == raw[key]
            assert base["selected_mask32"] == raw["selected_word"]
            assert base["stored_slot_ids16"] == raw["logical_slots16"]
            label = f'{frame["faction"]}/{frame["stage"]}/{base["base_id"]}'
            lines.append('\t{ ' + json.dumps(label) + ', ' + state(base) + ' },')
            count += 1
    assert len(public["captures"]) == 35 and count == 49
    lines += ['};', 'struct Control { const char* id; bool upkeep; SmacBaseAllocationState input, expected; std::array<int32_t,3> intake; };', 'inline const Control controls[] = {']
    for row in public["controls"]:
        raw = raw_controls[row["id"]]
        for record, name in ((row["inputs"], "before"), (row["expected"], "after")):
            assert record["selected_mask32"] == raw[name]["selected_word"]
            assert record["specialist_count"] == raw[name]["specialist_count"]
            logical = [value >> shift & 15 for value in bytes.fromhex(raw[name]["packed_slots_hex"]) for shift in (0, 4)]
            assert record["stored_slot_ids16"] == logical
        assert row["expected"]["intake"] == raw["after"]["intake"]
        upkeep = row["id"].startswith("padding_")
        lines.append('\t{ ' + json.dumps(row["id"]) + ', ' + str(upkeep).lower() + ', ' + state(row["inputs"]) + ', ' + state(row["expected"]) + ', ' + array(row["expected"]["intake"]) + ' },')
    lines += ['};', '}', '']
    output = '\n'.join(lines).encode()
    if args.check:
        assert args.output.read_bytes() == output, "Committed allocation authority projection differs"
    else:
        args.output.write_bytes(output)
    print(f"Verified {count} captured Base records and {len(public['controls'])} original allocation controls")

if __name__ == "__main__":
    main()
