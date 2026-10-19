"""Finite native Road/Tube directional overlay, composed with ordinary terrain."""
from river_reference import produce as prior_produce,render,verify_frames
from sea_fungus_reference import water_spec
from water_coast_reference import WaterMap
from terrain_geometry_reference import TerrainGeometry,planet_flat_center


def selection(center_land,center_items,center_base,neighbors):
    """Neighbors: None or (land,selectedItems,admittedBase), native8-neighbor order."""
    if not center_land or not (center_items&4 or center_base):return []
    tube=bool(center_items&8 or center_base);draws=[];connected=False;tube_connected=False
    for direction,n in enumerate(neighbors):
        if n is None:continue
        land,items,base=n
        if not land or not (items&4 or base):continue
        connected=True;use_tube=bool(tube and (items&8 or base))
        if use_tube:tube_connected=True
        draws.append(dict(kind='tube' if use_tube else 'road',index=1+((direction+2)&7)))
    if not center_base and ((tube and not tube_connected) or not connected):
        draws.append(dict(kind='tube' if tube and not tube_connected else 'road',index=0))
    return draws


def produce(spec,tables):
    effective=dict(spec,flags=spec.get('flags',0x21))
    commands,samples,_=prior_produce(effective,tables)
    world=WaterMap(water_spec(spec),tables['water_coast']);x,y=world.center
    bases={((x+dx)%80,y+dy):bool(owner==3 or visible&8) for dx,dy,owner,visible in spec.get('bases',[])}
    selected={((x+dx)%80,y+dy):items for dx,dy,band,contour,items in spec.get('patch',[])
        if 0<=y+dy<80 and (world.wrap or 0<=x+dx<80)}
    for bit,items in spec.get('neighbor_remembered',[]):
        dx,dy=tables['road_offsets'][bit];selected[((x+dx)%80,y+dy)]=items
    entries=[]
    for dx,dy in tables['road_offsets']:
        qx,qy=x+dx,y+dy
        if not 0<=qy<80 or (not world.wrap and not 0<=qx<80):entries.append(None);continue
        entries.append((world.cell(qx%80,qy)[0]>=3,selected.get((qx%80,qy),0),bases.get((qx%80,qy),False)))
    picked=selection(world.cell(x,y)[0]>=3,spec.get('remembered_items',spec['items']),bases.get((x,y),False),entries) if effective['flags']&0x20 else []
    if picked:
        bands={(xx,yy):world.cell(xx,yy)[0] for yy in range(80) for xx in range(yy&1,80,2)}
        geometry=TerrainGeometry(80,80,bands,world.wrap);cx,cy=planet_flat_center(x,y,x,y,world.wrap)
        vertices=[v for xy in geometry.vertices(x,y,cx,cy)[1:] for v in xy];values=[geometry.shade(x,y,k) for k in range(1,5)]
        for p in picked:commands.append(dict(mode='keyed_shaded',source_crop=tables[p['kind']+'_crops'][p['index']],vertices=vertices[:],values=values[:],uv=[0,0,55,0,55,55,0,55],shade_limits=[-4,4]))
    return commands,samples,picked


def verify_producer(cases):
    total=0
    for c in cases['producer_cases']:
        commands,samples,picked=produce(c['input'],cases['tables'])
        assert commands==c['commands'],(c['input']['id'],commands,c['commands'])
        assert samples==c['samples'],(c['input']['id'],'contours')
        assert picked==c['selection'],(c['input']['id'],'selection')
        total+=len(commands)
    return total


if __name__=='__main__':
    from pathlib import Path
    import argparse,json
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cases',default=str(Path(__file__).with_name('ordinary-road-tube-cases.json')));p.add_argument('--texture-pcx');p.add_argument('--rainfall-pcx');a=p.parse_args();cases=json.loads(Path(a.cases).read_text())
    print('PASS producer commands',verify_producer(cases))
    if a.texture_pcx or a.rainfall_pcx:
        if not a.texture_pcx or not a.rainfall_pcx:p.error('Supply both external PCX paths')
        # Reuse the prior verifier's asset checks, with this module's matching renderer.
        print('PASS frames/spans/attempts/writes',len(cases['frames']),verify_frames(cases,a.texture_pcx,a.rainfall_pcx))
