"""Clean-room semantic climate reference, 2026-10-18.
Inputs are named tile/rule authorities, not binary records. See
climate-recalculation-rules.md for admission, bit identities, limits and cases.
All arithmetic is bounded signed integer/truncation; overflow is not specified.
"""
from dataclasses import dataclass,field
import struct

def trunc(a,b):return (abs(a)//abs(b))*(-1 if (a<0)!=(b<0) else 1)
def clamp(v,lo,hi):return min(hi,max(lo,v))

@dataclass
class Tile:
 elevation:int
 temperature:int
 moisture:int
 contour:int
 aux:int
 items:int
 landmark_word:int
 cached_occupant:int=15
 rockiness:int=0
 remembered_items:list=field(default_factory=list)

class World:
 def __init__(self,tiles,width,height,flat=False):
  self.tiles=tiles;self.width=width;self.height=height;self.flat=flat;self.dirty_flags=0
  assert len(tiles)==width*height//2
 def coords(self):return [(x,y) for y in range(self.height) for x in range(y&1,self.width,2)]
 def at(self,x,y):
  if not self.flat:
   if x<0:x+=self.width
   elif x>=self.width:x-=self.width
  if x<0 or x>=self.width or y<0 or y>=self.height:return None
  return self.tiles[y*(self.width//2)+x//2]

def set_items(tile,value):tile.items=value&0xffffffff
def set_marks(tile,value):tile.landmark_word=value&0xffffffff

class Shared:
 def __init__(self,seed):self.seed=seed&0xffffffff;self.events=[]
 def reseed(self,seed):self.events.append(('seed',seed&0xffffffff));self.seed=seed&0xffffffff
 def draw(self,lo,hi):
  lo,hi=min(lo,hi),max(lo,hi);self.seed=(1664525*self.seed+1013904223)&0xffffffff
  value=lo+(((self.seed&65535)*(hi-lo))>>16);self.events.append(('draw',lo,hi,value));return value
def temperature(w,rule,rng,offsets):
 h=w.height;jitter=trunc(h,rule['jitter_divisor']);category=rule['temperature']
 rng.reseed(rule['pattern']+17)
 for x,y in w.coords():
  t=w.at(x,y);lm=t.landmark_word
  if lm&0x80000001==1 and lm&0xff000000<0x15000000:continue
  r1=rng.draw(0,2*jitter);r2=rng.draw(0,category+1)
  latitude=abs(trunc(h,2)-r1-y+jitter)-(category-1)*trunc(h,rule['latitude_divisor'])-rule['extra']*trunc(h,rule['warming_divisor'])
  span=trunc(h,rule['band_divisor']);index=trunc(trunc(span,2)+2*latitude,span)
  value=3 if index<=2 else 2 if index<=9 else 1;alt=t.elevation
  for dx,dy in offsets:
   n=w.at(x+dx,y+dy)
   if n is not None and n.items&0x1000000 and (alt<=4 or n.elevation>=alt):value+=1;break
  if value>1:
   if alt>5:
    if category<2 or rng.draw(0,2)==0:value-=1
   elif alt>4 and r2==0:value-=1
  value=clamp(value-1,0,2);t.temperature=value
 return w
def rivers(w,rng,dirs,boundaries):
 for t in w.tiles:set_items(t,t.items&~0x280)
 for sx,sy in w.coords():
  source=w.at(sx,sy)
  if not(source.items&0x100) or source.elevation<3:continue
  for t in w.tiles:set_marks(t,t.landmark_word&~0x800000)
  rng.reseed(47*sy+31*sx);direction=rng.draw(0,4);x,y=sx,sy;band=source.elevation;height=source.contour
  set_marks(source,source.landmark_word|0x800000);finish=True;cycle_previous=0;steps=0;length=1;same_band=1
  while not w.at(x,y).items&0x1000000:
   preferred=(rng.draw(0,2)+direction-(length&1))&3;back=direction^2;candidates=[];minimum_uphill=9999;uphill_weight=8;terminated=False
   for d,(dx,dy) in enumerate(dirs):
    if d==back:continue
    n=w.at(x+dx,y+dy)
    if n is None:continue
    nx,ny=x+dx,y+dy
    if not w.flat:nx%=w.width
    flags=n.items
    if flags&0x80:terminated=True;break
    if flags&0x100:
     crowd=sum(1 for ax,ay in dirs if (v:=w.at(nx+ax,ny+ay)) is not None and v.items&0x100)
     if crowd>1:terminated=True;break
    if n.elevation>band:continue
    nh=n.contour;weight=clamp(height-nh+2,1,9999)*32+(31 if d==preferred else 0)
    if nh>height:
     if nh>minimum_uphill:continue
     minimum_uphill=nh;weight=uphill_weight;uphill_weight+=1
    higher=0;cycle=0;reject=False
    for k,(ax,ay) in enumerate(dirs):
     if k==d^2:continue
     a=w.at(nx+ax,ny+ay)
     if a is None:continue
     if a.elevation<3 and n.elevation>=3 and same_band<5:weight=trunc(weight,2)
     if a.contour>nh:
      higher+=1
      if a.items&0x80:reject=True;break
     if a.landmark_word&0x800000:
      if cycle_previous or a.elevation<3:reject=True;break
      weight=trunc(weight,8);cycle=1
    if reject or not weight:continue
    if higher==3:weight=1
    candidates.append((d,weight,cycle))
   if terminated:finish=False;break
   if not candidates:break
   value=rng.draw(0,sum(z[1] for z in candidates));chosen=candidates[-1]
   for candidate in candidates:
    if value<candidate[1]:chosen=candidate;break
    value-=candidate[1]
   direction,_,cycle_previous=chosen;dx,dy=dirs[direction];x+=dx;y+=dy
   if not w.flat:x%=w.width
   t=w.at(x,y);new_band=t.elevation
   if new_band!=band:same_band=0
   band=new_band;nh=t.contour
   if nh>height:nh=max(height-1,boundaries[band]);t.contour=nh&255
   height=nh;steps+=1
   if steps>=1000:break
   set_marks(t,t.landmark_word|0x800000);length+=1;same_band+=1
  else:finish=False
  t=w.at(x,y);flags=t.items;owner=t.cached_occupant
  if band>=3 and not flags&0x80 and finish and not(flags&3 and owner<8):set_items(t,flags|0x200)
  for t in w.tiles:
   if t.landmark_word&0x800000:set_items(t,t.items|0x80)
 for t in w.tiles:set_marks(t,t.landmark_word&~0x800000)
 return w
def rainfall(w,rule,offsets9,offsets8):
 rain=rule['rainfall'];shadow_step=rule['shadow_step']
 for t in w.tiles:t.aux=0
 def mark(x,y,mask):
  n=w.at(x,y)
  if n is not None:n.aux|=mask
 for x,y in w.coords():
  t=w.at(x,y);band=t.elevation
  if t.items&0x400000:
   for dx,dy in offsets9:
    n=w.at(x+dx,y+dy)
    if n is not None:n.aux|=4 if n.aux&1 else 1
  if band<3:
   for k in range(3):
    a=w.at(x-2*(k+1),y);b=w.at(x-2*k,y)
    if a is not None and b is not None and a.elevation>=b.elevation and k<=rain:a.aux&=~16
   mark(x+2,y,128)
   if rain>0:
    n=w.at(x+4,y)
    if n is not None and ((n.temperature)>1 or rain>1):
     n.aux|=128
     if rain>1:
      a=w.at(x+6,y)
      if a is not None and a.temperature:a.aux|=128
  if band<=4:
   if band>=3 and t.items&0x200000:
    for dy in [-1,0,1]:mark(x+1,y+dy,64)
    mark(x+2,y,64)
  else:
   extent=rule['mountain_shadow'] if band>5 else rule['hill_shadow'];variation=(rain-1)*shadow_step
   for direction,limit,mask in [(-1,extent+variation,32),(1,extent-variation,16)]:
    for distance in range(1,limit+1):
     n=w.at(x+direction*2*distance,y)
     if n is None:continue
     if n.elevation<3 or n.elevation>=band:break
     n.aux|=mask
 wet=high=low=0
 for x,y in w.coords():
  t=w.at(x,y);band=t.elevation;temp=t.temperature;lm=t.landmark_word;f=t.aux;result=0;adjust=0
  if y==0 or y==w.height-1:t.moisture=0;continue
  if lm&0x80000040==0x40 or lm&0x80000100==0x100:wet=1
  if f&32:result=1;low=0;wet=1
  if f&128:
   wet=int(temp>1);high=0;low=0
   if rain>=1:adjust=1
  if f&16:
   if wet:wet-=1
   adjust-=1
  if f&64 and band<=4:high=0;low=0;adjust=min(0,adjust)
  if band>3:
   high+=1
   if high>4*shadow_step*rain+1 and wet:wet-=1
  elif band<3:low=0;wet=1
  if band==3:
   if wet and high==0:
    low+=1
    if low>4*rain+2:wet=0;low=0
  if band<=3 and high:
   high-=1+rain
   if high<1:
    high=0;low=0
    if rain!=0 or band<3:wet=1
  if t.items&0x80:
   if (x//2)%3 and adjust<0:adjust=0
   wet=1;low=0
  if f&2:result-=1;wet=0
  elif wet:adjust+=wet
  if band<=4+int(temp!=0):result+=adjust
  elif rain-band+temp+5>=2 and adjust>0:result+=1
  if lm&0x80000004==4 and band>=3 and band<6:
   detail=abs(struct.unpack('<b',bytes([lm>>24]))[0])
   result+=2 if detail<49 or (detail<81 and band<5) else 1
  result=0 if lm&0x80000040==0x40 else clamp(result,0,2)
  for flag in [1,4]:
   if f&flag:wet=1;result=min(2,result+1)
  t.moisture=result
 for x,y in w.coords():
  t=w.at(x,y)
  if t.moisture:continue
  if any((n:=w.at(x+dx,y+dy)) is not None and n.elevation>=3 and n.moisture==2 for dx,dy in offsets8):t.moisture=1;set_marks(t,t.landmark_word|0x400000)
 return w

class CRT:
 """Terrain CRT stream; separate from Shared, with actual raw draw outputs."""
 def __init__(self,seed):self.seed=seed&0xffffffff;self.events=[]
 def draw(self):
  self.seed=(214013*self.seed+2531011)&0xffffffff;value=(self.seed>>16)&32767;self.events.append(value);return value

def classify_contour(contour,offset,thresholds):
 value=contour-offset
 for category in range(6,-1,-1):
  if value>=thresholds[category]:return category
 return 0

def assign_surface_class(w,x,y,category,crt,ctx):
 t=w.at(x,y);old=t.elevation
 if ctx['turn']!=0:
  if old!=category and t.landmark_word:
   prior=t.landmark_word;t.landmark_word&=~0x80;detail=(t.landmark_word>>24)&255
   if detail<128:
    if ctx.get('invalidate_landmark_cache'):ctx['invalidate_landmark_cache'](prior,detail)
    t.landmark_word=(t.landmark_word&0xffffff)|((~detail&255)<<24);w.dirty_flags|=4
  if (old<3)!=(category<3):
   t.items&=0x7e1377a3;t.remembered_items=[t.items]*7
   if not ctx['terrain_only']:
    callback=ctx.get('entity_change')
    if callback:callback(w,x,y,old,category,crt)
    elif t.items&3 and t.cached_occupant<8:raise ValueError('Cross-category occupied tile needs real Base/Unit coupling')
  elif category>=3:
   rock=t.rockiness
   if category>old and rock<2 and rock+1<=category-3:
    if crt.draw()%(rock+4)==0:t.rockiness+=1;t.landmark_word|=0x400000;w.dirty_flags|=1
   elif category<old and rock>0 and rock>category-3:
    if crt.draw()%(rock+1)!=0:t.rockiness-=1;t.landmark_word|=0x400000;w.dirty_flags|=1
 t.elevation=category;w.dirty_flags|=4

def ensure_contour_class(w,x,y,category,crt,ctx):
 assign_surface_class(w,x,y,category,crt,ctx);t=w.at(x,y);thresholds=ctx['contour_thresholds']
 if classify_contour(t.contour,ctx['sea_offset'],thresholds)!=category:
  span=thresholds[category+1]-thresholds[category];residue=crt.draw()%span if span>1 else 0
  t.contour=(thresholds[category]+ctx['sea_offset']+residue)&255

def shape_height(w,x,y,category,crt,ctx):
 # These are per-call scratch summaries, not accumulated climate statistics.
 w.flood_counts=[0]*8;w.affected_Base=[-1]*8
 if w.at(x,y) is not None:ensure_contour_class(w,x,y,category,crt,ctx)
 ends=ctx['ring_ends'];offsets=ctx['shaping_offsets']
 for direction,max_radius in [(-1,category),(1,8-category)]:
  for radius in range(1,max_radius):
   target=category+direction*radius;changed=False
   for dx,dy in offsets[ends[radius-1]:ends[radius]]:
    nx=x+dx;ny=y+dy;t=w.at(nx,ny)
    if t is None:continue
    if (direction<0 and t.elevation<target) or (direction>0 and t.elevation>target):
     ensure_contour_class(w,nx%w.width if not w.flat else nx,ny,target,crt,ctx)
     if not(t.items&3 and t.cached_occupant<8) and not ctx['resource_bonus'](w,nx,ny):t.cached_occupant=15
     changed=True
   if not changed:break

def recalculate_altitudes(w,crt,ctx):
 for x,y in w.coords():assign_surface_class(w,x,y,classify_contour(w.at(x,y).contour,ctx['sea_offset'],ctx['contour_thresholds']),crt,ctx)
 for category in range(6,1,-1):
  for x,y in w.coords():
   if w.at(x,y).elevation==category:shape_height(w,x,y,category,crt,ctx)
 return w

def ecology_volcanic_landmark(w,x,y,crt,ctx):
 """Already admitted valid-coordinate ecology arm, suppressing new name."""
 assert w.at(x,y) is not None
 shape_height(w,x,y,0,crt,ctx);shape_height(w,x,y,6,crt,ctx)
 for detail,(dx,dy) in enumerate(ctx['shaping_offsets'][:9]):
  t=w.at(x+dx,y+dy)
  if t is None:continue
  t.landmark_word=((t.landmark_word|2)&0xffffff)|(detail<<24);w.dirty_flags|=4
  t.items&=0xee377e83
  if detail==0:t.items&=0xf3ffffff
  t.items|=0x100000;t.rockiness=2;t.landmark_word|=0x400000;w.dirty_flags|=1
 w.Mount_Planet=(x,y)
 return w
