"""Finite original Workshop bounds arithmetic; no executable/asset inputs."""
from fractions import Fraction as F
import struct

def r64(v,p=64):
 v=F(v)
 if not v:return v
 sign=-1 if v<0 else 1;v=abs(v);e=v.numerator.bit_length()-v.denominator.bit_length()
 if v<F(2)**e:e-=1
 step=F(2)**(e-p+1);q=v/step;n,rem=divmod(q.numerator,q.denominator)
 if 2*rem>q.denominator or (2*rem==q.denominator and n&1):n+=1
 return sign*n*step
def r32(v):return float(r64(v,24))
def mul(a,b):return r64(F(a)*F(b))
def add(a,b):return r64(F(a)+F(b))
def sub(a,b):return r64(F(a)-F(b))
def mv(m,v):return [r32(add(add(mul(m[k+1],v[1]),mul(m[k+2],v[2])),mul(m[k],v[0]))) for k in [0,3,6]]
ORDER=[[1,2,0],[0,1,2],[0,2,1],[0,2,1],[0,2,1],[2,1,0],[0,1,2],[2,1,0],[0,1,2]]
def mm(a,b):
 out=[]
 for cell,order in enumerate(ORDER):
  row,col=divmod(cell,3);terms=[mul(a[row*3+j],b[j*3+col]) for j in range(3)]
  out.append(r32(add(add(terms[order[0]],terms[order[1]]),terms[order[2]])))
 return out
def bounds_operands(i):
 R=i['rotation'];s=i['object_scale'];q=r32(mul(i['scalar'],s));D=mm(R,i['frame_matrix'])
 loc=[r32(add(r32(mul(x,s)),y)) for x,y in zip(mv(R,i['frame_location']),i['position'])]
 offset=[r32(mul(x,q)) for x in i['offset']]
 origin=[r32(add(a,b)) for a,b in zip(mv(D,offset),loc)]
 edges=[]
 for col in range(3):
  length=mul(i['dimensions'][col],q)
  if col==2:length=r32(length)
  edges.append([r32(mul(D[row*3+col],length)) for row in range(3)])
 return origin,edges
def corner_bounds(origin,edges):
 A,B,D=edges;v=list(map(F,origin));lo=list(origin);hi=list(origin)
 def visit():
  for j,x in enumerate(v):
   if x<F(lo[j]):lo[j]=r32(x)
   if x>F(hi[j]):hi[j]=r32(x)
 v=[F(r32(add(x,y))) for x,y in zip(v,A)];visit()
 v=[add(x,y) for x,y in zip(v,B)];visit()
 v=[sub(x,y) for x,y in zip(v,A)];v[2]=F(r32(v[2]));visit()
 v=[add(x,y) for x,y in zip(v,D)];visit()
 v=[add(x,y) for x,y in zip(v,A)];visit()
 v=[sub(x,y) for x,y in zip(v,B)];visit()
 v=[sub(x,y) for x,y in zip(v,A)];v[1]=F(r32(v[1]));v[2]=F(r32(v[2]));visit()
 return lo,hi
def bounds(i):return corner_bounds(*bounds_operands(i))
