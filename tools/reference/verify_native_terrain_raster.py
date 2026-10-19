"""Compile the production C++ raster and compare four original indexed frames.

Requires an installed stock TEXTURE.PCX and a GCC/Clang-compatible C++ driver.
Original texture pixels and generated frames stay in a temporary directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--texture-pcx", required=True)
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    reference = root / "docs/behavior"
    cases = json.loads((reference / "ordinary-land-triangle-raster-cases.json").read_text(encoding="utf-8"))
    replay = runpy.run_path(str(reference / "terrain_triangle_raster_reference.py"))
    texture = replay["pcx_crop"](args.texture_pcx, cases["texture"]["crop"])
    if hashlib.sha256(texture).hexdigest() != cases["texture"]["indexed_sha256"]:
        raise ValueError("Owned texture crop differs from the native receipt")
    with tempfile.TemporaryDirectory(prefix="glsmac-terrain-raster-") as directory:
        temp = Path(directory)
        source = temp / "texture.bin"
        source.write_bytes(texture)
        probe = temp / "probe"
        subprocess.run([args.cxx, "-std=c++17", "-O2", "-I", str(root / "src"),
                        str(root / "src/game/frontend/SmacTerrainRaster.cpp"),
                        str(root / "tools/reference/terrain_raster_probe.cpp"),
                        "-o", str(probe)], check=True)
        result = subprocess.run([str(probe), str(source), str(temp / "frame-")], check=True,
                                capture_output=True, text=True)
        counts = dict(line.split(":") for line in result.stdout.splitlines())
        for i, case in enumerate(cases["cases"]):
            pixels = (temp / f"frame-{i}.bin").read_bytes()
            digest = hashlib.sha256(pixels).hexdigest()
            if digest != case["frame_sha256"] or int(counts[str(i)]) != case["pixel_write_count"]:
                raise ValueError(f"Production raster differs from native frame: {case['id']}")
            print(f"PASS {case['id']}: {digest}")


if __name__ == "__main__":
    main()
