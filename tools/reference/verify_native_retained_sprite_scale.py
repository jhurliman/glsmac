"""Replay seven original retained-art tiles through production selection and scaling."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2]
    data=json.loads((root/'docs/behavior/native-retained-sprite-scale-cases.json').read_text())
    for name,digest in data['asset_hashes'].items():
        if hashlib.sha256((args.asset_directory/name).read_bytes()).hexdigest()!=digest:raise ValueError(f'Owned asset differs: {name}')
    args.out.mkdir(parents=True,exist_ok=True)
    subprocess.run([sys.executable,str(root/'tools/reference/capture_native_viewport.py'),
                    '--asset-directory',str(args.asset_directory),'--output-directory',str(args.out),
                    '--variation-low16',str(data['map_base_low16']),'--initial-index','65','--viewer','3',
                    '--exploration-profile','partial21','--prepare-only'],check=True)
    sources=['tools/reference/native_zoom_material_probe.cpp','src/game/frontend/NativeTerrainRenderer.cpp',
             'src/game/frontend/SmacTerrainRaster.cpp','src/game/frontend/SmacLandMaterial.cpp',
             'src/game/frontend/SmacWaterCoast.cpp','src/game/frontend/SmacTerritoryBoundary.cpp',
             'src/game/frontend/SmacRiver.cpp','src/game/frontend/SmacRoadTube.cpp',
             'src/game/frontend/SmacDevelopedOverlay.cpp','src/game/backend/map/SmacTerrainLighting.cpp',
             'src/game/backend/resource/SmacResourceBonus.cpp','src/game/backend/resource/SmacSiteSignal.cpp']
    exe=args.out/'probe';command=['g++','-std=c++17','-O1','-g','-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(exe)]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run(command,check=True);pixels=stores=0
    for c in data['cases']:
        z=c['profile']['zoom'];items=c['input']['items']
        output=subprocess.run([str(exe),str(args.out),str(z),str(items)],capture_output=True,text=True,check=True)
        raw=(args.out/f'retained-z{z}-items{items}.indexed').read_bytes()
        if hashlib.sha256(raw).hexdigest()!=c['frame_sha256']:raise AssertionError((c['input']['id'],'frame'))
        count=int(re.search(r'stores (\d+)',output.stdout)[1])
        if count!=c['store_count']:raise AssertionError((c['input']['id'],'stores',count,c['store_count']))
        pixels+=len(raw);stores+=count;print('PASS',c['input']['id'])
    print(f'PASS: 7 original retained tiles, {pixels} indexed pixels, {stores} paint stores')


if __name__=='__main__':main()
