"""Check production C++ opaque/keyed batches against 27 complete native frames.

Requires stock owned TEXTURE.PCX and a GCC/Clang-compatible C++ driver.
Loaded texture pixels and output frames stay in a temporary directory.
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
    cases = json.loads((reference / "ordinary-land-keyed-raster-cases.json").read_text(encoding="utf-8"))
    replay = runpy.run_path(str(reference / "terrain_triangle_raster_reference.py"))
    with tempfile.TemporaryDirectory(prefix="glsmac-terrain-polygons-") as directory:
        temp = Path(directory)
        for frame_index, frame in enumerate(cases["frames"]):
            for index, draw in enumerate(frame["draws"]):
                if draw["asset"] == "synthetic_key_pattern":
                    pixels = bytes([9, 10, 255, 1][(x+y) % 4] for y in range(56) for x in range(56))
                else:
                    pixels = replay["pcx_crop"](args.texture_pcx, draw["source_crop"])
                if hashlib.sha256(pixels).hexdigest() != draw["loaded_texture_sha256"]:
                    raise ValueError(f"Loaded crop differs from native receipt: {frame['id']}/{index}")
                (temp / f"texture-{frame_index}-{index}.bin").write_bytes(pixels)
        probe = temp / "probe"
        subprocess.run([args.cxx, "-std=c++17", "-O2", "-I", str(root / "src"),
                        str(root / "src/game/frontend/SmacTerrainRaster.cpp"),
                        str(root / "tools/reference/terrain_polygon_raster_probe.cpp"),
                        "-o", str(probe)], check=True)
        result = subprocess.run([str(probe), str(temp), str(temp / "frame-")], check=True,
                                capture_output=True, text=True)
        counts = {parts[0]: tuple(map(int, parts[1:])) for line in result.stdout.splitlines()
                  if (parts := line.split(":"))}
        for index, frame in enumerate(cases["frames"]):
            pixels = (temp / f"frame-{index}.bin").read_bytes()
            expected = (sum(draw["attempt_count"] for draw in frame["draws"]),
                        sum(draw["write_count"] for draw in frame["draws"]))
            digest = hashlib.sha256(pixels).hexdigest()
            if digest != frame["frame_sha256"] or counts[str(index)] != expected:
                raise ValueError(f"Production raster differs from native frame: {frame['id']}")
            print(f"PASS {frame['id']}: {digest}")


if __name__ == "__main__":
    main()
