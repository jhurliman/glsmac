"""Bounded ordinary zoom0 water/coast producer and indexed raster reference."""
from terrain_geometry_reference import TerrainGeometry,planet_flat_center
from terrain_polygon_raster_reference import generate_polygon_spans

NEIGHBORS=((1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2))


class WaterMap:
    def __init__(self,spec,tables):
        self.spec,self.tables=spec,tables
        self.center=spec.get('tile',[40,40]);self.wrap=not spec.get('flat_map',False)
        self.patch={((self.center[0]+dx)%80,self.center[1]+dy):(band,contour) for dx,dy,band,contour in spec.get('patch',[])}

    def cell(self,x,y):
        if not 0<=y<80:return None
        if self.wrap:x%=80
        if not 0<=x<80:return None
        return self.patch.get((x,y),(self.spec['band'],self.spec['contour']))

    def sample(self,x,y,sector,mode):
        center=self.cell(x,y)
        if center is None:return 0
        def at(dx,dy):return self.cell(x+dx,y+dy)
        def contour(c):return min(79,max(0,c[1])) if c is not None else 0
        ox=self.tables['sampler_corner_offsets_x'];oy=self.tables['sampler_corner_offsets_y']
        if mode==0:value=center[1]
        elif mode&1:
            dx,dy=NEIGHBORS[(mode+2*sector+3)&7]
            value=(contour(center)+contour(at(dx,dy)))>>1
        else:
            value=sum(contour(at(ox[4*sector+i],oy[4*sector+i])) for i in range(4))>>2
        if mode==2:
            a=at(ox[4*sector+1],oy[4*sector+1]);b=at(ox[4*sector+3],oy[4*sector+3])
            if a is not None and a[0]>=3:value+=7
            if b is not None and b[0]>=3:value-=3
        else:
            for i in range(3):
                q=at(*NEIGHBORS[(2*sector-4+i)&7])
                opposite=q is not None and ((q[0]>=3) if center[0]<3 else (q[0]<3))
                if opposite and ((mode==1 and i==0) or (mode==3 and i==2)):
                    overrides=(([57,67,67,57],[67,67,57,57]) if center[0]<3 else ([67,57,57,67],[57,57,67,67]))
                    return overrides[0 if mode==1 else 1][sector]
        return min(79,max(0,value))

    def coast_masks(self,x,y):
        masks=[0]*4
        for i,(dx,dy) in enumerate(NEIGHBORS):
            q=self.cell(x+dx,y+dy)
            if q is None or q[0]>=3:continue
            if i&1:masks[((i>>1)-2)&3]|=2
            else:
                masks[((i>>1)+1)&3]|=4
                masks[((i>>1)-2)&3]|=1
        return masks


def coast_mapping(pattern,x,y,tables):
    result=[]
    for i in range(53):
        value=tables['coast_classes'][8*i+pattern]
        result.append(value if value<=1 else tables['coast_colors'][2+value])
    if pattern==7 and (x-y)&2:
        result[5]=0;result[14:18]=[0]*4
        for start in [18,22,26]:result[start:start+4]=list(reversed(tables['coast_colors'][4:8]))
    return result


def produce(spec,tables,canonical_uv):
    world=WaterMap(spec,tables);x,y=world.center
    sea=world.cell(x,y)[0]<3;masks=world.coast_masks(x,y)
    cx,cy=planet_flat_center(x,y,x,y,world.wrap)
    bands={(2*(i%40)+(i//40&1),i//40):world.cell(2*(i%40)+(i//40&1),i//40)[0] for i in range(3200)}
    geometry=TerrainGeometry(80,80,bands,world.wrap)
    points=geometry.vertices(x,y,cx,cy);commands=[];samples=[];depths={}
    def command(mode,crop,vertices,values,uv,limits,**extra):
        return dict(mode=mode,source_crop=crop,vertices=[n for p in vertices for n in p],values=values,uv=uv,shade_limits=limits,**extra)
    if not sea:
        # Bounded fixture's dry/no-feature land fan; live callers reuse the
        # separately closed general moisture/Rock/Forest/Fungus selector.
        for i,ids in enumerate([(0,1,2),(0,2,3),(0,3,4),(0,4,1)]):
            commands.append(command('opaque_shaded',[1,58,56,56],[points[j] for j in ids],[geometry.shade(x,y,j) for j in ids],canonical_uv[i],[-4,4]))
    quads=[]
    for sector in range(4):
        a=points[sector+1];b=points[((sector-1)&3)+1];c=points[((sector+1)&3)+1]
        quad=[points[0],[(a[j]+b[j])>>1 for j in range(2)],a,[(a[j]+c[j])>>1 for j in range(2)]]
        quads.append(quad)
        if not sea and not masks[sector]:continue
        values=[]
        for mode in range(4):
            sample=world.sample(x,y,sector,mode);depth=tables['depth_lookup'][sample];values.append(depth)
            samples.append(dict(args=[x,y,sector,mode],result=sample,depth=depth))
        depths[sector]=values
        if sea:
            uv=tables['water_uv'][sector*8:sector*8+8]
            if any(v>=16 for v in values):commands.append(command('opaque_shaded',[280,136,56,56],quad,[v-16 for v in values],uv,[-256,256]))
            commands.append(command('depth_below',[280,79,56,56],quad,values,uv,[-256,256],threshold=16))
    if not sea:
        for sector in range(4):
            if masks[sector]:commands.append(command('coast_mapped',[280,79,56,56],quads[sector],depths[sector],tables['coast_uv'][sector*8:sector*8+8],[-4,4],mask_mapping=coast_mapping(masks[sector],x,y,tables),mask_crop=[0,0,56,56]))
    return commands,samples


def generated_masks(path,tables):
    from terrain_triangle_raster_reference import pcx_crop
    source=pcx_crop(path,[2,333,113,56]);lookup=[0]*256
    for i,value in enumerate(tables['coast_source_indices']):lookup[(value+10)&255]=i
    mapped=bytes(0 if value in [6,7,9] else lookup[value] for value in source)
    return [b''.join(mapped[y*113+x:y*113+x+56] for y in range(56)) for x in [0,57]]


def render(frame,load,width=1024,height=511,background=250):
    import struct,hashlib
    output=bytearray([background])*(width*height);results=[]
    for i,draw in enumerate(frame['draws']):
        mode=draw['mode'];values=draw['values'];uv=list(zip(draw['uv'][::2],draw['uv'][1::2]));vertices=list(zip(draw['vertices'][::2],draw['vertices'][1::2]))
        spans=[] if mode=='depth_below' and all(v>=draw['threshold'] for v in values) else generate_polygon_spans(vertices,uv,values,frame['clip'],i,tuple(draw['shade_limits']))
        source,mask=load(draw);attempts=bytearray();writes=bytearray()
        for _,y,left,right,u,v,du,dv,phase,step in spans:
            for x in range(left,right):
                offset=(v>>16)*56+(u>>16);assert 0<=offset<len(source)
                raw=source[offset];shade=phase>>8;shade=shade if shade<128 else shade-256
                cls=mask[offset] if mask is not None else 0
                selector=draw['mask_mapping'][cls] if mode=='coast_mapped' else 0
                will=shade<draw['threshold'] if mode=='depth_below' else selector!=0 if mode=='coast_mapped' else True
                result=selector if mode=='coast_mapped' and selector>1 else (raw+shade)&255
                attempts.extend(struct.pack('<HHIhBBBB',x,y,offset,shade,raw,cls,selector,will))
                if will:output[y*width+x]=result;writes.extend(struct.pack('<HHB',x,y,result))
                u+=du;v+=dv;phase=(phase+step)&65535
        results.append(dict(native_spans=spans,attempt_sha256=hashlib.sha256(attempts).hexdigest(),write_sha256=hashlib.sha256(writes).hexdigest(),attempt_count=len(attempts)//14,write_count=len(writes)//5))
    return bytes(output),results


def verify_producer(cases):
    count=0
    for case in cases['producer_cases']:
        commands,samples=produce(case['input'],cases['tables'],cases['canonical_uv'])
        assert commands==case['commands'],case['input']['id']
        assert samples==case['samples'],(case['input']['id'],'sampler')
        count+=len(commands)
    return count


def verify_frames(cases,texture_pcx,rainfall_pcx):
    import hashlib
    from terrain_triangle_raster_reference import pcx_crop
    masks=generated_masks(rainfall_pcx,cases['tables']);cache={};totals=[0,0,0]
    for i,mask in enumerate(masks):assert hashlib.sha256(mask).hexdigest()==cases['generated_mask_sha256'][str(i)]
    def load(draw):
        crop=tuple(draw['source_crop'])
        if crop not in cache:cache[crop]=pcx_crop(texture_pcx,crop)
        source=cache[crop];assert hashlib.sha256(source).hexdigest()==draw['loaded_source_sha256']
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
    parser.add_argument('--cases',default=str(Path(__file__).with_name('ordinary-water-coast-cases.json')))
    parser.add_argument('--texture-pcx');parser.add_argument('--rainfall-pcx')
    args=parser.parse_args();cases=json.loads(Path(args.cases).read_text())
    print('PASS',verify_producer(cases),'native ordered commands and all contour samples')
    if args.texture_pcx or args.rainfall_pcx:
        if not args.texture_pcx or not args.rainfall_pcx:parser.error('Supply both external PCX paths for pixel checks')
        print('PASS native spans/attempts/writes:',verify_frames(cases,args.texture_pcx,args.rainfall_pcx))
