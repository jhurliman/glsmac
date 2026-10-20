"""Check literal original retained-facing receipts; no game assets required."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OFFSETS = ((1, -1), (2, 0), (1, 1), (0, 2), (-1, 1), (-2, 0), (-1, -1), (0, -2))


def check(packet):
    assert packet["directions"] == [[i, *delta] for i, delta in enumerate(OFFSETS)]
    assert len(packet["ordinary"]) == 26 and len(packet["hostile"]) == 4
    for mode, first_store in (("ordinary", "0x5af979"), ("local", "0x570ebe")):
        rows = [row for row in packet["ordinary"] if row["id"].startswith(mode + "_")]
        assert len(rows) == 13
        for row in rows:
            inputs, actual = row["inputs"], row["expected"]
            name, direction = inputs["name"], inputs["direction"]
            assert row["factory_facing"] == 2 and inputs["initial_facing"] == 5
            if name.startswith("direction_") or name == "partial_success":
                dx, dy = OFFSETS[direction]
                assert actual["position"] == [8 + dx, 8 + dy]
                assert actual["facing"] == direction and actual["wrapper_return"] == 1
                assert actual["spent"] == (5 if name == "partial_success" else 3)
                assert actual["writes"] == [
                    {"pc": first_store, "prior": 5, "value": direction},
                    {"pc": "0x5b11df", "prior": direction, "value": direction},
                ]
            else:
                assert name in ("exhausted", "overspent", "water_blocked", "lottery_refused")
                assert actual["position"] == [8, 8] and actual["facing"] == 5
                assert actual["writes"] == []
                assert actual["wrapper_return"] == (1 if name == "lottery_refused" else 0)
                assert actual["spent"] == {"exhausted": 3, "overspent": 4,
                                           "water_blocked": 0, "lottery_refused": 3}[name]
    for row in packet["hostile"]:
        initial, actual = row["inputs"]["initial_units"], row["expected"]
        assert [unit["facing"] for unit in initial] == [5, 6]
        assert actual["wrapper_return"] == 0 and len(actual["units"]) == 1
        unit = actual["units"][0]
        if row["id"] == "human_defender_wins":
            assert (unit["owner"], unit["position"], unit["facing"]) == (2, [9, 9], 6)
            assert actual["selected"] == -1 and not actual["subsequent_key_emitted"]
            assert [(w["pc"], w["new_byte"]) for w in actual["writes"]] == [("0x5d8af7", 6)]
        else:
            assert row["id"] in ("human_neutral", "ai_control", "human_elite")
            assert (unit["owner"], unit["position"], unit["facing"]) == (1, [8, 8], 2)
            assert [(w["pc"], w["new_byte"]) for w in actual["writes"]] == [("0x5b11df", 2)]
            assert actual["subsequent_key_emitted"] == (row["id"] == "human_elite")


def check_private(packet, directory):
    for name, expected in packet["private_receipts"].items():
        raw = (directory / name).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == expected, name
        rows = json.loads(raw)
        if isinstance(rows, dict):
            for row in rows["cases"]:
                assert row["normal_return"]
                before, after = bytes.fromhex(row["before"]), bytes.fromhex(row["after"])
                assert len(before) == len(after) == 52 and before[41] == 5
                assert after[41] == row["facing_after"]
                assert bytes.fromhex(row["factory_record"])[41] == 2
        else:
            assert all(row["normal_return"] for row in rows)
    for name, expected in packet["producers"].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == expected, name


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--private-dir", type=Path)
    args = parser.parse_args()
    packet = json.loads((ROOT / "docs/behavior/unit-facing-after-commands-cases.json").read_text())
    check(packet)
    if args.private_dir:
        check_private(packet, args.private_dir)
    print("30 original retained-facing controls verified")
