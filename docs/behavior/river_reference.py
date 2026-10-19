"""Bounded ordinary River producer and indexed keyed-shaded composition."""
from sea_fungus_reference import produce as prior_produce,water_spec
from water_coast_reference import WaterMap
from terrain_geometry_reference import TerrainGeometry,planet_flat_center
from terrain_polygon_raster_reference import generate_polygon_spans


def produce(spec,tables):
    material=dict(spec,items=spec.get('remembered_items',spec['items']))
    commands,samples,_=prior_produce(material,tables)
    plain=water_spec(material);world=WaterMap(plain,tables['water_coast']);x,y=world.center
    bands={(xx,yy):world.cell(xx,yy)[0] for yy in range(80) for xx in range(yy&1,80,2)}
    geometry=TerrainGeometry(80,80,bands,world.wrap);cx,cy=planet_flat_center(x,y,x,y,world.wrap)
    vertices=[v for xy in geometry.vertices(x,y,cx,cy)[1:] for v in xy]
    values=[geometry.shade(x,y,k) for k in range(1,5)]
    patch={((x+dx)%80,y+dy):items for dx,dy,band,contour,items in spec.get('patch',[])
        if 0<=y+dy<80 and (world.wrap or 0<=x+dx<80)}
    def neighbor_items(dx,dy):
        qx,qy=x+dx,y+dy
        if not 0<=qy<80 or (not world.wrap and not 0<=qx<80):return 0
        return patch.get((qx%80,qy),0)
    def quad(crop):
        return dict(mode='keyed_shaded',source_crop=crop,vertices=vertices[:],values=values[:],uv=[0,0,55,0,55,55,0,55],shade_limits=[-4,4])
    if material['items']&0x200000 and world.cell(x,y)[0]>=3:
        mask=sum(1<<i for i,(dx,dy) in enumerate(tables['forest_neighbor_offsets']) if neighbor_items(dx,dy)&0x200000)
        pos=next((i for i,c in enumerate(commands) if c['mode']=='coast_mapped'),len(commands))
        commands.insert(pos,quad(tables['forest_crops'][mask]))
    mask=sum(1<<i for i,(dx,dy) in enumerate(tables['river_offsets']) if neighbor_items(dx,dy)&128)
    admitted=bool(spec.get('flags',0x401)&0x400 and spec['items']&128 and world.cell(x,y)[0]>=3)
    if admitted:
        commands.append(quad(tables['river_crops'][mask]))
        if spec['items']&0x200:commands.append(quad(tables['river_crops'][0]))
    return commands,samples,dict(admitted=admitted,mask=mask)


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
                will=raw!=9 if mode in ['keyed_unlit','keyed_shaded'] else shade<draw['threshold'] if mode=='depth_below' else selector!=0 if mode=='coast_mapped' else True
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
        assert commands==case['commands'],(case['input']['id'],commands,case['commands'])
        assert samples==case['samples'],(case['input']['id'],'samples')
        assert selection==case['selection'],(case['input']['id'],'selection')
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cases',default=str(Path(__file__).with_name('ordinary-river-cases.json')));p.add_argument('--texture-pcx');p.add_argument('--rainfall-pcx');a=p.parse_args()
    cases=json.loads(Path(a.cases).read_text());print('PASS producer commands',verify_producer(cases))
    if a.texture_pcx or a.rainfall_pcx:
        if not a.texture_pcx or not a.rainfall_pcx:p.error('Supply both external PCX paths')
        print('PASS frames/spans/attempts/writes',len(cases['frames']),verify_frames(cases,a.texture_pcx,a.rainfall_pcx))
