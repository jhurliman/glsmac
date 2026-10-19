"""Native software indexed line coverage, valid nonempty bitmap clips."""


def line_points(line,clip):
    l,t,r,b=clip
    if r<=l or b<=t:raise ValueError('Empty clips are outside the native safe line profile')
    x0,y0,x1,y1=line
    if (x0,y0)==(x1,y1):return []
    if x0==x1:points=((x0,y) for y in range(min(y0,y1),max(y0,y1)+1))
    elif y0==y1:points=((x,y0) for x in range(min(x0,x1),max(x0,x1)+1))
    else:
        if x0>x1:x0,y0,x1,y1=x1,y1,x0,y0
        dx=x1-x0;dy=abs(y1-y0);sy=1 if y1>y0 else -1;x=x0;y=y0;result=[]
        if dx>=dy:
            error=2*dy-dx
            for _ in range(dx+1):
                result.append((x,y))
                if error>0:y+=sy;error+=2*(dy-dx)
                else:error+=2*dy
                x+=1
        else:
            error=2*dx-dy
            for _ in range(dy+1):
                result.append((x,y))
                if error>0:x+=1;error+=2*(dx-dy)
                else:error+=2*dx
                y+=sy
        points=result
    return [[x,y] for x,y in points if l<=x<r and t<=y<b]


def verify(cases):
    import hashlib,struct
    w,h,pitch=cases['width'],cases['height'],cases['pitch']
    initial=b''.join(bytes([cases['background']])*w+bytes([cases['padding']])*(pitch-w) for _ in range(h))
    stores=0;normal=0;empty=0
    for c in cases['cases']:
        spec=c['input'];l,t,r,b=spec['clip']
        if r<=l or b<=t:points=[];empty+=1
        else:points=[] if spec.get('no_backing') else line_points(spec['line'],spec['clip']);normal+=1
        expected=[[x,y,spec['color']&255] for x,y in points]
        assert expected==c['writes'],(spec['id'],expected,c['writes'])
        raw=bytearray(initial)
        for x,y,color in expected:raw[y*pitch+x]=color
        packed=b''.join(struct.pack('<HHB',*p) for p in expected)
        assert hashlib.sha256(raw).hexdigest()==c['frame_sha256'],spec['id']
        assert hashlib.sha256(packed).hexdigest()==c['store_sha256'],spec['id']
        assert len(expected)==c['store_count'],spec['id']
        stores+=len(expected)
    return normal,empty,len(cases['hazards']),stores


if __name__=='__main__':
    import json
    from pathlib import Path
    cases=json.loads(Path(__file__).with_name('indexed-line-cases.json').read_text())
    print('PASS nonempty cases, empty diagnostics, bounded hazards, native stores:',verify(cases))
