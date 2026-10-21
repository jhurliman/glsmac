"""Independent stock-human 597de0 proposal producer; no fixture/outcome dispatch.

Inputs are current raw records, ownership, faction buffer and explicit scalar
entry state. For live generation, on_submit must execute preview policy and
return its updated raw pool; normal_open() supplies that callback. Producing a
batch against a frozen registry cannot reproduce mid-loop unexclusion.
AI owners, modified component tables, negative/PSI custom records
and integer-overflow strategic states are outside this producer's scope.
"""
import struct,types
from startup_technology_reference import has_technology
from workshop_upgrade_prefix_reference import upgrade_prefix
import startup_design_cost_reference as costs


def proposals(designs,factions,ownership,owner,tables,prerequisites,
              reactor_prerequisites,*,personality,threat=0,on_submit=None):
    assert 1<=owner<=7 and threat in (0,1,2,3)
    # Preview matching's PSI evaluation/scoring is a separate boundary.
    for i in range(owner*64,(owner+1)*64):
        a=i*52
        if struct.unpack_from('<H',designs,a+48)[0]&1:
            w,d=designs[a+37],designs[a+38]
            if int(tables['WEAPONS'][w][2])<0 or int(tables['DEFENSES'][d][2])<0:
                raise NotImplementedError('negative/PSI custom records require strategic evaluation')
    CH=tables['CHASSIS'];WE=tables['WEAPONS'];AR=tables['DEFENSES'];AB=tables['ABILITIES']
    tokens={r[1]:i for i,r in enumerate(tables['TECHNOLOGY'])}
    resolve=lambda s:-1 if s.lower()=='none' else -2 if s.lower()=='disable' else tokens[s]
    tech=lambda n:has_technology(owner,n,ownership,prerequisites)
    ability=lambda b:tech(resolve(AB[b][2]))
    neural=tech(tokens['Neural'])
    f=lambda a:struct.unpack_from('<i',factions,owner*0x20bc+a-0x95d1a8)[0]
    fv={a:f(a) for a in [0x95d594,0x95d588]}
    prefix=upgrade_prefix(designs,ownership,owner,tables,prerequisites,reactor_prerequisites,fv,True,on_submit)
    result=list(prefix['proposals'])
    re=next((r for r in range(4,0,-1) if tech(reactor_prerequisites[r-1])),1);power=2<<re
    env=dict(costs.__dict__,CH=CH,WE=WE,AR=AR,AB=AB)
    raw_cost=types.FunctionType(costs.raw_cost.__code__,env,argdefs=(None,))
    def price(ch,we,ar,mask=0):return raw_cost(dict(chassis=ch,weapon=we,armor=ar,reactor=re,abilities=[b for b in range(24) if mask&(1<<b)],custom_ability_cost=None))
    def emit(ch,we,ar,mask,category):
        proposal=dict(fields=dict(chassis=ch,weapon=we,armor=ar,mask=mask,reactor=re),category=category);result.append(proposal)
        if on_submit is not None:on_submit(proposal)
    def chassis(domain,minimum):
        chosen=-1;best=0
        for i,row in enumerate(CH):
            speed,dom,special=int(row[8]),int(row[9]),int(row[11])
            if not tech(resolve(row[14])) or domain>=0 and dom!=domain or special and minimum!=-1:continue
            if minimum>0:
                if speed<minimum:continue
                score=2 if speed==minimum else 1
            else:
                if domain==2 and minimum==-1 and not special:continue
                if domain==2 and minimum==-2 and int(row[10])!=1:continue
                score=speed+1
            if score>best:chosen=i;best=score
        return chosen
    def weapon(budget):
        chosen=0;best=-1
        for i,row in enumerate(WE):
            if not tech(resolve(row[6])):continue
            if budget<0:
                if int(row[3])==-budget:return i
                continue
            p=int(row[2])
            if int(row[4])>budget or int(row[3])>=3 or p>=99 or i==20:continue
            if 2*p>=best:chosen=i;best=2*p
        return chosen
    def armor(budget):
        chosen=0;best=-1
        for i,row in enumerate(AR):
            p=int(row[2])
            if tech(resolve(row[5])) and int(row[4])<=budget and p>=0 and 2*p>=best:chosen=i;best=2*p
        return chosen
    def clean():return f(0x95d1ac)>=6 and (f(0x95d590)!=0 or f(0x95d58c)!=0) and (f(0x95d594)==0 or f(0x95d584)<1) and ability(16)
    # First defensive family. Human path skips threat/Trance additions.
    ch=chassis(0,1)
    if ch>=0:
        ar=armor(99);mask=512 if ability(9) else 0
        if not mask and (f(0x95d584)<0 or personality<0) and ability(14):mask=16384
        we=weapon(1);budget=2
        while True:
            w=weapon(budget)
            if w==we or price(ch,w,ar,mask)>price(ch,we,ar,mask):break
            we=w;budget+=1
        emit(ch,we,ar,mask,2)
    # Human offensive budget99; original strategic armor budget remains live.
    ch=chassis(0,1)
    if ch>=0:
        n=2;budget=1;limit=f(0x95d594)+5
        if 2//(2*power)<=limit:
            while True:
                budget+=1;n+=1
                if (n*(n+1)//2)//(2*power)>limit:break
                assert n<10000
        strongest=weapon(99);p=int(WE[strongest][2]);assert p>0
        q=prefix['offense_factor']*power*4//p
        q=min(q,p) if q>=2 and p>=2 else 2
        ar=armor(max(1,min(99,max(q,budget//2))))
        p=int(WE[strongest][2]);d=int(AR[ar][2])
        while ((p+1)*d)//(2*power)<=limit:
            p+=1;assert p<100000
        we=weapon(p);mask=0
        if f(0x95e200)>=8 and f(0x95d1ac)>=6 and (f(0x95d590)!=0 or f(0x95d58c)!=0) and ability(16):mask=65536
        if personality<0 and not mask and ability(14):mask=16384
        if d>1 and not mask and f(0x95d58c)>0 and f(0x95d9d8)>1 and f(0x95d9d0)>=2*f(0x95d9c8) and ability(9):mask=512
        if not mask and f(0x95d584)<0 and ability(14):mask=16384
        if ability(10) and (not mask or neural):mask|=1024
        emit(ch,we,ar,mask,3)
        extra=(mask&~1024) if neural else 0
        if ability(4):emit(ch,we,ar,extra|16,11)
        if ability(3):emit(ch,we,ar,extra|8,12)
    # Artillery. Equal-cost armor and low-cost Rover upgrade loop.
    strongest=weapon(99);ch=chassis(0,1)
    if ability(15) and ch>=0:
        ar=armor(1)
        for _ in range(100):
            changed=False;faster=chassis(0,2)
            if int(CH[ch][8])==1 and faster>=0 and faster!=ch:
                old,new=price(ch,strongest,ar),price(faster,strongest,ar)
                if (new<=re+5 and new<=old+(owner&1)+1) or new<=old:ch=faster;changed=True
            candidate=armor(int(AR[ar][2])+1)
            if int(AR[candidate][2])>int(AR[ar][2]) and price(ch,strongest,candidate)<=price(ch,strongest,ar):ar=candidate;changed=True
            if not changed:break
        mask=0
        if not neural:
            pass
        else:
            mask=next((1<<b for b in [17,10,1] if ability(b)),0)
        mask|=32768
        emit(ch,strongest,ar,mask,1)
    # Fast land attack.
    ch=chassis(0,3)
    if ch<0:ch=chassis(0,2)
    if ch>=0:
        we=weapon(99);ar=armor(1)
        for _ in range(100):
            p=int(WE[we][2]);d=int(AR[ar][2])
            if d>=p-1:break
            candidate=armor(max(p//2,d+1))
            if int(AR[candidate][2])<=d or price(ch,we,candidate)>f(0x95d594)+f(0x95d584)+re+3:break
            ar=candidate
        mask=0
        if personality<0 or f(0x95d584)>0 and f(0x95d418)>=0:
            if ability(14):mask=16384
        if ability(10) and (not mask or neural):mask|=1024
        emit(ch,we,ar,mask,4)
    # Scout: fastest exact-speed preference, fallback antigrav only on Infantry.
    ch=chassis(0,3);mask=0
    if ch<0:ch=chassis(0,2)
    if ch<0:
        ch=chassis(0,1)
        if ability(10):mask=1024
    if ch>=0:
        if not mask:mask=next((1<<b for b in [1,2] if ability(b)),0)
        if ability(10):
            if neural:mask|=1024
            else:mask=1024
        emit(ch,weapon(1),armor(1),mask,10)
    # Naval combat, human strongest-weapon override after cost optimization.
    ch=chassis(1,0)
    if ch>=0:
        mask=next((1<<b for b in [1,8] if ability(b)),0)
        if threat&1 and ability(18) and (not mask or neural):mask|=1<<18
        for b in [8,1]:
            if ability(b) and not mask&(1<<b) and mask.bit_count()<2 and (not mask or neural):mask|=1<<b
        we=weapon(1);ar=armor(1)
        for _ in range(100):
            changed=False;candidate=we
            for delta in range(1,4):
                candidate=weapon(int(WE[we][2])+delta)
                if candidate!=we:break
            limit=f(0x95d594)+f(0x95d588)+max(5,2*re-re//2)
            if candidate!=we and price(ch,candidate,ar)<=limit:we=candidate;changed=True
            candidate=ar
            for delta in range(1,4):
                candidate=armor(int(AR[ar][2])+delta)
                if candidate!=ar:break
            if int(AR[candidate][2])>int(AR[ar][2]) and price(ch,we,candidate,mask)<=limit:ar=candidate;changed=True
            if not changed:break
        emit(ch,weapon(99),ar,mask,6)
    # Sea transport and utility ships. Prefer Foil for former/colony if known.
    ch=chassis(1,0)
    if ch>=0:
        ar=armor(99 if f(0x95d9ec)>f(0x95f230) and f(0x95d9ec)>f(0x95f22c) else 1)
        emit(ch,weapon(-7),ar,0,7)
        if ch!=3 and tech(resolve(CH[3][14])):ch=3
        ar=armor(1);we=weapon(-9)
        mask=1 if ability(0) else 8192 if ability(13) else 0
        if mask==1 and ability(13) and neural:mask=8193
        if we>=0:emit(ch,we,ar,mask,15)
        we=weapon(-8)
        if we>=0:emit(ch,we,ar,0,16)
    # Air attack/interceptor and finite-range copter. Threat remains an explicit
    # entry-stage input; the producer does not reconstruct diplomatic threat.
    for minimum,category in [(0,8),(-2,17)]:
        ch=chassis(2,minimum)
        if ch<0:continue
        ar=armor(1);mask=65536 if clean() else 0
        if threat&2 and not mask and ability(11):mask=2048
        if ability(1) and (not mask or neural):mask|=2
        emit(ch,weapon(99),ar,mask,category)
        if category==8:
            intercept=34 if ability(1) and neural else 262176 if threat&1 and ability(18) and neural else 32
            emit(ch,weapon(99),ar,intercept,9)
    ch=chassis(2,-1)
    if ch>=0:
        ar=armor(1)
        if tech(resolve(WE[20][6])):emit(ch,20,ar,0,13)
        we=next((i for i,row in enumerate(WE) if tech(resolve(row[6])) and int(row[2])>=99),None)
        if we is not None:emit(ch,we,ar,0,14)
    # Final land worker. The native uses fixed Former15, not its mode selector.
    if re<=1 and not ability(0):
        if ability(13):
            ch=chassis(0,1)
            if ch>=0:emit(ch,15,armor(1),8192,-1)
    else:
        ch=chassis(0,2)
        if ch>=0:
            mask=1 if ability(0) else 0
            if ability(13) and (not mask or neural):mask|=8192
            emit(ch,15,armor(1),mask,-1)
    return result


def normal_open(designs,factions,ownership,owner,tables,capability,language_rules,
                reactor_prerequisites,*,personality,unit_counts,production_counts):
    """New/inactive-slot normal-open composition for stock neutral-project state.

    Uses independently refreshed capabilities, generated proposals and preview
    submissions, including immediate exclusion side effects between proposals.
    Full banks use proposals()/submit() directly; no inactive UI slot is invented.
    """
    from startup_capability_reference import refresh
    from workshop_recommendation_policy_reference import submit
    from workshop_general_reference import automatic_name
    raw=bytearray(designs)
    selected=next((i for i in range(owner*64,(owner+1)*64) if not struct.unpack_from('<H',raw,i*52+48)[0]&1),None)
    if selected is None:raise ValueError('normal New selector requires inactive slot; use producer boundary for full bank')
    tokens={r[1]:i for i,r in enumerate(tables['TECHNOLOGY'])}
    resolve=lambda s:-1 if s.lower()=='none' else -2 if s.lower()=='disable' else tokens[s]
    tech=lambda n:has_technology(owner,n,ownership,capability['prerequisites'])
    re=next((r for r in range(4,0,-1) if tech(reactor_prerequisites[r-1])),1)
    ch=we=ar=0;speed=weapon_strength=armor_strength=0
    for i,row in enumerate(tables['CHASSIS']):
        if tech(resolve(row[14])) and int(row[9])==0 and int(row[8])>=speed:ch=i;speed=int(row[8])
    for i,row in enumerate(tables['WEAPONS']):
        p=int(row[2])
        if tech(resolve(row[6])) and p<99 and p>=weapon_strength:we=i;weapon_strength=p
    for i,row in enumerate(tables['DEFENSES']):
        p=int(row[2])
        if tech(resolve(row[5])) and p>=armor_strength:ar=i;armor_strength=p
    state=dict(mode=1,filter=-1,fields=dict(chassis=ch,weapon=we,armor=ar,reactor=re,mask=0));steps=[]
    current_factions=refresh(factions,ownership,capability['prerequisites'],capability['catalog'],owner=owner,rules=0x1a0e)['factions']
    def accept(proposal):
        nonlocal raw,state
        raw,state,result=submit(raw,ownership,capability['prerequisites'],owner,proposal['fields'],proposal['category'],state,unit_counts,production_counts)
        steps.append(dict(result=result,state=state));return raw
    generated=proposals(raw,current_factions,ownership,owner,tables,capability['prerequisites'],reactor_prerequisites,personality=personality,on_submit=accept)
    flags=struct.unpack_from('<H',raw,selected*52+48)[0]&~0x80;struct.pack_into('<H',raw,selected*52+48,flags)
    fields=state['fields']|dict(reactor=re)
    return dict(designs=raw,factions=current_factions,selected=selected,fields=fields,selector_slots=([b for b in range(24) if fields['mask']&(1<<b)]+[-1,-1])[:2],draft_name='',raw_flags=flags,automatic_name=automatic_name(raw,selected,fields,owner,language_rules),proposals=generated,steps=steps,controller_state=state)
