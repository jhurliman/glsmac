"""Bounded ordinary land bank/UV/ordered-pass selector; no asset bytes."""
def variation(seed_low16):
    return ((((seed_low16*1664525+1013904223)&65535)*2)>>16)

def context_tile(spec,x,y):
    if y<0 or y>=80:return None
    if spec.get('flat_map') and (x<0 or x>=80):return None
    x%=80;cx,cy=spec.get('tile',[40,40]);m=spec['moisture'];band=3;items=0
    if (x,y)==(cx,cy):m=spec.get('center_moisture',m);items=spec.get('items',0)
    for key in ['neighbor_moisture','neighbor_band','neighbor_items']:
        if key in spec:
            dx,dy,v=spec[key]
            if (x,y)==((cx+dx)%80,cy+dy):
                if key=='neighbor_moisture':m=v
                elif key=='neighbor_band':band=v
                else:items=v
    return dict(band=band,moisture=m,items=items)

def select(spec,seed_low16,tables):
    x,y=spec.get('tile',[40,40]);center=context_tile(spec,x,y);m=center['moisture'];rock=spec.get('rock',0);items=center['items']
    if m not in [0,1,2] or rock not in [0,1,2]:raise ValueError('Outside ordinary moisture/rock domain')
    mask=0;fungusmask=0;pattern=tables['pattern_lookup']
    for bit in range(8):
        dx,dy=tables['neighbor_offsets'][(bit-1)&7];t=context_tile(spec,x+dx,y+dy)
        if t is None:continue
        if t['band']<3 or t['moisture']>=m:mask|=1<<bit
        if t['band']>=2 and t['items']&0x20:fungusmask|=1<<bit
    idx,turn=pattern[mask];v=variation(seed_low16);farm=bool(items&0x8000)
    if farm:crop=tables['farm'][3*v+m]
    elif m==0:crop=tables['arid']
    else:crop=tables['moist' if m==1 else 'rainy'][idx]
    result=[]
    for tri in range(4):
        uv=tables['canonical_uv'][(tri+(turn if m and not farm else 0))&3]
        result.append(dict(role='base',crop=crop,uv=uv,flags='0xffffffff'))
        if rock:result.append(dict(role='rock',crop=tables['rocks'][2*(rock-1)+v],uv=tables['canonical_uv'][tri],flags='0xffffffff'))
    if items&0x200000:
        fmask=0
        for bit,(dx,dy) in enumerate(tables['forest_neighbor_offsets']):
            t=context_tile(spec,x+dx,y+dy)
            if t and t['items']&0x200000:fmask|=1<<bit
        result.append(dict(role='forest',crop=tables['forest'][fmask],uv=[0,0,55,0,55,55,0,55],flags=None))
    if items&0x20:
        fungus_index,fungus_turn=pattern[fungusmask]
        # Native quad producer reads vertex1 from each rotated canonical fan UV.
        quad_uv=[]
        for j in range(4):quad_uv.extend(tables['canonical_uv'][(fungus_turn+j)&3][2:4])
        result.append(dict(role='fungus',crop=tables['fungus'][fungus_index],uv=quad_uv,flags='0xffffffff'))
    return result

def common_palette_rgb(palette_pcx,index):
    """Closed common-palette range at native Gamma Correction100."""
    if not 10<=index<=245:raise ValueError('Reserved/system palette entries require the active table')
    data=bytes(palette_pcx)
    if data[-769]!=12:raise ValueError('Expected256-color PCX palette trailer')
    off=(index-10)*3;return tuple(min(v,235) for v in data[-768:][off:off+3])

def final_index(loaded_index,shade=0,keyed=False):
    if keyed and loaded_index==9:return None
    return (loaded_index+shade)&255

if __name__=='__main__':
    from pathlib import Path
    import json,argparse
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--palette-pcx');args=p.parse_args()
    d=json.loads(Path(__file__).with_name('ordinary-land-texture-palette-cases.json').read_text());count=0
    for c in d['cases']:
        got=select(c['input'],c['variation_seed_low16'],d['tables']);expected=c['draws']
        assert len(got)==len(expected),c['input']['id']
        for a,b in zip(got,expected):
            assert a['crop']==b['source_crop'],c['input']['id']
            for k in ['uv','flags']:
                assert a[k]==b[k],(c['input']['id'],k)
            count+=1
    if args.palette_pcx:
        data=Path(args.palette_pcx).read_bytes()
        for s in d['palette']['samples']:assert list(common_palette_rgb(data,s['index']))==s['rgb']
    assert final_index(9,2,True) is None and final_index(10,-2,True)==8
    print('PASS',len(d['cases']),'native contexts,',count,'ordered texture calls')
