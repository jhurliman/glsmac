"""Bounded native Detail Base visibility and9/16 University pop1 composite."""
from detail_minimap_reference import produce,flat_spans

def source_steps(count=800):
    high,low=16,9;q,r=divmod(high,low);error=r-2*low+(high if q&1 else 0);steps=[]
    for _ in range(count):
        error+=2*r;step=q
        if error>0:step+=1;error-=2*low
        steps.append(step)
    return steps

def admitted(spec):
    # Normal noneditor viewer3, explicitly active game, actual Base record0.
    if spec.get('no_base'):return False
    geography=bool(spec.get('known',8)&8) or spec.get('revealed',False)
    return geography and (spec.get('owner',3)==3 or bool(spec.get('visited',0)&8))

def verify(cases,assets,host_gdi_path,reserved):
    import json,hashlib,struct,runpy
    from pathlib import Path
    from territory_boundary_reference import palette_rgb
    from terrain_triangle_raster_reference import pcx_crop
    from developed_overlay_reference import shadow_lookup
    assets=Path(assets);colors=palette_rgb(assets/'PALETTE.PCX',reserved);flags={r['index']:r['common_palette_entry_flag'] for r in reserved}
    palette=bytes(v for i,rgb in enumerate(colors) for v in [*rgb,flags.get(i,5)])
    assert hashlib.sha256(palette).hexdigest()==cases['palette_sha256'];shadow=shadow_lookup(colors);assert list(shadow)==cases['shadow']
    host=runpy.run_path(str(host_gdi_path));C=host['C'];can=host['indexed_canvas'](239,119,palette)
    clip_api=host['api']('IntersectClipRect',[host['H'],host['I'],host['I'],host['I'],host['I']],host['I']);save=host['api']('SaveDC',[host['H']],host['I']);restore=host['api']('RestoreDC',[host['H'],host['I']],host['I'])
    fonts={}
    for f in cases['fonts']:
        record=f['font'];fid,_=host['font'](bytes.fromhex(record['LOGFONT_hex']));fonts[record['id']]=(fid,record)
    totals=[0,0,0];steps=source_steps()
    for c in cases['cases']:
        s=c['input'];view,entries,fills=produce(s);assert entries==c['entries'] and fills==c['fills'],s['id']
        output=bytearray(240*119)
        for fill in fills:
            for y,l,r in flat_spans(list(zip(fill['vertices'][::2],fill['vertices'][1::2])),fill['clip']):output[y*240+l:y*240+r]=bytes([fill['color']])*(r-l)
        visible=admitted(s);assert len(c['base_calls'])==int(visible) and not c['unit_calls'],(s['id'],'object admission')
        assert len(c['sprites'])==int(visible);attempts=[]
        if visible:
            tile=s.get('base_tile',[40,40]);entry=next(e for e in entries if e['tile']==tile);anchor=[entry['args'][-2],entry['args'][-1]-14]
            assert c['base_calls'][0][1:]==[0,*anchor,-7,0]
            sprite=c['sprites'][0];wall=s.get('cdf',-1)>=0;crop=[1,77 if wall else 1,100,75];raw=pcx_crop(assets/'UNIV.PCX',crop)
            points=[(i%100,i//100) for i,v in enumerate(raw) if v!=9];ox=min(x for x,y in points);oy=min(y for x,y in points);w=max(x for x,y in points)+1-ox;h=max(y for x,y in points)+1-oy
            source=b''.join(raw[(oy+y)*100+ox:(oy+y)*100+ox+w] for y in range(h))
            assert sprite['offset']==[ox,oy] and sprite['size']==[w,h] and sprite['ratio']==[9,16]
            assert sprite['args'][:3]==[9,*anchor] and hashlib.sha256(source).hexdigest()==sprite['source_sha256']
            def trunc16(value):return value//16 if value>=0 else -((-value)//16)
            left=trunc16(anchor[0]*16+ox*9);top=trunc16(anchor[1]*16+oy*9);right=left+w*9//16;bottom=top+h*9//16
            x0,x1=max(0,left),min(239,right);y0,y1=max(0,top),min(119,bottom)
            sy=sum(steps[:y0-top])
            for y in range(y0,y1):
                sx=sum(steps[:x0-left])
                for x in range(x0,x1):
                    raw=source[sy*w+sx];attempts.append((x,y,sx,sy,raw))
                    if raw!=9:output[y*240+x]=shadow[output[y*240+x]] if raw==0 else raw
                    sx+=steps[x-left]
                sy+=steps[y-top]
            rect=[anchor[0],anchor[1],anchor[0]+10,anchor[1]+11]
            expected=[dict(entry='0x5fc003',rect=rect,colors=[0,0])]
            if s.get('owner',3)!=3:expected.append(dict(entry='0x5f8ad0',rect=[rect[0]+1,rect[1]+1,rect[2]-1,rect[3]-1],colors=[252]))
            assert c['primitives']==expected,(s['id'],'population geometry',c['primitives'],expected)
            for p in expected:
                l,t,r,b=p['rect']
                for y in range(max(t,0),min(b,119)):
                    for x in range(max(l,0),min(r,239)):
                        if p['entry']=='0x5f8ad0' or x in [l,r-1] or y in [t,b-1]:output[y*240+x]=p['colors'][0]
            for text in c['text']:
                expected_rgb=[216,224,235] if s.get('owner',3)==3 else [0,0,0]
                assert text['text']=='1' and [text['x'],text['y']]==[anchor[0]+3,anchor[1]+9] and text['rgb']==expected_rgb
                assert text['alignment']==24 and text['background_mode']==1 and text['clip']==[0,0,239,119]
                fid,f=fonts[text['font']];assert (f['face'],f['height'],f['weight'])==('Arial',-9,700)
                C.memmove(can['bits'],bytes(output),len(output));saved=save(can['dc']);host['select'](can['dc'],host['fonts'][fid]);host['set_mode'](can['dc'],1);host['set_align'](can['dc'],24);host['set_color'](can['dc'],expected_rgb[0]|expected_rgb[1]<<8|expected_rgb[2]<<16);clip_api(can['dc'],0,0,239,119)
                assert host['text'](can['dc'],anchor[0]+3,anchor[1]+9,b'1',1);restore(can['dc'],saved);output=bytearray(C.string_at(can['bits'],len(output)));totals[2]+=1
            totals[0]+=1
        else:assert not c['primitives'] and not c['text']
        packed=b''.join(struct.pack('<HHHHB',*a) for a in attempts)
        assert len(attempts)==c['samples'] and hashlib.sha256(packed).hexdigest()==c['sample_sha256'],(s['id'],'sample stream')
        assert hashlib.sha256(output).hexdigest()==c['frame_sha256'],(s['id'],'complete indexed frame')
        totals[1]+=len(attempts)
    return totals
