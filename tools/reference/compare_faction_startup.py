"""Compare pure parser projections with a separately retained original-x86 receipt.

The proprietary install and research report are explicit local read-only inputs.
No original execution, dynamic ownership, social refresh or startup is performed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

REPORT_SHA256 = "068bd93903781b0bdf4a06d25ad6f5b5f39cbac0e48bf66b79f01e06998648af"
FIXTURE_SHA256 = "b0b44e3fbb72012be8ac11f48216b867412aa140b3fb64054aab355c22204485"
EXE_SHA256 = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
ALPHA_SHA256 = "38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4"


def checked_json(path, digest):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != digest:
        raise ValueError(f"Unexpected evidence fingerprint: {path}")
    return json.loads(raw)


def installed_file(directory, filename):
    matches = [path for path in directory.iterdir()
               if path.is_file() and path.name.lower() == filename.lower()]
    if len(matches) != 1:
        raise ValueError(f"Expected one case-insensitive installed file: {filename}")
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--install", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--fixtures", required=True, type=Path)
    args = parser.parse_args()
    report = checked_json(args.report, REPORT_SHA256)
    fixture = checked_json(args.fixtures, FIXTURE_SHA256)
    if report["executable_sha256"] != EXE_SHA256 or report["alpha_sha256"] != ALPHA_SHA256 or report["fixture_sha256"] != FIXTURE_SHA256:
        raise ValueError("Evidence provenance mismatch")
    if hashlib.sha256(installed_file(args.install, "alpha.txt").read_bytes()).hexdigest() != ALPHA_SHA256:
        raise ValueError("Unexpected installed alpha.txt")
    for name, digest in report["faction_sha256"].items():
        if hashlib.sha256(installed_file(args.install, name + ".txt").read_bytes()).hexdigest() != digest:
            raise ValueError(f"Unexpected installed faction source: {name}")
    rows = [row for row in fixture["cases"] if row["kind"] == "bonus_startup_upkeep_composition"]
    actual_by_id = {row["id"]: row for row in report["results"]}
    # These are the explicit neutral port tag mappings, not a wire compatibility claim.
    tags = {0: 0, 3: 2, 2: 3, 4: 4, 7: 5, 11: 6}
    wanted = []
    lines = []
    for row in rows:
        original = actual_by_id[row["id"]]
        if not original["passed"]:
            raise ValueError(f"Unsuccessful original fixture: {row['id']}")
        observed = original["actual"]
        words = [str(observed["extra_tech_count"])]
        words += [f"{tags[kind]}:{parameter}:{amount}" for kind, parameter, amount in observed["parsed"]]
        wanted.append(" ".join(words))
        lines.append(",".join(token for pair in row["inputs"]["bonus_pairs"] for token in pair))
    if len(rows) != 16:
        raise ValueError("Unexpected parser projection case count")
    run = subprocess.run([str(args.probe), str(args.install), "--lines"], input="\n".join(lines) + "\n", text=True, capture_output=True, check=True)
    if run.stdout.splitlines() != wanted:
        raise ValueError("C++ parser projection differs from original parsed records/count")
    installed = subprocess.run([str(args.probe), str(args.install), "--installed"], text=True, capture_output=True, check=True)
    identities = ["GAIANS", "HIVE", "UNIVERSITY", "MORGANITES", "SPARTANS", "BELIEVERS", "PEACEKEEPERS"]
    if installed.stdout.splitlines() != [name + " " + value for name, value in zip(identities, wanted[:7])]:
        raise ValueError("Installed canonical source profile differs from seven original projections")
    print("16/16 original parsed-record/count projections and 7/7 installed source bindings match.")
    print("Excluded: ownership, deferred awards, social effects, early reset/difficulty, full startup and SMAX.")


if __name__ == "__main__":
    main()
