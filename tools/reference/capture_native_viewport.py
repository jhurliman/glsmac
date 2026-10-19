"""Capture production indexed viewport frames from purchased raw Planet and art.

This diagnostic emits real RenderPrepared output; it does not inject captured
queries or draw lists. Comparison receipts are supplied separately. Generated
art-derived pixels remain outside Git.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import struct
import subprocess


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory',required=True)
    parser.add_argument('--output-directory',required=True)
    parser.add_argument('--variation-low16',type=lambda x:int(x,0),required=True)
    parser.add_argument('--initial-index',type=int,required=True)
    parser.add_argument('--automatic-resources',action='store_true')
    parser.add_argument('--observer0-alias',action='store_true',help='Diagnostic only: viewer0 remembered items alias the landmark word')
    parser.add_argument('--reference-receipt',help='Original indexed-pixel receipt; fail if any complete frame differs')
    parser.add_argument('--reference-frame-directory',help='Optional private original frames for mismatch coordinates')
    parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2];data=root/'docs/behavior'
    assets=Path(args.asset_directory);out=Path(args.output_directory);out.mkdir(parents=True,exist_ok=True)
    pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']
    dimensions=[]
    for name,filename in [('texture','TEXTURE.PCX'),('ter1','TER1.PCX'),('rainfall','RAINFALL.PCX')]:
        path=assets/filename;x0,y0,x1,y1=struct.unpack_from('<4H',path.read_bytes(),4)
        width,height=x1-x0+1,y1-y0+1;dimensions.extend([width,height])
        (out/(name+'.bin')).write_bytes(bytes(pcx(path,[0,0,width,height])))
    shadow=json.loads((data/'named-landmark-quad-marker-art-cases.json').read_text())['shadow']
    (out/'shadow.bin').write_bytes(bytes(shadow))
    planet=(assets/'maps/planet.MP').read_bytes()
    pinned=json.loads((data/'planet-negative-landmark-full-cases.json').read_text())['asset_sha256']
    if hashlib.sha256(planet).hexdigest()!=pinned:raise ValueError('Planet asset differs from frozen geometry receipt')
    (out/'planet.bin').write_bytes(planet)
    (out/'input.txt').write_bytes((' '.join(map(str,dimensions+[args.variation_low16,int(args.automatic_resources),args.initial_index,int(args.observer0_alias)]))+'\n').encode())
    sources=['tools/reference/native_viewport_probe.cpp','src/game/frontend/NativeTerrainRenderer.cpp',
        'src/game/frontend/SmacTerrainRaster.cpp','src/game/frontend/SmacLandMaterial.cpp',
        'src/game/frontend/SmacWaterCoast.cpp','src/game/frontend/SmacTerritoryBoundary.cpp',
        'src/game/frontend/SmacRiver.cpp','src/game/frontend/SmacRoadTube.cpp',
        'src/game/frontend/SmacDevelopedOverlay.cpp','src/game/backend/map/SmacTerrainLighting.cpp',
        'src/game/backend/resource/SmacResourceBonus.cpp','src/game/backend/resource/SmacSiteSignal.cpp']
    command=[args.cxx,'-std=c++17','-O1','-g','-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(out/'probe')]
    if args.sanitize:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
    subprocess.run(command,check=True)
    result=subprocess.run([str(out/'probe'),str(out)],text=True,capture_output=True,check=True)
    print(result.stdout,end='')
    failures=[]
    receipt=json.loads(Path(args.reference_receipt).read_text()) if args.reference_receipt else None
    if receipt:
        if receipt['asset_sha256']!=pinned or receipt['map_base_low16']!=args.variation_low16 or receipt['size']!=[1024,511] or len(receipt['cases'])!=4:
            raise ValueError('Reference receipt differs from selected capture profile')
    for i in range(4):
        frame=(out/f'frame-{i}.bin').read_bytes();digest=hashlib.sha256(frame).hexdigest();print(i,digest)
        if receipt and digest!=receipt['cases'][i]['frame_sha256']:
            detail={'frame':i,'id':receipt['cases'][i]['input']['id'],'expected':receipt['cases'][i]['frame_sha256'],'actual':digest}
            if args.reference_frame_directory:
                original=(Path(args.reference_frame_directory)/(detail['id']+'.indexed.bin')).read_bytes()
                if len(original)!=len(frame) or hashlib.sha256(original).hexdigest()!=detail['expected']:
                    raise ValueError('Private original pixels differ from receipt')
                changed=[k for k,(a,b) in enumerate(zip(original,frame)) if a!=b]
                detail.update(changed_pixels=len(changed),first_coordinates=[[k%1024,k//1024,original[k],frame[k]] for k in changed[:8]])
            failures.append(detail)
    (out/'comparison.json').write_bytes((json.dumps({'mismatches':failures},indent=2)+'\n').encode())
    if failures:raise RuntimeError(f'Complete viewport pixel mismatches: {failures}')
    if receipt:print('PASS: all four original complete indexed viewport frames match')


if __name__=='__main__':main()
