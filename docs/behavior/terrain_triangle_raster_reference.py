"""Replay native measured land spans; requires the user's stock TEXTURE.PCX.

This closes horizontal sampling/shading, not a general vertex-to-span converter.
Run: python terrain_triangle_raster_reference.py --texture-pcx PATH
"""
from pathlib import Path
import argparse,hashlib,json,struct

def pcx_crop(path,rect,index_offset=10):
    data=Path(path).read_bytes()
    if data[0:4]!=bytes([10,5,1,8]) or data[65]!=1:
        raise ValueError('Expected stock single-plane8-bit RLE PCX')
    xmin,ymin,xmax,ymax=struct.unpack_from('<4H',data,4)
    stride=struct.unpack_from('<H',data,66)[0];height=ymax-ymin+1
    pixels=bytearray();pos=128
    while len(pixels)<stride*height:
        value=data[pos];pos+=1
        if value&192==192:
            count=value&63;value=data[pos];pos+=1
        else:count=1
        pixels.extend(bytes([value])*count)
    x,y,w,h=rect
    crop=b''.join(pixels[(y+j)*stride+x:(y+j)*stride+x+w] for j in range(h))
    # The original PCX load uses first palette index10; pinned crop verifies this
    # rebase byte-for-byte before any terrain-light addition.
    return bytes((v+index_offset)&255 for v in crop)

def shade_index(texture_index,phase_8_8):
    # Native ADD AL,CH: byte arithmetic, including negative shade and wrapping.
    return (texture_index+((phase_8_8&65535)>>8))&255

def replay(texture,case,width=1024,height=511,background=250):
    output=bytearray([background])*(width*height)
    writes=0;coverage=set()
    for span in case['spans']:
        triangle,y,x0,x1,u,v,du,dv,shade,dshade=span
        for x in range(x0,x1):
            offset=(v//65536)*56+(u//65536)
            if not 0<=offset<len(texture):raise ValueError('Texture sample outside crop')
            output[y*width+x]=shade_index(texture[offset],shade)
            writes+=1;coverage.add((x,y))
            u+=du;v+=dv;shade=(shade+dshade)&65535
    return bytes(output),writes,len(coverage)

def verify(texture,cases):
    if hashlib.sha256(texture).hexdigest()!=cases['texture']['indexed_sha256']:
        raise ValueError('Texture crop differs from pinned original fixture')
    total=0
    for case in cases['cases']:
        output,writes,unique=replay(texture,case,*cases['viewport'],cases['background'])
        assert hashlib.sha256(output).hexdigest()==case['frame_sha256'],case['id']
        assert writes==case['pixel_write_count'] and unique==case['unique_pixels']
        for s in case['samples']:assert output[s['y']*1024+s['x']]==s['index']
        total+=writes
    assert shade_index(1,(-2*256)&65535)==255
    assert shade_index(255,2*256)==1
    return total

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--texture-pcx',required=True)
    args=parser.parse_args()
    cases=json.loads(Path(__file__).with_name('ordinary-land-triangle-raster-cases.json').read_text())
    print('PASS',verify(pcx_crop(args.texture_pcx,cases['texture']['crop']),cases),'native pixels')
