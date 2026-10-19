"""Verify production owner CLUTs, classification mask and43 native indexed frames.

Original assets and output pixel buffers remain in a temporary directory.
"""
from pathlib import Path
import argparse,hashlib,json,runpy,struct,subprocess,sys,tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory',required=True);parser.add_argument('--cxx',default='c++')
    parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
    root=Path(__file__).resolve().parents[2];data=root/'docs/behavior';assets=Path(args.asset_directory)
    sys.path.insert(0,str(data));cases=json.loads((data/'territory-boundary-cases.json').read_text(encoding='utf-8'))
    reference=runpy.run_path(str(data/'territory_boundary_reference.py'))
    pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']
    with tempfile.TemporaryDirectory(prefix='glsmac-territory-boundary-') as directory:
        temp=Path(directory);rainfile=assets/'RAINFALL.PCX';header=rainfile.read_bytes()[:128]
        x0,y0,x1,y1=struct.unpack_from('<4H',header,4);width,height=x1-x0+1,y1-y0+1
        (temp/'rainfall.bin').write_bytes(pcx(rainfile,[0,0,width,height]))
        colors=reference['palette_rgb'](assets/'PALETTE.PCX',cases['tables']['reserved_palette_entries'])
        (temp/'palette.bin').write_bytes(bytes(c for rgb in colors for c in rgb))
        targets=[pcx(assets/'PALETTE.PCX',[580+8*i,56,1,1])[0] for i in range(8)]
        if targets!=cases['tables']['global_colors']:raise ValueError('Global owner color samples differ')
        (temp/'targets.bin').write_bytes(bytes(targets));(temp/'system.txt').write_bytes((assets/'system.txt').read_bytes())
        metadata=[]
        for row in cases['tables']['faction_rows']:
            color=pcx(assets/row['asset'],[161,747,1,1])[0];raw=pcx(assets/row['asset'],[161,757,1,1])[0]
            if (color,raw)!=(row['color'],row['raw_opacity']):raise ValueError('Faction metadata differs: '+row['asset'])
            metadata.extend([row['owner'],color,raw])
        (temp/'factions.bin').write_bytes(bytes(metadata));cache={}
        for index,frame in enumerate(cases['frames']):
            for i,draw in enumerate(frame['draws']):
                if draw['mode']=='owner_clut':continue
                crop=tuple(draw['source_crop'])
                if crop not in cache:cache[crop]=pcx(assets/'TEXTURE.PCX',crop)
                pixels=cache[crop]
                if hashlib.sha256(pixels).hexdigest()!=draw['loaded_source_sha256']:raise ValueError('Loaded texture differs')
                (temp/f'texture-{index}-{i}.bin').write_bytes(pixels)
        probe=temp/'probe';flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.sanitize else ['-O2']
        subprocess.run([args.cxx,'-std=c++17',*flags,'-I',str(root/'src'),str(root/'tools/reference/territory_boundary_probe.cpp'),str(root/'src/game/frontend/SmacTerritoryBoundary.cpp'),str(root/'src/game/frontend/SmacWaterCoast.cpp'),str(root/'src/game/frontend/SmacTerrainRaster.cpp'),'-o',str(probe)],check=True)
        result=subprocess.run([str(probe),str(temp),str(temp/'output-'),str(width),str(height)],check=True,capture_output=True,text=True)
        counts={int(parts[0]):tuple(map(int,parts[1:])) for line in result.stdout.splitlines() if (parts:=line.split(':'))}
        for name,key in [('initial','global_clut_sha256'),('active','active_clut_sha256'),('classes','mask_sha256')]:
            if hashlib.sha256((temp/f'output-{name}.bin').read_bytes()).hexdigest()!=cases[key]:raise ValueError('Native '+name+' differs')
        for index,frame in enumerate(cases['frames']):
            digest=hashlib.sha256((temp/f'output-{index}.bin').read_bytes()).hexdigest()
            expected=tuple(sum(draw[field] for draw in frame['draws']) for field in ['attempt_count','write_count'])
            if digest!=frame['frame_sha256'] or counts[index]!=expected:raise ValueError('Native frame differs: '+frame['id'])
            print('PASS',frame['id'],digest)
        print('PASS4096 original initial/active CLUT entries, generated mask and43 native indexed frames')


if __name__=='__main__':main()
