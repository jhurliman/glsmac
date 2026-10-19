"""Source-derived startup score/deficit/terrain compensation; runtime map inputs.

This models 0x5c7c90 arithmetic and tile-item edits. Unit construction and its
visibility effects stay with the engine. No installed map or score table here.
"""
import struct

def truncdiv(a,b):return (abs(a)//abs(b))*(-1 if (a<0)!=(b<0) else 1)
def clamp(x,lo,hi):return max(lo,min(x,hi))
def distance(a,b,width,flags):
 dx=abs(a[0]-b[0]);dy=abs(a[1]-b[1])
 if not flags&1 and dx>width//2:dx=width-dx
 return max(dx,dy)-((((dx+dy)//2)-min(dx,dy)+1)//2)

def evaluate(world):
 tiles=bytearray.fromhex(world['tiles_hex']);width=world['width'];height=world['height'];flags=world['map_flags'];units=world['state']['units']
 def index(x,y):return y*(width//2)+x//2
 def tile(pos):return tiles[44*index(*pos):44*(index(*pos)+1)]
 def nearest(f,origin,excluded=-1):
  best=-1;dist=9999
  for u in units:
   if u['owner']!=f or u['id']==excluded:continue
   d=distance(origin,u['position'],width,flags)
   if d<=dist:best=u['id'];dist=d
  return next((u for u in units if u['id']==best),None)
 def wrap(x):
  if not flags&1:
   if x<0:x+=width
   elif x>=width:x-=width
  return x
 def items_at(pos):return struct.unpack_from('<I',tiles,index(*pos)*44+8)[0]
 def site_at(x,y):
  t=tile((x,y));items=items_at((x,y));rules=world['state']['rules']
  if items&0x102000:return 0
  if rules&0x2000:return 2 if items&0xc000000 else 0
  if items&0x10000000:return 1
  pattern=world['resource_pattern']
  if not pattern:return 0
  a=(y+x)//2;b=x-a
  if t[0]&0xe0>=0x60 and (a&3)+4*(b&3)==(pattern-5*(a>>2)-3*(b>>2))&15:return 2
  m0,m1=world['pattern_multipliers']
  return int((a&3)+4*(b&3)==(pattern+m1*truncdiv(a,4)+m0*truncdiv(b,4)+8)&31)
 anchors={}
 for f in range(1,8):
  u=nearest(f,(0,0))
  if u is not None and tile(u['position'])[3]>=64:u=nearest(f,u['position'],u['id'])
  if u is not None:anchors[f]=u['position']
 scores={};components=[]
 for f,pos in anchors.items():
  region=tile(pos)[3];area=world['regions'][7*region]
  spacing=min((truncdiv(3200*distance(pos,p,width,flags),world['tile_count']) for other,p in anchors.items() if other!=f and tile(p)[3]==region),default=9999)
  space=clamp(truncdiv(area+31,32),1,8) if spacing>=1000 else truncdiv(spacing,5)-3
  count=0
  for dx,dy in zip(world['offset_x'][:21],world['offset_y'][:21]):
   x=wrap(pos[0]+dx);y=pos[1]+dy
   if not (0<=x<width and 0<=y<height):continue
   t=tile((x,y));rain=(t[0]>>3)&3;alt=(t[0]>>5)-2;rock=t[5]>>6
   if rain<1:continue
   if not (alt<=0 and not items_at((x,y))&0x80) and rock<2:
    if rock==1 or alt>1 or rain>1:count+=1
   if rock==1 and alt>1:count+=1
  terrain=clamp(truncdiv(count,2),0,8);region_term=clamp(truncdiv(area,64),0,3)
  score=space+terrain+region_term;scores[f]=score
  components.append(dict(faction=f,position=pos,region=region,region_area=area,nearest_normalized_spacing=spacing,spacing_component=space,terrain_counter=count,terrain_component=terrain,region_component=region_term,score=score))
 maximum=max([0,*scores.values()]);seed=world['state']['crt'];deficits=[];pods=[];writes=[]
 for f,pos in anchors.items():
  seed=(214013*seed+2531011)&0xffffffff;orientation=((seed>>16)&32767)&15
  bonus=world['starting_bonuses'][f-1];credit=0
  for j in range(bonus[0]):
   if bonus[1+j]==1:credit+=3 if world['design_roles'][str(bonus[9+j])]==8 else 1
  d=maximum-scores[f]-credit;deficits.append(dict(faction=f,maximum=maximum,scores=[scores.get(i) for i in range(1,8)],unit_credit=credit,deficit=d))
  difficulty=world['difficulty'];human=bool(world['state']['human']&(1<<f))
  if difficulty>1 and (d>=3 or difficulty>=(4 if human else 5)):
   pods.append(f);d-=3
  if difficulty<2:d=truncdiv(d+1,2)
  i=0;special=0
  while i<d:
   ring_index=(7*(orientation+i+1))&15;x=wrap(pos[0]+world['offset_x'][9+ring_index]);y=pos[1]+world['offset_y'][9+ring_index]
   if 0<=x<width and 0<=y<height:
    value=items_at((x,y))
    if site_at(x,y) and not value&0xc000000:scores[f]-=1
    if special<2 and d-i>=3 and not ((i^special)&1):mask=0x4000000;d-=1;special+=1
    else:mask=0x8000000
    value|=mask;offset=index(x,y)*44+8;struct.pack_into('<I',tiles,offset,value)
    writes.append(dict(faction=f,position=[x,y],offset=offset,mask=mask,value=value))
   i+=1
 return dict(anchors=anchors,components=components,maximum=maximum,deficits=deficits,pod_owners=pods,terrain_writes=writes,tiles_hex=tiles.hex(),final_crt=seed)
