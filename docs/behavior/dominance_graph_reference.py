"""Retained signed-short dominance history and lower-console graph geometry."""
from terrain_geometry_reference import trunc_div


def new_history():
    return [[0]*8 for _ in range(1000)]


def publish(history,turn,composite_scores):
    """Scores are the provider's pre-consumption values, not rank sentinels."""
    if turn<0:raise ValueError('Negative native turn indices are outside the valid storage domain')
    if turn>=1000:return
    for faction in range(1,8):history[turn][faction]=((composite_scores[faction]+32768)&65535)-32768


def serialize(history):
    import struct
    return struct.pack('<8000h',*(v for row in history for v in row))


def deserialize(payload):
    import struct
    if len(payload)!=16000:raise ValueError('Expected1000 rows of8 little-endian signed shorts')
    values=struct.unpack('<8000h',payload)
    return [list(values[i:i+8]) for i in range(0,8000,8)]


def graph_rectangle(column_clip):
    l,t,r,b=column_clip
    return [l+3,t+3,r-3,b-3]


def graph(turn,rect,font_advance,current_scores,history,filenames,colors,spartan_colors):
    """Returns unclipped native line requests; clip them to rect during drawing."""
    if not 0<=turn<1000:raise ValueError('Reader beyond the retained history capacity is unclosed')
    l,t,r,b=rect;width=r-l;available=b-t-font_advance-4
    if width<=0 or available<=0:raise ValueError('Invalid graph dimensions')
    x_step=1
    while turn//x_step>width:x_step+=1
    maximum=max(1,*current_scores[1:8]);y_divisor=max(1,(maximum+available-1)//available)
    reads=[];lines=[]
    for faction in range(1,8):
        previous=None
        color=(spartan_colors if filenames[faction]=='SPARTANS' else colors)[faction]
        for point,sample_turn in enumerate(range(1,turn+1,x_step)):
            value=history[sample_turn][faction]
            reads.append(dict(turn=sample_turn,faction=faction,value=value))
            xy=[l+point,b-trunc_div(value,y_divisor)-1]
            if previous is not None:lines.append([*previous,*xy,color])
            previous=xy
    return dict(rect=rect,available_height=available,x_step=x_step,y_divisor=y_divisor),reads,lines


def verify(cases):
    import hashlib
    history=new_history();writer=cases['writer']
    for c in writer['cases']:
        scores=[0]+[4*c['population'][f] if c['active_mask']&(1<<f) else 0 for f in range(1,8)]
        assert c['events'][0]['scores']==scores,c['id']
        publish(history,c['turn'],scores)
        if c['turn']<1000:assert history[c['turn']]==c['history_row'],c['id']
        assert len(c['writes'])==(7 if c['turn']<1000 else 0),c['id']
        if c['mode']==0:assert c['working_scores']==scores,c['id']
        else:assert c['working_scores']==[0]+[-1]*7,c['id']
    assert hashlib.sha256(serialize(history)).hexdigest()==writer['serialization'][0]['payload_sha256']
    assert serialize(deserialize(serialize(history)))==serialize(history)
    restored=bytes((i*37+11)&255 for i in range(16000))
    assert hashlib.sha256(restored).hexdigest()==writer['serialization'][1]['payload_sha256']
    assert serialize(deserialize(restored))==restored
    count=0
    for c in cases['reader_cases']:
        spec=c['input'];turn=spec['turn'];mult=spec.get('history_multiplier',1)
        history=[[1234]+[((sample*f+17*f)%300)*mult for f in range(1,8)] for sample in range(1000)]
        populations=spec.get('populations',[4*f*spec.get('population_multiplier',1) for f in range(8)])
        scores=[0]+[4*populations[f] if spec.get('active_mask',254)&(1<<f) else 0 for f in range(1,8)]
        assert scores==c['current_scores'],spec['id']
        publish(history,turn,scores);assert history[turn]==c['current_history'],spec['id']
        rect=graph_rectangle(cases['column_clips'][spec.get('column',1)])
        geometry,reads,lines=graph(turn,rect,cases['font_advance'],scores,history,cases['filenames'],cases['colors'],cases['spartan_colors'])
        assert geometry==c['geometry'],spec['id']
        assert reads==c['history_reads'],(spec['id'],'reads')
        assert lines==c['lines'],(spec['id'],'lines')
        count+=len(lines)
    return len(writer['cases']),len(cases['reader_cases']),count


if __name__=='__main__':
    from pathlib import Path
    import json
    cases=json.loads(Path(__file__).with_name('dominance-graph-cases.json').read_text())
    print('PASS writer contexts, graph contexts, native line requests:',verify(cases))
