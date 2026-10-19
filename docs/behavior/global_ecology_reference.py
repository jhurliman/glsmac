"""Independent global forest/kelp spread and bounded empty-tile wildlife rules.

No assets/instructions included. Wildlife occupied-tile ambush and sea brood
phases are explicitly unsupported here; callers must implement those separately.
"""
import struct

NEIGHBORS = [(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]

def crt(seed):
    seed = (214013 * seed + 2531011) & 0xffffffff
    return seed, (seed >> 16) & 32767

def sample(seed, width, height):
    assert width > 1 and height > 1 and width % 2 == 0
    seed, x = crt(seed); seed, y = crt(seed)
    x %= width; y %= height
    return seed, (x + (y & 1) - (x & 1), y)

def at(width, x, y):
    return 44 * (y * (width // 2) + x // 2)

def word(data, offset):
    return struct.unpack_from('<I', data, offset)[0]

def putword(data, offset, value):
    struct.pack_into('<I', data, offset, value & 0xffffffff)

def spread(raw_map, *, width, height, tile_count, turn, seed,
           flat=False, observer_status=None):
    assert turn >= 0 and tile_count >= 0
    data = bytearray(raw_map); status = observer_status or {}
    attempts = tile_count // (32 + turn // 4)
    samples = []; changes = []
    for iteration in range(attempts):
        seed, (x, y) = sample(seed, width, height); samples.append([x,y])
        source = at(width,x,y); sea = (data[source] >> 5) < 3
        feature = 0x8000 if sea else 0x200000
        if not word(data,source+8) & feature:
            continue
        best = 0; selected = None
        for dx,dy in NEIGHBORS:
            xx = x+dx; yy = y+dy
            if not flat: xx %= width
            if not (0 <= xx < width and 0 <= yy < height): continue
            target = at(width,xx,yy); climate = data[target]
            band = climate >> 5; rain = (climate >> 3) & 3
            if (band < 3) != sea or band < 2 or (band > 4 and rain == 0): continue
            landmarks = word(data,target+12)
            if landmarks & 0x80000002 == 2 or landmarks & 0x80000040 == 0x40: continue
            rock = data[target+5] >> 6
            if rock > 1 or word(data,target+8) & 0x208051: continue
            score = 4 * (3-rain) - rock
            if score > best: best = score; selected = (xx,yy,target)
        if selected is None: continue
        xx,yy,target = selected
        if word(data,target+8) & feature: continue
        value = (word(data,target+8) | feature) & ~0x20
        putword(data,target+8,value)
        changes.append(dict(iteration=iteration,source=[x,y],target=[xx,yy],feature=feature,score=best,value=value))
        for owner in range(1,8):
            if not (status.get(owner,0) & 0x200 or data[source+4] & (1<<owner)): continue
            if not word(data,source+12+4*owner) & feature: continue
            putword(data,target+12+4*owner,(word(data,target+12+4*owner)|feature)&~0x20)
    return dict(map=data,seed=seed,attempts=attempts,samples=samples,changes=changes)

def distance(x,y,bx,by,width,flat=False):
    dx=abs(x-bx);dy=abs(y-by)
    if not flat and dx > width//2: dx=abs(width-dx)
    return max(dx,dy)-((abs(dx-dy)//2+1)//2)

def empty_land_wildlife(raw_map, *, width, height, turn, life, rules,
                        Voice_marker, unit_count, seed, bases, site_present,
                        flat=False):
    assert 0 <= life <= 2
    if rules & 0x200000 or Voice_marker != -1 or unit_count >= 0x700:
        return dict(seed=seed,samples=[],spawns=[])
    if turn % (2*(5-life)) == 0:
        raise NotImplementedError('Sea brood producer precedes land sampling on this turn')
    data=bytearray(raw_map);samples=[];spawns=[]
    for iteration in range(2*life+2):
        seed,(x,y)=sample(seed,width,height);samples.append([x,y]);a=at(width,x,y)
        flags=word(data,a+8);region=data[a+3]
        if data[a]>>5 < 3 or not flags&0x20 or region&0x3f in [0,63]:continue
        owner=data[a+2]&15
        if flags&1 and owner<8:continue
        best=9999;selected=None
        for base in bases:
            bx,by=base['position']
            if data[at(width,bx,by)+3] != region:continue
            d=distance(x,y,bx,by,width,flat)
            if d<=best:best=d;selected=base['id']
        if selected is None:
            raise NotImplementedError('No-Base branch preserves prior native distance output')
        if flags&2 and owner<8:
            if owner==0 or best<=2:continue
            raise NotImplementedError('Occupied positive-owner ambush branch')
        if not 1<best<6 or site_present(x,y,data[a:a+44]):continue
        spawns.append(dict(design=8,owner=0,position=[x,y],nearest_Base=selected,distance=best))
        putword(data,a+8,flags|2);data[a+2]&=0xf0
    return dict(seed=seed,samples=samples,spawns=spawns)
