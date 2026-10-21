"""Read the bounded owned-host Arial10 reference without a GDI dependency."""
from pathlib import Path
import json,zlib,hashlib
ROOT=Path(__file__).resolve().parents[2]

def load(root=ROOT):
    root=Path(root);r=json.loads((root/'docs/behavior/design-workshop-arial10-cases.json').read_text())
    blob=(root/'tmp/reference/independent-workshop-arial10-evidence-20261021'/r['cell']['compressed_file']).read_bytes()
    assert hashlib.sha256(blob).hexdigest()==r['cell']['compressed_sha256']
    cells=zlib.decompress(blob);assert hashlib.sha256(cells).hexdigest()==r['cell']['uncompressed_sha256']
    return r,cells

def glyph(receipt,cells,code):
    if not 32<=code<=126:return None
    r=receipt['glyphs'][code-32];off=r['packed_offset'];mask=bytes((b>>(7-j))&1 for b in cells[off:off+512] for j in range(8))
    assert hashlib.sha256(mask).hexdigest()==r['mask_sha256']
    bb=r['baseline_relative_ink_bounds']
    if bb is None:return dict(advance=r['advance'],ABC=r['ABC'],left=0,top=0,width=0,height=0,mask=b'')
    l,t,rr,b=bb;bitmap=bytes(mask[(y+48)*64+x+16]*255 for y in range(t,b) for x in range(l,rr))
    return dict(advance=r['advance'],ABC=r['ABC'],left=l,top=-t,width=rr-l,height=b-t,mask=bitmap)
