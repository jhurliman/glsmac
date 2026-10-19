"""Bounded original University city selection and population composite."""
from detail_minimap_reference import produce,flat_spans
from detail_minimap_bases_reference import source_steps

def body_selection(spec,standard_limits=(7,14)):
    pop=spec.get('population',9)
    if not 0<=pop<=127:raise ValueError('Signed population byte outside measured0..127')
    complex_limit,dome_limit=spec.get('limits',standard_limits)
    if not all(isinstance(v,int) and 0<=v<=126 for v in (complex_limit,dome_limit)):raise ValueError('Rules limits outside bounded nonoverflowing profile')
    size=0 if pop<4 else 1 if pop<max(7,complex_limit+1) else 2 if pop<max(10,dome_limit+1) else 3
    facilities=spec.get('facilities',[])
    if 1 in facilities:size=min(3,size+1)
    cdf=spec.get('cdf',False) or spec.get('cdf_owner',-1)==spec.get('owner',3)
    defense=2 if 5 in facilities else 1 if 4 in facilities or cdf else 0
    return int(spec.get('sea',False)),size,defense

def filled_population_box(spec,viewer=3):
    unit=spec.get('unit_tile');owner=spec.get('owner',3)
    if unit==[40,40]:return True
    access=(owner==viewer or spec.get('owner_relation',0)&1 or spec.get('viewer_relation',0)&0x1000 or spec.get('revealed') or spec.get('empath') or spec.get('governor'))
    if unit and spec.get('unit_owner')==viewer and (unit[0]-40,unit[1]-40) in [(-2,0),(-1,-1),(0,-2),(1,-1),(2,0),(1,1),(0,2),(-1,1)]:access=True
    return not access

def verify(n,assets,host_gdi_path,reserved):
    from pathlib import Path
    import hashlib,struct,runpy
    from territory_boundary_reference import palette_rgb
    from developed_overlay_reference import shadow_lookup
    from terrain_triangle_raster_reference import pcx_crop
    assets=Path(assets);assert hashlib.sha256((assets/'alpha.txt').read_bytes()).hexdigest()==n['alpha_sha256'];assert hashlib.sha256((assets/'UNIV.PCX').read_bytes()).hexdigest()==n['university_sha256']
    colors=palette_rgb(assets/'PALETTE.PCX',reserved);flags={r['index']:r['common_palette_entry_flag'] for r in reserved};palette=bytes(v for i,rgb in enumerate(colors) for v in [*rgb,flags.get(i,5)])
    assert hashlib.sha256(palette).hexdigest()==n['palette_sha256'];shadow=shadow_lookup(colors)
    slots=[{name:pcx_crop(assets/'PALETTE.PCX',[580+8*i,y,1,1])[0] for name,y in [('primary',23),('contrast',31),('text',39)]} for i in range(8)];assert slots==n['owner_slots']
    section=(assets/'alpha.txt').read_text().split('#FACTIONS\n',1)[1].split('\n#',1)[0]
    assert n['faction_filenames']==['']+[line.split(',')[0].strip() for line in section.splitlines() if ',' in line]
    banks={}
    for b in n['bindings']:
        sea,size,defense=b['sea'],b['size_class'],b['defense'];crop=[1+101*size,1+76*(3*sea+defense),100,75];assert b['crop']==crop and b['slot']==3*(size+4*sea)+defense
        raw=pcx_crop(assets/'UNIV.PCX',crop);points=[(i%100,i//100) for i,v in enumerate(raw) if v!=9];ox=min(x for x,y in points);oy=min(y for x,y in points);w=max(x for x,y in points)+1-ox;h=max(y for x,y in points)+1-oy
        source=b''.join(raw[(oy+y)*100+ox:(oy+y)*100+ox+w] for y in range(h))
        assert b['offset']==[ox,oy] and b['size']==[w,h] and hashlib.sha256(source).hexdigest()==b['source_sha256']
        banks[sea,size,defense]=(b,source)
    assert len(banks)==24
    host=runpy.run_path(str(host_gdi_path));C=host['C'];canvas=host['indexed_canvas'](239,119,palette)
    clip=host['api']('IntersectClipRect',[host['H'],host['I'],host['I'],host['I'],host['I']],host['I']);save=host['api']('SaveDC',[host['H']],host['I']);restore=host['api']('RestoreDC',[host['H'],host['I']],host['I'])
    fonts={f['font']['id']:f['font'] for f in n['fonts']};font_instances={};totals=[0,0,0]
    for c in n['cases']:
        s=c['input'];main=s.get('mode')=='main';population=s.get('population',9);owner=s.get('owner',3);zoom=0 if main else -7;anchor=[48,16] if main else [112,42]
        if main:
            assert not c['entries'] and not c['fills'];output=bytearray([22])*(240*119)
        else:
            _,entries,fills=produce(s)
            if s.get('sea'):
                for fill in fills:fill['color']=190
            assert entries==c['entries'] and fills==c['fills'],(s['id'],'terrain')
            output=bytearray(240*119)
            for fill in fills:
                for y,l,r in flat_spans(list(zip(fill['vertices'][::2],fill['vertices'][1::2])),fill['clip']):output[y*240+l:y*240+r]=bytes([fill['color']])*(r-l)
        assert len(c['base_calls'])==1 and c['base_calls'][0][1:]==[0,*anchor,zoom,0] and not c['unit_calls']
        assert c['facility_calls']==[1,4,5]
        assert len(c['sprites'])==1;sp=c['sprites'][0];b,source=banks[body_selection(s,n['rules'])]
        ratio=[zoom+16,16];assert sp['bank']==b['bank'] and sp['size']==b['size'] and sp['offset']==b['offset'] and sp['source_sha256']==b['source_sha256'] and sp['ratio']==ratio
        assert sp['args'][:3]==[9,*anchor]
        ox,oy=b['offset'];w,h=b['size'];num=ratio[0];left=int((anchor[0]*16+ox*num)/16);top=int((anchor[1]*16+oy*num)/16);right=left+w*num//16;bottom=top+h*num//16
        steps=[1]*800 if main else source_steps();attempts=[];sy=0
        for y in range(top,bottom):
            sx=0
            for x in range(left,right):
                value=source[sy*w+sx]
                if 0<=x<239 and 0<=y<119:
                    attempts.append((x,y,sx,sy,value))
                    if value!=9:output[y*240+x]=shadow[output[y*240+x]] if value==0 else value
                sx+=steps[x-left]
            sy+=steps[y-top]
        if not main:
            packed=b''.join(struct.pack('<HHHHB',*a) for a in attempts);assert len(attempts)==c['sample_count'] and hashlib.sha256(packed).hexdigest()==c['sample_sha256'],(s['id'],'source samples');totals[2]+=len(attempts)
        assert len(c['text'])==len(c['measurements'])==1
        text=c['text'][0];font=fonts[text['font']]
        if text['font'] not in font_instances:
            fid,metrics=host['font'](bytes.fromhex(font['LOGFONT_hex']));assert metrics.hex()==font['metrics_hex'];font_instances[text['font']]=fid
        fid=font_instances[text['font']];requested=(50*(zoom+16)//16)//3;assert (font['face'],font['height'],font['weight'])==('Arial',-requested,700)
        logical_height=font['ascent']-font['internal_leading'];measure_text='8' if population<10 else '88';measure_width=host['width'](fid,measure_text)[0]
        measurement=c['measurements'][0];assert measurement['text']==measure_text and measurement['font_height']==logical_height and measurement['width']==measure_width
        filled=filled_population_box(s);special=n['faction_filenames'][owner]=='SPARTANS';border=slots[owner]['contrast'] if filled and special else 0;foreground=(44 if special else 0) if filled else slots[owner]['text']
        l,t=anchor;r=l+measure_width+5;bottom=t+logical_height+4;rect=[l,t,r,bottom]
        expected=[dict(entry='0x5fc003',rect=rect,colors=[border,border])]
        if filled:expected.append(dict(entry='0x5f8ad0',rect=[l+1,t+1,r-1,bottom-1],colors=[slots[owner]['primary']]))
        assert c['primitives']==expected,(s['id'],'population box',c['primitives'],expected)
        for p in expected:
            l,t,r,bottom=p['rect']
            for y in range(max(t,0),min(bottom,119)):
                for x in range(max(l,0),min(r,239)):
                    if p['entry']=='0x5f8ad0' or x in [l,r-1] or y in [t,bottom-1]:output[y*240+x]=p['colors'][0]
        x,y=anchor[0]+3,anchor[1]+logical_height+2;rgb=colors[foreground]
        assert text['text']==str(population) and [text['x'],text['y']]==[x,y] and text['rgb']==rgb and text['alignment']==24 and text['background_mode']==1 and text['clip']==[0,0,239,119],(s['id'],'text request')
        C.memmove(canvas['bits'],bytes(output),len(output));saved=save(canvas['dc']);host['select'](canvas['dc'],host['fonts'][fid]);host['set_mode'](canvas['dc'],1);host['set_align'](canvas['dc'],24);host['set_color'](canvas['dc'],rgb[0]|rgb[1]<<8|rgb[2]<<16);clip(canvas['dc'],0,0,239,119)
        assert host['text'](canvas['dc'],x,y,str(population).encode(),len(str(population)));restore(canvas['dc'],saved);output=bytes(C.string_at(canvas['bits'],len(output)))
        assert hashlib.sha256(output).hexdigest()==c['frame_sha256'],(s['id'],'complete indexed frame')
        totals[int(main)]+=1
    return totals
