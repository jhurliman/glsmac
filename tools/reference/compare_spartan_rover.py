"""Compare the owned Spartan Rover/material adapter with retained original outputs.

Only installed assets and public view inputs reach production. Original palette,
shade, camera, normalized light and kernel bytes are never supplied to the port.
The original target-culled case compares metadata only: target copy is separate.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import struct
import tempfile

from compare_cvr_pixels import installed_asset

PINS = {
    "cases": "eacd4d9d32e6e32768fa4cc44b6b58eec81158c876d4598a402973a0a119b7b0",
    "composition": "24131603b688b75a0c467bb9cba59a5e098ecab9628f2b48e1a9cc725c3e1329",
    "inputs": "c3af2e46ead2bd1b73d37b7dd066c0095c13c01f8540d56c5fce5dea953db418",
    "report": "9d5619b3e4b0fadf59c7efd6d5d0398acb008f1c8229c78999a3740d62c39a6f",
    "composition_report": "81615c245dd57b2da520c104400d12f6535edb8dfd99cf81491abbf381f838ab",
    "detail_cases": "ad74cb962164657691e6f8310c5cebb8c8b7cf8332ffd354f32847139cac60d1",
}
EXE = "0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01"
PALETTE = "54341b1bee31736997950e432ff3312a620f7cf722016c154098a4c78d28f866"
BODY = "00eedcd3a2a666d704dffc42551be497150b666343cc10819ae5a66a1fae9c8b"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("probe", "assets", "output", *PINS):
        parser.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    parser.add_argument("--material-only", action="store_true")
    args = parser.parse_args()
    evidence = {}
    for name, expected in PINS.items():
        raw = getattr(args, name).read_bytes()
        if sha(raw) != expected:
            raise ValueError(f"Unexpected evidence fingerprint: {name}")
        evidence[name] = json.loads(raw)
    cases, composition = evidence["cases"], evidence["composition"]
    report, composition_report = evidence["report"], evidence["composition_report"]
    if (report["executable_sha256"] != EXE or composition_report["executable_sha256"] != EXE
            or not report["passed"] or composition_report["passed"] != 20
            or len(cases["colors"]) != 15 or len(cases["views"]) != 20
            or cases["colors"] != report["colors"]
            or [v["id"] for v in cases["views"]] != [v["id"] for v in report["views"]]):
        raise ValueError("Unexpected original identity, ordered cases or completion")
    palette, body = (installed_asset(args.assets, name) for name in ("PALETTE.PCX", "VBP.CVR"))
    if sha(palette.read_bytes()) != PALETTE or sha(body.read_bytes()) != BODY:
        raise ValueError("Unexpected source palette/body")
    asset_hashes = {v["name"].lower(): v["sha256"] for v in composition_report["loaded_files"]}
    for name in composition["cases"][0]["expected"]["component_files"]:
        if sha(installed_asset(args.assets, name).read_bytes()) != asset_hashes[name.lower()]:
            raise ValueError("Unexpected composition asset: " + name)
    if sha(installed_asset(args.assets, "system.txt").read_bytes()) != report["system_sha256"]:
        raise ValueError("Unexpected system source")
    palette_rgb = [None] * 256
    source_rgb = palette.read_bytes()[-768:]
    for index in range(236):
        palette_rgb[index + 10] = source_rgb[index * 3:index * 3 + 3]
    for reserved in evidence["inputs"]["reserved_software_palette_entries"]:
        palette_rgb[reserved["index"]] = bytes(reserved["rgb"])
    if any(color is None for color in palette_rgb):
        raise ValueError("Incomplete palette input contract")
    expected_rgb = b"".join(palette_rgb)
    materials, views, details, failures = [], [], [], []
    with tempfile.TemporaryDirectory(prefix="spartan-rover-") as temporary:
        output = Path(temporary) / "output.bin"
        for case in cases["colors"]:
            faction = installed_asset(args.assets, case["inputs"]["faction_file"])
            if sha(faction.read_bytes()) != case["expected"]["pcx_sha256"]:
                raise ValueError("Unexpected faction artwork")
            run = subprocess.run([str(args.probe.resolve()), "--material", str(palette), str(faction),
                                  str(body), str(output)], capture_output=True, text=True, timeout=30, check=True)
            data = output.read_bytes()
            if run.stderr or run.stdout.strip() != "6913" or len(data) != 6913:
                raise ValueError("Unexpected material framing")
            rgb, shades, tint = data[:768], data[768:6912], data[-1]
            expected = case["expected"]
            if (rgb != expected_rgb or sha(shades) != expected["material_row_sha256"] or tint != expected["vehicle_tint_index"]
                    or list(rgb[tint * 3:tint * 3 + 3]) != expected["vehicle_tint_rgb"]):
                failures.append(case["id"])
            for reserved in evidence["inputs"]["reserved_software_palette_entries"]:
                i = reserved["index"]
                if list(rgb[i * 3:i * 3 + 3]) != reserved["rgb"]:
                    failures.append(case["id"] + "_reserved_" + str(i))
            materials.append({"id": case["id"], "material_sha256": sha(shades), "tint": tint})
        if not args.material_only:
            lod = evidence["detail_cases"]
            lights = installed_asset(args.assets, lod["asset"])
            if sha(lights.read_bytes()) != lod["asset_sha256"]:
                raise ValueError("Unexpected detail asset")
            synthetic = Path(temporary) / "owned-detail.bin"
            synthetic.write_bytes(bytes.fromhex(lod["synthetic"]["payload_hex"]))
            view_queries = [{"id": case["id"], "scale_bits": struct.unpack("<I", struct.pack("<f", case["scale"]))[0],
                             "selected_level": case["selected_level"]} for case in lod["recon_scale_cases"]]
            for mode, path, queries in (("--details", lights, lod["installed_selection_cases"] + view_queries),
                                        ("--details-raw", synthetic, lod["synthetic"]["cases"])):
                for index, query in enumerate(queries):
                    run = subprocess.run([str(args.probe.resolve()), mode, str(path), str(output), str(query["scale_bits"])],
                                         capture_output=True, text=True, timeout=30, check=True)
                    data, values = output.read_bytes(), list(map(int, run.stdout.split()))
                    if run.stderr or values != [query["selected_level"], 5 if mode == "--details" else 3, len(data)]:
                        failures.append("detail_" + mode + "_" + str(index))
                    if mode == "--details-raw":
                        if data != synthetic.read_bytes():
                            failures.append("owned_detail_bytes")
                    else:
                        if len(data) != 11689 or sha(data) != "a2132d0f984086b5e09068a3c45b58a7bd1885d84d8f07de832688f19f8a91aa":
                            failures.append("loaded_detail_bytes")
                        for level in lod["levels"]:
                            offset, size = level["offset_in_chain"], level["next_level_offset"]
                            block = data[offset:offset + size]
                            if sha(block) != level["local_block_sha256"] or sha(block[42:-5]) != level["records_sha256"]:
                                failures.append("detail_block_" + str(level["level"]))
                    details.append({"profile": mode, "scale_bits": query["scale_bits"], "selected_level": values[0], "owned_bytes_sha256": sha(data)})
            for case, prior in zip(cases["views"], composition["cases"]):
                value = case["inputs"]
                if value != prior["inputs"]:
                    raise ValueError("Original view pairing changed")
                argv = [value["facing"], value["zoom"], *value["ratio"], *value["anchor"]]
                run = subprocess.run([str(args.probe.resolve()), "--render", str(args.assets.resolve()),
                                      str(output), *map(str, argv)], capture_output=True, text=True, timeout=60, check=True)
                data, stats = output.read_bytes(), list(map(int, run.stdout.split()))
                if run.stderr or len(data) != 458752 or len(stats) != 6 or stats[-1] != len(data):
                    raise ValueError("Unexpected renderer framing")
                color, depth, rgba = data[:65536], data[65536:196608], data[196608:]
                expected = case["expected"]
                compared = expected["kernel_count"] != 0
                if stats[3:5] != prior["expected"]["target_anchor"]:
                    failures.append(case["id"] + "_anchor")
                if compared and (stats[0] != expected["kernel_count"] or sha(color) != expected["indexed_sha256"] or sha(depth) != expected["depth_sha256"]):
                    failures.append(case["id"])
                expected_rgba = b"".join(palette_rgb[index] + bytes([0 if index == 9 else 255]) for index in color)
                if rgba != expected_rgba:
                    failures.append(case["id"] + "_keyed_rgba")
                views.append({"id": case["id"], "original_internal_compared": compared,
                              "indexed_sha256": sha(color), "depth_sha256": sha(depth), "rgba_sha256": sha(rgba),
                              "draws_records_writes": stats[:3], "target_anchor": stats[3:5]})
    result = {"evidence": PINS, "executable_sha256": EXE, "probe_sha256": sha(args.probe.read_bytes()),
              "material_projections": len(materials), "metadata_views": len(views),
              "drawing_views": sum(v["original_internal_compared"] for v in views),
              "detail_queries": len(details), "failures": failures, "materials": materials, "details": details, "views": views}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in ("material_projections", "metadata_views", "drawing_views", "detail_queries", "failures")}))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
