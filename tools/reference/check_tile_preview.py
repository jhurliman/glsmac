"""Compare the production independent preview painter to seven original frames.

Purchased art and generated indexed output stay external to the repository.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--asset-directory',required=True)
    p.add_argument('--output-directory',required=True)
    p.add_argument('--cxx',default='c++')
    p.add_argument('--sanitize',action='store_true')
    args=p.parse_args();root=Path(__file__).resolve().parents[2]
    assets=Path(args.asset_directory);out=Path(args.output_directory);out.mkdir(parents=True,exist_ok=True)
    data=root/'docs/behavior';pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']
    dimensions=[]
    for name,file in [('texture','TEXTURE.PCX'),('ter1','TER1.PCX')]:
        path=assets/file;x0,y0,x1,y1=struct.unpack_from('<4H',path.read_bytes(),4)
        w,h=x1-x0+1,y1-y0+1;dimensions.extend([w,h])
        (out/(name+'.bin')).write_bytes(bytes(pcx(path,[0,0,w,h])))
    shadow=json.loads((data/'named-landmark-quad-marker-art-cases.json').read_text())['shadow']
    (out/'shadow.bin').write_bytes(bytes(shadow));(out/'input.txt').write_text(' '.join(map(str,dimensions)))
    sources=['tools/reference/tile_preview_probe.cpp','src/game/frontend/SmacTilePreview.cpp',
        'src/game/frontend/SmacTerrainRaster.cpp','src/game/frontend/SmacLandMaterial.cpp',
        'src/game/frontend/SmacRoadTube.cpp','src/game/frontend/SmacDevelopedOverlay.cpp',
        'src/game/backend/resource/SmacResourceBonus.cpp','src/game/backend/resource/SmacSiteSignal.cpp']
    command=[args.cxx,'-std=c++17','-O1','-g','-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(out/'probe')]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run(command,check=True)
    result=subprocess.run([str(out/'probe'),str(out)],check=True,capture_output=True,text=True)
    counts=[int(line.split()[1]) for line in result.stdout.splitlines()]
    receipt=json.loads((data/'lower-console-terrain-preview-cases.json').read_text())
    for i,case in enumerate(receipt['cases']):
        digest=hashlib.sha256((out/f'frame-{i}.bin').read_bytes()).hexdigest()
        if counts[i]!=case['store_count'] or digest!=case['frame_sha256']:
            raise RuntimeError(f"{case['input']['id']}: stores {counts[i]}, frame {digest}")
        print('PASS',case['input']['id'],counts[i],digest)
    print('PASS seven full original indexed previews, unknown-memory and unsupported-profile guards')


if __name__=='__main__':main()
