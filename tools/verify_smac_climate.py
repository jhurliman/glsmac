#!/usr/bin/env python3
"""Replay frozen climate controls through the production C++ probe.

Default: twelve original synthetic controls, without installed assets.
--prepared-planet accepts a LOCAL 3200x44-byte plane with the frozen admission
hash, for all43 original controls. No original map data is distributed here.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS=('elevation','temperature','moisture','contour','aux','items','landmark_word','cached_occupant','rockiness')
RULES=('pattern','extra','temperature','rainfall','latitude_divisor','band_divisor','jitter_divisor','warming_divisor','mountain_shadow','hill_shadow','shadow_step')

def decode(raw):
    return [dict(zip(FIELDS,(t[0]>>5,t[0]&7,(t[0]>>3)&3,t[1],t[6],*struct.unpack_from('<II',t,8),t[2]&15,t[5]>>6)),remembered_items=list(struct.unpack_from('<7I',t,16)))
        for t in [raw[i:i+44] for i in range(0,len(raw),44)]]

def encode(tiles,raw):
    out=bytearray(raw)
    for i,t in enumerate(tiles):
        at=i*44;out[at]=(t['elevation']<<5)|(t['moisture']<<3)|t['temperature'];out[at+1]=t['contour'];out[at+6]=t['aux']
        out[at+2]=(out[at+2]&240)|t['cached_occupant'];out[at+5]=(out[at+5]&63)|(t['rockiness']<<6)
        struct.pack_into('<II7I',out,at+8,t['items'],t['landmark_word'],*t['remembered_items'])
    return bytes(out)

def synthetic(name):
    out=bytearray(3200*44)
    for y in range(80):
        for x in range(y&1,80,2):
            at=(y*40+x//2)*44;out[at]=3<<5|2;out[at+1]=50+(x%8)
            if name!='river_uniform_land' and (y<8 or y>71):out[at]=2<<5|2;out[at+1]=44
    def item(x,y,word):struct.pack_into('<I',out,(y*40+x//2)*44+8,word)
    item(40,40,0x100)
    if name=='river_terminal_feature':item(40,40,0x100|0x1000000)
    if name=='river_crowded_source':
        for dx,dy in [(1,-1),(1,1),(-1,1),(-1,-1)]:item(40+dx,40+dy,0x100)
    if name.startswith('river_seam'):item(40,40,0);item(0,40,0x100)
    # uniform, terminal-feature and crowded-source controls have no polar sea.
    if name in ('river_terminal_feature','river_crowded_source'):
        for y in list(range(8))+list(range(72,80)):
            for x in range(y&1,80,2):at=(y*40+x//2)*44;out[at]=3<<5|2;out[at+1]=50+(x%8)
    return bytes(out)

def probe(binary,mode,raw,case,sea=0,turn=90,arguments=(0,0,0)):
    rules=case['rules'];head=[mode,case['width'],case['height'],int(not case['flat']),*[rules[k] for k in RULES],sea,turn,0,
        case['shared_seed_before'],case['CRT_seed_before'],*arguments]
    tiles=decode(raw)
    text=' '.join(map(str,head))+'\n'+''.join(' '.join(map(str,[*[t[k] for k in FIELDS],*t['remembered_items']]))+'\n' for t in tiles)
    result=subprocess.run([str(binary)],input=text,text=True,capture_output=True,check=True)
    lines=result.stdout.splitlines();state=list(map(int,lines[0].split()));seeds=list(map(int,lines[1].split()));draws=list(map(int,lines[2].split()))
    assert seeds[0]==len(seeds)-1 and draws[0]==len(draws)-1
    output=[]
    for line in lines[3:]:
        values=list(map(int,line.split()));assert len(values)==16
        output.append(dict(zip(FIELDS,values[:9]),remembered_items=values[9:]))
    assert len(output)==len(tiles)
    return encode(output,raw),state,seeds[1:],draws[1:]

def inputs(case,baseline,binary,cases):
    name=case['id'];raw=synthetic(name) if name.startswith('river_') else bytes(baseline)
    tiles=decode(raw)
    def tile(x,y):return tiles[y*40+x//2]
    if name=='temperature_landmark_and_feature':
        for x,detail in [(10,20),(12,21),(14,255)]:tile(x,40)['landmark_word']=1|(detail<<24)
        tile(20,40)['items']|=0x1000000
    if name.startswith('rainfall_features_') or name=='rainfall_flat':
        for x,mask in [(10,0x400000),(12,0x400000),(18,0x200000)]:tile(x,40)['items']|=mask
        for x,lm in [(26,0x40),(28,0x100),(30,4|(48<<24)),(32,4|(49<<24)),(34,4|(80<<24)),(36,4|(81<<24)),(38,4|(255<<24))]:tile(x,40)['landmark_word']=lm
    raw=encode(tiles,raw)
    if name=='complete_recalc_after_volcanic_geometry':raw=probe(binary,'volcano',raw,cases['ecology_volcanic_geometry'],arguments=(24,16,6))[0]
    return raw

def replay(binary,prepared=None,quiet=False):
    data=json.loads((ROOT/'docs/behavior/climate-recalculation-rules-cases.json').read_bytes());cases={c['id']:c for c in data['cases']}
    baseline=Path(prepared).read_bytes() if prepared else None
    if baseline is not None:assert len(baseline)==140800 and hashlib.sha256(baseline).hexdigest()==cases['temperature_extra0']['map_before_sha256']
    count=0
    for case in data['cases']:
        name=case['id']
        if baseline is None and not name.startswith('river_'):continue
        raw=inputs(case,baseline,binary,cases)
        assert hashlib.sha256(raw).hexdigest()==case['map_before_sha256'],(name,'input differs from original control')
        mode='rivers' if name.startswith('river') else 'temperature' if name.startswith('temperature') else 'rainfall' if name.startswith('rainfall') else 'altitude' if name.startswith('altitude') else 'shape' if name.startswith('shape') else 'volcano' if name.startswith('ecology_volcanic') else 'complete'
        sea=1 if name in ('altitude_offset1','altitude_turn0','complete_recalc_rising_four_passes') else -1 if name in ('altitude_offset-1','complete_recalc_falling_four_passes') else 0
        args=tuple(case['arguments'][:3]) if mode=='shape' else (*case['arguments'][:2],6) if mode=='volcano' else (0,0,0)
        after,state,seeds,draws=probe(binary,mode,raw,case,sea,0 if name=='altitude_turn0' else 90,args)
        assert hashlib.sha256(after).hexdigest()==case['map_after_sha256'],(name,'complete output differs from original')
        assert state[:4]==[case['shared_seed_after'],case['CRT_seed_after'],case['shared_draw_count'],len(case['CRT_draws'])],(name,'random state/count')
        assert seeds==case['shared_seed_requests'] and draws==case['CRT_draws'],(name,'random trace')
        count+=1
        if not quiet:print('PASS',name,'tiles',case['changed_tile_count'],'shared',state[2],'CRT',state[3])
    return count

def replay_retained7(binary,prepared=None):
    data=json.loads((ROOT/'docs/behavior/climate-retained-class7-cases.json').read_bytes())
    baseline=Path(prepared).read_bytes() if prepared else None
    if baseline is not None:
        old=json.loads((ROOT/'docs/behavior/climate-recalculation-rules-cases.json').read_bytes())
        expected=next(c for c in old['cases'] if c['id']=='temperature_extra0')['map_before_sha256']
        assert len(baseline)==140800 and hashlib.sha256(baseline).hexdigest()==expected
    count=0
    for case in data['cases']:
        name=case['id']
        mixed=name in ('temperature_retained7','rainfall_retained7','recalculate_altitudes_incoming7','full_recalculate_incoming7')
        if mixed and baseline is None:continue
        raw=bytearray(baseline if mixed else synthetic('river_uniform_land'))
        if mixed:
            for i in range(3200):
                if name in ('recalculate_altitudes_incoming7','full_recalculate_incoming7') or (i//40==40 and i%40 in (10,11,12,13,14)):
                    raw[44*i]=(raw[44*i]&31)|224
        elif name in ('river_initial7_uphill7','assign_incoming7_to6','shape6_retains_neighbor7'):
            for i in range(3200):
                at=44*i;raw[at:at+44]=bytes(44);raw[at]=224|2;raw[at+1]=110 if name.startswith('river') else 150;raw[at+2]=15
            if name.startswith('river'):
                at=44*(40*40+20);raw[at+1]=100;struct.pack_into('<I',raw,at+8,0x100)
        else:
            struct.pack_into('<I',raw,44*(40*40+20)+8,0)
            at=44*(16*40+12);raw[at]=(192 if name=='assign_requested7' else 224)|2;raw[at+1]=150
        assert hashlib.sha256(raw).hexdigest()==case['before_map_sha256'],(name,'input differs from original control')
        normalized=dict(case,width=80,height=80,flat=False,shared_draw_count=case['shared_draws'])
        mode='temperature' if name.startswith('temperature') else 'rainfall' if name.startswith('rainfall') else 'rivers' if name.startswith('river') else 'assign' if name.startswith('assign') else 'shape' if name.startswith('shape') else 'altitude' if name.startswith('recalculate') else 'complete'
        arguments=tuple(case['arguments'][:3]) if case['arguments'] else (0,0,0)
        after,state,seeds,draws=probe(binary,mode,raw,normalized,arguments=arguments)
        assert hashlib.sha256(after).hexdigest()==case['after_map_sha256'],(name,'complete output differs from original')
        assert state[:4]==[case['shared_seed_after'],case['CRT_seed_after'],case['shared_draws'],len(case['CRT_draws'])],(name,'random state/count')
        assert draws==case['CRT_draws'],(name,'CRT trace')
        count+=1
        print('PASS',name,'tiles',case['changed_tiles'],'shared',state[2],'CRT',state[3])
    return count

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--probe',type=Path,required=True);parser.add_argument('--prepared-planet',type=Path);args=parser.parse_args()
    print('Passed',replay(args.probe,args.prepared_planet)+replay_retained7(args.probe,args.prepared_planet),'original complete-plane controls')
