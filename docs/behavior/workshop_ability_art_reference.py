"""Source-derived stock ability artwork; names are labels, mask bits are authority."""
from workshop_stock_composition_reference import op, body, weapon, armor, reactor

# Original4d8700 branch order. All operations direct-copy frame0, own shades.
ART = (
    (0, 'ssf.cvr'), (13, 'sft.cvr'), (5, 'sas.cvr'), (7, 'scd.cvr'),
    (10, 'sags.cvr'), (15, 'vhr.cvr'), (21, 'srb.cvr'),
    (4, 'sdp.cvr'), (9, 'scj.cvr'), (18, 'sht.cvr'),
)

def decorations(mask):
    return [op(asset) for bit, asset in ART if mask & (1 << bit)]

def composition(kind, owner, ch, we, ar, re, mask, power, mode, strength, phase=0):
    result = body(owner, ch, we, ar, re, mode, strength, phase)
    result += weapon(owner, ch, we, power, mode)
    if kind == 'strip':
        result += decorations(mask)
    result += armor(ch, we, ar, 0, mode, phase)
    result += reactor(ch, we, re, mask, mode)
    if kind == 'main':
        result += decorations(mask)
    return result
