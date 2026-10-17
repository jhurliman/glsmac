#!/usr/bin/env python3
"""Project recorded region seeds/poles, without reimplementing the region model."""
import argparse
import hashlib
import json
from pathlib import Path

FIXTURE = "849390f6a4c815020c5b5bc281b1734ea8328d2aab25a3464a105db7adcbcf76"
REPORT = "4a00ba8dc7a2f737edc1e353326af6e2a40f8b5b3cbdb4bdb425349490667fdc"

def read(path, pin):
    raw = path.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == pin, path
    return json.loads(raw)

def project(root):
    public = read(root / "docs/behavior/water-region-rebuild-and-freshwater-cases.json", FIXTURE)
    report = read(root / "tmp/reference/regions-freshwater-original-x86-results-20261017.json", REPORT)
    assert report["fixture_sha256"] == FIXTURE
    assert report["total"] == report["passed"] == len(public["cases"]) == len(report["results"]) == 25
    assert report["rebuild_passes"] == 27
    rows = {}
    for case, recorded in zip(public["cases"], report["results"]):
        assert case["id"] == recorded["id"] and recorded["passed"]
        assert len(case["expected_steps"]) == len(recorded["steps"])
        q = case["inputs"]
        sea = list(q["sea"])
        for step, actual in zip(case["expected_steps"], recorded["steps"]):
            assert actual["passed"] and actual["region_ids_match"] and actual["all_counts_match"]
            if step["phase"]:
                sea += q["augment"]
            key = (q["width"], q["height"], q["wrap"], q["all_sea"], tuple(sorted(map(tuple, sea))))
            probes = [(tuple(c["seed"]), c["region"]) for c in step["components"]]
            probes += [(tuple(p), int(region)) for region, points in step["polar_region_squares"].items() for p in points]
            if key in rows:
                assert rows[key][1] == probes
            else:
                rows[key] = (case["id"] + ":" + str(step["phase"]), probes)
    assert len(rows) == 16
    lines = ["#pragma once", "#include <array>", "#include <vector>", "#include <cstdint>",
             "namespace gse::tests::territory_topology_fixture {", f"// Public {FIXTURE}; original report {REPORT}.",
             "// Public seed/polar projections verified by the original report's complete region-ID comparison.",
             "// Sixteen distinct supplied terrain states from27 recorded rebuilds. No modeled output here.",
             "using Coordinate = std::array<int32_t,2>;", "struct Probe { Coordinate position; uint8_t region; };",
             "struct Case { const char* id; int32_t width,height; bool wrap,all_sea; std::vector<Coordinate> sea; std::vector<Probe> probes; };",
             "inline const std::array<Case,16> cases{{"]
    for (w,h,wrap,all_sea,sea), (name,probes) in rows.items():
        coords = ",".join("{%d,%d}" % p for p in sea)
        expected = ",".join("{{%d,%d},%d}" % (*p,r) for p,r in probes)
        lines.append('    {"%s",%d,%d,%s,%s,{%s},{%s}},' % (name,w,h,str(wrap).lower(),str(all_sea).lower(),coords,expected))
    return "\n".join(lines + ["}};", "}", ""])

if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("root",type=Path); p.add_argument("output",type=Path)
    args = p.parse_args()
    args.output.write_text(project(args.root),encoding="utf-8",newline="\n")
    print("Projected16 distinct region seed/polar states from27 original rebuilds")
