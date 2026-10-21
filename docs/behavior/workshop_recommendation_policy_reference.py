"""597390 preview modes 1..3 for conventional human Workshop proposals.

This models the controller's submission boundary, not proposal generation.
No record creation, replacement scoring or mode-zero AI behavior is performed.
Existing excluded/unused own records are allocation candidates before matching.
"""
import struct
from startup_technology_reference import has_technology


def submit(designs, ownership, prerequisites, owner, job, category, state,
           unit_counts, production_counts):
    raw=bytearray(designs);out=dict(state);mode=out['mode']
    assert mode in (1,2,3) and 1<=owner<=7
    if out['filter']>=0 and out['filter']!=category:return raw,out,-1
    available=False
    for i in [*range(64),*range(owner*64,(owner+1)*64)]:
        a=i*52;flags=struct.unpack_from('<H',raw,a+48)[0];own=i>=64
        obsolete=bool(raw[a+44]&(1<<owner));units=unit_counts[i];production=production_counts[i]
        if own and flags&1 and obsolete and not units and not production:
            available=True;continue
        if not flags&1:
            available|=own;continue
        if not own and not has_technology(owner,struct.unpack_from('<h',raw,a+50)[0],ownership,prerequisites):continue
        mask,ch,we,ar,re=struct.unpack_from('<I4B',raw,a+32)
        matches=(ch,we,ar,re)==tuple(job[k] for k in ['chassis','weapon','armor','reactor']) and not ((mask^job['mask'])&0x44ae1b) and mask&job['mask']==mask
        if matches:
            if mode<=2:out.update(mode=2,fields=dict(job))
            raw[a+44]&=~(1<<owner)&255
            return raw,out,i
        if own:
            # Preview only asks whether some positive replacement score exists.
            # These gates are sufficient/necessary in the admitted conventional
            # human path: counts penalty is clamped63, minimum base score64.
            available|=(obsolete and not units) or raw[a+43]==category or (not units and not production)
    if available and mode<=2:out.update(mode=3,filter=category,fields=dict(job))
    return raw,out,-1
