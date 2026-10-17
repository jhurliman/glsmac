#!/usr/bin/env python3
"""Project complete recorded territory writes; never execute an original oracle."""
import argparse
import hashlib
import json
from pathlib import Path

PINS = {
    "land": ("54052e272f25e3f8b1802294936a4ab39bcecc562271cfa55567be3f9538d2dd",
             "bb50de7bd33bc579901e0591b11bcdc62f23e04dac568b37aa898c019cd6cbdd"),
    "sea": ("67a80dbd241e3e4d5d70f43b45761351c5299a36efbc32f9a705663110db4a6e",
            "4d48482290c394cecb1f7d42ab3264ee2708bcc6929b2e830eba745a751a2d62"),
}
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def read(path, pin):
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == pin, path
    return json.loads(raw)


def project(root):
    lines = ["#pragma once", "", '#include "game/backend/map/SmacTerritoryRules.h"',
             "#include <array>", "#include <vector>", "", "namespace gse::tests::smac_territory_fixture {",
             "// Literal original claim writes; '.' means unclaimed. No modeled output."]
    for family, pins in PINS.items():
        lines.append(f"// {family} public fixture {pins[0]}; original report {pins[1]}.")
    lines += ["struct Worker { bool recognized; uint32_t unavailable; std::array<int32_t,21> raw, owners; };",
              "struct Case { const char* name; bool coast; int32_t border; std::vector<game::backend::map::smac_territory::Base> bases; const char* claims; std::vector<Worker> workers; };",
              "inline const std::array<Case,14> cases{{"]
    for family, pins in PINS.items():
        public = read(root / f"docs/behavior/{family}-territory-producer-cases.json", pins[0])
        report = read(root / f"tmp/reference/{family}-territory-producer-original-x86-results-20261017.json", pins[1])
        assert report["executable_sha256"] == EXE and report["fixture_sha256"] == pins[0]
        assert report["total"] == report["passed"] == len(public["cases"]) == len(report["results"]) == 7
        for case, recorded in zip(public["cases"], report["results"]):
            assert recorded["case"] == case and recorded["passed"] is True
            writes = recorded["claim_writes"]
            assert len(writes) == 1024 and [w["tile"] for w in writes] == list(range(1024))
            assert all(w["pc"] == "0x5390e7" and w["value"] in (255, 1, 2) for w in writes)
            literal = [w["value"] for w in writes]
            # Independently cross-check public coordinates against recorded bytes.
            coordinates = [255] * 1024
            for owner, positions in case["claimed_coordinates"].items():
                for x, y in positions:
                    assert 0 <= x < 64 and 0 <= y < 32 and x % 2 == y % 2
                    i = y * 32 + x // 2
                    assert coordinates[i] == 255
                    coordinates[i] = int(owner)
            assert coordinates == literal
            if family == "land":
                assert case["horizontal_wrap"] and recorded["exploration_preserved"] and recorded["units_preserved"]
                # These actual selector inputs pin every supplied region below.
                calls = recorded["selector_calls"]
                assert len(calls) == 1024
                for i, call in enumerate(calls):
                    y, halfx = divmod(i, 32)
                    assert call["arguments"] == [2 * halfx + y % 2, y, -1, 63 if y in (0, 31) else 1, -1, -1]
            else:
                assert report["region_counts"] == {"63": 33, "127": 31, "1": 495, "65": 465}
            bases = ",".join("{{%d,%d},%d}" % (*b["coordinate"], b["owner"]) for b in case["bases"])
            lines.append('    {"%s-%s",%s,%d,{%s},' % (family, case["name"], str(family == "sea").lower(), case["border_distance"], bases))
            for y in range(32):
                lines.append('        "' + "".join("." if v == 255 else str(v) for v in literal[y*32:(y+1)*32]) + '"')
            workers = []
            for worker in case["worker_frames"]:
                raw = worker.get("raw_radius_claims", worker.get("radius_claims"))
                owners = worker.get("recognized_radius_owners", worker.get("recognized_owners"))
                assert len(raw) == len(owners) == 21
                workers.append("{%s,%du,{%s},{%s}}" % (str(worker["mutual_recognition_granted"]).lower(), worker["unavailable_mask"], ",".join(map(str, raw)), ",".join(map(str, owners))))
            lines.append("        ,{" + ",".join(workers) + "}},")
    lines += ["}};", "}", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path, help="Read-only original receipt root")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(project(args.root), encoding="utf-8", newline="\n")
    print("Projected 14 complete maps / 14,336 original claim writes and 12 worker frames")
