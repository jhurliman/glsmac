"""Flat orthographic Scout shadow camera from scene/light/ground inputs.

No measured projection matrix, translation, mask or model pixels are inputs.
Arithmetic rounds x87 intermediates to64 significant bits and binary32 stores
to24; host sin/cos covers the checked flat-camera/facing Euler values.
"""
from fractions import Fraction as F
import math,struct

def rounded(v,p=64):
    v=F(v)
    if not v:return v
    sign=-1 if v<0 else 1;v=abs(v);e=v.numerator.bit_length()-v.denominator.bit_length()
    if v<F(2)**e:e-=1
    step=F(2)**(e-p+1);q=v/step;n,r=divmod(q.numerator,q.denominator)
    if 2*r>q.denominator or (2*r==q.denominator and n&1):n+=1
    return sign*n*step

def r32(v):return float(rounded(v,24))
def mul(a,b):return rounded(F(a)*F(b))
def add(a,b):return rounded(F(a)+F(b))
def sub(a,b):return rounded(F(a)-F(b))
def mv(m,v):return [r32(add(add(mul(m[k+1],v[1]),mul(m[k+2],v[2])),mul(m[k],v[0]))) for k in [0,3,6]]
def mm(a,b):return [r32(add(add(mul(a[r*3+1],b[3+c]),mul(a[r*3+2],b[6+c])),mul(a[r*3],b[c]))) for r in range(3) for c in range(3)]
def bits(v):return struct.unpack('<f',struct.pack('<I',v))[0]

def flat_inverse(b):
    """641ce0 for the checked symmetric flat helper matrix.

    Native cofactors and reciprocal determinant are stored to binary32 before
    their later products. A transpose or ideal orthonormal inverse differs.
    """
    assert b[:3]==[1,0,0] and b[3]==b[6]==0 and b[5]==b[7] and b[8]==-b[4]
    co,sn=b[4],b[5];q=r32(add(mul(co,co),mul(sn,sn)));iq=r32(rounded(1/F(q)))
    return [r32(mul(q,iq)),0.,0.,0.,r32(mul(co,iq)),r32(mul(sn,iq)),0.,r32(mul(sn,iq)),r32(-mul(co,iq))]

def flat_scout_scene(facing,zoom=0,ratio=(0,1),surface=(256,256)):
    """Existing original flat helper defaults; returns runtime scene inputs."""
    tilt=bits(0xbf85f92d);co=r32(math.cos(tilt));sn=r32(math.sin(tilt))
    b=[1.,0.,0.,0.,co,sn,-0.,sn,-co]
    face=0 if facing==-1 else facing;k=(face-3)&7;phi=r32(mul(k,bits(0x3f490e56)))
    # Native facing-step constant is 0x3f490e56, not pi/4.
    s=r32(math.sin(phi));c=r32(math.cos(phi));orientation=[c,-s,0.,s,c,0.,0.,0.,1.]
    n,d=ratio;base=r32(F(bits(0x3d8f5c29))*n/d) if n>0 else r32(F(bits(0x3d8f5c29))*(zoom+16)/16)
    scale=r32(mul(base,F(5,4)));light=bits(0x3f13cd36)
    return dict(position=[0.,0.,0.],orientation=orientation,object_scale=1.,direction=mv(flat_inverse(b),[-light,light,light]),ground=0.,xy_scale=[1.,1.],offset=[0.,0.],pixel_offset=[0.,0.],aux_position=[r32(mul(v,-10000)) for v in b[6:]],aux_matrix=b,aux_scale=scale,width=surface[0],height=surface[1],perspective=0.)

def produce(scene):
    """Native653fb0 shadow branch, orthographic mode with no perspective factor."""
    assert scene.get('perspective',0)==0
    dx,dy,dz=scene['direction'];epsilon=bits(0x358637bd)
    if -epsilon<dz<0:dz=-epsilon
    elif 0<=dz<epsilon:dz=epsilon
    rx=rounded(F(dx)/F(dz));ry=rounded(F(dy)/F(dz))
    plane=[1.,0.,r32(-rx),0.,1.,r32(-ry),0.,0.,0.]
    p=mv(plane,scene['position']);plane_position=p[:]
    # Ratios are stored to binary32 locals before the ground offset uses them.
    p=[r32(add(p[0],mul(r32(rx),scene['ground']))),r32(add(p[1],mul(r32(ry),scene['ground']))),r32(add(p[2],scene['ground']))]
    delta=[r32(sub(v,a)) for v,a in zip(p,scene['aux_position'])]
    before_translation=mv(scene['aux_matrix'],delta)
    before_matrix=mm(mm(scene['aux_matrix'],plane),scene['orientation'])
    scale=mul(scene['object_scale'],scene['aux_scale']);sx=mul(scale,scene['xy_scale'][0]);sy=mul(scale,scene['xy_scale'][1])
    matrix=[r32(mul(v,sx if j<3 else sy)) if j<6 else v for j,v in enumerate(before_matrix)]
    centers=[scene['width']//2+.5,scene['height']//2+.5]
    translation=[]
    for j,axis_scale in enumerate([sx,sy]):
        scaled=r32(mul(before_translation[j],axis_scale))
        v=mul(scene['offset'][j],scene['xy_scale'][j]);v=add(v,scene['pixel_offset'][j]);v=add(v,centers[j]);v=add(v,scaled)
        translation.append(r32(v))
    translation.append(before_translation[2])
    extents=[r32(math.hypot(matrix[0],matrix[2])),r32(math.hypot(matrix[4],matrix[5]))]
    extents=[max(x,epsilon) for x in extents]
    return dict(plane=plane,plane_position=plane_position,before_translation=before_translation,before_matrix=before_matrix,projection=matrix,translation=translation,extents=extents,direction_z_after_clamp=dz)
