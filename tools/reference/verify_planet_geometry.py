"""Verify authored Planet cache traversal and painter geometry against original returns."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--planet-map',required=True)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    frozen=json.loads((root/'docs/behavior/planet-negative-landmark-full-cases.json').read_text())
    asset=Path(args.planet_map)
    if hashlib.sha256(asset.read_bytes()).hexdigest()!=frozen['asset_sha256']:
        raise ValueError('Planet asset differs from the original receipt')
    with tempfile.TemporaryDirectory(prefix='glsmac-planet-geometry-') as directory:
        temp=Path(directory);renderer=frozen['renderer']
        (temp/'renderer-input.txt').write_text(str(len(renderer))+'\n'+'\n'.join(' '.join(map(str,c['tile'])) for c in renderer))
        command=[args.cxx,'-std=c++17','-Wall','-Wextra','-Werror','-I',str(root/'src'),
            str(root/'tools/reference/planet_geometry_probe.cpp'),str(root/'src/game/backend/map/SmacTerrainLighting.cpp'),'-o',str(temp/'probe')]
        if args.sanitize:command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])
        subprocess.run(command,check=True);subprocess.run([str(temp/'probe'),str(asset),str(temp)],check=True)
        for traversal in frozen['traversals']:
            reverse=int(traversal['reverse'])
            actual=[list(map(int,line.split())) for line in (temp/f'traversal-{reverse}.txt').read_text().splitlines()]
            expected=[c['tile']+c['projection']+c['heights']+c['shades'] for c in traversal['results']]
            if len(actual)!=len(expected):raise RuntimeError('Truncated whole-map traversal')
            for index,(a,e) in enumerate(zip(actual,expected)):
                if a!=e:raise RuntimeError(f'Traversal reverse={reverse}, tile {e[:2]}: expected {e}, got {a}')
        actual=[list(map(int,line.split())) for line in (temp/'renderer.txt').read_text().splitlines()]
        expected=[c['tile']+c['anchor']+[value for d in c['draws'] for value in d['vertices']+d['shade']] for c in renderer]
        if actual!=expected:
            for a,e in zip(actual,expected):
                if a!=e:raise RuntimeError(f'Painter tile {e[:2]}: expected {e}, got {a}')
            raise RuntimeError('Truncated original painter controls')
    print('PASS:6400 projectors,32000 heights,32000 shades and508 original primary triangles')


if __name__=='__main__':main()
