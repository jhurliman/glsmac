#!/usr/bin/env python3
"""Project owned inputs and literal guest outputs, without importing research code."""
import argparse
import hashlib
import json
from pathlib import Path

FIXTURE = "a6b11009ef265f2c44b0448ffff2a7c4d7c095a41103af262cd6a403740d1457"
REPORT = "44a9e50623dd06d6474b4f78527da83c6a7960902c43aed45a2af60d8afcd086"
EXECUTABLE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
CHECKER = "d2f0cde498944e24c485117c5d576f5b923b68502b0e6e57d6f12b274dc17b74"
SUPPORT = "ea4e1eae9fdf67507f9e49ae3efed667b6b81e9c4c668283d6f6f8e38a07051a"
OMISSIONS = ("exploration", "claims", "unit_visibility", "unit_orders", "base_selections", "relations")


def read(path, pin):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError(f"Fingerprint mismatch: {path}")
    return json.loads(raw)


def flag(value):
    if type(value) is not bool:
        raise ValueError("Expected a boolean")
    return str(value).lower()


def array(values):
    return "{ " + ", ".join(map(str, values)) + " }"


def mask(indices, limit=8):
    if len(indices) != len(set(indices)) or any(type(i) is not int or not 0 <= i < limit for i in indices):
        raise ValueError("Invalid set of bit indices")
    return sum(1 << i for i in indices)


def option(value):
    return "std::nullopt" if value is None else str(value)


def public_frame(value):
    # Input representation conversion only. The original full order field is
    # projected to the convoy predicate consumed by this boundary.
    value = json.loads(json.dumps(value))
    for unit in value["units"]:
        unit["active_convoy"] = unit.pop("order") == 3
    return value


def results(value):
    fields = []
    for index, row in enumerate(value):
        if row["id"] != index or any(len(row[key]) != 21 for key in ("claims", "recognized", "explored")):
            raise ValueError("Invalid literal result shape")
        if not 0 <= row["selected_mask"] < 1 << 21 or not 0 <= row["unavailable_mask"] < 1 << 21:
            raise ValueError("Receipt exceeds the selected21 projection")
        fields.append(array([array(row["coordinate"]), str(row["selected_mask"])+"u",
                             str(row["unavailable_mask"])+"u", array(row["claims"]), array(row["recognized"]),
                             str(mask([i for i, present in enumerate(row["explored"]) if present], 21))+"u"]))
    return array(fields)


def project(fixture_path, report_path):
    fixture, report = read(fixture_path, FIXTURE), read(report_path, REPORT)
    if report["executable_sha256"] != EXECUTABLE or report["source_sha256"] != CHECKER or report["dependencies"]["worker_frame_replay_support.py"] != SUPPORT:
        raise ValueError("Original executable/source provenance differs")
    if report["total"] != 7 or report["passed"] != 7 or len(report["results"]) != 7 or len(fixture["frames"]) != 5 or len(fixture["cases"]) != 35:
        raise ValueError("Original report count differs")
    expected = {}
    states = []
    baseline_count = 0
    for faction in report["results"]:
        if len(faction["saved_frames"]) != 5:
            raise ValueError("Missing captured state")
        for captured in faction["saved_frames"]:
            public = fixture["cases"][len(states)]
            index = next(i for i, f in enumerate(fixture["frames"]) if f["id"] == public["frame_id"])
            target = fixture["frames"][index]
            converted = public_frame(captured["frame"])
            digest = hashlib.sha256(json.dumps(converted, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
            if (public["faction"], public["stage"], public["passed"]) != (faction["faction"], captured["label"], True) or converted != target["frame"] or digest != target["semantic_sha256"]:
                raise ValueError("Ordered state/input binding differs")
            if captured["replayed"] != captured["expected"] or set(captured["omissions"]) != set(OMISSIONS):
                raise ValueError("Original replay/omission binding differs")
            actual = [captured["replayed"]] + [captured["omissions"][name] for name in OMISSIONS]
            if index in expected and expected[index] != actual:
                raise ValueError("Identical semantic frames have different literal outputs")
            expected[index] = actual
            baseline_count += len(captured["replayed"])
            states.append((faction["faction"] + "/" + captured["label"], index))
    if baseline_count != 49 or any(not f["json_roundtrip_replayed"] for f in fixture["frames"]):
        raise ValueError("Original base/roundtrip count differs")
    lines = ["#pragma once", "// Generated from pinned inputs and actual guest readbacks; no expected model.",
             '#include "game/backend/map/SmacWorkerMapFrame.h"',
             "namespace gse::tests::smac_worker_frame_fixture {", "using Frame = game::backend::map::SmacWorkerMapFrame;",
             "struct Run { size_t count; Frame::Tile tile; };",
             "struct Expected { Frame::Coordinate center; uint32_t selected, unavailable; std::array<int32_t,21> claims, recognized; uint32_t explored; };",
             "struct Input { Frame::Data data; std::vector<Run> runs; std::array<std::vector<Expected>,7> actual; };",
             "inline const std::array<Frame::Coordinate,21> offsets = {{ " + ", ".join(array(v) for v in fixture["radius_offsets"]) + " }};",
             "inline const Input inputs[] = {"]
    for index, record in enumerate(fixture["frames"]):
        frame = record["frame"]
        relations = array(array([flag(v["foreign_unit_exemption"]), flag(v["territory_recognition"])]) for v in frame["observer_relations"])
        bases = array(array([v["owner"], array(v["coordinate"]), str(mask(v["selected_indices"], 21))+"u"]) for v in frame["bases"])
        units = array(array([v["owner"], array(v["coordinate"]), mask(v["visible_to_factions"]), flag(v["active_convoy"])]) for v in frame["units"])
        data = array([frame["width"], frame["height"], frame["observer"], flag(frame["horizontal_wrap"]), flag(frame["ordinary_local_mode"]), flag(frame["foreign_disclosure"]), mask(frame["all_map_revealed"]), "{"+relations+"}", "{}", bases, units])
        if sum(run["count"] for run in frame["tiles_rle"]) != frame["width"] * frame["height"] // 2:
            raise ValueError("Invalid RLE coverage")
        runs = array(array([run["count"], array([flag(run["tile"]["sea"]), mask(run["tile"]["explored_by"]), option(run["tile"]["claimant"])])]) for run in frame["tiles_rle"])
        actual = "{{ " + ", ".join(results(value) for value in expected[index]) + " }}"
        lines.append("    " + array([data, runs, actual]) + ",")
    lines += ["};", "struct State { const char* id; size_t frame; };", "inline const State states[] = {"]
    lines += ["    " + array([json.dumps(identifier), index]) + "," for identifier, index in states]
    lines += ["};", "}"]
    return "\n".join(lines)+"\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    args.output.write_text(project(args.fixture, args.report), encoding="utf-8", newline="\n")
    print("Projected 5 semantic inputs, 35 baseline states/49 Base results and 210 omission states/294 Base results.")
