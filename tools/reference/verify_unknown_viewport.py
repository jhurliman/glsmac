"""Compare production unknown-view scheduling/backing with original receipts.

Only the purchased Planet map and supplied exploration footprint enter the C++
probe. Captured queries, cache bytes and line geometry are comparison targets.
Partial-known raster pixels are deliberately outside this probe's art profile.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--planet', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--receipt', type=Path)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    receipt = args.receipt or root/'docs/behavior/unknown-map-query-order-and-backing-cases.json'
    data = json.loads(receipt.read_text())
    sha = lambda value: hashlib.sha256(value).hexdigest()
    if sha(args.planet.read_bytes()) != data['map_sha256']:
        raise ValueError('Planet input differs from the original control')
    args.out.mkdir(parents=True, exist_ok=True)
    sources = ['tools/reference/unknown_viewport_probe.cpp',
               'src/game/frontend/NativeTerrainRenderer.cpp', 'src/game/frontend/SmacTerrainRaster.cpp',
               'src/game/frontend/SmacLandMaterial.cpp', 'src/game/frontend/SmacWaterCoast.cpp',
               'src/game/frontend/SmacTerritoryBoundary.cpp', 'src/game/frontend/SmacRiver.cpp',
               'src/game/frontend/SmacRoadTube.cpp', 'src/game/frontend/SmacDevelopedOverlay.cpp',
               'src/game/backend/map/SmacTerrainLighting.cpp',
               'src/game/backend/resource/SmacResourceBonus.cpp', 'src/game/backend/resource/SmacSiteSignal.cpp']
    command = [args.cxx, '-std=c++17', '-O1', '-g', '-I', str(root/'src'),
               *[str(root/source) for source in sources], '-o', str(args.out/'probe')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run(command, check=True)
    counts = {'queries': 0, 'outlines': 0, 'cache_bytes': 0, 'pixels': 0}
    cases = data['cases']
    for sequence in range(2):
        out = args.out/str(sequence)
        out.mkdir(exist_ok=True)
        known = bytearray(3200)
        if sequence:
            for dx, dy in data['supplied21_offsets']:
                x, y = (22+dx) % 80, 38+dy
                known[y*40+x//2] = 1
        (out/'known.bin').write_bytes(known)
        subprocess.run([str(args.out/'probe'), str(args.planet), str(out/'known.bin'), str(out)], check=True)
        for frame in range(4):
            case = cases[sequence*4+frame]
            stem = out/str(frame)
            queries = []
            for row in Path(str(stem)+'.queries').read_text().splitlines():
                kind, x, y, vertex, a, b = map(int, row.split())
                queries.append({'kind': ['height', 'shade', 'projector'][kind],
                                'args': [x, y] if kind == 2 else [x, y, vertex],
                                'result': [a, b] if kind == 2 else a})
            digest = sha(json.dumps(queries, separators=(',', ':')).encode())
            if digest != case['derived_query_sha256']:
                raise AssertionError(f"Query sequence/returns differ: {case['id']} ({digest})")
            lines = [list(map(int, row.split())) for row in Path(str(stem)+'.lines').read_text().splitlines()]
            if lines != case['unknown_grid_lines']:
                mismatch = next((i for i, pair in enumerate(zip(lines, case['unknown_grid_lines'])) if pair[0] != pair[1]), None)
                raise AssertionError(f"Unknown outline differs: {case['id']}, first mismatch {mismatch}")
            cache = Path(str(stem)+'.cache').read_bytes()
            if sha(cache) != case['semantic_cache_sha256']:
                raise AssertionError(f"Geometry cache differs: {case['id']}")
            counts['queries'] += len(queries)
            counts['outlines'] += len(lines)
            counts['cache_bytes'] += len(cache)
            if not sequence:
                pixels = Path(str(stem)+'.indexed').read_bytes()
                if sha(pixels) != case['indexed_frame_sha256']:
                    raise AssertionError(f"Retained unknown pixels differ: {case['id']}")
                counts['pixels'] += len(pixels)
            print('PASS', case['id'])
    (args.out/'comparison.json').write_text(json.dumps(counts, indent=2)+'\n')
    print('PASS production query/outline/cache/backing comparisons:', counts)


if __name__ == '__main__':
    main()
