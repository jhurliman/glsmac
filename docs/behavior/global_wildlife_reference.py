"""Complete reachable stock537790 fauna policy, independent of original code.

Use with global_ecology_reference.spread first. Constructor/capacity/order/stack
actions below express the existing native helper contracts. The stock inactive
native AI-reference profile produces no constructor RNG; an engine factory must
retain any RNG/side effects belonging to its broader constructor context.
"""
from global_ecology_reference import sample,at,word,putword,distance,NEIGHBORS
from global_ecology_reference import spread

AMBUSH_OFFSETS=[(2,-2),(2,2),(-2,2),(-2,-2),(1,-3),(3,-1),(3,1),(1,3),
                (-1,3),(-3,1),(-3,-1),(-1,-3),(4,0),(-4,0),(0,4),(0,-4)]

def produce_fauna(raw_map, *, width,height,turn,life,difficulty,rules,secondary,
                  Voice_marker,seed,bases,units,region_areas,observer_status,
                  diplomacy0,technology_masks,site_present,native_stored_morale=1,
                  flat=False):
    assert 0<=life<=2 and 0<=difficulty<=5 and turn>=0
    assert native_stored_morale==1, 'Stock newborn native scalar0 profile'
    data=bytearray(raw_map);roster=[dict(u) for u in units];actions=[]
    sea_samples=[];land_samples=[];queries=[];adjacency_queries=[];brood=None
    initial_count=len(roster)
    def xy(x,y):
        if not flat:x%=width
        return (x,y) if 0<=x<width and 0<=y<height else None
    def nearest(x,y,region):
        best=9999;selected=-1
        for b in bases:
            bx,by=b['position']
            if data[at(width,bx,by)+3]!=region:continue
            d=distance(x,y,bx,by,width,flat)
            if d<=best:best=d;selected=b['id']
        queries.append(dict(position=[x,y],region=region,Base=selected,distance=best))
        return selected,best
    def create(design,x,y):
        uid=len(roster)
        if uid>=2048:
            actions.append(dict(op='create',design=design,owner=0,position=[x,y],result=-1));return -1
        existing=any(u['position']==[x,y] for u in roster)
        roster.append(dict(id=uid,owner=0,design=design,position=[x,y],morale=1,order=0))
        a=at(width,x,y)
        if not existing and not word(data,a+8)&1:data[a+2]&=0xf0
        putword(data,a+8,word(data,a+8)|2)
        actions.append(dict(op='create',design=design,owner=0,position=[x,y],result=uid))
        return uid
    def adjacent_blocker(x,y):
        result=0
        for dx,dy in NEIGHBORS:
            p=xy(x+dx,y+dy)
            if p is None:continue
            a=at(width,*p)
            if not word(data,a+8)&3:continue
            owner=data[a+2]&15
            if owner<8 and owner!=0 and not diplomacy0.get(owner,0)&1:
                result=owner+1;break
        adjacency_queries.append(dict(position=[x,y],owner=0,result=result));return result
    if rules&0x200000 or Voice_marker!=-1 or initial_count>=1792:
        return dict(seed=seed,actions=actions,sea_samples=sea_samples,land_samples=land_samples,
                    queries=queries,adjacency_queries=adjacency_queries,brood=brood,unit_count=len(roster))
    early=turn<2*(10*(3-life)-difficulty)
    if turn%(2*(5-life))==0:
        chosen=None
        for attempt in range(1,1001):
            seed,(x,y)=sample(seed,width,height);sea_samples.append([x,y])
            if attempt>=1000:break # Last pair is consumed but cannot be accepted.
            a=at(width,x,y);flags=word(data,a+8);band=data[a]>>5;region=data[a+3]
            if flags&0x20 and band>=2:
                chosen=(x,y);break # Deliberately bypasses land/occupancy/area checks.
            if band>=3:continue
            if flags&3 and (data[a+2]&15)<8:continue
            if region&63 in [0,63] or region_areas[region]<50:continue
            chosen=(x,y);break
        if chosen is not None:
            x,y=chosen
            raw_requested=((2 if secondary&0x100000 else 1)*turn)//(4*(4*(8-life)-difficulty))+life+1
            requested=1 if early else raw_requested
            carrier=create(9,x,y)
            if carrier>=0:
                capacity=native_stored_morale+1 # Stock predefined nonzero-capacity Psi Isle.
                passengers=min(requested,capacity)
                brood=dict(position=[x,y],raw_requested=raw_requested,early=early,requested=requested,capacity=capacity,passengers=passengers,carrier=carrier)
                for i in range(passengers):
                    uid=create(8,x,y);assert uid>=0 # Entry gate keeps stock births below2048.
                    roster[uid]['order']=1
                    actions.append(dict(op='sentry',unit=uid,payload=-1,extra_order_word=0))
                actions.append(dict(op='promote_stack',unit=carrier))
                if any(technology_masks):create(10,x,y)
    for iteration in range(2*life+2):
        seed,(x,y)=sample(seed,width,height);land_samples.append([x,y]);a=at(width,x,y)
        flags=word(data,a+8);region=data[a+3];owner=data[a+2]&15
        if data[a]>>5<3 or not flags&0x20 or region&63 in [0,63]:continue
        if flags&1 and owner<8:continue
        b,d=nearest(x,y,region)
        if flags&2 and owner<8:
            if owner==0 or (b>=0 and d<=2):continue
            for dx,dy in AMBUSH_OFFSETS:
                p=xy(x+dx,y+dy)
                if p is None:continue
                xx,yy=p;t=at(width,xx,yy)
                if data[t]>>5<3 or observer_status.get(owner,0)&0x200:continue
                if data[t+4]&(1<<owner):continue
                if word(data,t+8)&3 and (data[t+2]&15)<8:continue
                if adjacent_blocker(xx,yy):continue
                create(8,xx,yy);break
            continue
        if 1<d<6 and not site_present(x,y,data[a:a+44]):create(8,x,y)
    # Native's next >=limit guard repeats exactly the failed <limit loop test.
    # No normal-entry edge can reach its following cluster/strength block.
    return dict(seed=seed,actions=actions,sea_samples=sea_samples,land_samples=land_samples,
                queries=queries,adjacency_queries=adjacency_queries,brood=brood,unit_count=len(roster))

def produce_global(raw_map, *, tile_count, **inputs):
    """Original order: live flora mutation, then fauna using the resulting seed/map."""
    growth=spread(raw_map,width=inputs['width'],height=inputs['height'],
                  tile_count=tile_count,turn=inputs['turn'],seed=inputs['seed'],
                  flat=bool(inputs.get('flat',False)),
                  observer_status=inputs['observer_status'])
    following=dict(inputs);following['seed']=growth['seed']
    fauna=produce_fauna(growth['map'],**following)
    return dict(growth=growth,fauna=fauna,seed=fauna['seed'])
