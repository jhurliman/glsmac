"""Portable native opaque/keyed3/4-vertex polygon span reference, bounded domain."""
from fractions import Fraction

def truncdiv(n,d):
    if d==0:raise ZeroDivisionError
    return (1 if (n<0)==(d<0) else -1)*(abs(n)//abs(d))

def x87_chop64(value):
    """Round a finite rational to x87's64-bit significand, toward zero."""
    value=Fraction(value)
    if not value:return value
    sign=-1 if value<0 else 1;value=abs(value)
    e=value.numerator.bit_length()-value.denominator.bit_length()
    if value < (Fraction(2)**e):e-=1
    unit=Fraction(2)**(e-63)
    return sign*(value//unit)*unit

def horizontal_step(delta,width,scale=65536):
    reciprocal=x87_chop64(Fraction(1,width<<16))
    product=x87_chop64(Fraction(delta)*reciprocal)
    product=x87_chop64(product*scale)
    return truncdiv(product.numerator,product.denominator)

def generate_polygon_spans(vertices,uv,lights=None,clip=(0,0,1024,511),triangle=0,shade_limits=(-4,4),shaded=True):
    """Return [triangle,y,left,right,u,v,du,dv,shade8.8,dshade8.8].

    Coordinates and intermediate fixed-point products must fit signed int32.
    Deliberately preserves input winding and first-index tie selection.
    """
    n=len(vertices)
    if not shaded:lights=[0]*n
    bias=32768 if shaded else 0
    if n not in [3,4] or len(uv)!=n or lights is None or len(lights)!=n:raise ValueError('Matching3/4-vertex polygon inputs required')
    if clip[0]>=clip[2] or clip[1]>=clip[3]:return []
    if any(type(k) is not int for p in [*vertices,*uv] for k in p) or any(type(k) is not int for k in lights):raise ValueError('Integer inputs required')
    if any(not -32765<k<32767 for p in vertices for k in p):raise ValueError('Outside bounded coordinate domain')
    if any(max(p[j] for p in vertices)-min(p[j] for p in vertices)>=32768 for j in [0,1]):raise ValueError('Coordinate extent overflows signed16.16')
    if any(not -8192<=k<8192 for p in uv for k in p):raise ValueError('Outside bounded UV domain')
    light=[max(shade_limits[0],min(shade_limits[1],v)) for v in lights]
    low=min(range(n),key=lambda i:vertices[i][1]);high=max(range(n),key=lambda i:vertices[i][1])
    y0=vertices[low][1];y1=vertices[high][1]
    if y0==y1:return []
    def edge(start,direction):
        if start==high:return None
        end=(start+direction)%n
        while vertices[end][1]==vertices[start][1]:
            start=end
            if start==high:return None
            end=(start+direction)%n
        dy=vertices[end][1]-vertices[start][1]
        if dy<=0:return None
        dx=vertices[end][0]-vertices[start][0]
        u=(uv[start][0]<<16)+bias;v=(uv[start][1]<<16)+bias;s=light[start]<<16
        return dict(end=end,direction=direction,remaining=dy,dy=dy,x=vertices[start][0],q=truncdiv(dx,dy),rem=abs(dx)%dy,err=1-dy if dx<0 else 0,sign=-1 if dx<0 else 1,u=u,v=v,s=s,du=truncdiv((uv[end][0]<<16)-u,dy),dv=truncdiv((uv[end][1]<<16)-v,dy),ds=truncdiv((light[end]<<16)-s,dy))
    def advance(e):
        e=e.copy();e['remaining']-=1
        if not e['remaining']:return edge(e['end'],e['direction'])
        for key,step in [('x','q'),('u','du'),('v','dv'),('s','ds')]:e[key]+=e[step]
        e['err']+=e['rem']
        if e['err']>0:e['x']+=e['sign'];e['err']-=e['dy']
        return e
    left=edge(low,-1);right=edge(low,1);result=[]
    for y in range(y0,min(y1,clip[3])):
        if left is None or right is None:break
        l,r=left,right;left,right=advance(left),advance(right)
        width=r['x']-l['x']
        if width<=0 or y<clip[1] or r['x']<=clip[0] or l['x']>=clip[2]:continue
        du=horizontal_step(r['u']-l['u'],width);dv=horizontal_step(r['v']-l['v'],width)
        ds=horizontal_step(r['s']-l['s'],width,256)
        # Masked-invalid x87 FISTP word stores its signed indefinite0x8000.
        ds=(ds if -32768<=ds<=32767 else -32768)&65535
        x0=max(l['x'],clip[0]);x1=min(r['x'],clip[2]);skip=x0-l['x']
        u=l['u']+skip*du;v=l['v']+skip*dv;phase=((l['s']>>8)+skip*ds)&65535
        result.append([triangle,y,x0,x1,u,v,du,dv,phase,ds])
    return result


def render_frame(frame, texture_loader, width=1024, height=511, background=250):
    """Generate coverage from vertices, then sample in submission order.

    texture_loader(draw) returns loaded indexed crop bytes; no captured spans
    are consumed. Return final indexed pixels and per-draw verification data.
    """
    import hashlib
    import struct
    output=bytearray([background])*(width*height)
    results=[]
    for draw in frame['draws']:
        shaded=draw['mode']!='keyed_unlit'
        spans=generate_polygon_spans(list(zip(draw['vertices'][::2],draw['vertices'][1::2])),
            list(zip(draw['uv'][::2],draw['uv'][1::2])),draw['lights'],draw['clip'],draw['index'],shaded=shaded)
        texture=texture_loader(draw)
        assert hashlib.sha256(texture).hexdigest()==draw['loaded_texture_sha256']
        attempts=bytearray();writes=bytearray()
        crop_width=draw['source_crop'][2]
        for _,y,x0,x1,u,v,du,dv,phase,ds in spans:
            for x in range(x0,x1):
                offset=(v>>16)*crop_width+(u>>16)
                if not 0<=offset<len(texture):raise ValueError('Outside bounded loaded crop')
                raw=texture[offset]
                shade=(phase>>8);shade=shade if shade<128 else shade-256
                will=draw['mode']=='opaque_shaded' or raw!=9
                attempts.extend(struct.pack('<HHIhBB',x,y,offset,shade,raw,will))
                if will:
                    result=(raw+shade)&255 if shaded else raw
                    output[y*width+x]=result
                    writes.extend(struct.pack('<HHB',x,y,result))
                u+=du;v+=dv;phase=(phase+ds)&65535
        results.append(dict(spans=spans,attempt_sha256=hashlib.sha256(attempts).hexdigest(),write_sha256=hashlib.sha256(writes).hexdigest(),attempt_count=len(attempts)//12,write_count=len(writes)//5))
    return bytes(output),results


def verify(cases,texture_loader):
    import hashlib
    totals=[0,0,0]
    for frame in cases['frames']:
        output,results=render_frame(frame,texture_loader,*cases['viewport'],cases['background'])
        for draw,result in zip(frame['draws'],results):
            for key,native in [('spans','native_spans'),('attempt_sha256','attempt_sha256'),('write_sha256','write_sha256'),('attempt_count','attempt_count'),('write_count','write_count')]:
                assert result[key]==draw[native],(frame['id'],draw['index'],key,result[key] if key=='spans' else result[key],draw[native])
            totals[0]+=len(result['spans']);totals[1]+=result['attempt_count'];totals[2]+=result['write_count']
        assert hashlib.sha256(output).hexdigest()==frame['frame_sha256'],frame['id']
    return totals


if __name__=='__main__':
    import argparse,json
    from pathlib import Path
    from terrain_triangle_raster_reference import pcx_crop
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--texture-pcx',required=True)
    parser.add_argument('--cases',default=str(Path(__file__).with_name('ordinary-land-keyed-raster-cases.json')))
    args=parser.parse_args()
    def load(draw):
        if draw['asset']=='synthetic_key_pattern':return bytes([9,10,255,1][(x+y)%4] for y in range(56) for x in range(56))
        return pcx_crop(args.texture_pcx,draw['source_crop'])
    print('PASS spans/attempts/writes:',verify(json.loads(Path(args.cases).read_text()),load))
