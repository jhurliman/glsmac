"""Independent normal-open recommendation for the declared stock-early profile.

Admits seven natural starting-tech owners with only early bank records active, or an
all-technology owner with an empty custom bank. No excluded relevant records.
This reduces equivalent alias proposals; it is not an arbitrary AI controller.
"""
import struct
from startup_technology_reference import has_technology
from startup_capability_reference import refresh
from workshop_general_reference import automatic_name
import startup_design_cost_reference as costs

def normal_open(designs,factions,ownership,owner,tables,capability,rules,reactor_prerequisites):
    raw=bytearray(designs)
    tech=lambda n:has_technology(owner,n,ownership,capability['prerequisites'])
    token_ids={r[1]:i for i,r in enumerate(tables['TECHNOLOGY'])}
    resolve=lambda token:-1 if token.lower()=='none' else -2 if token.lower()=='disable' else token_ids[token]
    CH=tables['CHASSIS'];WE=tables['WEAPONS'];AR=tables['DEFENSES'];AB=tables['ABILITIES']
    costs.CH=CH;costs.WE=WE;costs.AR=AR;costs.AB=AB
    natural=list(capability['starting_named_technologies'].values())[owner-1]
    known={i for i,v in enumerate(ownership) if v&(1<<owner)}
    advanced=known==set(range(89));assert advanced or known==set(natural)
    relevant=[*range(64),*range(owner*64,(owner+1)*64)]
    flags=lambda i:struct.unpack_from('<H',raw,i*52+48)[0]
    active=[i for i in range(owner*64,(owner+1)*64) if flags(i)&1]
    assert all(not raw[i*52+44]&(1<<owner) for i in relevant)
    if advanced:assert not active
    else:
        assert all(bytes(raw[i*52+32:i*52+40]) in [struct.pack('<I4B',0,0,0,1,1),struct.pack('<I4B',0,1,0,0,1)] for i in active)
    selected=next(i for i in range(owner*64,(owner+1)*64) if not flags(i)&1)
    re=next((i for i in range(4,0,-1) if tech(reactor_prerequisites[i-1])),1)
    # 595910(owner,land,minimumSpeed=1): exact minimum scores2; faster scores1;
    # strict score improvement keeps the earlier ID on ties.
    ch=-1;score=0
    for i,row in enumerate(CH):
        if not tech(resolve(row[14])) or int(row[9])!=0 or int(row[11]):continue
        speed=int(row[8]);s=2 if speed==1 else 1 if speed>1 else 0
        if s>score:ch=i;score=s
    assert ch>=0
    armor=0;best=-1
    for i,row in enumerate(AR):
        strength=int(row[2])
        if tech(resolve(row[5])) and int(row[4])<=99 and strength>=0 and 2*strength>=best:armor=i;best=2*strength
    # First defensive role: Comm Jammer takes precedence in the all-tech case.
    # None of the admitted natural starting grants admits fallback abilities.
    mask=(1<<9) if tech(resolve(AB[9][2])) else 0
    assert mask or not advanced
    def weapon_with_budget(budget):
        chosen=0;score=-1
        for i,row in enumerate(WE):
            power=int(row[2])
            if tech(resolve(row[6])) and int(row[4])<=budget and int(row[3])<3 and power<99 and i!=20 and power*2>=score:chosen=i;score=power*2
        return chosen
    def cost(weapon):return costs.raw_cost(dict(chassis=ch,weapon=weapon,armor=armor,reactor=re,abilities=[i for i in range(24) if mask&(1<<i)],custom_ability_cost=None))
    we=weapon_with_budget(1);budget=2;cost_steps=[]
    while True:
        candidate=weapon_with_budget(budget)
        if candidate==we:cost_steps.append(dict(budget=budget,candidate=candidate,stop='same_weapon'));break
        old,new=cost(we),cost(candidate);cost_steps.append(dict(budget=budget,candidate=candidate,old_cost=old,new_cost=new))
        if new>old:break
        we=candidate;budget+=1
    first=dict(chassis=ch,weapon=we,armor=armor,mask=mask,reactor=re)
    def match(job):
        wanted=struct.pack('<I4B',job['mask'],job['chassis'],job['weapon'],job['armor'],job['reactor'])
        for i in relevant:
            if flags(i)&1 and bytes(raw[i*52+32:i*52+40])==wanted and (i>=64 or tech(struct.unpack_from('<h',raw,i*52+50)[0])):return i
        return None
    found=match(first)
    if found is None:
        # Fresh proposal publishes mode3/category2. Later categories cannot
        # change its component outputs; no later excluded aliases in profile.
        final=first;mode=3
    else:
        assert not advanced
        # Early duplicate/defensive/offensive aliases stay mode2. The final
        # admitted category10 Scout is fastest known land, no armor/abilities.
        scout=0;speed=0
        for i,row in enumerate(CH):
            if tech(resolve(row[14])) and int(row[9])==0 and not int(row[11]) and int(row[8])>speed:scout=i;speed=int(row[8])
        final=dict(chassis=scout,weapon=0,armor=0,mask=0,reactor=1)
        mode=2 if match(final) is not None else 3
    struct.pack_into('<H',raw,selected*52+48,flags(selected)&~0x80)
    cap=refresh(factions,ownership,capability['prerequisites'],capability['catalog'],owner=owner,rules=0x1a0e)
    slots=([i for i in range(24) if final['mask']&(1<<i)]+[-1,-1])[:2]
    return dict(designs=raw,factions=cap['factions'],selected=selected,fields=final,selector_slots=slots,draft_name='',automatic_name=automatic_name(raw,selected,final,owner,rules),raw_flags=flags(selected),first_defender=first,cost_steps=cost_steps,recommendation_mode_before_clear=mode)
