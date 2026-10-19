"""Finite SMAC Turn Complete state, placement and event-driven frame reference."""
def display_state(state):
    complete=bool(state['scenario_flags']&2)
    network=bool(state['network']);sim=bool(state['simultaneous_flags']&16)
    mask=state['done_mask'];pending=bool(state['command_pending'])
    active=complete and (not network or not sim or state['active_faction']==state['viewer'])
    active=active and (pending or (network and mask not in [0,255]))
    waiting=network and complete and not sim and mask!=255 and bool(mask&(1<<state['viewer']))
    return bool(active),'WAITING FOR OTHERS' if waiting else 'TURN COMPLETE'


def label_position(rect,width,centering_height=11,baseline_advance=11):
    left,top,right,bottom=rect
    # Signed C truncation, distinct from floor for hypothetical negative space.
    trunc2=lambda n:abs(n)//2*(-1 if n<0 else 1)
    return left+trunc2(right-left-width),top+trunc2(bottom-top-centering_height)+baseline_advance


def hit(rect,xy):
    return rect[0]<=xy[0]<rect[2] and rect[1]<=xy[1]<rect[3]


class FrameCursor:
    def __init__(self,frames,count=19):
        self.frames=frames;self.count=count;self.index=0;self.cursor=0

    def draw(self,active):
        if not active:self.index=0;self.cursor=0
        if self.index==self.count:self.index=0;self.cursor=1
        result=dict(index=self.index,file_offset=self.frames[self.cursor]['offset'])
        self.index+=1;self.cursor+=1
        return result


def verify(cases):
    initial=cases['initial'];rect=initial['rect']
    assert rect==[783,61,1016,84] and (rect[2]-rect[0],rect[3]-rect[1])==(233,23)
    for case in cases['cases']:
        active,label=display_state(case['input'])
        assert int(active)==case['animation_active'],case['id']
        text=case['text'];assert len(text)==1 and text[0]['text']==label
        assert list(label_position(rect,initial['text_widths'][label]))==[text[0]['x'],text[0]['y']]
        assert text[0]['rgb']==[228,104,24]
    cursor=FrameCursor(cases['asset']['physical_frames'])
    for case in cases['active_redraw_sequence']:
        assert [cursor.draw(True)]==case['decoded'],case['invocation']
        assert cursor.index==case['next_index']
    for case in cases['hit_tests']:
        assert hit(rect,case['xy'])==(case['control_id']==5)
    for case in cases['timer']:
        if case['id']=='admitted':
            assert case['counter_after']==11 and len(case['decoded'])==1
        else:assert case['counter_after']==10 and not case['decoded'] and case['frame_after']==case['frame_before']
    by_id={c['id']:c for c in cases['interaction']}
    assert by_id['enter']['hover_after']==1 and len(by_id['enter']['decoded'])==1
    assert by_id['repeat']['hover_after']==1 and not by_id['repeat']['decoded']
    assert by_id['exit']['hover_after']==0 and len(by_id['exit']['decoded'])==1
    assert by_id['click_inside']['calls']==['0x52aa80'] and not by_id['click_outside']['calls']
    for case in cases['activation']:
        proceed=bool(case['command_enabled']) and (bool(case['flags_before']&2) or bool(case['confirmation_answer']))
        assert case['flags_after']==(case['flags_before']|2 if proceed else case['flags_before'])
        assert case['command_enabled_after']==(0 if proceed else case['command_enabled'])
        assert case['pending_command_after']==(0 if proceed else 7)
    return len(cases['cases']),len(cases['active_redraw_sequence']),len(cases['interaction']),len(cases['hit_tests']),len(cases['timer']),len(cases['activation'])


def decode_turn_frame(data,offset,image,background):
    """Only measured turn.flc chunks4/15/7, direct indexed mode/key0.

    Palette chunks do not change this indexed comparison; RGB authority is
    the initialized common palette. Background is the captured backing image.
    """
    import struct
    w,h=233,23
    size,magic,chunks=struct.unpack_from('<IHH',data,offset)
    assert magic==0xf1fa
    end=offset+size;p=offset+16;out=bytearray(image)
    def put(x,y,raw):
        assert 0<=x<w and 0<=y<h
        i=y*w+x;out[i]=raw if raw else background[i]
    for _ in range(chunks):
        chunk_size,kind=struct.unpack_from('<IH',data,p);q=p+6;chunk_end=p+chunk_size
        if kind==4:pass
        elif kind==15:
            for y in range(h):
                q+=1;x=0 # Native ignores BRUN packet count, uses row width.
                while x<w:
                    count=struct.unpack_from('<b',data,q)[0];q+=1
                    if count>0:
                        raw=data[q];q+=1
                        for _ in range(count):put(x,y,raw);x+=1
                    else:
                        assert count<0
                        for _ in range(-count):put(x,y,data[q]);x+=1;q+=1
        elif kind==7:
            lines=struct.unpack_from('<H',data,q)[0];q+=2;y=0
            while lines:
                code=struct.unpack_from('<H',data,q)[0];q+=2
                if code&0xc000==0xc000:
                    y-=struct.unpack('<h',struct.pack('<H',code))[0];continue
                if code&0x8000:
                    put(w-1,y,code&255);continue
                x=0
                for _ in range(code):
                    skip,count=struct.unpack_from('<Bb',data,q);q+=2;x+=skip
                    if count>=0:
                        for _ in range(2*count):put(x,y,data[q]);q+=1;x+=1
                    else:
                        pair=data[q:q+2];q+=2
                        for _ in range(-count):
                            for raw in pair:put(x,y,raw);x+=1
                lines-=1;y+=1
        else:raise ValueError('Unmeasured FLC chunk')
        assert q<=chunk_end
        p=chunk_end
    assert p==end
    return bytes(out)


def verify_art(cases,path):
    import hashlib
    from pathlib import Path
    data=Path(path).read_bytes();assert hashlib.sha256(data).hexdigest()==cases['asset']['sha256']
    background=bytes([100])*(233*23);image=background
    for frame in cases['active_redraw_sequence']:
        image=decode_turn_frame(data,frame['decoded'][0]['file_offset'],image,background)
        assert hashlib.sha256(image).hexdigest()==frame['indexed_sha256'],frame['invocation']
    return len(cases['active_redraw_sequence'])


if __name__=='__main__':
    from pathlib import Path
    import argparse,json
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cases',default=str(Path(__file__).with_name('turn-complete-cases.json')))
    parser.add_argument('--turn-flc')
    args=parser.parse_args()
    cases=json.loads(Path(args.cases).read_text())
    print('PASS state/frame/interaction/hit/timer/activation:',verify(cases))
    if args.turn_flc:print('PASS',verify_art(cases,args.turn_flc),'native indexed art frames')
