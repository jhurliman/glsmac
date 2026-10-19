"""Finite sea-Fungus continuation composed with closed water/coast passes."""
from water_coast_reference import produce as water_coast_produce,NEIGHBORS
from terrain_geometry_reference import TerrainGeometry,planet_flat_center
from terrain_polygon_raster_reference import generate_polygon_spans


def water_spec(spec):
    result={k:v for k,v in spec.items() if k not in ['patch','items','center_band','center_contour']}
    result['patch']=[p[:4] for p in spec.get('patch',[])]
    if 'center_band' in spec or 'center_contour' in spec:
        result['patch'].append([0,0,spec.get('center_band',spec['band']),spec.get('center_contour',spec['contour'])])
    return result


def fungus_selection(spec,tables):
    x,y=spec.get('tile',[40,40]);wrap=not spec.get('flat_map',False)
    patch={((x+dx)%80,y+dy):(band,contour,items) for dx,dy,band,contour,items in spec.get('patch',[])
        if 0<=y+dy<80 and (wrap or 0<=x+dx<80)}
    mask=0
    for bit in range(8):
        dx,dy=NEIGHBORS[(bit-1)&7];qx,qy=x+dx,y+dy
        if not 0<=qy<80 or (not wrap and not 0<=qx<80):continue
        band,_,items=patch.get((qx%80,qy),(spec['band'],spec['contour'],0))
        if band>=2 and items&32:mask|=1<<bit
    center_band=spec.get('center_band',spec['band'])
    admitted=bool(spec['items']&32) and center_band>=2
    index,turn=tables['pattern_lookup'][mask]
    return dict(mask=mask,admitted=admitted,crop=tables['sea_fungus_crops' if center_band<3 else 'land_fungus_crops'][index],turn=turn)


def produce(spec,tables):
    plain=water_spec(spec)
    commands,samples=water_coast_produce(plain,tables['water_coast'],tables['canonical_uv'])
    selection=fungus_selection(spec,tables)
    if selection['admitted']:
        from water_coast_reference import WaterMap
        world=WaterMap(plain,tables['water_coast']);x,y=world.center
        bands={(2*(i%40)+(i//40&1),i//40):world.cell(2*(i%40)+(i//40&1),i//40)[0] for i in range(3200)}
        geometry=TerrainGeometry(80,80,bands,world.wrap);cx,cy=planet_flat_center(x,y,x,y,world.wrap)
        vertices=geometry.vertices(x,y,cx,cy)[1:];uv=[]
        for i in range(4):uv.extend(tables['canonical_uv'][(selection['turn']+i)&3][2:4])
        command=dict(mode='keyed_unlit',source_crop=selection['crop'],vertices=[n for p in vertices for n in p],values=[],uv=uv,shade_limits=[-256,256] if spec.get('center_band',spec['band'])<3 else [-4,4])
        position=next((i for i,c in enumerate(commands) if c['mode']=='coast_mapped'),len(commands))
        commands.insert(position,command)
    return commands,samples,selection


def render(frame,load,width=1024,height=511,background=250):
    import struct,hashlib
    output=bytearray([background])*(width*height);results=[]
    for i,draw in enumerate(frame['draws']):
        mode=draw['mode'];unlit=mode=='keyed_unlit';values=draw['values'];uv=list(zip(draw['uv'][::2],draw['uv'][1::2]));vertices=list(zip(draw['vertices'][::2],draw['vertices'][1::2]))
        spans=[] if mode=='depth_below' and all(v>=draw['threshold'] for v in values) else generate_polygon_spans(vertices,uv,values,frame['clip'],i,tuple(draw['shade_limits']),shaded=not unlit)
        source,mask=load(draw);attempts=bytearray();writes=bytearray()
        for _,y,left,right,u,v,du,dv,phase,step in spans:
            for x in range(left,right):
                offset=(v>>16)*56+(u>>16);assert 0<=offset<len(source)
                raw=source[offset];shade=phase>>8;shade=shade if shade<128 else shade-256
                cls=mask[offset] if mask is not None else 0
                selector=draw['mask_mapping'][cls] if mode=='coast_mapped' else 0
                will=raw!=9 if unlit else shade<draw['threshold'] if mode=='depth_below' else selector!=0 if mode=='coast_mapped' else True
                result=raw if unlit else selector if mode=='coast_mapped' and selector>1 else (raw+shade)&255
                attempts.extend(struct.pack('<HHIhBBBB',x,y,offset,shade,raw,cls,selector,will))
                if will:output[y*width+x]=result;writes.extend(struct.pack('<HHB',x,y,result))
                u+=du;v+=dv;phase=(phase+step)&65535
        results.append(dict(native_spans=spans,attempt_sha256=hashlib.sha256(attempts).hexdigest(),write_sha256=hashlib.sha256(writes).hexdigest(),attempt_count=len(attempts)//14,write_count=len(writes)//5))
    return bytes(output),results


def verify_producer(cases):
    count=0
    for case in cases['producer_cases']:
        commands,samples,selection=produce(case['input'],cases['tables'])
        assert commands==case['commands'],case['input']['id']
        assert samples==case['samples'],(case['input']['id'],'samples')
        assert selection['mask']==case['native_gate']['mask'],(case['input']['id'],'mask')
        count+=len(commands)
    return count


def verify_frames(cases,texture_pcx,rainfall_pcx):
    import hashlib
    from water_coast_reference import generated_masks
    from terrain_triangle_raster_reference import pcx_crop
    masks=generated_masks(rainfall_pcx,cases['tables']['water_coast']);cache={};totals=[0,0,0]
    def load(draw):
        key=tuple(draw['source_crop'])
        if key not in cache:cache[key]=pcx_crop(texture_pcx,key)
        source=cache[key];assert hashlib.sha256(source).hexdigest()==draw['loaded_source_sha256']
        mask=masks[0] if draw['mode']=='coast_mapped' else None
        if mask is not None:assert hashlib.sha256(mask).hexdigest()==draw['mask_sha256']
        return source,mask
    for frame in cases['frames']:
        output,results=render(frame,load,*cases['viewport'],cases['background'])
        for expected,result in zip(frame['draws'],results):
            for key in ['native_spans','attempt_count','write_count','attempt_sha256','write_sha256']:assert expected[key]==result[key],(frame['id'],expected['index'],key)
            totals[0]+=len(result['native_spans']);totals[1]+=result['attempt_count'];totals[2]+=result['write_count']
        assert hashlib.sha256(output).hexdigest()==frame['frame_sha256'],frame['id']
    return totals


if __name__=='__main__':
    from pathlib import Path
    import argparse,json
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cases',default=str(Path(__file__).with_name('ordinary-sea-fungus-cases.json')))
    parser.add_argument('--texture-pcx');parser.add_argument('--rainfall-pcx')
    args=parser.parse_args();cases=json.loads(Path(args.cases).read_text())
    print('PASS',verify_producer(cases),'native ordered commands and all contour/adjacency inputs')
    if args.texture_pcx or args.rainfall_pcx:
        if not args.texture_pcx or not args.rainfall_pcx:parser.error('Supply both external PCX paths for full composed pixel checks')
        print('PASS native spans/attempts/writes:',verify_frames(cases,args.texture_pcx,args.rainfall_pcx))
