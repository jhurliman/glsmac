"""View-local height/shade cache with original cold shade side effects."""
from planet_negative_geometry_reference import PlanetGeometry
from terrain_geometry_reference import TerrainGeometry

def candidate_tiles(first,span,width=80,height=80):
    fx,fy=first;sx,sy,ox,oy=span
    return [[(fx+(row&1)+2*col)%width,fy+row]
            for row in range(sy+oy+1) for col in range(sx if row&1 else ox)
            if 0<=fy+row<height]

def in_region(tile,rect):
    x,y=tile;l,t,r,b=rect
    return t<=y<=b and (l<=x<r if l<=r else x>=l or x<r)

def select_tiles(first,span,regions=None,objects=False):
    tiles=candidate_tiles(first,span)
    if regions is None:return tiles
    if objects:regions=[[l-2,t-2,r+2,b+2] for l,t,r,b in regions]
    return [q for q in tiles if any(in_region(q,r) for r in regions)]

def horizontal_pan_regions(old_first,new_first,span):
    """Bounded small interior horizontal pan, fixed zoom/viewport and unchanged Y."""
    old,oy=old_first;new,ny=new_first;sx,sy,ox,ey=span
    assert oy==ny and abs(new-old)==2 and min(old,new)>2 and max(old,new)+sx+ox+1<80
    a=[old+sx+ox-3,ny,new+sx+ox+1,ny+sy+ey] if new>old else [new-1,ny,old+2,ny+sy+ey]
    return [a,[0,0,0,0]]

class ViewGeometryCache(PlanetGeometry):
    def __init__(self,*args,**kw):
        super().__init__(*args,**kw);self.face_cache={}

    def height(self,x,y,vertex):
        q=self.coord(x,y)
        if vertex and q is not None and self.bands[q]>=3 and self.cache.get((*q,vertex),-1)==-1:
            # Native boundary exit retains center queries made before the first
            # invalid/sea contributor. Those partial cache writes are observable.
            for c in self.corner_cells(*q,vertex):
                if c is None or self.bands[c]<3:break
                self.height(*c,0)
        return super().height(x,y,vertex)

    def face_shade(self,x,y,face):
        # Face lighting rejects invalid coordinates rather than wrapping them.
        if not (0<=x<self.width and 0<=y<self.rows):return 0
        key=(x,y,face)
        if key in self.face_cache:return self.face_cache[key]
        if self.bands[x,y]<3:value=0
        else:
            for k in (1,2,3,4):self.height(x,y,k)
            value=TerrainGeometry.face_shade(self,x,y,face)
        self.face_cache[key]=value
        return value

    def configure(self,zoom,relief):
        if getattr(self,'zoom',None)!=zoom:self.cache.clear();self.face_cache.clear()
        elif getattr(self,'relief',None)!=relief:self.cache.clear()
        self.zoom=zoom;self.relief=relief;self.height_step=relief*(zoom+16)//16

    def projector_at(self,x,y,first,span,unit):
        fx,fy=first;dx=x-fx;wrapped=dx%self.width
        if fy<=y<fy+span[1]+span[3] and wrapped<span[0]+span[2]:dx=wrapped
        else:
            if dx>self.width//2:dx-=self.width
            if dx<-self.width//2:dx+=self.width
        hs=[self.height(x,y,k) for k in (4,3,2,1)]
        return [(dx-1)*4*unit,(y-fy-1)*2*unit-(sum(hs)>>2)]
