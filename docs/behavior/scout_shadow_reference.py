"""Bounded ungrouped/grouped delta-stream shadow raster; no captured mask input."""
import math,struct

def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def away(v):return math.floor(v+.5) if v>=0 else math.ceil(v-.5)
def mv(m,v):return [f32(sum(m[r*3+k]*v[k] for k in range(3))) for r in range(3)]
def mm(a,b):return [f32(sum(a[r*3+k]*b[k*3+c] for k in range(3))) for r in range(3) for c in range(3)]

def project(frame,projection,translation,scalar):
    m=mm(projection,frame['matrix'])
    # Native shadow wrapper scales XY rows, leaving the unused depth row alone.
    m=[f32(v*scalar) if j<6 else v for j,v in enumerate(m)]
    p=[f32(v+t) for v,t in zip(mv(projection,frame['location']),translation)]
    return m,p

def kernel_inputs(matrix,location,origin):
    p=[f32(v+t) for v,t in zip(mv(matrix,origin),location)]
    start=[away(p[j]*256) for j in range(2)]
    basis=[[away(f32(v*256)) for v in matrix[j*3:j*3+3]] for j in range(2)]
    directions=[(x,y,z) for z in [-1,0,1] for y in [-1,0,1] for x in [-1,0,1] if (x,y,z)!=(0,0,0)]
    # Encoded direction's fast axis multiplies the last matrix column.
    deltas=[tuple(sum(v*b for v,b in zip(reversed(xyz),row)) for row in basis) for xyz in directions]
    return start,deltas

def stamp(plane,matrix,location,origin,stream,count,flags=33,width=256):
    """Measured flags33 path: one pixel, count excludes trailing terminator."""
    assert flags==33 and count>=1 and len(stream)>=(count-1)*3
    (x,y),deltas=kernel_inputs(matrix,location,origin);stores=[]
    for i in range(count-1):
        direction=stream[i*3]>>3
        assert direction<26
        dx,dy=deltas[direction];x+=dx;y+=dy;px=x>>8;py=y>>8
        assert 0<=px<width and 0<=py<len(plane)//width
        plane[py*width+px]=128;stores.append((py*width+px,128))
    return stores

def blend(target,pitch,mask,lookup,origin=(546,210),width=256):
    """Source byte is a key test, never an alpha or lookup index."""
    stores=[]
    for i,value in enumerate(mask):
        if value:
            off=(origin[1]+i//width)*pitch+origin[0]+i%width
            target[off]=lookup[target[off]];stores.append((off,target[off]))
    return stores
