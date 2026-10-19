"""Ordinary developed-tile sprite selection; zoom0 pixel composition.

Nutrients are a resolved result of ordinary-tile-nutrients, evaluated from live
game state. Eligibility uses the viewer-selected item word instead.
"""
from math import cos,sin,pi

def native_hsv(rgb):
    r,g,b=(c/256 for c in rgb);v=max(r,g,b)
    # Original642b70 compares B against the maximum at642c06, then replaces
    # its minimum with B. This is deliberately not conventional HSV.
    low=b if b<v else min(r,g)
    s=(v-low)/v if v else 0
    if not s:return -1,0,v
    delta=v-low
    h=((g-b)/delta if r==v else 2+(b-r)/delta if g==v else 4+(r-g)/delta)*60
    if h<0:h+=360
    return h,s,v

def select(items,band,nutrients,live_landmark=0,flags=0x21,zoom=0):
    land=band>=3;out=[]
    def add(name,bank):out.append(dict(kind=name,bank=hex(bank)))
    # Kelp precedes Road/Tube and is controlled by terrain bit1, with no zoom gate.
    if not land and flags&1 and items&0x8000:add('kelp',0x781ef8)
    if not flags&0x20 or zoom<=-8:return out
    if land and items&0x800:add('bunker',0x7815b8)
    if land and items&0x40000:add('airbase',0x781588)
    if items&0x80000000:add('sensor',0x781558)
    if land and items&0x8000:
        index=max(0,min(3,nutrients-1))
        add('soil_farm' if items&0x80000 else 'farm',(0x781a90 if items&0x80000 else 0x781b70)+44*index)
    if items&0x10 and (land or band==2):add('mine' if land else 'mining_platform',0x781fe8 if land else 0x781f58)
    if items&0x40 and (land or band==2):add('solar' if land else 'sea_solar',0x781fb8 if land else 0x781f88)
    if items&0x400000:add('condenser',0x781ec8)
    if items&0x800000:add('mirror',0x781e98)
    if items&0x1000000:add('borehole',0x781e38 if live_landmark&0x80001000==0x1000 else 0x781e68)
    if land and items&0x88000==0x88000 and nutrients>=5:add('extra_soil',0x781a90)
    return out

def shadow_lookup(palette):
    """Native brightness -43 lookup: HSV cylindrical nearest, candidates10..245.

    Palette is256 RGB triples from the active game palette, not embedded TER1.
    Stock palette constructor supplies no protected candidate ranges.
    """
    hsv=[native_hsv(rgb) for rgb in palette]
    def point(h,s,v):return (s*cos(h*pi/180),s*sin(h*pi/180),v)
    candidates=[point(*hsv[i]) for i in range(10,246)]
    result=[]
    for h,s,v in hsv:
        target=point(h,s,v*.57)
        result.append(10+min(range(236),key=lambda i:sum((a-b)**2 for a,b in zip(target,candidates[i]))))
    return bytes(result)

def sprite_pixels(binding,ter1_path):
    from terrain_triangle_raster_reference import pcx_crop
    raw=pcx_crop(ter1_path,binding['crop'],index_offset=10)
    raw=bytes(9 if c in (6,7) else c for c in raw)
    w,h=binding['crop'][2:];points=[(i%w,i//w) for i,c in enumerate(raw) if c!=binding['key']]
    if not points:return bytes(),[0,0],[0,0]
    l=min(x for x,y in points);t=min(y for x,y in points);r=max(x for x,y in points)+1;b=max(y for x,y in points)+1
    return b''.join(raw[y*w+l:y*w+r] for y in range(t,b)),[l,t],[r-l,b-t]

def paint(frame,pitch,rows,clip,binding,anchor,pixels,shadow=None):
    """Row-major attempts and stores; key before lookup, source0 literal unless shadow."""
    ox,oy=binding['offset'];w,h=binding['size'];key=binding['key'];attempts=[];stores=[]
    for sy in range(h):
        y=anchor[1]+oy+sy
        for sx in range(w):
            x=anchor[0]+ox+sx
            if not (0<=x<pitch and 0<=y<rows and clip[0]<=x<clip[2] and clip[1]<=y<clip[3]):continue
            src=pixels[sy*w+sx];dest=y*pitch+x;old=frame[dest];will=src!=key
            result=shadow[old] if shadow is not None and src==0 else src
            attempts.append((x,y,sy*w+sx,src,old,int(will),result))
            if will:frame[dest]=result;stores.append((x,y,result))
    return attempts,stores

def produce(spec,tables,nutrients,seed_low16):
    from road_tube_reference import produce as prior
    from land_texture_selector_reference import select as land_select
    commands,samples,_=prior(spec,tables)
    band=spec.get('center_band',spec['band'])
    items=spec.get('remembered_items',spec['items'])
    if band>=3:
        material=dict(spec,moisture=0,center_moisture=spec.get('moisture',0),items=items)
        selected=land_select(material,seed_low16,tables['land'])
        # Existing prefix already owns Forest/Fungus/coast/River/Road geometry.
        for draw,pick in zip(commands[:4],selected[:4]):
            assert pick['role']=='base'
            draw['source_crop']=pick['crop'];draw['uv']=pick['uv']
    picked=select(items,band,nutrients,spec.get('landmark',0),spec.get('flags',0x21))
    return commands,samples,picked

def verify_frames(cases,asset_dir):
    from pathlib import Path
    import hashlib,struct
    from river_reference import render
    from water_coast_reference import generated_masks
    from terrain_triangle_raster_reference import pcx_crop
    assets=Path(asset_dir);cache={};masks=generated_masks(assets/'RAINFALL.PCX',cases['tables']['water_coast'])
    shadow=bytes(cases['shadow']);totals=[0,0,0]
    def load(d):
        k=tuple(d['source_crop'])
        if k not in cache:cache[k]=pcx_crop(assets/'TEXTURE.PCX',k)
        assert hashlib.sha256(cache[k]).hexdigest()==d['loaded_source_sha256']
        return cache[k],masks[0] if d['mode']=='coast_mapped' else None
    for f in cases['frames']:
        polygons=[d for d in f['draws'] if not d['mode'].startswith('sprite_')]
        out,results=render(dict(clip=f['clip'],draws=polygons),load,*cases['viewport'],cases['background']);out=bytearray(out)
        for d,r in zip(polygons,results):
            for k in ['native_spans','attempt_count','write_count','attempt_sha256','write_sha256']:assert d[k]==r[k],(f['id'],d['index'],k)
            totals[0]+=len(r['native_spans']);totals[1]+=r['attempt_count'];totals[2]+=r['write_count']
        for d in f['draws'][len(polygons):]:
            b=d['binding'];pixels,offset,size=sprite_pixels(b,assets/'TER1.PCX')
            assert offset==b['offset'] and size==b['size'] and b['pitch']==size[0],(f['id'],d['bank'],'trim',offset,size,b)
            assert hashlib.sha256(pixels).hexdigest()==b['retained_sha256']
            attempts,stores=paint(out,*cases['viewport'],f['clip'],b,d['anchor'],pixels,shadow if d['mode']=='sprite_shadow' else None)
            ab=b''.join(struct.pack('<HHIBBBB',*a) for a in attempts);wb=b''.join(struct.pack('<HHB',*s) for s in stores)
            assert len(attempts)==d['attempt_count'] and len(stores)==d['write_count'],(f['id'],d['index'],'counts')
            assert hashlib.sha256(ab).hexdigest()==d['attempt_sha256'],(f['id'],d['index'],'attempts')
            assert hashlib.sha256(wb).hexdigest()==d['write_sha256'],(f['id'],d['index'],'stores')
            totals[1]+=len(attempts);totals[2]+=len(stores)
        assert hashlib.sha256(out).hexdigest()==f['frame_sha256'],(f['id'],'frame')
    return totals
