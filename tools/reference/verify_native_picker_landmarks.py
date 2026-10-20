#!/usr/bin/env python3
"""Compare production landmark/saturation picker traces and all height-cache bytes."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory', required=True)
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    data = json.loads((root / 'docs/behavior/native-picker-landmarks-cases.json').read_text())
    out = Path(args.output_directory)
    out.mkdir(parents=True, exist_ok=True)
    command = [args.cxx, '-std=c++17', '-O1', '-g', '-I', str(root / 'src'),
               str(root / 'tools/reference/native_terrain_picking_probe.cpp'),
               str(root / 'src/game/backend/map/SmacTerrainLighting.cpp'), '-o', str(out / 'probe')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run(command, check=True)
    totals = dict.fromkeys(['points', 'queries', 'edges', 'equalities'], 0)
    for case in data['profiles']:
        spec = case['input']
        text = [' '.join(map(str, [80, 80, 1024, 511, *spec['camera'], spec['zoom'],
                                  25, 1, 0, len(case['controls'])]))]
        if spec['terrain'] == 'planet':
            cells = zip(data['planet_metadata']['bands'], data['planet_metadata']['words'])
        else:
            cells = [(spec['band'], 0)] * 3200
        text += [f'{band} {word}' for band, word in cells]
        text += [' '.join(map(str, control)) for control in case['controls']]
        path, result = out / 'input.txt', out / 'result.bin'
        path.write_bytes(('\n'.join(text) + '\n').encode())
        subprocess.run([str(out / 'probe'), str(path), str(result), '--landmarks'], check=True)
        if hashlib.sha256(result.read_bytes()).hexdigest() != case['event_and_cache_sha256']:
            raise ValueError(f'Original result/query/edge/cache stream differs: {spec["id"]}')
        for key in totals:
            totals[key] += case['counts'][key]
    assert list(totals.values()) == [data[key] for key in
                                   ['picks', 'height_queries', 'edges', 'equality_branches']]
    print(f'PASS: {len(data["profiles"])} profiles, {totals["points"]} picks, '
          f'{totals["queries"]} height queries, {totals["edges"]} edges, '
          f'{totals["equalities"]} equalities; all 16000 height bytes after every pick')


if __name__ == '__main__':
    main()
