"""Admitted turn-zero named technology grants and canonical ownership query."""
import struct
BASE=0x95d1a8

def acquire_named(record,ownership,*,faction,technology,source=0):
 raw=bytearray(record);owned=bytearray(ownership);assert 0<=faction<8 and len(owned)==89
 if technology<0:return dict(record=raw,ownership=owned)
 assert technology<89,'Prototype/map/diplomacy awards are different acquisition paths'
 target=0x95d574-BASE
 if struct.unpack_from('<i',raw,target)[0]==technology:struct.pack_into('<i',raw,target,-1)
 raw[0x95d514-BASE+technology]=(source if source>0 else 0)&255
 owned[technology]|=1<<faction
 p=0x95f1f4-BASE;struct.pack_into('<I',raw,p,(struct.unpack_from('<I',raw,p)[0]+1)&0xffffffff)
 return dict(record=raw,ownership=owned)

def dispatch_named(record,ownership,*,faction,bonuses):
 raw=bytearray(record);owned=bytearray(ownership);calls=[]
 for kind,value in bonuses:
  if kind!=0:continue
  calls.append(value);result=acquire_named(raw,owned,faction=faction,technology=value,source=0);raw=result['record'];owned=result['ownership']
 return dict(record=raw,ownership=owned,calls=calls)

def has_technology(faction,technology,ownership,prerequisites):
 if faction<=0:return False
 if technology==-1:return True
 if not 0<=technology<88:return False
 first,second=prerequisites[technology]
 if first< -1 or (second< -1 and first!=-1):return False
 return bool(ownership[technology]&(1<<faction))
