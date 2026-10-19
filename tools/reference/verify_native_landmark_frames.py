"""Replay443 original landmark indexed frames through the production rasterizer.

This consumes captured commands; the separate landmark selector verifier checks
C++ named-art submissions. It does not prove the complete live scene assembler.

Reads purchased assets externally; temporary pixel buffers are not published.
"""
from pathlib import Path
import argparse,hashlib,json,runpy,struct,subprocess,sys,tempfile


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-directory',required=True)
    parser.add_argument('--cxx',default='c++');parser.add_argument('--sanitize',action='store_true')
    args=parser.parse_args();root=Path(__file__).resolve().parents[2];data=root/'docs/behavior'
    sys.path.insert(0,str(data));assets=Path(args.asset_directory)
    cases=json.loads((data/'named-landmark-quad-marker-art-cases.json').read_text(encoding='utf-8'))
    material=json.loads((data/'landmark-material-cases.json').read_text(encoding='utf-8'))
    if cases['shadow']!=material['shadow'] or cases['viewport']!=material['viewport'] or cases['background']!=material['background']:
        raise ValueError('Inconsistent captured landmark raster state')
    cases['frames']+=material['frames']
    cases['producer_cases']=[]
    prior=json.loads((data/'territory-boundary-cases.json').read_text(encoding='utf-8'))
    pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']
    palette=runpy.run_path(str(data/'territory_boundary_reference.py'))['palette_rgb'](assets/'PALETTE.PCX',prior['tables']['reserved_palette_entries'])
    modes={'opaque_shaded':0,'keyed_shaded':1,'keyed_unlit':2,'depth_below':3,'coast_mapped':4,'sprite_plain':6,'sprite_shadow':7}
    def dimensions(path):
        x0,y0,x1,y1=struct.unpack_from('<4H',path.read_bytes(),4)
        return x1-x0+1,y1-y0+1
    def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    with tempfile.TemporaryDirectory(prefix='glsmac-developed-') as directory:
        temp=Path(directory);tw,th=dimensions(assets/'TER1.PCX');rw,rh=dimensions(assets/'RAINFALL.PCX')
        (temp/'ter1.bin').write_bytes(pcx(assets/'TER1.PCX',[0,0,tw,th]))
        (temp/'rainfall.bin').write_bytes(pcx(assets/'RAINFALL.PCX',[0,0,rw,rh]))
        (temp/'palette.bin').write_bytes(bytes(c for rgb in palette for c in rgb))
        numbers=[tw,th,rw,rh,0]
        numbers.append(len(cases['frames']));cache={}
        for index,frame in enumerate(cases['frames']):
            numbers.extend([*frame['clip'],cases['background'],len(frame['draws'])])
            for i,d in enumerate(frame['draws']):
                numbers.append(modes[d['mode']])
                if d['mode'].startswith('sprite_'):
                    numbers.extend([*d['binding']['crop'],*d['anchor']]);continue
                crop=tuple(d['source_crop']);numbers.extend(crop)
                if crop not in cache:cache[crop]=pcx(assets/'TEXTURE.PCX',crop)
                pixels=cache[crop]
                if hashlib.sha256(pixels).hexdigest()!=d['loaded_source_sha256']:raise ValueError('Loaded terrain differs')
                (temp/f'texture-{index}-{i}.bin').write_bytes(pixels)
                count=len(d['vertices'])//2;numbers.append(count)
                for v in range(count):numbers.extend([*d['vertices'][2*v:2*v+2],*d['uv'][2*v:2*v+2],d['values'][v] if d['values'] else 0])
                numbers.extend(d['shade_limits'])
                if d['mode']=='coast_mapped':numbers.extend(d['mask_mapping'])
        (temp/'input.txt').write_text(' '.join(map(str,numbers)),encoding='ascii')
        flags=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if args.sanitize else ['-O2']
        sources=['tools/reference/developed_overlay_probe.cpp','src/game/frontend/SmacDevelopedOverlay.cpp','src/game/frontend/SmacWaterCoast.cpp','src/game/frontend/SmacTerrainRaster.cpp']
        subprocess.run([args.cxx,'-std=c++17',*flags,'-I',str(root/'src'),*[str(root/s) for s in sources],'-o',str(temp/'probe')],check=True)
        result=subprocess.run([str(temp/'probe'),str(temp)],capture_output=True,text=True)
        if result.returncode:raise RuntimeError(str(result.returncode)+' '+result.stderr+' '+result.stdout[-1000:])
        if (temp/'shadow-out.bin').read_bytes()!=bytes(cases['shadow']):raise ValueError('Native shadow lookup differs')
        counts={}
        for line in result.stdout.splitlines():
            parts=line.split();kind=parts[0];values=list(map(int,parts[1:]))
            if kind=='F':counts[values[0]]=tuple(values[1:]);continue
            index,i,*geometry=values;binding=cases['frames'][index]['draws'][i]['binding']
            if geometry!=binding['offset']+binding['size'] or digest(temp/f'sprite-{index}-{i}.bin')!=binding['retained_sha256']:
                raise ValueError('Original retained sprite differs')
        for index,frame in enumerate(cases['frames']):
            expected_counts=tuple(sum(d[field] for d in frame['draws']) for field in ['attempt_count','write_count'])
            if digest(temp/f'frame-{index}.bin')!=frame['frame_sha256'] or counts[index]!=expected_counts:
                raise ValueError('Original indexed frame differs: '+frame['id']+' '+str(counts[index])+' '+str(expected_counts))
            print('PASS',frame['id'])
        print('PASS443 complete original indexed frames and256 shadow entries; captured-command replay, not live scene parity')


if __name__=='__main__':main()
