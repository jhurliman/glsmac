"""Source-derived custom-bank upgrade prefix at 597de0..598433.

Admits installed component tables and conventional weapon/armor values.
Plans3/5 skip, plan4 shares0, plan6 requires declared faction values.
This generates prefix submissions only; it is not the later role controller.
"""
import struct
from startup_technology_reference import has_technology


def upgrade_prefix(designs,ownership,owner,tables,prerequisites,reactor_prerequisites, faction_values=None, with_state=False, on_submit=None):
    raw=bytearray(designs);token_ids={r[1]:i for i,r in enumerate(tables['TECHNOLOGY'])}
    resolve=lambda s:-1 if s.lower()=='none' else -2 if s.lower()=='disable' else token_ids[s]
    tech=lambda t:has_technology(owner,t,ownership,prerequisites)
    ch=tables['CHASSIS'];we=tables['WEAPONS'];ar=tables['DEFENSES']
    reactor=next((i for i in range(4,0,-1) if tech(reactor_prerequisites[i-1])),1)
    def record(i):
        a=i*52;mask,c,w,d,r=struct.unpack_from('<I4B',raw,a+32)
        return dict(chassis=c,weapon=w,armor=d,reactor=r,mask=mask)
    def eligible(i):
        a=i*52
        return bool(struct.unpack_from('<H',raw,a+48)[0]&1) and not raw[a+44]&(1<<owner) and (i>=64 or tech(struct.unpack_from('<h',raw,a+50)[0]))
    active=[i for i in [*range(64),*range(owner*64,(owner+1)*64)] if eligible(i)]
    eligible_we=[record(i)['weapon'] for i in active if not int(ch[record(i)['chassis']][11])]
    eligible_ar=[record(i)['armor'] for i in active]
    new_we=None;new_ar=None;best=-1
    for i,row in enumerate(we):
        power=int(row[2])
        if not tech(resolve(row[6])) or not 0<power<99 or i==20 or power<best:continue
        for w in eligible_we:
            if int(we[w][2])>=power:break
            new_we=i;best=power
    best=-1
    for i,row in enumerate(ar):
        power=int(row[2])
        if not tech(resolve(row[5])) or power<=1 or power<best:continue
        for d in eligible_ar:
            if int(ar[d][2])>=power:break
            new_ar=i;best=power
    if new_we is None and new_ar is None:return dict(proposals=[],offense_factor=len(ar)) if with_state else []
    out=[];offense_factor=len(ar)
    for i in range(owner*64,(owner+1)*64):
        if not eligible(i):continue
        a=i*52;j=record(i)
        if int(ch[j['chassis']][11]):continue
        plan=raw[a+42]
        if plan>6:continue
        if plan in (3,5):continue
        if plan in (0,4):
            if new_we is None:continue
            j['weapon']=new_we
        elif plan==2:
            if new_ar is None:continue
            j['armor']=new_ar
        elif plan==1:
            if new_ar is not None:offense_factor=2 if reactor==1 else 1
            candidate=new_we if new_we is not None else j['weapon']
            power=int(we[candidate][2]);assert power>0
            limit=(2 if reactor==1 else 1)*(2<<reactor)*4//power
            limit=min(limit,power) if limit>=2 and power>=2 else 2
            if new_ar is not None and int(ar[new_ar][2])<=limit:j['armor']=new_ar
            elif new_we is None:continue
            j['weapon']=candidate
        else:
            if faction_values is None:raise NotImplementedError('plan6 requires faction values')
            import types,startup_design_cost_reference as costs
            env=dict(costs.__dict__,CH=ch,WE=we,AR=ar,AB=tables['ABILITIES'])
            cost=types.FunctionType(costs.raw_cost.__code__,env,argdefs=(None,))
            def price(w,d):return cost(dict(chassis=j['chassis'],weapon=w,armor=d,reactor=reactor,abilities=[b for b in range(24) if j['mask']&(1<<b)],custom_ability_cost=None))
            limit=faction_values[0x95d594]+faction_values[0x95d588]+max(5,reactor*reactor)
            if new_we is not None and price(new_we,j['armor'])<=limit:j['weapon']=new_we
            changed=False
            if new_ar is not None and price(j['weapon'],new_ar)<=limit:j['armor']=new_ar;changed=True
            if not changed and j['weapon']!=new_we:continue
        j['reactor']=reactor
        proposal=dict(source_id=i,fields=j,category=raw[a+43]);out.append(proposal)
        if on_submit is not None:raw=bytearray(on_submit(proposal))
    return dict(proposals=out,offense_factor=offense_factor) if with_state else out
