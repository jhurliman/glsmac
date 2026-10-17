#!/usr/bin/env python3
"""Compare supplied-input production stat queries with literal original outputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

FIXTURE = "bf6b3dffba9c7a031df4a865c1b1f392b3c1a636f8b201e6346d2e7c0b691dc7"
REPORT = "8a4374ff62e161718a637758acb5c587fdb81fca35dbf993b8fdfe73faf49e32"
SOURCE = "a220c2dd13aea6086551d1b64e7f560cca0fc8bf83e4164c6a96e8ba75397dac"
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pinned(path, fingerprint):
    if digest(path) != fingerprint:
        raise ValueError(f"Evidence fingerprint changed: {path}")
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "fixture", "report", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    fixture = pinned(args.fixture, FIXTURE)
    report = pinned(args.report, REPORT)
    if (report["fixture_sha256"] != FIXTURE or report["source_sha256"] != SOURCE
            or report["executable_sha256"] != EXE or report["total"] != 7174
            or report["passed"] != 7174):
        raise ValueError("Original report identity/count changed")
    if [r["id"] for r in report["results"]] != [c["id"] for c in fixture["cases"]]:
        raise ValueError("Ordered original case IDs differ")
    homes = {"none": (0, 0, 0), "ordinary": (1, 0, 0), "creche": (1, 1, 0),
             "riots": (1, 0, 1), "creche_riots": (1, 1, 1)}
    domains = {0: 0, 1: 0, 3: 1, 6: 2}  # Supplied installed chassis metadata.
    requests = []
    expected = []
    morale_rows = movement_rows = 0
    for case, captured in zip(fixture["cases"], report["results"]):
        if captured["passed"] is not True:
            raise ValueError("Original failed row")
        i, actual = case["inputs"], captured["actual"]
        if case["kind"] == "morale_query":
            morale_rows += 1
            for mode in (0, 1):
                requests.append([0, i["stored_morale"], i["retained_morale"], i["faction_morale_scalar"],
                                 int(i["negative_morale_immunity"]), *homes[i["home"]], mode])
                expected.append((case["id"], mode, actual[mode]))
        elif case["kind"] == "movement_query":
            movement_rows += 1
            # This producer starts AFTER the basic design query. Supply its
            # separately observed output; compare only the full allowance.
            requests.append([1, actual[0], i["movement_quantum"], domains[i["chassis"]], i["reactor"],
                             i["damage"], i["stored_morale"], int(i["maritime_project_owned"]), int(i["suppress_elite_bonus"])])
            expected.append((case["id"], 1, actual[1]))
        else:
            raise ValueError("Unexpected original query kind")
    if (morale_rows, movement_rows, len(expected)) != (2310, 4864, 9484):
        raise ValueError("Supported comparison count changed")
    result = subprocess.run([str(args.probe.resolve())], input="".join(" ".join(map(str, row)) + "\n" for row in requests),
                            text=True, capture_output=True, timeout=30)
    if result.returncode:
        raise RuntimeError(f"Production probe failed: {result.stderr}")
    outputs = result.stdout.splitlines()
    if len(outputs) != len(expected):
        raise ValueError("Production result count differs")
    failures = [{"id": ident, "mode": mode, "expected": want, "actual": got}
                for (ident, mode, want), got in zip(expected, outputs) if got != f"ok {want}"]
    receipt = {"fixture_sha256": FIXTURE, "report_sha256": REPORT, "source_sha256": SOURCE,
               "original_executable_sha256": EXE, "probe_sha256": digest(args.probe),
               "morale_rows_two_modes": morale_rows, "full_allowance_rows": movement_rows,
               "compared_outputs": len(expected), "matched_outputs": len(expected) - len(failures),
               "scope": "Supplied conventional query inputs; basic-design query output supplied, not reproduced; no creation/startup/RNG/lifecycle comparison.",
               "failures": failures}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: receipt[key] for key in ("compared_outputs", "matched_outputs", "failures")}))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
