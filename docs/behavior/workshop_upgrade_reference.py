"""Original human Upgrade7 candidate/quote/conversion rules, resolved inputs."""
import struct

def _u16(b,o):return struct.unpack_from('<H',b,o)[0]
def _i32(b,o):return struct.unpack_from('<i',b,o)[0]
def _put32(b,o,v):struct.pack_into('<i',b,o,v)
def _s8(v):return v-256 if v>=128 else v

def candidates(designs,owner,source,weapon_strengths,armor_strengths):
    old=designs[source*52:source*52+52];out=[]
    for i in range(owner*64,(owner+1)*64):
        new=designs[i*52:i*52+52]
        if i==source or not new[48]&1 or new[44]&(1<<owner):continue
        if new[36]!=old[36] or (_i32(new,32)^_i32(old,32))&0x8000:continue
        if new[42]!=old[42] and (old[42]>3 or new[42]>3):continue
        ow,nw=weapon_strengths[old[37]],weapon_strengths[new[37]]
        if (ow!=0 and ow>nw) or (ow==0 and old[37]!=new[37]):continue
        oa,na=armor_strengths[old[38]],armor_strengths[new[38]]
        if oa>na and (na==1 or (ow!=0 and nw<=ow)):continue
        out.append(i)
    return out

def quote_rows(designs,target,source,weapon_costs,armor_costs,chassis_speeds,half=False):
    old=designs[source*52:source*52+52];new=designs[target*52:target*52+52]
    base=new[41]
    if target>=64 and not new[48]&4:
        base += (base+1)//2
        base *= base+1
    result=(base+max(0,weapon_costs[new[37]]-weapon_costs[old[37]])+
        max(0,armor_costs[new[38]]-armor_costs[old[38]])+
        max(0,chassis_speeds[new[36]]-chassis_speeds[old[36]]))
    return result//2 if half else result

def convert(designs,factions,bases,units,history,*,owner,target,source,unit_count,
            base_count,weapon_costs,armor_costs,chassis_speeds,half=False):
    """Predict all supplied buffers for native504cc0; no UI or receiver layer."""
    d,f,b,u,h=map(bytearray,[designs,factions,bases,units,history])
    stride=0x20bc;fo=owner*stride
    source_count=fo+0x854+source;target_count=fo+0x854+target
    charge=quote_rows(d,target,source,weapon_costs,armor_costs,chassis_speeds,half)*10*f[source_count]
    _put32(f,fo+0x220,_i32(f,fo+0x220)-charge)
    old_high=bool(_i32(d,source*52+32)&0x4000)
    new_high=bool(_i32(d,target*52+32)&0x4000)
    for i in range(unit_count):
        a=i*52
        if u[a+14]!=owner or struct.unpack_from('<h',u,a+10)[0]!=source:continue
        f[source_count]=(f[source_count]-1)&255;f[target_count]=(f[target_count]+1)&255
        struct.pack_into('<h',u,a+10,target)
        if new_high and not old_high:u[a+36]=max(0,min(6,u[a+36]+1))
        if old_high and not new_high:u[a+36]=max(0,min(6,u[a+36]-1))
    for i in range(base_count):
        a=i*304
        if b[a+4]!=owner:continue
        count=_i32(b,a+0x4c)
        for j in range(count+1):
            o=a+0x50+4*j
            if _i32(b,o)==source:_put32(b,o,target)
    if target>=64 and charge:d[target*52+48]|=4
    if source>=64:d[source*52+48]&=254
    return dict(designs=d,factions=f,bases=b,units=u,history=h),charge
