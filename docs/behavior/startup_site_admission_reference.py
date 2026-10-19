"""Turn-zero native search/refinement algorithm with an owning quality callback.

quality(x,y,tiles) computes the existing uncached land-site rating. Captured
candidate coordinates, ratings and random values are never inputs here.
"""
import struct
from startup_site_balance_reference import distance,truncdiv,clamp

def select(world,faction,quality):
 assert world['state']['turn']==0
 width=world['width'];height=world['height'];flags=world['flags'];tiles=bytearray.fromhex(world['map_hex']);seed=world['state']['crt'];bases=world['state']['bases'];units=world['state']['units'];attempts=[];queries=[];writes=[];refinements=[]
 def ix(x,y):return (y*(width//2)+x//2)*44
 def cell(p):return tiles[ix(*p):ix(*p)+44]
 def word(p,offset):return struct.unpack_from('<I',tiles,ix(*p)+offset)[0]
 def wrap(x):
  if not flags&1:
   if x<0:x+=width
   elif x>=width:x-=width
  return x
 def valid(p):return 0<=p[0]<width and 0<=p[1]<height
 def nearby(center,offsets):
  for dx,dy in offsets:
   p=(wrap(center[0]+dx),center[1]+dy)
   if valid(p):yield p
 def nearest(center,region=None):
  chosen=-1;best=9999
  for b in bases:
   if region is not None and cell(b['position'])[3]!=region:continue
   d=distance(center,b['position'],width,flags)
   if d<=best:chosen=b['id'];best=d
  return chosen,best
 def rating(pos,caller):
  if word(pos,8)&0x1000:return 0
  cached=cell(pos)[2]>>4
  if cached:return cached
  value=quality(pos[0],pos[1],tiles);queries.append(dict(caller=caller,position=list(pos),value=value,attempt=len(attempts)))
  a=ix(*pos)+2;old=tiles[a];tiles[a]=(old&15)|((value&15)<<4);writes.append(dict(offset=a,before=old,value=tiles[a]))
  return value
 def draw(span):
  nonlocal seed
  if span<=1:return 0
  seed=(214013*seed+2531011)&0xffffffff
  return ((seed>>16)&32767)%span
 human=bool(world['state']['human']&(1<<faction));difficulty=world['difficulty'];offsets=world['offsets'];counter=0
 while True:
  mx=clamp(truncdiv((1024-counter)*width,8192),0,9999);my=clamp(truncdiv((1024-counter)*height,8192),0,9999)
  x=draw(width-2*mx if flags&1 else width)+(mx if flags&1 else 0);y=draw(height-2*my)+my;x=(x&~1)+(y&1);pos=(x,y)
  bucket=32
  for ring in range(8):
   lo=world['rings'][ring] if ring else 0;hi=world['rings'][ring+1]
   if any(0<(word(p,12)&0xff000000)<0x80000000 for p in nearby(pos,offsets[lo:hi])):bucket=ring;break
  good=0
  for p in nearby(pos,offsets[:21]):
   t=cell(p);alt=t[0]>>5
   if alt>=3 and not word(p,8)&0x20 and t[5]>>6<=1 and t[0]&0x18:good+=1
  t=cell(pos);region=t[3];mask=sum((1<<(i+1)) for i,row in enumerate(world['presence']) if row[region]);baseid,dist=nearest(pos,region)
  if baseid<0:dist=32
  for unit in units:
   if cell(unit['position'])[3]==region:mask|=1<<unit['owner'];dist=min(dist,distance(pos,unit['position'],width,flags))
  counter+=1
  row=dict(index=counter-1,position=list(pos),crt_after_pair=seed,good_tiles=good,landmark_ring=bucket,nearest=baseid,distance=dist,region=region,presence=mask);attempts.append(row)
  reason=None;area=world['regions'][region][0]
  if counter>1024 and not human:reason='exhausted'
  elif t[0]&0xe0<0x60:reason='elevation_low'
  elif word(pos,8)&0x20:reason='fungus'
  elif t[5]&0xc0>0x40:reason='rocky'
  elif dist<truncdiv(6*world['map_scale'],56):reason='initial_distance'
  elif t[0]&0xe0>=0xa0:reason='elevation_high'
  elif word(pos,8)&3 and t[2]&15<8:reason='occupied'
  elif y<2 or y>=height-2:reason='polar'
  elif good<12-truncdiv(counter,80):reason='good_tiles'
  elif dist<truncdiv((16-truncdiv(counter,64))*world['map_scale'],56):reason='relaxed_distance'
  elif counter<900 and area<36-truncdiv(counter,64):reason='early_region_area'
  elif truncdiv(world['regions'][region][1]*(world['region_density']+1),2)<24+(8*(6-difficulty) if human else 8)-truncdiv(counter,16):reason='region_density'
  elif area<48*(5-(difficulty if human else 3))-truncdiv(counter,4):reason='region_area'
  elif mask and truncdiv(area,mask.bit_count()+1)<64-truncdiv(counter,64):reason='shared_region_area'
  else:
   score=rating(pos,'0x5c969f');row['effective_score']=score
   if score<15-truncdiv(counter,64) and not world['state']['rules']&0x10:reason='site_rating'
   elif bucket<truncdiv(1024-counter,128) or bucket==0:reason='landmark_distance'
  row['reason']=reason
  if reason=='exhausted':return dict(refused=True,attempts=attempts,queries=queries,writes=writes,final_crt=seed,map_hex=tiles.hex())
  if reason is not None:continue
  break
 center=pos;best=-1;selected_index=0
 for i,p in enumerate(nearby(center,offsets[:21])):
  # Recover the actual offset index even when clipping skips earlier offsets.
  index=next(j for j,(dx,dy) in enumerate(offsets[:21]) if (wrap(center[0]+dx),center[1]+dy)==p)
  t=cell(p);alt=t[0]&0xe0;items=word(p,8)
  if alt<0x60 or alt>=0xa0 or t[3]!=region or items&0x20 or (items&3 and t[2]&15<8):continue
  _,dist=nearest(p)
  # Original Unit comparison uses accepted center, not refinement candidate.
  dist=min([dist,*[distance(center,u['position'],width,flags) for u in units]])
  if dist<6:continue
  value=rating(p,'0x5c99be')
  if any(cell(n)[0]&0xe0<0x60 for n in nearby(p,world['coast_offsets'])):value+=1
  refinements.append(dict(index=index,position=list(p),score=value,old_best=best))
  if value>best:best=value;selected_index=index
 dx,dy=offsets[selected_index];final=[wrap(center[0]+dx),center[1]+dy]
 return dict(refused=False,attempts=attempts,accepted_center=list(center),final_position=final,queries=queries,writes=writes,refinements=refinements,final_crt=seed,map_hex=tiles.hex())
