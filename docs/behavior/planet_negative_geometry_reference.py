"""Bounded authored-Planet negative-detail lookup; no suppression-row rewrite."""
from landmark_geometry_reference import LandmarkGeometry,profile_value
from terrain_geometry_reference import trunc_div

class PlanetGeometry(LandmarkGeometry):
    def face_shade(self,x,y,face):
        # A cold native slope cache obtains its two corners through height calls.
        if self.band(x,y) is not None and self.band(x,y)>=3:
            self.height(x,y,face+1);self.height(x,y,(face+1)%4+1)
        return super().face_shade(x,y,face)

    def height(self,x,y,vertex):
        q=self.coord(x,y)
        if q is None or self.bands[q]<3:return 0
        key=(*q,vertex)
        if key in self.cache and self.cache[key]!=-1:return self.cache[key]
        word=self.words.get(q,0);detail=word>>24
        if word&0xb03 and detail>=128:
            row=self.tables['negative_mapping'].get(str(detail-256))
            if row is None:raise ValueError('Negative detail outside authored Planet receipt')
            bit=next(b for b in (2,1,0x200,0x100,0x800) if word&b)
            index=row[vertex];profile=self.tables['profiles'][str(bit)][index]
            if bit!=2 and profile>=128:profile-=256
        else:profile=profile_value(word,vertex,self.tables)
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
