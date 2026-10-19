"""Portable original known-land vertex-to-span reference (bounded int32 domain)."""
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

def generate_spans(vertices,uv,lights,clip=(0,0,1024,511),triangle=0,shade_limits=(-256,256)):
    """Return [triangle,y,left,right,u,v,du,dv,shade8.8,dshade8.8].

    Coordinates and intermediate fixed-point products must fit signed int32.
    Deliberately preserves input winding and first-index tie selection.
    """
    n=len(vertices)
    if n!=3 or len(uv)!=n or len(lights)!=n:raise ValueError('Triangle inputs required')
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
        u=(uv[start][0]<<16)+32768;v=(uv[start][1]<<16)+32768;s=light[start]<<16
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

if __name__=='__main__':
    import argparse,json,runpy,copy
    from pathlib import Path
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--texture-pcx');args=parser.parse_args()
    d=json.loads(Path(__file__).with_name('ordinary-land-triangle-raster-cases.json').read_text())
    count=0
    for c in d['cases']:
        got=[]
        for j,t in enumerate(c['triangles']):
            v=list(zip(t['vertices'][::2],t['vertices'][1::2]));uv=list(zip(t['uv'][::2],t['uv'][1::2]))
            got.extend(generate_spans(v,uv,t['shade'],triangle=j))
        assert got==c['spans'],(c['id'],next(((a,b) for a,b in zip(got,c['spans']) if a!=b),None),len(got),len(c['spans']))
        c['spans']=got
        count+=len(got)
    print('PASS',count,'native spans from vertices')
    extra=Path(__file__).with_name('ordinary-land-triangle-span-cases.json')
    if extra.exists():
        edgecases=json.loads(extra.read_text())['cases'];edgecount=0
        for c in edgecases:
            got=generate_spans(c['vertices'],c['uv'],c['lights'],c['clip'])
            assert got==c['spans'],c['id'];edgecount+=len(got)
        assert horizontal_step(926882,11)==84261
        print('PASS',len(edgecases),'edge cases,',edgecount,'native spans')
    if args.texture_pcx:
        raster=runpy.run_path(str(Path(__file__).with_name('terrain_triangle_raster_reference.py')))
        tex=raster['pcx_crop'](args.texture_pcx,d['texture']['crop'])
        print('PASS',raster['verify'](tex,d),'native pixels using generated spans')
