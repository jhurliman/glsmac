"""Independent design record classification and ability compatibility.

Cost arithmetic is the existing checked shared reference, not a new oracle.
Naming accepts resolved language fields, never native generated result strings.
"""
import struct,types
from startup_technology_reference import has_technology
import startup_design_cost_reference as cost_reference

def compatible(ability,chassis,weapon,armor,rules):
 c=rules['chassis'][chassis];w=rules['weapons'][weapon];a=rules['armor'][armor];f=rules['abilities'][ability]['flags']
 if c['special'] or not f&(1<<c['domain']):return False
 kind=8 if w['offense']!=0 else 16 if w['mode']==9 else 32
 if not f&kind:return False
 if w['mode']==11 and f&64:return False
 if (w['offense']<0 or a<0) and f&128:return False
 if f&256 and w['mode']!=7:return False
 if f&512 and c['speed']>1:return False
 return True

def sanitize(mask,slots,chassis,weapon,armor,second,rules):
 slots=list(slots)
 if rules['chassis'][chassis]['special']:return 0,[-1,-1]
 for bit in range(24):
  flag=1<<bit
  if not mask&flag:continue
  count=mask.bit_count()
  remove=(flag==128 and mask&64) or not compatible(bit,chassis,weapon,armor,rules) or count>2 or (count==2 and not second)
  if remove:
   mask&=~flag;slots=([j for j in range(24) if mask&(1<<j)]+[-1,-1])[:2]
 return mask,slots

def classification(chassis,weapon,armor,mask,rules):
 c=rules['chassis'][chassis];w=rules['weapons'][weapon];defense=rules['armor'][armor];offense=w['offense'];speed=c['speed'];mode=w['mode']
 if c['special']:return (5,14) if offense>=99 else (0,13)
 if mode>=7:return mode,mode+32
 if c['domain']==1:return 6,6
 if c['domain']==2:return (4,9) if mask&32 else (0,8)
 plan=2 if defense>1 else 0
 if offense>=defense and plan==2:plan=1
 if speed>1 and offense>defense:plan=0
 if mask&0x8018:plan=0
 category=3
 if defense>offense:category=2
 if offense>2*defense:category=1
 if speed>=2:category=4
 if speed>=3:category=5
 if offense==defense==1:
  category=10
  if speed>1:plan=3
  if mask&0x400000:plan=2
 return plan,category

def build(designs,index,job,rules,tables,ownership):
 raw=bytearray(designs);ch,we,ar,re,mask=[job[k] for k in ['chassis','weapon','armor','reactor','mask']];owner=index//64
 known=[False]*3
 if index>=64:
  for i in [*range(64),*range(owner*64,(owner+1)*64)]:
   a=i*52;flags=struct.unpack_from('<H',raw,a+48)[0]
   if not flags&1:continue
   if i<64:
    if not has_technology(owner,struct.unpack_from('<h',raw,a+50)[0],ownership,rules['prerequisites']):continue
   elif not flags&4:continue
   hc,hw,ha=raw[a+36:a+39]
   known[0]|=hw==we or (rules['weapons'][we]['offense']>0 and rules['weapons'][hw]['offense']>=rules['weapons'][we]['offense'])
   known[1]|=ha==ar or (rules['armor'][ar]>0 and rules['armor'][ha]>=rules['armor'][ar])
   known[2]|=hc==ch or (rules['chassis'][hc]['domain']==rules['chassis'][ch]['domain'] and rules['chassis'][hc]['speed']>=rules['chassis'][ch]['speed'])
 flags=1
 if all(known):flags=0x115 if struct.unpack_from('<H',raw,(owner*64+63)*52+48)[0]&16 else 0x105
 a=index*52;raw[a+32:a+40]=struct.pack('<I4B',mask,ch,we,ar,re)
 env=dict(cost_reference.__dict__,WE=tables['WEAPONS'],AR=tables['DEFENSES'],CH=tables['CHASSIS'],AB=tables['ABILITIES'])
 cost=types.FunctionType(cost_reference.raw_cost.__code__,env,argdefs=(None,))(dict(chassis=ch,weapon=we,armor=ar,reactor=re,abilities=[i for i in range(24) if mask&(1<<i)],custom_ability_cost=None))
 raw[a+41]=cost&255
 carry=0
 if rules['weapons'][we]['mode']==7:
  carry=int(tables['CHASSIS'][ch][12])
  if rules['chassis'][ch]['domain']==1:carry*=re
  if mask&0x800000:carry//=2
  if mask&0x80000:carry=(3*carry+1)//2
 raw[a+40]=carry&255;raw[a+42:a+44]=bytes(classification(ch,we,ar,mask,rules));raw[a+44]=0;raw[a+45]=255 if index<64 else (1<<owner)&255;raw[a+46]=255;struct.pack_into('<H',raw,a+48,flags)
 return raw

def land_name_variants(job,owner,index,rules):
 """Conventional physical land and admitted ordinary noncombat naming branches.

 Higher-reactor disambiguation scan is false for these post-build records,
 whose own active record has the target reactor. Collision handling is separate.
 """
 ch,we,ar,re,mask=[job[k] for k in ['chassis','weapon','armor','reactor','mask']];c=rules['chassis'][ch];w=rules['weapons'][we];a=rules['armor'][ar];txt=rules['text'];f=txt['chassis_fields'][ch];labels=txt['labels'];speed=c['speed'];assert c['domain']==0 and not c['special'] and w['offense']>=0
 prefix=''
 for bit in range(24):
  if not mask&(1<<bit) or not txt['ability'][bit] or bit==15:continue
  if bit==3 and speed==1 and w['offense']>=a and not mask&0x8000:continue
  prefix=txt['ability'][bit]+' ';break
 full=short=prefix;compact=False;elite=False;variant=((re-1)^owner)&1
 if w['mode']<3:
  threshold=3 if a<=w['offense'] else 6;elite=min(a,w['offense'])>threshold and bool(f['elite']) and bool(f['elite_defense'])
 if speed>=max(w['offense'],a) and w['mode']<3:
  compact=True;variant=int(speed>1)
  if mask&0x400000:
   full=full[:-1] if full else full;short=short[:max(0,len(full)-1)]
  else:
   label=labels['0x2d8'] if w['offense']>=2 else labels['0x248'];full+=label;short+=label
 if a>w['offense'] and w['mode']<3 and not mask&0x8000:
  if not compact:full+=txt['armor'][ar];short+=txt['armor'][ar];variant^=a&1
  full+=' '+f['defense']
  short+=' '+(f['elite_defense'] if elite else f['defense']) if compact and index&1 else ' '+f['defense_alternate']
 else:
  if w['mode']>=3 and ch!=0:
   if re>1:full+=txt['reactor'][re-1]+' ';short+=txt['reactor'][re-1]+' '
   full+=f['name']+' ';short+=f['ordinary']+' '
  if not compact:full+=txt['weapon'][we];short+=txt['weapon'][we];variant^=w['offense']&1
  if w['mode']<3:
   if mask&0x8000:full+=' '+labels['0x87c'];short+=' '+labels['0x880']
   elif mask&8 and speed==1 and w['offense']>a:full+=' '+labels['0x998'];short+=' '+labels['0x99c']
   else:full+=' '+(f['elite'] if elite else f['name']);short+=' '+f['ordinary']
 return full,short,variant

def automatic_name(designs,index,job,owner,rules):
 full,short,variant=land_name_variants(job,owner,index,rules);same=full==short;marker=rules['text']['labels']['0xe5c'];wanted=struct.pack('<I4B',job['mask'],job['chassis'],job['weapon'],job['armor'],job['reactor'])
 def bump(s):
  pos=s.rfind(marker)
  if pos>=0 and len(s)-pos<=len(marker)+1 and s[-1:] in '2345678':return s[:-1]+str(int(s[-1])+1)
  return s+' '+marker+'2'
 for _ in range(64):
  changed=False
  for i in [*range(64),*range(owner*64,(owner+1)*64)]:
   if i==index or designs[i*52+32:i*52+40]==wanted:continue
   name=bytes(designs[i*52:i*52+32]).split(b'\0',1)[0].decode('cp1252')
   if name==full:
    if not same:variant=1
    full=bump(full);changed=True;break
   if not same and name==short:variant=0;short=bump(short);changed=True;break
  if not changed:return short if variant else full
 raise ValueError('collision chain outside bounded reference')
