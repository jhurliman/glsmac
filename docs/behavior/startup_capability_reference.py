"""Full577140 capability refresh, retaining raw faction bytes and CPU store order."""
import struct
from startup_technology_reference import has_technology
from startup_site_balance_reference import truncdiv
F0=0x95d1a8;STRIDE=0x20bc

def refresh(factions,ownership,prerequisites,catalog,*,owner,rules=0,psi_percent=0,ratio=(3,2),dream_twister_owner=-1,neural_amplifier_owner=-1):
 assert 1<=owner<=7 and len(factions)==8*STRIDE
 raw=bytearray(factions);writes=[]
 def at(f,a):return f*STRIDE+a-F0
 def get(f,a):return struct.unpack_from('<i',raw,at(f,a))[0]
 def put(a,v,pc):
  off=at(owner,a);v&=0xffffffff;struct.pack_into('<I',raw,off,v);writes.append([off,4,v,pc])
 def tech(t):return has_technology(owner,t,ownership,prerequisites)
 worm=catalog['mind_worm'];eligible=bool(worm['flags']&1) and not worm['exclusion']&(1<<owner) and tech(worm['prerequisite']) and not (worm['plan']==8 and rules&0x400000)
 old_attack=get(owner,0x95d9d0);old_defense=get(owner,0x95d9d4)
 def evaluate(power,weapon):
  if power>=0:return power
  v=truncdiv(old_defense*ratio[0],ratio[1]) if weapon else truncdiv(old_attack*ratio[1],ratio[0])
  if psi_percent:v=truncdiv(v*(100+psi_percent),100)
  if (dream_twister_owner if weapon else neural_amplifier_owner)==owner:v+=truncdiv(v,2)
  return v
 put(0x95d9c0,evaluate(catalog['weapons'][12][0],True) if eligible else 0,'0x577181');put(0x95d9bc,1,'0x577187')
 for power,prereq in catalog['weapons']:
  if not tech(prereq) or power>=99:continue
  value=evaluate(power,True)
  if power<0 and value>=get(owner,0x95d9c0):put(0x95d9c0,value,'0x5771c0')
  if value>=get(owner,0x95d9bc):put(0x95d9bc,value,'0x5771ce')
 put(0x95d9c4,evaluate(catalog['armor'][9][0],False) if eligible else 0,'0x577200');put(0x95d9c8,1,'0x577206')
 for power,prereq in catalog['armor']:
  if not tech(prereq):continue
  value=evaluate(power,False)
  if power<0 and value>=get(owner,0x95d9c4):put(0x95d9c4,value,'0x577241')
  if value>=get(owner,0x95d9c8):put(0x95d9c8,value,'0x57724f')
 put(0x95d9cc,1,'0x577261')
 for speed,domain,prereq in catalog['chassis']:
  if tech(prereq) and domain==0 and speed>=get(owner,0x95d9cc):put(0x95d9cc,speed,'0x577297')
 for a,pc in [(0x95d9d8,'0x5772ad'),(0x95d9d4,'0x5772b3'),(0x95d9d0,'0x5772b9'),(0x95d9e0,'0x5772bf'),(0x95d9dc,'0x5772c5')]:put(a,0,pc)
 peers=[]
 for tier in range(4):
  if get(owner,0x95d9d0)!=0:break
  for f in range(1,8):
   if f==owner:continue
   relation=get(owner,F0+0x18+4*f)
   if relation&1 or tier==0 and not relation&16 or tier<=1 and relation&2 or tier<=2 and not relation&8:continue
   peers.append([tier,f])
   for own,foreign,pc in [(0x95d9bc,0x95d9d0,'0x57733a'),(0x95d9c8,0x95d9d4,'0x57734e'),(0x95d9cc,0x95d9d8,'0x577363'),(0x95d9c0,0x95d9dc,'0x577378'),(0x95d9c4,0x95d9e0,'0x57738d')]:put(foreign,max(get(owner,foreign),get(f,own)),pc)
 return dict(factions=raw,writes=writes,peers=peers,native_seed_eligible=eligible)
