"""Turn-zero faction initialization5c89bc..5c9051, before site search.

Input buffers belong to the caller. This does not implement the startup
controller, site/package, design, Base, Unit or later social producers.
"""
import struct
BASE=0x95d1a8;STRIDE=0x20bc

def initialize(record,scratch_slots,*,faction,definition,difficulty,human_mask,
               active_mask,rules,secondary,network,starting_energy,crt,
               channel_prefix,channel_separator=' ',mode=-1):
 raw=bytearray(record);scratch=bytearray(scratch_slots);draws=[];writes=[]
 assert len(raw)==STRIDE and len(scratch)==512 and 0<=faction<8
 def put(address,value,size=4):
  off=address-BASE;value&=(1<<(size*8))-1;raw[off:off+size]=value.to_bytes(size,'little');writes.append([off,size,value])
 def get(address):return struct.unpack_from('<I',raw,address-BASE)[0]
 def zero(address,count,size=4):
  for i in range(count):put(address+size*i,0,size)
 def draw(caller):
  nonlocal crt
  before=crt;crt=(214013*crt+2531011)&0xffffffff;value=(crt>>16)&32767;draws.append(dict(caller=caller,before=before,after=crt,value=value));return value
 for a in [0x95d59c,0x95d598,0x95d60c]:put(a,-1)
 #58fce0 initializes100 complete16-byte entries across two tables.
 for start,count in [(0x95eba0,75),(0x95f050,25)]:
  for i in range(count):
   a=start+16*i;put(a,-1,2);put(a+2,0,2);put(a+8,0);put(a+4,0);put(a+12,0)
 #5941e0 makes each of64 eight-byte slots entirely FF.
 scratch[:]=b'\xff'*512
 put(0x95d1a8,0);put(0x95d378,0xc0dfbe41 if difficulty==0 else 0xdfbe41)
 for a in [0x95d1b4,0x95d1b8]:put(a,0)
 for a,key in [(0x95d584,'fight'),(0x95d594,'tech'),(0x95d58c,'power'),(0x95d590,'growth'),(0x95d588,'wealth')]:put(a,definition[key])
 put(0x95d610,0)
 for i in range(8):
  put(0x95d6d4+4*i,0);text=(channel_prefix+channel_separator+str(i+1)).encode('cp1252')+b'\0';assert len(text)<=24
  off=0x95d614-BASE+24*i;raw[off:off+len(text)]=text
 for i in range(4):put(0x95d3e0+4*i,0);put(0x95d3d0+4*i,0)
 zero(0x95d3f4,11)
 for a in [0x95d3f0,0x95f200,0x95f204,0x95f208,0x95f20c,0x95f210]:put(a,0)
 put(0x95d1b0,difficulty if human_mask&(1<<faction) else 3)
 for a in [0x95d2cc,0x95d2d0,0x95d374,0x95d3c8,0x95d37c,0x95d380,0x95d608,0x95d268]:put(a,0)
 put(0x95d5a4,-999)
 if secondary&0x800000:
  for a,pc,span,minus in [(0x95d584,'0x5c8c54',3,1),(0x95d594,'0x5c8c68',2,0),(0x95d58c,'0x5c8c7f',2,0),(0x95d590,'0x5c8c96',2,0),(0x95d588,'0x5c8cad',2,0)]:put(a,draw(pc)%span-minus)
 if rules&0x40:put(0x95d584,1)
 for a in [0x95e1fc,0x95e200,0x95e204,0x95e208,0x95e20c,0x95e210,0x95d384]:put(a,0)
 for i in range(8):put(0x95d388+4*i,0);put(0x95d3a8+4*i,0)
 put(0x95d580,0)
 for i in range(128):
  put(0x95e220+2*i,0,2);put(0x95e320+i,0,1);put(0x95e3a0+i,0,1);put(0x95e420+2*i,0,2);put(0x95e520+2*i,0,2)
 for i in range(11):put(0x95d5a8+4*i,0);put(0x95d5d4+4*i,0)
 zero(0x95f214,10)
 for a in [0x95f1e0,0x95f1e4,0x95f1e8,0x95f1ec,0x95f1f0,0x95f1f4]:put(a,0)
 put(0x95f1f8,1)
 put(0x95d3c8,get(0x95d3c8)+definition.get('energy',0)+(100 if network else starting_energy));put(0x95d3cc,0);put(0x95d578,-1);put(0x95d1bc,0)
 for i in range(8):
  a=0x95d240+4*i;put(a-0x80,0);put(a,0);span=3*(5 if rules&0x40 else difficulty)+8;v=draw('0x5c8e3f')%span if span>1 else 0;put(a-0x40,max(1,min(16,v)))
  for address in [a+0x94,a+0xb4,a+0xd4,a+0x114,a+0xf4]:put(address,0)
  put(0x95d260+i,0,1);put(a-0x20,-1)
  for address in [a+0x6c,a+0x2c,a+0x4c]:put(address,0)
 zero(0x95d9fc,512,1);zero(0x95dbfc,512,1);zero(0x95ddfc,512,2)
 zero(0x95d4d4,12);zero(0x95d514,22);put(0x95d56c,0,1)
 for a in [0x95d600,0x95d9ac,0x95d9b0,0x95d9b4,0x95d9b8,0x95d510,0x95d504,0x95d50c,0x95d570,0x95d57c,0x95d508,0x95f220]:put(a,0)
 put(0x95d604,0,1);put(0x95d574,-1)
 for a in [0x95d5a0,0x95d4a4,0x95d4a8,0x95d4b8,0x95d4b4,0x95d4ac,0x95d4bc,0x95d4c0,0x95d4c4,0x95d4c8,0x95d4b0,0x95d9bc,0x95d9c8,0x95d9cc,0x95d9c0,0x95d9c4,0x95d9d0,0x95d9d4,0x95d9d8,0x95d9dc,0x95d9e0]:put(a,0)
 searching=faction!=0 and bool(active_mask&(1<<faction)) and mode>=-1
 if faction and active_mask&(1<<faction) and mode< -1:active_mask&=~(1<<faction)
 return dict(record=raw,scratch_slots=scratch,crt=crt,rng=draws,active_mask=active_mask,searching=searching,writes=writes)
