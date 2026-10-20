#!/usr/bin/env python3
"""Verify production terrain picking against original numeric controls; no assets needed."""
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
    data = json.loads((root / 'docs/behavior/native-terrain-picking-cases.json').read_text())
    out = Path(args.output_directory)
    out.mkdir(parents=True, exist_ok=True)
    command = [args.cxx, '-std=c++17', '-O1', '-g', '-I', str(root / 'src'),
               str(root / 'tools/reference/native_terrain_picking_probe.cpp'),
               str(root / 'src/game/backend/map/SmacTerrainLighting.cpp'), '-o', str(out / 'probe')]
    if args.sanitize:
        command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    subprocess.run(command, check=True)
    points = queries = edges = ties = 0
    for case in data['profiles']:
        spec = case['input']
        header = [*case['world'], *case['viewport'], *case['camera'], spec['zoom'],
                  8 if spec.get('low_relief') else 25, int(not spec.get('flat', False)),
                  ['plain', 'hill', 'slope', 'coast'].index(spec['pattern']), len(case['pixels'])]
        path, result = out / 'input.txt', out / 'result.bin'
        text = [' '.join(map(str, header))]
        text += [' '.join(map(str, row)) for row in case['pixels']]
        path.write_bytes(('\n'.join(text) + '\n').encode())
        subprocess.run([str(out / 'probe'), str(path), str(result)], check=True)
        if hashlib.sha256(result.read_bytes()).hexdigest() != case['event_sha256']:
            raise ValueError(f'Original picker result/query/edge stream differs: {spec["id"]}')
        points += case['counts']['points']
        queries += case['counts']['queries']
        edges += case['counts']['edges']
        ties += case['counts']['equalities']
    print(f'PASS: {len(data["profiles"])} profiles, {points} picks, {queries} height queries, {edges} edges, {ties} equalities')


if __name__ == '__main__':
    main()
