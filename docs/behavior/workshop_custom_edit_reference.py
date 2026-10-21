"""Independent bounded Workshop allocation/alias/toggle reference.

The builder portion intentionally accepts only the two no-ability Hand Weapons
Infantry/Fission armor edits exercised by this receipt. UI/retirement side
effects are separately retained as complete native record deltas.
"""
import struct
from startup_technology_reference import has_technology

def flags(raw,i):return struct.unpack_from('<H',raw,i*52+48)[0]
def text(raw,i):return bytes(raw[i*52:i*52+32]).split(b'\0',1)[0].decode('cp1252')
def auto_name(tables,owner,armor):
    variant=(owner&1)^(int(tables['DEFENSES'][armor][2])&1)
    return tables['DEFENSES'][armor][1]+' '+tables['CHASSIS'][0][6 if variant else 4]
def tuple_at(raw,i):return bytes(raw[i*52+32:i*52+40])
def draft_tuple(state):
    d=state['draft'];return struct.pack('<I4B',d['0x14208'],d['0x141fc'],d['0x14200'],d['0x14204'],d['0x1420c'])
def _tech_ok(raw,i,owner,technology):
    return has_technology(owner,struct.unpack_from('<h',raw,i*52+50)[0],bytes.fromhex(technology['ownership_hex']),technology['prerequisites'])
def decision(designs,factions,state,technology,*,answer=1):
    raw=bytearray(designs);owner=state['draft']['0x141f4'];selected=state['selected'];wanted=draft_tuple(state);name=state['draft_name'];mask=1<<owner
    def unit_count(i):return factions[owner*0x20bc+(0x95d9fc-0x95d1a8)+i]
    def production_count(i):return factions[owner*0x20bc+(0x95dbfc-0x95d1a8)+i]
    for i in [*range(64),*range(owner*64,(owner+1)*64)]:
        if flags(raw,i)&1 and tuple_at(raw,i)==wanted and (i>=64 or _tech_ok(raw,i,owner,technology)):
            if i!=selected:
                if raw[i*52+44]&mask:struct.pack_into('<H',raw,i*52+48,flags(raw,i)|8)
                raw[i*52+44]&=~mask
            if i>=64 and name and flags(raw,selected)&2:
                struct.pack_into('<H',raw,i*52+48,flags(raw,i)|2);label=name.encode('cp1252')+b'\0';raw[i*52:i*52+len(label)]=label
            return dict(selected=i,result=1,kind='alias',retire=False,confirmation=False,designs=raw)
    provisional=selected
    if selected<64:
        if tuple_at(raw,selected)==wanted:return dict(selected=selected,result=0,kind='intrinsic',retire=False,confirmation=False,designs=raw)
        for i in range(owner*64,(owner+1)*64):
            if raw[i*52+44]&mask and unit_count(i)==production_count(i)==0:provisional=i;break
            if not flags(raw,i)&1:provisional=i
        if provisional<64:return dict(selected=provisional,result=0,kind='intrinsic',retire=False,confirmation=False,designs=raw)
    selected=provisional;best=False;empty=False
    if flags(raw,selected)&1 and tuple_at(raw,selected)!=wanted:
        for i in range(owner*64,(owner+1)*64):
            if not flags(raw,i)&1:selected=i;empty=True;break
            if raw[i*52+44]&mask and unit_count(i)==0:
                if production_count(i)==0:selected=i;best=True
                elif not best and raw[i*52+43]==raw[selected*52+43]:selected=i
    confirmation=not empty and not best and bool(flags(raw,selected)&1) and unit_count(selected)>0
    return dict(selected=selected,result=0 if confirmation and not answer else 1,kind='build',retire=confirmation and bool(answer),confirmation=confirmation,provisional=provisional,designs=raw)

def build_bounded(raw,index,state,tables,owner=1):
    raw=bytearray(raw);d=state['draft'];assert (d['0x141fc'],d['0x14200'],d['0x1420c'],d['0x14208'])==(0,0,1,0)
    armor=d['0x14204'];assert armor in [1,2];a=index*52
    raw[a+32:a+40]=struct.pack('<I4B',0,0,0,armor,1)
    for off,value in [(40,0),(41,2),(42,2),(43,2),(44,0),(45,1<<owner),(46,255)]:raw[a+off]=value
    name=state['draft_name'] or auto_name(tables,owner,armor)
    struct.pack_into('<H',raw,a+48,1|(2 if state['draft_name'] and name!=auto_name(tables,owner,armor) else 0))
    label=name.encode('cp1252')+b'\0';raw[a:a+len(label)]=label
    return raw

def toggle(raw,index,owner=1):
    raw=bytearray(raw);a=index*52;f=flags(raw,index);mask=1<<owner
    if raw[a+44]&mask:f|=8|(0x200 if f&8 else 0)
    else:f&=0xfdf7
    struct.pack_into('<H',raw,a+48,f);raw[a+44]^=mask
    return raw

def browse_list(raw,factions,technology,tables,owner=1):
    rows=[];mask=1<<owner
    for i in [*range(64),*range(owner*64,(owner+1)*64)]:
        active=bool(flags(raw,i)&1)
        if i<64 and (not active or int(tables['WEAPONS'][raw[i*52+37]][2])<0 or not _tech_ok(raw,i,owner,technology)):continue
        key=i
        if not active:key+=0x400
        elif raw[i*52+44]&mask:
            key+=0x2000 if i<64 else 0x800 if factions[owner*0x20bc+(0x95d9fc-0x95d1a8)+i] else 0x1000
        rows.append((key,i))
    return [i for _,i in sorted(rows)]
