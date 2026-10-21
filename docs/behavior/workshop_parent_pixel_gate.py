"""Compare a matching1024x768 port capture to a private native reference PNG."""
import argparse,json,sys
from PIL import Image, ImageChops

REGIONS = {'parent': (216,168,839,555), 'console': (0,511,1024,768),
           'full': (0,0,1024,768), 'decoration': (216,168,839,555),
           'data': (376,275,460,373)}  # Stock DATA heading and six rows only.


def compare(reference, candidate, region='parent', skin=None):
    a,b = Image.open(reference).convert('RGB'),Image.open(candidate).convert('RGB')
    if a.size != (1024,768) or b.size != a.size:
        raise ValueError('Both captures must be1024x768 at the same native scale.')
    rect = REGIONS[region]
    if region == 'decoration':
        if skin is None:
            raise ValueError('Decoration comparison requires the installed DESWIN.PCX via --skin.')
        art=Image.open(skin)
        if art.mode!='P' or art.size!=(623,368):
            raise ValueError('Expected original indexed623x368 DESWIN.PCX.')
        indices=art.tobytes();ar,br=a.tobytes(),b.tobytes();tested=0;bad=[]
        for x0,y0,x1,y1 in [(0,0,24,368),(27,0,565,4),(565,0,568,344)]:
            for y in range(y0,y1):
                for x in range(x0,x1):
                    if ((indices[y*623+x]+10)&255)==9:
                        continue
                    tested+=1;sx,sy=x+216,y+168;offset=3*(sy*1024+sx)
                    if ar[offset:offset+3]!=br[offset:offset+3]:
                        bad.append((sx,sy))
        bounds=[min(x for x,y in bad),min(y for x,y in bad),max(x for x,y in bad)+1,max(y for x,y in bad)+1] if bad else None
        return dict(region=region,screen_rect=rect,pixels=tested,
                    mismatched_pixels=len(bad),exact_match=not bad,
                    mismatch_screen_bounds=bounds,
                    scope='Opaque DESWIN shell/top pixels only; excludes economic text, vehicles and uncovered map.')
    a,b = a.crop(rect),b.crop(rect)
    delta = ImageChops.difference(a,b)
    pixels=delta.tobytes()
    mismatch = sum(any(pixels[i:i+3]) for i in range(0,len(pixels),3))
    bounds = delta.getbbox()
    if bounds:
        bounds = [bounds[0]+rect[0],bounds[1]+rect[1],bounds[2]+rect[0],bounds[3]+rect[1]]
    return dict(region=region,screen_rect=rect,pixels=a.width*a.height,
                mismatched_pixels=mismatch,exact_match=mismatch==0,
                mismatch_screen_bounds=bounds)


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference');p.add_argument('candidate')
    p.add_argument('--region',choices=REGIONS,default='parent')
    p.add_argument('--skin',help='Installed DESWIN.PCX, required for decoration-only comparison.')
    args=p.parse_args()
    result=compare(args.reference,args.candidate,args.region,args.skin)
    print(json.dumps(result,indent=2))
    sys.exit(0 if result['exact_match'] else 1)
