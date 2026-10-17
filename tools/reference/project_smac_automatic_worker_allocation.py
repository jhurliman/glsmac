#!/usr/bin/env python3
"""Project 51 pinned original full-intake returns; never execute the originals."""
import argparse
import hashlib
import json
from pathlib import Path

EXECUTABLE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
SOURCES = [
    ("worker-intake-integration", 10, "823718431f75ccddd028cf4fd29b3c17de86a76f0a5f0bf03bfff606d925febc", "1bfd54b873cbaf83e6b3ed1567ba91e95bc6fb9943d12eeb19dbb051394947e0", "check_worker_intake_integration.py", "07e36bb25ddaaa7357c57c070eab0ff1a3988d4b61905d8d6ae966b9fd4e3264"),
    ("existing-specialist-cleanup-full", 7, "58eabfc94262859cb33f75cad4f9b6be396bd814e5d3464c0f79e5c1ed2abb20", "9fbfcbb9f600d60e02e3000918661046981937b7308ec37a93911cd5b494dd9a", "check_specialist_cleanup_full.py", "4a025f3120225cdf0ca3a87a981e302793528a5339d3e77f60745af500018c97"),
    ("specialist-competition-finalization", 7, "40314a624b6e37fab59274eb7c00db16c6fa18450342976412805eb15a5689fb", "3a0cc0218bfdf695f1abe0d63ae6f3dab83dc8e33a66e3b6ee5064357e7065f5", "check_specialist_competition_finalization.py", "a732c1ba68d723033b1a9fa94d253f84d845b7b5942048f14a864625fa00d821"),
    ("worker-allocation-priorities-full", 7, "718aad5636252179fc8d7e465d5c776294c2148d5893b75242c1105c6fc28f25", "f9982c588864618b4784ca2bfb07d12970f3cbd4c829b23187cfee8c48d38076", "check_worker_priority_full.py", "28cdd19e4b31c7aae8a1186f1709650d35ef40be928e5f3bb72e18dd48aedeba"),
]
NAMES = ["Technician", "Doctor", "Librarian", "Engineer", "Empath", "Thinker", "Transcend"]
ORCHESTRATION = {
    "docs/behavior/worker-allocator-orchestration-cases.json": "4cc49b1edace8b958d6e64b92d9867ab3b5609ff944f933f69c2ce6af6d5f1dc",
    "tmp/reference/worker-allocator-orchestration-original-x86-results-20261017.json": "002daba6b96163c8e8725890ef4c6f554307c5f453fff36e87c3b31f700d0950",
    "tmp/reference/check_worker_allocator_orchestration.py": "1d9af2d70b707a440f6818680d906a60e0411b94b9a0e697ee7ac2db8ed41c04",
    "tmp/reference/installed-specialist-catalog-original-x86-results-20261017.json": "9a0e4203d9bb96de02adf3519ec68bd03cd5cab4b49cbbf0a12ff150648486d1",
}


def pinned(path, fingerprint):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != fingerprint:
        raise ValueError(f"Fingerprint mismatch: {path}")
    return raw


def array(values):
    return "{ " + ", ".join(map(str, values)) + " }"


def boolean(value):
    return "true" if value else "false"


def mask(indices):
    if len(indices) != len(set(indices)) or any(not 0 <= i <= 20 for i in indices):
        raise ValueError("Invalid radius mask")
    return str(sum(1 << i for i in indices)) + "u"


def inputs(family, value, installed=None):
    # Resolved inputs reproduce the pinned harness setup, not its oracle.
    # Every map has center and exactly two explored, usable land squares.
    pop, turn, selected, count = 1, 0, [0], 0
    yields = [[2, 1, 1], [0, 0, 0], [0, 0, 0]]
    types, psych, economy, labs = [1] * 16, [0, 2, 0, 0, 0, 0, 0], [0] * 7, [0] * 7
    human, governor, citizen, upkeep = True, False, False, False
    directives, faction, available = [], [0] * 5, {1}
    if family == 0:
        pop, selected, count = value["population"], value["selected_indices"], value["retained_specialists"]
        yields[1:] = [value["old_yield"], value["new_yield"]]
    elif family == 1:
        pop, turn, count = value["population"], 100, value["specialist_count"]
        governor = citizen = value["managed_allocation"]
        upkeep = value["upkeep_context"]
        types = [NAMES.index(n) for n in (value["stored_types"] + [value["unused_stored_type"]] * 16)[:16]]
        psych = [0, 2, 0, 0, 2, 1, 2]
        yields[0][0] = 50
    elif family == 2:
        pop, turn, count = value["population"], value["turn"], value["retained_doctors"]
        governor = citizen = value["managed_allocation"]
        yields[0][0] = value["center_nutrients"]
        economy[0], available = 3, {0, 1}
    elif family == 3:
        turn, human, governor, citizen = 100, value["human"], value["governor_active"], value["automatic_workers"]
        directives = value["directives"]
        faction = [value["faction_priorities"][key] for key in ["aggression", "Explore", "Discover", "Build", "Conquer"]]
        yields[1:] = [[1, 4, 0], [1, 0, 4]]
    elif family == 4:
        pop, turn, count = value["population"], 100, value["retained_specialists"]
        selected, yields[1:] = value["selected_indices"], value["candidate_yields"]
        human, governor, citizen, upkeep = (value[k] for k in ["human", "governor_active", "citizen_management", "upkeep"])
        # The original setup fills both packed words with Doctor, then writes
        # the FIRST word to zero for the retained-Technician controls.
        if value["stored_specialist_type"] == "Technician":
            types[:8] = [0] * 8
        economy, psych, labs = ([r[k] for r in installed["records"]] for k in ["economy", "psych", "labs"])
        # No owned technologies: only literal None prerequisite is available;
        # all stock obsolete references are either technologies or Disable.
        available = {r["id"] for r in installed["records"] if r["prerequisite"] == -1}
        if any(r["obsolete"] == -1 for r in installed["records"]):
            raise ValueError("Unexpected available stock obsolete token")
    else:
        raise ValueError("Unknown fixture family")
    priorities = "{ " + ", ".join([boolean(human), boolean(governor)] + [boolean(n in directives) for n in ["Explore", "Discover", "Build", "Conquer"]] + [array(faction)]) + " }"
    catalog = "{ 5, {{ " + ", ".join("{ " + ", ".join([str(economy[i]), str(psych[i]), str(labs[i]), boolean(i in available), "false"]) + " }" for i in range(7)) + " }} }"
    # Unavailable includes center; cleanup preserves an existing center bit.
    # 0x1ffff9 excludes every worker index except 1 and 2. Only Doctor is the
    # initial overflow selector in these supplied catalogs (also when 0 is available).
    result = "{ " + ", ".join([str(pop), str(turn), "2", "0", "SmacWorkerProduction::Unit", priorities, boolean(citizen), boolean(upkeep), mask(selected), "2097145u", "{ 0, 0, 0 }", "{ " + array(types) + ", " + str(count) + " }", catalog, "uint8_t( 1 )"]) + " }"
    return result, "{{ " + ", ".join(array(y) for y in yields) + " }}"


def query(index, state):
    return "{ " + ", ".join([str(index), str(state["visited"]) + "u", str(state["slots"]), array(state["intake"])]) + " }"


def orchestration(source_root, lines, identifiers):
    blobs = {p: pinned(source_root / p, sha) for p, sha in ORCHESTRATION.items()}
    fixture = json.loads(blobs["docs/behavior/worker-allocator-orchestration-cases.json"])
    report = json.loads(blobs["tmp/reference/worker-allocator-orchestration-original-x86-results-20261017.json"])
    installed = json.loads(blobs["tmp/reference/installed-specialist-catalog-original-x86-results-20261017.json"])
    if report["executable_sha256"] != EXECUTABLE or installed["executable_sha256"] != EXECUTABLE:
        raise ValueError("Orchestration executable differs")
    if report["source_sha256"] != ORCHESTRATION["tmp/reference/check_worker_allocator_orchestration.py"] or report["catalog_sha256"] != ORCHESTRATION["tmp/reference/installed-specialist-catalog-original-x86-results-20261017.json"]:
        raise ValueError("Orchestration setup differs")
    for name, sha in report["dependencies"].items():
        pinned(source_root / "tmp/reference" / name, sha)
    if report["total"] != 20 or report["normal_returns"] != 20 or len(fixture["cases"]) != 20:
        raise ValueError("Orchestration count differs")
    if installed["minimum_specialist_size"] != 5 or [r["id"] for r in installed["records"]] != list(range(7)):
        raise ValueError("Installed specialist source differs")
    scores, admissions = 0, 0
    for case, actual in zip(fixture["cases"], report["results"]):
        value, expected = case["inputs"], case["expected"]
        original, final = actual["inputs"], actual["final"]
        if case["id"] != original["id"] or not actual["normal_return"]:
            raise ValueError("Orchestration row identity/return differs")
        if any(value[k] != original[k] for k in ["population", "selected_indices", "retained_specialists", "upkeep"]) or value["candidate_yields"] != [original["old_yield"], original["new_yield"]]:
            raise ValueError("Orchestration input projection differs")
        flags = original.get("governor_flags", 0x80000040 if original["policy"] == "managed_human" else 0)
        if [value[k] for k in ["human", "governor_active", "citizen_management"]] != [original["policy"] != "ai", bool(flags & 0x80000000), bool(flags & 0x40)]:
            raise ValueError("Policy input projection differs")
        if mask(expected["selected_indices"]) != str(final["selected"]) + "u" or any(expected[k] != final[v] for k, v in [("specialists", "specialists"), ("specialist_types", "types"), ("intake", "intake")]):
            raise ValueError("Orchestration final projection differs")
        fill = [e for e in actual["events"] if e["pc"] == "0x4fe7fb"]
        if len(fill) != 1 or fill[0]["visited"] != final["selected"] or fill[0]["intake"] != final["intake"] or fill[0]["slots"] != final["specialists"]:
            raise ValueError("Unexpected observed pre-fill boundary")
        callback_queries = []
        scan_events = [e for e in actual["events"] if e["pc"] == "0x4fdd36"]
        for scan in scan_events:
            tests = [t for t in actual["candidate_tests"] if t["scan"] == scan["scan"]]
            sampled = [s for s in actual["candidate_scores"] if s["scan"] == scan["scan"]]
            if [t["index"] for t in tests] != list(range(1,21)) or [t["index"] for t in tests if t["admitted"]] != [s["index"] for s in sampled]:
                raise ValueError("Actual admission/scoring order differs")
            for s in sampled:
                if any(s[k] != scan[k] for k in ["visited", "slots", "intake"]):
                    raise ValueError("Scoring changed whole-query state")
                callback_queries.append(query(s["index"], s))
            admissions += len(tests)
        scores += len(callback_queries)
        repair = "{}"
        if actual["repair_center_queries"]:
            gate = [e for e in actual["events"] if e["pc"] == "0x4fdb29"]
            if len(gate) != 1 or [q["selector"] for q in actual["repair_center_queries"]] != [0,1,2,3] or any(q["visited"] != gate[0]["visited"] or q["slots"] != gate[0]["slots"] for q in actual["repair_center_queries"]):
                raise ValueError("Repair center observation differs")
            # One whole-tile callback projects the state at selector0; this
            # deliberately does not equate four selectors with four callbacks.
            repair = "Query" + query(0, gate[0])
        inp, yields = inputs(4, value, installed)
        types = final["types"]
        before = "std::array<int32_t,2>" + array([fill[0]["retained"], fill[0]["provisional"]])
        lines.append("\t{ " + ", ".join([json.dumps(case["id"]), "4", inp, yields, str(final["selected"]) + "u", array(final["intake"]), str(final["specialists"]), array(types + [0] * (16 - len(types))), str(len(types)), before, "{ " + ", ".join(callback_queries) + " }", repair]) + " },")
        identifiers.append(case["id"])
    if (admissions, scores) != (300, 26):
        raise ValueError("Orchestration admission/score observation counts differ")


def project(source_root):
    lines = ["#pragma once", "// Literal original return projections; regenerate with project_smac_automatic_worker_allocation.py.",
             "#include <array>", "#include <cstddef>", "#include <cstdint>", "#include <optional>", "#include <vector>",
             '#include "game/backend/base/SmacAutomaticWorkerAllocation.h"',
             "namespace gse::tests::smac_automatic_worker_fixture {",
             "using namespace game::backend::base;",
             "struct Query { int32_t index; uint32_t visited; int32_t slots; std::array<int32_t,3> intake; };",
             "struct Case { const char* id; int32_t family; SmacAutomaticWorkerInput input; std::array<std::array<int32_t,3>,3> yields; uint32_t visited; std::array<int32_t,3> intake; int32_t count; std::array<uint8_t,16> types; size_t reported_types; std::optional<std::array<int32_t,2>> before_fill; std::vector<Query> scan_queries; std::optional<Query> repair_center; };",
             "inline const Case cases[] = {"]
    provenance, identifiers = [], []
    for family, (name, count, fpin, rpin, harness, hpin) in enumerate(SOURCES):
        fpath = Path("docs/behavior") / (name + "-cases.json")
        rpath = Path("tmp/reference") / (name + "-original-x86-results-20261016.json")
        hpath = Path("tmp/reference") / harness
        fixture = json.loads(pinned(source_root / fpath, fpin))
        report = json.loads(pinned(source_root / rpath, rpin))
        pinned(source_root / hpath, hpin)
        if report["executable_sha256"] != EXECUTABLE or report["fixture_sha256"] != fpin:
            raise ValueError("Original report provenance differs")
        if len(fixture["cases"]) != count or report["named_count"] != count or report["passed"] != report["total"]:
            raise ValueError("Original report count/result differs")
        actual_rows = {row["id"]: row for row in report["results"]}
        if len(actual_rows) != len(report["results"]):
            raise ValueError("Duplicate original result ID")
        for case in fixture["cases"]:
            actual = actual_rows[case["id"]]
            if not actual["passed"] or actual["actual"] != case["expected"]:
                raise ValueError("Fixture does not match actual original output")
            actual = actual["actual"]  # Expected values below come only from this object.
            value = fixture["defaults"] | case["overrides"]
            inp, yields = inputs(family, value)
            visited = actual["visited_indices"] if family == 0 else actual["selected_indices"]
            final_count = actual["specialists"] if family in (0, 3) else actual["final_specialist_count"]
            reported = actual.get("specialist_types", actual.get("stored_types_after", []))
            types = [NAMES.index(n) for n in reported]
            if len(types) > 16:
                raise ValueError("Only physically stored first16 types may be projected")
            before = "{}" if family != 2 else "std::array<int32_t,2>" + array([actual["retained_before_fill"], actual["provisional_before_fill"]])
            lines.append("\t{ " + ", ".join([json.dumps(case["id"]), str(family), inp, yields, mask(visited), array(actual["intake"]), str(final_count), array(types + [0] * (16 - len(types))), str(len(types)), before]) + " },")
            identifiers.append(case["id"])
        provenance.append({"fixture": str(fpath).replace("\\", "/"), "fixture_sha256": fpin, "report": str(rpath).replace("\\", "/"), "report_sha256": rpin, "input_harness": str(hpath).replace("\\", "/"), "input_harness_sha256": hpin, "named_cases": count})
    if len(identifiers) != 31 or len(set(identifiers)) != 31:
        raise ValueError("Expected exactly31 distinct named cases")
    orchestration(source_root, lines, identifiers)
    if len(identifiers) != 51 or len(set(identifiers)) != 51:
        raise ValueError("Expected exactly51 distinct named cases")
    result = "\n".join(lines + ["};", "}", ""])
    manifest = {"schema": 1, "executable_sha256": EXECUTABLE, "sources": provenance, "orchestration_sources": ORCHESTRATION, "ids": identifiers, "projection_sha256": hashlib.sha256(result.encode()).hexdigest(),
                "scope": "51 named complete original intake returns projected onto detached allocation with resolved availability and tile yields; no original execution or oracle import.",
                "observations": "Final masks/intake/counts, reported stored-type prefixes only, 27 directly observed pre-fill R/Q pairs, and 26 ordered scan query states from 20 observation-only calls. Repair center projects state before selector 0 as one whole-tile query, not four selector callbacks. No modeled candidate scores, unobserved padding, food-marker API, cleanup-to-prefill relabeling, ecology, or full original prefix claim."}
    return result, json.dumps(manifest, indent=2) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    result, manifest = project(args.source_root)
    if args.check:
        if args.output.read_text() != result or args.manifest.read_text() != manifest:
            raise SystemExit("Owned projection or manifest differs")
    else:
        args.output.write_text(result, newline="\n")
        args.manifest.write_text(manifest, newline="\n")
    print("51 actual final outputs, 27 pre-fill R/Q pairs and 26 scan query states match pinned sources")


if __name__ == "__main__":
    main()
