"""Replay known nonzero-zoom terrain controls through the production material selector."""
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
    data=json.loads((root/'docs/behavior/native-zoom-material-cases.json').read_text())
    sha=lambda b:hashlib.sha256(b).hexdigest()
    for name,digest in data['asset_hashes'].items():
        if sha((args.asset_directory/name).read_bytes())!=digest:raise ValueError(f'Owned asset differs: {name}')
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
    subprocess.run(command,check=True)
    outputs={}
    for zoom in [-1,1,3,8]:
        output=subprocess.run([str(exe),str(args.out),str(zoom)],capture_output=True,text=True,check=True)
        outputs[zoom]=output.stdout;print(output.stdout,end='')
    pixels=0
    for i,c in enumerate(data['cases']):
        zoom=c['profile']['zoom'];name=f'frame-{[0,2,3][i]}.bin' if zoom<0 else f'tile-z{zoom}.indexed'
        if zoom>0:
            match=re.search(r'tile (\d+) stores (\d+)',outputs[zoom])
            if not match or int(match[2])!=c['store_count']:raise AssertionError((c['input']['id'],'paint stores'))
        raw=(args.out/name).read_bytes()
        if sha(raw)!=c['visible_frame_sha256']:raise AssertionError((c['input']['id'],'pixels',sha(raw),c['visible_frame_sha256']))
        pixels+=len(raw);print('PASS',c['input']['id'])
    print(f'PASS: 6 original known-material controls, {pixels} complete indexed pixels (3 mixed viewports; 3 clipped single tiles)')


if __name__=='__main__':main()
