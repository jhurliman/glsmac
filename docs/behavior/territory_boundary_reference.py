"""Bounded territory-boundary palette construction, admission and compositor."""
from river_reference import produce as prior_produce
from sea_fungus_reference import water_spec
from water_coast_reference import WaterMap
from terrain_geometry_reference import TerrainGeometry,planet_flat_center
from terrain_polygon_raster_reference import generate_polygon_spans


def generated_mask(rainfall_pcx):
    from terrain_triangle_raster_reference import pcx_crop
    source=pcx_crop(rainfall_pcx,(2,254,60,60))
    lut={(v+10)&255:i for i,v in enumerate([211,208,41,143,116,151,206,95,192])}
    return bytes(9 if v==9 else lut.get(v,0) for v in source)


def palette_rgb(palette_pcx,reserved):
    from pathlib import Path
    data=Path(palette_pcx).read_bytes();assert data[-769]==12
    colors=[[0,0,0] for _ in range(256)]
    for row in reserved:colors[row['index']]=row['rgb']
    trailer=data[-768:]
    for i in range(236):colors[10+i]=[min(c,235) for c in trailer[3*i:3*i+3]]
    return colors


def clut_row(colors,color,opacity):
    row=bytearray(range(256));target=colors[color]
    for dst in range(10,246):
        rgb=[((100-opacity)*colors[dst][c]+opacity*target[c])//100 for c in range(3)]
        row[dst]=min(range(10,246),key=lambda k:sum((rgb[c]-colors[k][c])**2 for c in range(3)))
    return bytes(row)


def make_cluts(palette_pcx,faction_directory,tables):
    from pathlib import Path
    from terrain_triangle_raster_reference import pcx_crop
    colors=palette_rgb(palette_pcx,tables['reserved_palette_entries'])
    sampled=[pcx_crop(palette_pcx,(580+8*i,56,1,1))[0] for i in range(8)]
    assert sampled==tables['global_colors']
    global_rows=[clut_row(colors,color,opacity) for color,opacity in zip(sampled,tables['global_opacities'])]
    active=global_rows[:]
    for row in tables['faction_rows']:
        path=Path(faction_directory)/row['asset'];color=pcx_crop(path,(161,747,1,1))[0];raw=pcx_crop(path,(161,757,1,1))[0]
        opacity=((raw-10)&255)*100//255
        assert (color,raw,opacity)==(row['color'],row['raw_opacity'],row['opacity'])
        active[row['owner']]=clut_row(colors,color,opacity)
    return b''.join(global_rows),b''.join(active)


def effective_spec(spec,tables):
    out=dict(spec,patch=list(spec.get('patch',[])))
    if 'neighbor_band' in spec:
        for bit in spec['different']:
            dx,dy=tables['boundary_offsets'][bit]
            out['patch'].append([dx,dy,spec['neighbor_band'],spec['contour'],0])
    return out


def owner_query(raw_owner,viewer,relation=0,scenario_bypass=False):
    """Normal no-output-pointer query; relation belongs to viewer toward owner."""
    if raw_owner<=0:return -1
    if raw_owner==viewer or scenario_bypass or relation&0x08000008==0x08000008:return raw_owner
    return -1


def mapping_for_neighbors(center_owner,center_band,center_region,neighbors):
    """Eight entries: None, or (band,region,resolvedOwner), in native order."""
    mapping=[0]*10;needed=False
    if center_owner<0:return False,mapping
    for bit,entry in enumerate(neighbors):
        if entry is None:continue
        band,region,owner=entry
        if (band<3)!=(center_band<3) or region!=center_region or owner==center_owner:continue
        needed=True;v=(center_owner+1)&255
        if bit&1:mapping[bit+1]=v
        elif bit==0:mapping[8]=mapping[2]=mapping[1]=v
        else:mapping[bit]=mapping[bit+2]=mapping[bit+1]=v
    return needed,mapping


def selection(spec,tables):
    world=WaterMap(water_spec(effective_spec(spec,tables)),tables['water_coast']);x,y=world.center
    owner=owner_query(spec['owner'],3,0 if spec.get('no_pact') else 0x08000008)
    mapping=[0]*10;needed=False;entries=[]
    eligible=bool(spec.get('flags',0x10001)&0x10000 and owner>=0)
    if eligible:
        for bit,(dx,dy) in enumerate(tables['boundary_offsets']):
            qx,qy=x+dx,y+dy
            if not 0<=qy<80 or (not world.wrap and not 0<=qx<80):entries.append(None);continue
            different=bit in spec['different']
            entries.append((world.cell(qx%80,qy)[0],spec.get('other_region',1) if different else 1,-1 if different else owner))
        needed,mapping=mapping_for_neighbors(owner,world.cell(x,y)[0],1,entries)
    return dict(admitted=eligible and needed,owner_mapping=mapping)


def produce(spec,tables):
    effective=effective_spec(spec,tables);effective['flags']=spec.get('flags',0x10001)
    commands,samples,river=prior_produce(effective,tables)
    chosen=selection(spec,tables)
    if chosen['admitted']:
        world=WaterMap(water_spec(effective),tables['water_coast']);x,y=world.center
        bands={(xx,yy):world.cell(xx,yy)[0] for yy in range(80) for xx in range(yy&1,80,2)}
        geometry=TerrainGeometry(80,80,bands,world.wrap);cx,cy=planet_flat_center(x,y,x,y,world.wrap)
        command=dict(mode='owner_clut',source_crop=[0,0,60,60],vertices=[v for xy in geometry.vertices(x,y,cx,cy)[1:] for v in xy],values=[],uv=[0,0,59,0,59,59,0,59],shade_limits=[-256,256] if world.cell(x,y)[0]<3 else [-4,4],owner_mapping=chosen['owner_mapping'])
        river_count=(1+bool(spec['items']&0x200)) if river['admitted'] else 0
        position=next((i for i,c in enumerate(commands) if c['mode']=='coast_mapped'),len(commands)-river_count)
        commands.insert(position,command)
    return commands,samples,chosen


def render(frame,load,clut,width=1024,height=511,background=250):
    import struct,hashlib
    output=bytearray((bytes(range(256))*4)*height if frame.get('background_mode')=='x_mod256' else bytes([background])*(width*height));results=[]
    for i,draw in enumerate(frame['draws']):
        mode=draw['mode'];owner=mode=='owner_clut';unlit=mode=='keyed_unlit' or owner;values=draw['values'];uv=list(zip(draw['uv'][::2],draw['uv'][1::2]));vertices=list(zip(draw['vertices'][::2],draw['vertices'][1::2]))
        spans=[] if mode=='depth_below' and all(v>=draw['threshold'] for v in values) else generate_polygon_spans(vertices,uv,[0]*len(vertices) if owner else values,frame['clip'],i,tuple(draw['shade_limits']),shaded=mode!='keyed_unlit')
        source,mask=load(draw);attempts=bytearray();writes=bytearray()
        for _,y,left,right,u,v,du,dv,phase,step in spans:
            for x in range(left,right):
                offset=(v>>16)*(60 if owner else 56)+(u>>16);assert 0<=offset<len(source)
                raw=source[offset];shade=phase>>8;shade=shade if shade<128 else shade-256
                before=output[y*width+x];cls=before if owner else mask[offset] if mask is not None else 0
                selector=draw['owner_mapping'][raw] if owner else draw['mask_mapping'][cls] if mode=='coast_mapped' else 0
                will=selector!=0 if owner else raw!=9 if mode in ['keyed_unlit','keyed_shaded'] else shade<draw['threshold'] if mode=='depth_below' else selector!=0 if mode=='coast_mapped' else True
                result=(clut[(selector-1)*256+before] if selector else before) if owner else raw if unlit else selector if mode=='coast_mapped' and selector>1 else (raw+shade)&255
                attempts.extend(struct.pack('<HHIhBBBB',x,y,offset,shade,raw,cls,selector,will))
                if will:output[y*width+x]=result;writes.extend(struct.pack('<HHB',x,y,result))
                u+=du;v+=dv;phase=(phase+step)&65535
        results.append(dict(native_spans=spans,attempt_sha256=hashlib.sha256(attempts).hexdigest(),write_sha256=hashlib.sha256(writes).hexdigest(),attempt_count=len(attempts)//14,write_count=len(writes)//5))
    return bytes(output),results


def verify_producer(cases):
    count=0
    for c in cases['producer_cases']:
        commands,samples,chosen=produce(c['input'],cases['tables'])
        assert commands==c['commands'],c['input']['id']
        assert samples==c['samples'],(c['input']['id'],'samples')
        assert chosen==c['selection'],(c['input']['id'],'selection')
        count+=len(commands)
    return count


def verify_artifacts(cases,asset_directory):
    from pathlib import Path
    import hashlib
    from water_coast_reference import generated_masks
    from terrain_triangle_raster_reference import pcx_crop
    asset=Path(asset_directory);tables=cases['tables']
    initial,active=make_cluts(asset/'PALETTE.PCX',asset,tables)
    assert hashlib.sha256(initial).hexdigest()==cases['global_clut_sha256']
    assert hashlib.sha256(active).hexdigest()==cases['active_clut_sha256']
    boundary=generated_mask(asset/'RAINFALL.PCX');assert hashlib.sha256(boundary).hexdigest()==cases['mask_sha256']
    masks=generated_masks(asset/'RAINFALL.PCX',tables['water_coast']);cache={};totals=[0,0,0]
    def load(d):
        if d['mode']=='owner_clut':source=boundary;mask=None
        else:
            key=tuple(d['source_crop'])
            if key not in cache:cache[key]=pcx_crop(asset/'TEXTURE.PCX',key)
            source=cache[key];mask=masks[0] if d['mode']=='coast_mapped' else None
        assert hashlib.sha256(source).hexdigest()==d['loaded_source_sha256']
        return source,mask
    for f in cases['frames']:
        output,results=render(f,load,active,*cases['viewport'],cases['background'])
        for expected,got in zip(f['draws'],results):
            for key in ['native_spans','attempt_count','write_count','attempt_sha256','write_sha256']:assert expected[key]==got[key],(f['id'],expected['index'],key)
            totals[0]+=len(got['native_spans']);totals[1]+=got['attempt_count'];totals[2]+=got['write_count']
        assert hashlib.sha256(output).hexdigest()==f['frame_sha256'],f['id']
    return totals


if __name__=='__main__':
    from pathlib import Path
    import argparse,json
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cases',default=str(Path(__file__).with_name('territory-boundary-cases.json')));p.add_argument('--asset-directory');a=p.parse_args()
    cases=json.loads(Path(a.cases).read_text());print('PASS producer commands',verify_producer(cases))
    if a.asset_directory:print('PASS mask/CLUT and frames/spans/attempts/writes',len(cases['frames']),verify_artifacts(cases,a.asset_directory))
