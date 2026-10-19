"""Native239x119 Detail fresh terrain painter, zoom-7, ordinary80x80 map."""
from minimap_navigation_reference import view_geometry
from terrain_geometry_reference import TerrainGeometry

def flat_spans(vertices,clip):
    """63fd90/63fbe0 integer edge walkers, including right/bottom-1 exclusion."""
    n=len(vertices);top=min(range(n),key=lambda i:vertices[i][1]);bottom=max(range(n),key=lambda i:vertices[i][1])
    if vertices[top][1]>=vertices[bottom][1]:return []
    def edge(start,direction):
        while start!=bottom:
            end=(start+direction)%n;x,y=vertices[start];xx,yy=vertices[end];dy=yy-y
            if dy<0:return None
            if not dy:start=end;continue
            dx=xx-x;return dict(end=end,dir=direction,x=x,dy=dy,left=dy,q=(abs(dx)//dy)*(-1 if dx<0 else 1),step=-1 if dx<0 else 1,error=1-dy if dx<0 else 0,remainder=abs(dx)%dy)
        return None
    def advance(e):
        e['left']-=1
        if e['left']==0:return edge(e['end'],e['dir'])
        e['x']+=e['q'];e['error']+=e['remainder']
        if e['error']>0:e['x']+=e['step'];e['error']-=e['dy']
        return e
    a,b=edge(top,-1),edge(top,1);y=vertices[top][1];out=[]
    while a is not None and b is not None and y<clip[3]-1:
        if y>=clip[1]:
            left,right=sorted([a['x'],b['x']]);left=max(left,clip[0]);right=min(right,clip[2]-1)
            if right>left:out.append([y,left,right])
        a=advance(a)
        if a is None:break
        b=advance(b);y+=1
    return out

def produce(spec):
    center=spec.get('camera',[40,40]);flat=spec.get('flat',False);view=view_geometry([80,80],[239,119],center,-7,flat)
    bands={(x,y):((x//2+y)%8 if spec.get('mixed') else 3) for y in range(80) for x in range(y&1,80,2)}
    g=TerrainGeometry(80,80,bands,not flat,height_step=14);fx,fy=view['first'];sx,sy,ox,oy=view['span'];entries=[];fills=[]
    for row in range(sy+oy+1):
        y=fy+row
        for col in range(sx if row&1 else ox):
            rawx=fx+(row&1)+2*col;x=rawx%80 if not flat else rawx
            if not 0<=x<80 or not 0<=y<80:continue
            dx=x-fx
            if y<fy+sy+oy:
                if not flat:
                    if dx<0:dx+=80
                    elif dx>=80:dx-=80
            else:
                if dx>40:dx-=80
                if dx<-40:dx+=80
            outer=[g.height(x,y,k) for k in range(1,5)];anchor=[28*(dx-1),14*(y-fy-1)-(sum(outer)>>2)]
            entries.append(dict(tile=[x,y],args=[-7,3,-1,-2147483518,x,y,*anchor],caller='0x47669c'))
            known=(bool((x//2+y)&1) if spec.get('checker') else bool(spec.get('known',8)&8))
            if not (known or spec.get('survey') or spec.get('revealed')):continue
            cx,cy=28*dx,14*(y-fy);offsets=[[-28,0],[0,-14],[28,0],[0,14]]
            vertices=[v for k,(xx,yy) in enumerate(offsets) for v in [cx+xx,cy+yy-outer[k]]]
            band=bands[(x,y)];fills.append(dict(tile=[x,y],vertices=vertices,color=190 if band<3 else 2*(14-band),clip=[0,0,239,119]))
    return view,entries,fills

def verify(cases):
    import hashlib,struct,collections
    totals=[0,0,0]
    for c in cases['cases']:
        s=c['input'];view,entries,fills=produce(s)
        assert view['first']==c['view']['first'] and view['span']==c['view']['span'],(s['id'],'view')
        assert entries==c['entries'],(s['id'],'entries',entries,c['entries'])
        assert fills==c['fills'],(s['id'],'fills',fills,c['fills'])
        raw=bytearray([s.get('background',0)])*(cases['pitch']*119);writes=[]
        for fill in fills:
            points=list(zip(fill['vertices'][::2],fill['vertices'][1::2]))
            for y,l,r in flat_spans(points,fill['clip']):
                totals[1]+=1
                for x in range(l,r):raw[y*cases['pitch']+x]=fill['color'];writes.append((x,y,fill['color']))
        wb=b''.join(struct.pack('<HHB',*v) for v in writes)
        assert len(writes)==c['write_count'],(s['id'],'write count',len(writes),c['write_count'])
        assert hashlib.sha256(wb).hexdigest()==c['write_sha256'],(s['id'],'writes')
        assert hashlib.sha256(raw).hexdigest()==c['frame_sha256'],(s['id'],'frame')
        assert {str(k):v for k,v in collections.Counter(raw).items()}=={str(k):v for k,v in c['histogram'].items()}
        totals[0]+=len(fills);totals[2]+=len(writes)
    return totals
