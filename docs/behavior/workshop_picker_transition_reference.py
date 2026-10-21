"""Independent accepted-picker mutation bodies, separate from modal admission.

Caller supplies an admitted mapped component ID/rating or accepted ability ID.
Technology/suggestion profile is the checked owner1 all-tech fixture, optionally
without Neural Grafting, with automatic suggestions enabled/disabled explicitly.
"""
import struct
from workshop_general_reference import compatible,sanitize

def transition(before,job,rules,flight_ranges):
    state=dict(before['fields']);slots=list(before['slots']);second=job.get('second',True)
    def rebuild():return ([i for i in range(24) if state['mask']&(1<<i)]+[-1,-1])[:2]
    def cleanup():
        nonlocal slots
        state['mask'],slots=sanitize(state['mask'],slots,state['chassis'],state['weapon'],state['armor'],second,rules)
    def suggest(flag):
        nonlocal slots
        cleanup();bit=flag.bit_length()-1
        if not job.get('auto',True) or not compatible(bit,state['chassis'],state['weapon'],state['armor'],rules):return
        count=state['mask'].bit_count()
        if count>1 or count==1 and not second:return
        state['mask']|=flag;slots=rebuild()
    CH=rules['chassis'];WE=rules['weapons'];AR=rules['armor'];kind=job['kind'];new=job['new']
    if kind=='chassis':
        old_domain=CH[state['chassis']]['domain'];state['chassis']=new
        if CH[new]['special']:
            state['armor']=0
            if WE[state['weapon']]['mode']>=2:state['weapon']=0
            if state['weapon']!=13:state['weapon']=20
        elif WE[state['weapon']]['offense']>=99 or state['weapon']==20:state['weapon']=0
        domain=CH[new]['domain']
        if domain!=old_domain:
            if domain:
                suggest(2)
                if domain==2 and (WE[state['weapon']]['mode']==11 or WE[state['weapon']]['mode']==9 and flight_ranges[new]):state['weapon']=0
            elif state['mask']&2:state['mask']&=~2;slots=rebuild()
        cleanup()
    elif kind=='weapon':
        old=state['weapon'];state['weapon']=new;ch=state['chassis']
        if CH[ch]['domain']==2:
            if WE[state['weapon']]['mode']==11 or WE[state['weapon']]['mode']==9 and flight_ranges[ch]:state['weapon']=old
            if CH[ch]['special']:
                if state['weapon']!=13:state['weapon']=20
            elif state['weapon'] in [13,20]:state['weapon']=old
        elif state['weapon'] in [13,20]:state['weapon']=old
        if WE[state['weapon']]['offense']>=99:state['mask']=0;slots=[-1,-1]
        if state['mask']==0 and state['weapon']!=old:
            mode=WE[state['weapon']]['mode']
            if mode==7:suggest(0x80000)
            if mode==9:suggest(1);suggest(0x2000)
            if WE[state['weapon']]['offense']>=2*AR[state['armor']]:suggest(0x20000)
        cleanup()
    elif kind=='armor':
        state['armor']=new
        if AR[new]>1 and WE[state['weapon']]['offense']<=1:suggest(0x200)
        cleanup()
    elif kind=='reactor':state['reactor']=new;cleanup()
    elif kind=='ability':
        slots[job['slot']]=new if new<24 else -1
        state['mask']=sum(1<<i for i in set(slots) if i>=0)
    else:raise ValueError(kind)
    raw=bytearray.fromhex(before['draft_hex']);offsets={'chassis':8,'weapon':12,'armor':16,'mask':20,'reactor':24}
    for key,offset in offsets.items():struct.pack_into('<I',raw,offset,state[key])
    struct.pack_into('<2i',raw,36,*slots)
    return dict(fields=state,slots=slots,name=before['name'],draft_hex=raw.hex())
