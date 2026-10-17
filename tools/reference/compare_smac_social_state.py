#!/usr/bin/env python3
"""Compare production neutral social state to captured original rating vectors."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ALPHA_SHA = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"
EXE_SHA = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
SOURCES = (
    ("startup", "068bd93903781b0bdf4a06d25ad6f5b5f39cbac0e48bf66b79f01e06998648af",
     "b0b44e3fbb72012be8ac11f48216b867412aa140b3fb64054aab355c22204485", 28),
    ("immunity", "113fc86c3715ff139a245644315ebcd54a6f92cb4ec50e5b9585f07f0b79d5d7",
     "bb4ac69aa602078c0b8bb52c666393ee8b014468728c868dd4043ba423b38f08", 459),
    ("duplicates", "fedd520771ae72b0ed9c71db50ba0a823d29f19146716c69288dff1e6ce2a7be",
     "2f7f35b8c84cd812d24a0a602a7857fc08a2ffb5686f1dd524983af72f25b22c", 176),
)
AXES = "ECONOMY EFFIC SUPPORT TALENT MORALE POLICE GROWTH PLANET PROBE INDUSTRY RESEARCH".split()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path, expected):
    if digest(path) != expected:
        raise ValueError(f"Evidence fingerprint changed: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def quote(text):
    return '"' + text.replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "alpha", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    for name, *_ in SOURCES:
        for kind in ("report", "fixtures"):
            parser.add_argument(f"--{name}-{kind}", type=Path, required=True)
    args = parser.parse_args()
    if digest(args.alpha) != ALPHA_SHA:
        raise ValueError("Supplied alpha fingerprint changed")
    comparisons, excluded, counts = [], [], {}
    for name, report_sha, fixture_sha, count in SOURCES:
        report = read(getattr(args, name + "_report"), report_sha)
        fixtures = read(getattr(args, name + "_fixtures"), fixture_sha)
        if (report["executable_sha256"] != EXE_SHA or report["alpha_sha256"] != ALPHA_SHA
                or report["fixture_sha256"] != fixture_sha or len(report["results"]) != count
                or [c["id"] for c in fixtures["cases"]] != [r["id"] for r in report["results"]]):
            raise ValueError(f"Original identity/ordered cases changed: {name}")
        matched = 0
        for case, row in zip(fixtures["cases"], report["results"]):
            if not row["passed"]:
                raise ValueError("Original execution did not pass")
            inputs = case["inputs"]
            reason = None
            if name == "startup" and case["kind"] != "bonus_startup_upkeep_composition":
                reason = "isolated early-reset context (covered by native value tests)"
            elif inputs.get("selected_model_positions", [0, 0, 0, 0]) != [0, 0, 0, 0]:
                reason = "nonzero model selections"
            elif any(key.upper() == "MORALE" for key, _ in inputs["bonus_pairs"]):
                reason = "direct MORALE profile not admitted by this checkpoint"
            if reason:
                excluded.append({"source": name, "id": case["id"], "reason": reason})
                continue
            override = inputs.get("resolved_intrinsic_override", {})
            if len(override) > 1:
                raise ValueError("Unsupported multiple resolved overrides")
            axis, value = next(iter(override.items())) if override else (None, 0)
            request = f"{inputs.get('faction', 7)} {AXES.index(axis) if axis else -1} {value} {len(inputs['bonus_pairs'])}\n"
            request += "\n".join(quote(key) + " " + quote(value) for key, value in inputs["bonus_pairs"]) + "\n"
            process = subprocess.run([str(args.probe.resolve()), str(args.alpha.resolve())], input=request,
                                     text=True, capture_output=True, timeout=10)
            if process.returncode:
                raise RuntimeError(f"Production rejected {name}/{case['id']}: {process.stderr}")
            actual = json.loads(process.stdout)
            # Read captured guest-memory outputs only, never fixture expectations.
            if name == "startup":
                captured = row["actual"]
                expected = {"intrinsic": captured["intrinsic_social"], "current": captured["current_social"],
                            "retained": captured["retained_social_snapshot"], "records": len(captured["parsed"]),
                            "early_models": captured["startup_models"], "early_current": captured["startup_current_ratings"]}
            else:
                captured = row["actual_vectors"]
                expected = {key: captured[key] for key in ("intrinsic", "current", "retained")}
                expected["records"] = len(row["private_parsed_records"] if name == "immunity" else row["parsed_records"])
            for key, values in expected.items():
                if actual[key] != values:
                    raise AssertionError(f"Mismatch {name}/{case['id']}/{key}: {actual[key]} != {values}")
            if not actual["roundtrip"] or not actual["early_unknown"]:
                raise AssertionError("Production codec or explicit unknown state failed")
            comparisons.append({"source": name, "id": case["id"], "actual": actual})
            matched += 1
        counts[name] = matched
    if counts != {"startup": 16, "immunity": 454, "duplicates": 143}:
        raise ValueError(f"Supported comparison subset changed: {counts}")
    result = {"sources": [{"name": n, "report_sha256": r, "fixture_sha256": f} for n, r, f, _ in SOURCES],
              "alpha_sha256": ALPHA_SHA, "executable_sha256": EXE_SHA, "probe_sha256": digest(args.probe),
              "matched": counts, "total": len(comparisons), "excluded": excluded,
              "scope": "Captured intrinsic/current/retained social vectors and admitted-record count for neutral selected models; production owned codec roundtrip. Excludes direct MORALE, nonzero models, units, yields, third-vector lifecycle, complete original startup/upkeep.",
              "comparisons": comparisons}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Matched {len(comparisons)} captured projections and production roundtrips: {counts}; excluded {len(excluded)} contexts")


if __name__ == "__main__":
    main()
