from pathlib import Path
import argparse,hashlib,json,runpy,struct,subprocess,tempfile
parser=argparse.ArgumentParser(description="Verify production River raster against37 original indexed frames using owned assets.")
parser.add_argument('--texture-pcx',required=True);parser.add_argument('--rainfall-pcx',required=True)
parser.add_argument('--cxx',default='c++');parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
data=root/'docs/behavior';cases=json.loads((data/'ordinary-river-cases.json').read_text())
mask_hashes=json.loads((data/'ordinary-water-coast-cases.json').read_text())['generated_mask_sha256']
pcx=runpy.run_path(str(data/'terrain_triangle_raster_reference.py'))['pcx_crop']

with tempfile.TemporaryDirectory(prefix='glsmac-water-coast-') as directory:
 p=Path(directory);rainfile=Path(args.rainfall_pcx);hdr=rainfile.read_bytes()[:128]
 x0,y0,x1,y1=struct.unpack_from('<4H',hdr,4);rw,rh=x1-x0+1,y1-y0+1
 (p/'rainfall.bin').write_bytes(pcx(rainfile,[0,0,rw,rh]));cache={}
 for index,frame in enumerate(cases['frames']):
  for i,draw in enumerate(frame['draws']):
   crop=tuple(draw['source_crop'])
   if crop not in cache:cache[crop]=pcx(args.texture_pcx,crop)
   source=cache[crop];assert hashlib.sha256(source).hexdigest()==draw['loaded_source_sha256']
   (p/f'texture-{index}-{i}.bin').write_bytes(source)
 probe=p/'probe'
 subprocess.run([args.cxx,'-std=c++17',*(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.sanitize else ['-O2']),'-I',str(root/'src'),str(root/'tools/reference/river_raster_probe.cpp'),str(root/'src/game/frontend/SmacWaterCoast.cpp'),str(root/'src/game/frontend/SmacTerrainRaster.cpp'),'-o',str(probe)],check=True)
 result=subprocess.run([str(probe),str(p),str(p/'output-'),str(rw),str(rh)],check=True,capture_output=True,text=True)
 counts={int(v[0]):tuple(map(int,v[1:])) for line in result.stdout.splitlines() if (v:=line.split(':'))}
 for i in range(2):assert hashlib.sha256((p/f'output-mask-{i}.bin').read_bytes()).hexdigest()==mask_hashes[str(i)]
 for index,frame in enumerate(cases['frames']):
  actual=hashlib.sha256((p/f'output-{index}.bin').read_bytes()).hexdigest()
  expected_counts=tuple(sum(draw[field] for draw in frame['draws']) for field in ['attempt_count','write_count'])
  assert actual==frame['frame_sha256'] and counts[index]==expected_counts,(frame['id'],actual,counts[index],expected_counts)
  print('PASS',frame['id'],actual)
 print('PASS both generated original coast masks;37 original full indexed frames')
