"""Native resource/Supply Pod/Monolith art caller, zoom0 ordinary prefix."""
from developed_overlay_reference import produce as developed_produce,verify_frames

def resource_type(x,y,band,items,pattern,automatic):
    forced=bool(items&0x400)
    if not forced and (not pattern or (band>=3 and not automatic)):return 0
    u=(x+y)//2;v=x-u;phase=(u&3)+4*(v&3)
    if not forced and phase!=(-5*(u//4)-3*(v//4)+pattern)&15:return 0
    if band<2:return 0
    kind=phase%3+1 if band==2 else (phase%5)&3
    if kind and not items&0x20000000:return kind
    return 3 if items&0x10000 else 2 if items&0x20000 else 1

def site_class(x,y,band,items,pattern,automatic,coefficients=(11,61)):
    if items&0x102000:return 0
    if automatic:return 2 if items&0xc000000 else 0
    if items&0x10000000:return 1
    if not pattern:return 0
    u=(x+y)//2;v=x-u;phase=(u&3)+4*(v&3)
    if band>=3 and phase==(-5*(u//4)-3*(v//4)+pattern)&15:return 2
    trunc4=lambda n:n//4 if n>=0 else -((-n)//4)
    return int(phase==(pattern+coefficients[0]*trunc4(u)+coefficients[1]*trunc4(v)+8)&31)

def select(spec,nutrients=0):
    from developed_overlay_reference import select as developed_select
    x,y=spec.get('tile',[40,40]);band=spec.get('center_band',spec['band']);sea=int(band<3)
    live=spec['items'];selected=spec.get('remembered_items',live);flags=spec.get('flags',0x221)
    common=developed_select(selected,band,nutrients,spec.get('landmark',0),flags)
    if not flags&0x200:return common,[]
    kind=resource_type(x,y,band,live,spec.get('pattern',0),spec.get('automatic',False))
    queries=[dict(entry='0x5a9460',args=[x,y,0],result=kind)]
    early=[]
    if kind:
        index=4*(kind-1)+2*sea+((x//2+y)&1)
        early.append(dict(kind='resource',bank=hex(0x781880+44*index)))
    site=site_class(x,y,band,live,spec.get('pattern',0),spec.get('automatic',False))
    queries.append(dict(entry='0x5a9570',args=[x,y],result=site))
    late=[]
    if site:late.append(dict(kind='pod',bank=hex(0x781618+44*(2*((7*x+5*y)%3)+sea))))
    if live&0x2000:late.append(dict(kind='monolith',bank='0x7815e8'))
    # Kelp belongs before early-resource, while all other developed sprites are
    # between early-resource and Pod/Monolith. Its sea Road gate remains false.
    kelp=[p for p in common if p['kind']=='kelp'];rest=[p for p in common if p['kind']!='kelp']
    return kelp+early+rest+late,queries

def produce(spec,tables,nutrients,seed_low16):
    commands,samples,_=developed_produce(spec,tables,nutrients,seed_low16)
    picked,queries=select(spec,nutrients)
    return commands,samples,picked,queries
