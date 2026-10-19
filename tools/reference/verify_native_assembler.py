"""Assemble original tile inputs through NativeTerrainRenderer and compare indices.

No captured draw commands are injected. Captured resolved nutrient totals are
inputs; gameplay nutrient rules and actual backend snapshot transport have their
own checks. An injective diagnostic palette recovers exact output indices while
the original shadow lookup remains unchanged. This is not whole-screen parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import subprocess
import tempfile
from contextlib import nullcontext


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory',required=True)
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--output-dir',help='Keep generated probe inputs and frames for mismatch diagnosis')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2];data=root/'docs/behavior'
    filenames=['named-landmark-quad-marker-art-cases.json','landmark-material-cases.json',
               'resource-pod-monolith-art-cases.json']
    collections=[json.loads((data/name).read_text()) for name in filenames]
    cases=[]
    for collection in collections:
        if collection['shadow']!=collections[0]['shadow']:raise ValueError('Mixed native shadow state')
        frames={f['id']:f for f in collection['frames'] if f['clip']==[0,0,1024,511]}
        for case in collection['producer_cases']:
            frame=frames[case['input']['id']]
            cases.append((case,frame))
    assets=Path(args.asset_directory)
    pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']
    def bitmap(path):
        x0,y0,x1,y1=struct.unpack_from('<4H',path.read_bytes(),4)
        width,height=x1-x0+1,y1-y0+1
        return width,height,pcx(path,[0,0,width,height])
    if args.output_dir:Path(args.output_dir).mkdir(parents=True,exist_ok=True)
    with (nullcontext(args.output_dir) if args.output_dir else tempfile.TemporaryDirectory(prefix='glsmac-native-assembler-')) as directory:
        temp=Path(directory);numbers=[]
        for name,filename in [('texture','TEXTURE.PCX'),('ter1','TER1.PCX'),('rainfall','RAINFALL.PCX')]:
            width,height,pixels=bitmap(assets/filename);numbers.extend([width,height]);(temp/(name+'.bin')).write_bytes(pixels)
        (temp/'shadow.bin').write_bytes(bytes(collections[0]['shadow']))
        numbers.append(len(cases))
        for case,frame in cases:
            spec=case['input'];x,y=spec.get('tile',[40,40])
            numbers.extend([x,y,spec['band'],spec['contour'],spec.get('moisture',0),spec['items'],
                spec.get('remembered_items',spec['items']),spec.get('landmark',0),case.get('resolved_nutrients',0),
                spec.get('pattern',0),int(spec.get('automatic',False)),spec.get('flags',0x221),case['variation_seed_low16'],
                spec.get('center_band',spec['band']),spec.get('center_contour',spec['contour'])])
            patches=spec.get('patch',[]);numbers.append(len(patches))
            for patch in patches:numbers.extend(patch)
            words=spec.get('neighbor_landmarks',[]);numbers.append(len(words))
            for word in words:numbers.extend(word)
        (temp/'input.txt').write_text(' '.join(map(str,numbers)),encoding='ascii')
        sources=['tools/reference/native_assembler_probe.cpp','src/game/frontend/NativeTerrainRenderer.cpp',
            'src/game/frontend/SmacTerrainRaster.cpp','src/game/frontend/SmacLandMaterial.cpp',
            'src/game/frontend/SmacWaterCoast.cpp','src/game/frontend/SmacTerritoryBoundary.cpp',
            'src/game/frontend/SmacRiver.cpp','src/game/frontend/SmacRoadTube.cpp',
            'src/game/frontend/SmacDevelopedOverlay.cpp','src/game/backend/map/SmacTerrainLighting.cpp',
            'src/game/backend/resource/SmacResourceBonus.cpp','src/game/backend/resource/SmacSiteSignal.cpp']
        command=[args.cxx,'-std=c++17','-O1','-g','-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(temp/'probe')]
        if args.sanitize:command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'])
        subprocess.run(command,check=True)
        result=subprocess.run([str(temp/'probe'),str(temp)],capture_output=True,text=True,check=True)
        counts={int(p[0]):tuple(map(int,p[1:])) for line in result.stdout.splitlines() if (p:=line.split())}
        failures=[]
        for index,(case,frame) in enumerate(cases):
            digest=hashlib.sha256((temp/f'frame-{index}.bin').read_bytes()).hexdigest()
            expected=tuple(sum(d[key] for d in frame['draws']) for key in ['attempt_count','write_count'])
            if digest!=frame['frame_sha256'] or counts[index]!=expected:
                failures.append((frame['id'],digest==frame['frame_sha256'],counts[index],expected))
        if failures:raise RuntimeError(f'{len(failures)}/{len(cases)} assembler mismatches; first controls: {failures[:12]}')
    print(f'PASS:{len(cases)} original tile inputs assembled into exact indexed frames and pixel counts')


if __name__=='__main__':main()
