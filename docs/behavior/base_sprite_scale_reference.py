"""Bounded native retained-sprite downscale,12/16 and8/16, indexed pixels."""
def source_steps(numerator,denominator,count):
    if (numerator,denominator) not in [(12,16),(8,16)]:
        raise ValueError('Only measured Base-card downscale ratios supported')
    high,low=max(numerator,denominator),min(numerator,denominator)
    quotient,remainder=divmod(high,low)
    error=remainder-2*low+(high if quotient&1 else 0)
    steps=[]
    for _ in range(count):
        error+=2*remainder
        step=quotient
        if error>0:
            step+=1;error-=2*low
        steps.append(step)
    return steps


def retained_copy(source,width,height,offset,anchor,ratio,canvas,canvas_size,clip,key=9,zero_lookup=None):
    """Source is tightly packed retained bytes, with logical-frame offsets.

    The caller supplies initialized indexed canvas and optional256-byte remap
    for source index0. Return pixels, unclipped destination and sampled coords.
    Bounds here deliberately restrict the supported positive-anchor profile.
    """
    n,d=ratio
    if min(*anchor,*offset)<0:raise ValueError('Negative placement outside measured profile')
    steps=source_steps(n,d,800)
    left=anchor[0]+offset[0]*n//d;top=anchor[1]+offset[1]*n//d
    w=width*n//d;h=height*n//d
    right,bottom=left+w,top+h
    x0=max(left,clip[0]);x1=min(right,clip[2]);y0=max(top,clip[1]);y1=min(bottom,clip[3])
    output=bytearray(canvas);attempts=[]
    if x0>=x1 or y0>=y1:return bytes(output),[left,top,right,bottom],attempts
    if w>800 or h>800:raise ValueError('Native step table extent exceeded')
    sx0=sum(steps[:x0-left]);sy=sum(steps[:y0-top])
    for y in range(y0,y1):
        sx=sx0
        for x in range(x0,x1):
            if not (0<=sx<width and 0<=sy<height):raise ValueError('Sample outside retained crop')
            raw=source[sy*width+sx];attempts.append((x,y,sx,sy,raw))
            if raw!=key:
                pos=y*canvas_size[0]+x
                if raw==0:
                    if zero_lookup is None:raise ValueError('Source0 requires destination-index remap')
                    output[pos]=zero_lookup[output[pos]]
                else:output[pos]=raw
            sx+=steps[x-left]
        sy+=steps[y-top]
    return bytes(output),[left,top,right,bottom],attempts


def verify(cases,pcx_path):
    import hashlib,struct
    from terrain_triangle_raster_reference import pcx_crop
    source_spec=cases['source'];full=pcx_crop(pcx_path,source_spec['crop'])
    ox,oy=source_spec['retained_offset'];w,h=source_spec['retained_size'];logical_width=source_spec['crop'][2]
    source=b''.join(full[(oy+y)*logical_width+ox:(oy+y)*logical_width+ox+w] for y in range(h))
    assert hashlib.sha256(source).hexdigest()==source_spec['sha256']
    remap=bytes([source_spec['zero_lookup_uniform_value']])*256
    assert hashlib.sha256(remap).hexdigest()==source_spec['zero_lookup_sha256']
    size=cases['canvas'];total=0
    for case in cases['cases']:
        before=bytes([cases['background']])*(size[0]*size[1])
        if case['id'].startswith('roster'):
            before,_,_=retained_copy(source,w,h,[ox,oy],[34,66],[12,16],before,size,[0,0,*size],zero_lookup=remap)
            before=bytearray(before)
            # Existing selected-card population border precedes roster body.
            for x in range(34,48):before[66*size[0]+x]=0;before[81*size[0]+x]=0
            for y in range(66,82):before[y*size[0]+34]=0;before[y*size[0]+47]=0
        result,rect,attempts=retained_copy(source,w,h,[ox,oy],case['anchor'],case['ratio'],before,size,case['clip'],zero_lookup=remap)
        assert rect==case['destination']
        assert len(attempts)==case['attempts']
        packed=b''.join(struct.pack('<HHHHB',*a) for a in attempts)
        assert hashlib.sha256(packed).hexdigest()==case['attempt_sha256'],case['id']
        assert hashlib.sha256(result).hexdigest()==case['frame_sha256'],case['id']
        total+=len(attempts)
    return total


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--faction-pcx',required=True)
    parser.add_argument('--cases',default=str(Path(__file__).with_name('base-sprite-scale-cases.json')))
    args=parser.parse_args()
    print('PASS',verify(json.loads(Path(args.cases).read_text()),args.faction_pcx),'native samples and complete frames')
