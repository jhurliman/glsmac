"""Native land height/shade profiles, positive mapped detail0..48, zoom0.

The per-corner cache is shared by four tiles. Conflicting synthetic landmark
fields may therefore depend on which tile requests a corner first.
"""
from terrain_geometry_reference import TerrainGeometry,trunc_div

HEIGHT_PRIORITY=(2,1,0x200,0x100,0x800)
SHADE_PRIORITY=(1,2,0x200)

def profile_value(word,vertex,tables,shade=False):
    if not word&(0x203 if shade else 0xb03):return 0
    detail=word>>24
    if detail>=128:
        if shade:return 0
        raise ValueError('Negative height detail reads before the normal mapping table')
    if detail>=49:raise ValueError('Detail outside measured49-row footprint')
    profiles=tables['shade_profiles' if shade else 'profiles']
    for bit in SHADE_PRIORITY if shade else HEIGHT_PRIORITY:
        if word&bit:
            value=profiles[str(bit)][tables['mapping'][detail*5+vertex]]
            return value if not shade and bit==2 else value if value<128 else value-256
    return 0

def prepare_profiles(tables,words):
    """Native5a9f30 restores stock profiles, then clears suppressed1/2 entries.

    Does not rewrite tile words; this is a shared-table modification and can
    affect another positive-detail tile with the same mapped vertices.
    """
    profiles={k:list(v) for k,v in tables['profiles'].items()}
    for word in words:
        detail=word>>24
        if detail<128:continue
        row=255-detail
        if row>=49:raise ValueError('Suppression row outside measured table')
        for bit in [1,2]:
            if word&bit:
                for index in tables['mapping'][row*5:row*5+5]:profiles[str(bit)][index]=0
    return dict(tables,profiles=profiles)

class LandmarkGeometry(TerrainGeometry):
    def __init__(self,*args,words,tables,**kw):
        super().__init__(*args,**kw);self.words=words;self.tables=tables;self.cache={}

    def height(self,x,y,vertex):
        q=self.coord(x,y)
        if q is None or self.bands[q]<3:return 0
        key=(*q,vertex)
        if key in self.cache:return self.cache[key]
        profile=profile_value(self.words.get(q,0),vertex,self.tables)
        if vertex==0:base=self.height_step*(self.bands[q]-3)
        else:
            cells=self.corner_cells(*q,vertex)
            base=0 if any(c is None or self.bands[c]<3 for c in cells) else sum(self.height(*c,0) for c in cells)>>2
        value=base+trunc_div(self.height_step*profile,100)
        if profile>0:value=min(127,value)
        elif profile<0:value=max(-127,value)
        value=min(127,value)
        if vertex:
            for c,v in zip(self.corner_cells(*q,vertex),(3,4,1,2)):
                if c is not None:self.cache[(*c,v)]=value
        else:self.cache[key]=value
        return value

    def shade(self,x,y,vertex):
        return super().shade(x,y,vertex)+profile_value(self.words.get((x,y),0),vertex,self.tables,shade=True)

    def projector(self,x,y,cx,cy):
        return [cx-48,cy-24-(sum(self.height(x,y,k) for k in range(1,5))>>2)]
