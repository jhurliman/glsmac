"""Source-derived native planning policy; helper execution stays with the engine.

No original code/assets. Scope: scalar upkeep admission, weighted Base target,
and normal eight-neighbor land/sea/air-compatible candidate competition. Later
combat resolution, pillaging and damaged-unit hesitation are separate.
"""
from global_ecology_reference import crt,distance

FEATURE_MASK=0x80208054
DIRECTIONS=[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]
DIRECTIVE_OFFSETS=[(0,0)]+DIRECTIONS+[(2,-2),(2,2),(-2,2),(-2,-2),(1,-3),(3,-1),(3,1),(1,3),(-1,3),(-3,1),(-3,-1),(-1,-3)]

def wrapped(x,y,width,height,flat):
    if not flat:x%=width
    return (x,y) if 0<=x<width and 0<=y<height else None

def even_manhattan(a,b,width,flat):
    dx=abs(a[0]-b[0]);dy=abs(a[1]-b[1])
    if not flat:dx=min(dx,width-dx)
    return (dx+dy)&~1

def carrier_directive(*,position,target,tile,width,height,flat=False,source_Base=-1,
                      Base_water_connected=None):
    """57ae30(uid,99,targetX,targetY), returning choice without executing it.

    tile returns sea,region,flags,affiliation,explored. A carrier inside a Base
    must use canonical521ea0(Base,seaRegion), not invented ocean connectivity.
    """
    source=tile(*position);goal=tile(*target)
    if source['sea'] and source['region']==goal['region']:
        return dict(waypoint=list(target),status=99,order=24,terms=[],direct=True)
    best=0 if not goal['sea'] else -1;chosen=None;terms=[]
    for dx,dy in DIRECTIVE_OFFSETS:
        p=wrapped(target[0]+dx,target[1]+dy,width,height,flat)
        if p is None:continue
        c=tile(*p)
        if not c['sea']:continue
        if source_Base>=0:
            assert Base_water_connected is not None
            if not Base_water_connected(source_Base,c['region']):continue
        elif c['region']!=source['region']:continue
        count=0
        for xx,yy in DIRECTIONS:
            q=wrapped(p[0]+xx,p[1]+yy,width,height,flat)
            if q is None:continue
            t=tile(*q)
            if not t['sea'] and t['region']==goal['region'] and not (t['flags']&1 and t['affiliation']<8):count+=1
        terms.append(dict(position=list(p),shore_count=count))
        if count>best:best=count;chosen=p
    return dict(waypoint=list(chosen) if chosen else None,status=99,order=24 if chosen else None,terms=terms,direct=False)

def carrier_step(*,position,target_Base,work_after_increment,land_triad_count,
                 capacity,tile,width,height,flat=False):
    """57cfbd..57d3a0. Stack query counts triad0, any owner/order/payload."""
    if not capacity or not land_triad_count:return dict(action='normal_competition')
    if work_after_increment>24:return dict(action='native_disband')
    for direction,(dx,dy) in enumerate(DIRECTIONS):
        p=wrapped(position[0]+dx,position[1]+dy,width,height,flat)
        if p is None:continue
        c=tile(*p)
        if c['flags']&2 and 0<c['affiliation']<8:
            if c['sea']:return dict(action='immediate_sea_attack',direction=direction,position=list(p))
            continue
        if c['sea'] or c['region']!=target_Base['region']:continue
        if even_manhattan(p,target_Base['position'],width,flat)<=12:
            return dict(action='activate_entire_stack_and_exhaust_carrier',position=list(p),observation_or=c['explored'])
    return dict(action='directive',directive=carrier_directive(position=position,target=target_Base['position'],tile=tile,width=width,height=height,flat=flat))

def life_level(turn):
    return 0 if turn<45 else 1 if turn<90 else 2 if turn<170 else 3 if turn<250 else 4 if turn<330 else 6

def command_repeat(*,result,count_before,count_after,order,triad,retries):
    """58f875..58f8be; caller then rechecks live order and movement budget."""
    if result!=0 or count_before!=count_after:return dict(finish_current_scan_Unit=True,retries=retries,exhaust=False)
    retries+=1 if order==24 or triad==2 else 2
    return dict(finish_current_scan_Unit=False,retries=retries,exhaust=retries>32)

def command_pass_admits(*,priority,role,triad,damage,owner,first_pass_helper,
                        land_triad_count,weapon,ability16=False):
    """58f6f7..58f824. Engine must use canonical design/ability helpers.

    Remaining movement and order1 are separate live checks after this filter.
    """
    if priority==0 and not first_pass_helper:return False
    if priority==1 and not (role==2 or (role==7 and land_triad_count)):return False
    if priority==2 and not (triad==2 and role!=4):return False
    if priority==3 and role!=1:return False
    if priority==4 and damage!=0:return False
    if priority<7 and (ability16 or role==4 or weapon==0x15):return False
    if priority==8 and not (role==7 and owner!=0):return False
    return True

def upkeep_native_unit(*,triad,sea,ordinary_Base,flags,order,human=False,stack_transport_roles=0):
    """Decision in5773c0, after its earlier maintenance/planning work.

    stack_transport_roles is mode2/role7 count over the actual linked stack,
    with no owner restriction. This is not an assigned payload/cargo predicate.
    """
    actions=[]
    if not human and order==1 and (triad!=0 or not sea):
        order=0;actions.append('clear_sentry')
    if sea and triad==0 and not ordinary_Base:
        if not stack_transport_roles:
            if not flags&0x20:actions.append('delete')
        elif order!=1:
            order=1;actions.append('sentry')
    if not sea and triad==1 and not ordinary_Base and not flags&0x20:
        actions.append('delete')
    return dict(order=order,actions=actions)

def native_base_target(*,position,region,bases,home,tag,planet_by_owner,width,flat=False):
    """57cc80 returns Base identity and weighted metric in global91871c.

    Base inputs: id,position,region,owner,c0,c4,field48,role12_count. The c0/c4
    and field48 names deliberately retain source offsets, not guessed meanings.
    All divisions here operate on valid stock nonnegative distance/denominators.
    """
    best=9999;chosen=-1;terms=[]
    for b in bases:
        if region<64 and b['region']!=region:continue
        d=distance(*position,*b['position'],width,flat)
        den=b['c0']+b['c4']+32;assert den>0
        score=32*d//den
        if b['role12_count']:score//=2
        if b['id']==home:score//=2
        if tag!=b['owner'] and planet_by_owner.get(b['owner'],0)>0 and b['field48']==0:score*=2
        terms.append(dict(Base=b['id'],distance=d,denominator=den,metric=score))
        if score<=best:best=score;chosen=b['id']
    return dict(Base=chosen,metric=best,terms=terms)

def choose_neighbors(*,position,current_sea,triad,facing,turn,difficulty,state,seed,
                     candidates):
    """57d4e6..57d7ae normal competition, in supplied direction-table order.

    Each candidate has direction,position,in_bounds,sea,flags,affiliation,
    optional head {owner,design,weapon_offense,triad}. Entity inputs describe
    actual linked stack head selection, not a substitute native return.
    """
    level=life_level(turn);best=0;selected=-1;terms=[]
    for c in candidates:
        reason=None
        if not c['in_bounds']:reason='bounds'
        elif c['sea'] and triad==0:reason='land_to_water'
        elif not c['sea'] and triad==1:reason='sea_to_land'
        elif not c['sea'] and current_sea and triad==0 and c['flags']&2 and 0<c['affiliation']<8:reason='beaching_into_positive_Unit'
        if reason:
            terms.append(dict(direction=c['direction'],rejected=reason));continue
        h=c.get('head');base_bonus=0
        if c['flags']&1 and 0<c['affiliation']<8:base_bonus=9999
        elif h and h['owner']!=0:
            if h['design']<64 and h['weapon_offense']<0:
                terms.append(dict(direction=c['direction'],rejected='negative_predefined_weapon'));continue
            if (triad==2)!=(h['triad']==2):
                terms.append(dict(direction=c['direction'],rejected='air_mismatch'));continue
            base_bonus=999
        features=4*(c['flags']&FEATURE_MASK).bit_count()
        road=8 if difficulty>2 and c['flags']&0x2000 and not state&0x100 else 0
        seed,draw=crt(seed);jitter=draw%8
        angle=((facing^4)-c['direction'])&7;angle=min(angle,8-angle)
        heading=(level+1)*angle+(level*level if c['direction']==facing else 0)
        score=base_bonus+features+road+jitter+heading
        terms.append(dict(direction=c['direction'],position=c['position'],base_bonus=base_bonus,features=features,road=road,jitter=jitter,heading=heading,score=score))
        if score>best:best=score;selected=c['direction']
    return dict(seed=seed,level=level,best_score=best,direction=selected,terms=terms)
