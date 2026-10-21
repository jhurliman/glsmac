"""Turn-zero Base allocation/record seeding through4faa7b, before radius/territory.

Names and catalog/table fields are caller data. Later resource/territory bodies
and automatic name-list cursor management are separate owners.
"""
import struct
from startup_technology_reference import has_technology
FBASE=0x95d1a8
def trunc(a,b):return abs(a)//abs(b)*(-1 if (a<0)!=(b<0) else 1)

def mineral_factor(*,owner,human_mask,difficulty,network,reference_owner,map_category,multiplier,industry,ai_factors):
 factor=10 if human_mask&(1<<owner) else ai_factors[difficulty]-int(not network and bool(human_mask&(1<<reference_owner)))
 if multiplier!=10:factor=trunc(multiplier*factor,10)
 if map_category==1:factor=trunc(9*factor,10)
 elif map_category==0:factor=trunc(8*factor,10)
 if -7<=industry<=-3:return trunc(13*factor+9,10)
 if industry==-2:return trunc(6*factor+4,5)
 if industry==-1:return trunc(11*factor+9,10)
 if industry==0:return factor
 if industry==1:return trunc(9*factor+9,10)
 if industry==2:return trunc(4*factor+4,5)
 if industry==3:return trunc(7*factor+9,10)
 if industry==4:return trunc(3*factor+4,5)
 return trunc(factor+1,2)

def seed(record,tiles,faction_record,*,owner,position,width,height,base_count,name,
         ownership,prerequisites,design_costs,formers_prerequisite,free_facility_tech,
         bonuses,difficulty,human_mask,map_category=2,mineral_multiplier=10,
         ai_factors=(13,12,11,10,8,7),reference_owner=0,network=False,
         previous_base_same_owner=False,network_extra_population=False,
         network_population_penalty=False):
 raw=bytearray(record);plane=bytearray(tiles);faction=bytearray(faction_record)
 assert len(raw)==304 and len(faction)==0x20bc
 if base_count>=512:return dict(record=raw,tiles=plane,faction_record=faction,result=-1,base_count=base_count)
 def fp(address):return struct.unpack_from('<i',faction,address-FBASE)[0]
 def fput(address,v):struct.pack_into('<I',faction,address-FBASE,v&0xffffffff)
 def put(offset,v,size=4):raw[offset:offset+size]=(v&((1<<(size*8))-1)).to_bytes(size,'little')
 x,y=position;a=44*(width//2*y+x//2);region=plane[a+3];items=struct.unpack_from('<I',plane,a+8)[0]|1;struct.pack_into('<I',plane,a+8,items);plane[a+2]=(plane[a+2]&240)|(owner&15)
 if owner:struct.pack_into('<I',plane,a+12+4*owner,items)
 count=fp(0x95e200)+1;fput(0x95e200,count);p=0x95e320-FBASE+region;faction[p]=(faction[p]+1)&255
 put(0,x,2);put(2,y,2);put(4,owner,1);put(5,owner,1)
 for i in [7,8,9]:put(i,0,1)
 population=3 if previous_base_same_owner else 1;disclosure=plane[a+4]|(1<<owner);put(10,disclosure,1)
 text=name.encode('cp1252')+b'\0';assert len(text)<=25;raw[19:19+len(text)]=text
 for i in [48,52,60,64,72,76,120,124,128,292]:put(i,0)
 put(56,fp(0x95d378));put(268,-1);put(272,-1,2)
 target=1 if has_technology(owner,formers_prerequisite,ownership,prerequisites) else 2 if fp(0x95d1b0)>1 else 0
 put(80,target)
 factor=mineral_factor(owner=owner,human_mask=human_mask,difficulty=difficulty,network=network,reference_owner=reference_owner,map_category=map_category,multiplier=mineral_multiplier,industry=fp(0x95d418),ai_factors=ai_factors)
 put(64,trunc(factor*design_costs[target],2));put(44,-1,2);put(46,-1,2);put(140,0);put(144,0,1)
 for i in [288,292,296,300]:put(i,0)
 def facility(index):raw[140+trunc(index,8)]|=1<<(index&7)
 if count==1:
  facility(1)
  if network:
   population=3+int(network_extra_population)-int(network_population_penalty);facility(3)
   if difficulty>=3:facility(6)
   if fp(0x95d478)>0:facility(7)
  else:population=1
 put(6,population,1)
 if plane[a]>>5<3:facility(26)
 for viewer in range(8):put(11+viewer,population if viewer==owner or disclosure&(1<<viewer) else 0,1)
 for kind,value in bonuses:
  if kind==2:facility(max(1,min(38,value)))
 for i,tech in enumerate(free_facility_tech,1):
  if has_technology(owner,tech,ownership,prerequisites):facility(i)
 put(68,target);put(152,struct.unpack_from('<I',raw,64)[0]);put(148,0)
 return dict(record=raw,tiles=plane,faction_record=faction,result=base_count,base_count=base_count+1,history_entry=struct.pack('<BBhhh',0,owner,0,x,y))
