#!/usr/bin/env python3
"""Compare production known-grid rendering against 13 original indexed frames."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory', required=True)
    parser.add_argument('--output-directory', required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    data = root / 'docs/behavior'
    original = json.loads((data / 'known-grid-and-console-fog-cases.json').read_text())
    material = json.loads((data / 'mixed-knowledge-indexed-pixel-cases.json').read_text())
    assets, out = Path(args.asset_directory), Path(args.output_directory)
    out.mkdir(parents=True, exist_ok=True)
    for name, digest in material['asset_hashes'].items():
        if hashlib.sha256((assets / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f'Original asset differs: {name}')
    subprocess.run([sys.executable, str(root / 'tools/reference/capture_native_viewport.py'),
                    '--asset-directory', str(assets), '--output-directory', str(out),
                    '--variation-low16', str(material['map_base_low16']),
                    '--initial-index', '65', '--viewer', '3',
                    '--exploration-profile', 'partial21', '--prepare-only'], check=True)
    sources = ['tools/reference/native_viewport_probe.cpp', 'src/game/frontend/NativeTerrainRenderer.cpp',
               'src/game/frontend/SmacTerrainRaster.cpp', 'src/game/frontend/SmacLandMaterial.cpp',
               'src/game/frontend/SmacWaterCoast.cpp', 'src/game/frontend/SmacTerritoryBoundary.cpp',
               'src/game/frontend/SmacRiver.cpp', 'src/game/frontend/SmacRoadTube.cpp',
               'src/game/frontend/SmacDevelopedOverlay.cpp', 'src/game/backend/map/SmacTerrainLighting.cpp',
               'src/game/backend/resource/SmacResourceBonus.cpp', 'src/game/backend/resource/SmacSiteSignal.cpp']
    command = [args.cxx, '-std=c++17', '-O1', '-g', '-I', str(root / 'src'),
               *[str(root / name) for name in sources], '-o', str(out / 'probe')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run(command, check=True)
    groups = {}
    for case in original['cases']:
        groups.setdefault(case['input']['sequence'], []).append(case)
    pixels = 0
    for sequence, cases in groups.items():
        first = cases[0]['input']
        command = [str(out / 'probe'), str(out), str(first['preference']), str(first['flags'])]
        if first['exploration'] == 'none':
            command += ['unknown']
        subprocess.run(command, check=True)
        for index, case in enumerate(cases):
            frame = (out / f'frame-{index}.bin').read_bytes()
            if len(frame) != 1024 * 511 or hashlib.sha256(frame).hexdigest() != case['indexed_frame_sha256']:
                raise ValueError(f'Complete original frame differs: {case["id"]}')
            (out / f'{case["id"]}.indexed').write_bytes(frame)
            pixels += len(frame)
            print('PASS', case['id'], flush=True)
    print(f'PASS: {sum(map(len, groups.values()))} complete original frames, {pixels} indexed pixels')


if __name__ == '__main__':
    main()
