"""Source-derived generic WEAPON picker plan; no raster or captured coordinates."""
from workshop_stock_composition_reference import weapon, op


def trunc_half(n):
    return n // 2 if n >= 0 else -((-n) // 2)


def admitted(definitions):
    return [i for i, row in enumerate(definitions) if row['prerequisite'] >= -1]


def initial_ordinal(definitions, current_id):
    ordinal = 0
    for ident, row in enumerate(definitions):
        if row['prerequisite'] >= -1:
            if ident == current_id:
                break
            ordinal += 1
    return ordinal


def initial_window(initial_ordinal):
    first = 0
    while initial_ordinal >= first + 20:
        first += 5
    return first


def cell(ordinal, first=0):
    row, col = divmod(ordinal - first, 5)
    return [27 + 108*col, 4 + 94*row, 132 + 108*col, 95 + 94*row]


def attachment_rect(width, height, cap_top=26, tile=34, cap_bottom=25):
    numerator = -(height - 13) + cap_bottom + cap_top + 44
    quotient = abs(numerator) // tile
    if numerator < 0:
        quotient = -quotient
    control_height = cap_top + cap_bottom - quotient*tile - 10
    return [width-43, 21, width-19, 21+control_height]


def sprite_source(selector):
    """Loaded frame0 only; selector validity follows the caller's signed test.

    UNITS loader initializes selectors0..9. Nonnegative out-of-bank selectors
    are unchecked by the original callback; do not invent safe wrapping.
    """
    if not 0 <= selector <= 9:
        return None
    return [716, 233, 100, 75] if selector == 9 else [2, 2 + 77*selector, 100, 75]


def plan(ident, power, mode, stat_byte, icon, probe_icon, known=True,
         owner=1, clip=(30, 7, 129, 92)):
    if ident in (0, 13, 20):
        scene = [op({0: 'vw00.cvr', 13: 'vw13.cvr', 20: 'vm.cvr'}[ident])]
        scale = 0x3e4ccccd if ident == 0 else 0x3d75c28f
    else:
        scene = weapon(owner, -1, ident, power, mode)
        scale = (0x3e4ccccd if mode < 3 or mode == 11 else
                 0x3da3d70a if mode == 9 else 0x3dcccccd)
    selector = probe_icon if mode == 11 else icon
    underlay = None
    if mode in (8, 9, 11) and selector >= 0:
        x0, y0, x1, y1 = clip
        underlay = dict(selector=selector, sprite=0x780740+352*selector,
                        source=sprite_source(selector),
                        anchor=[x0+trunc_half(x1-x0-100),
                                y0+trunc_half(y1-y0-75)], key=9)
    x0, y0, x1, y1 = clip
    return dict(scene=scene, scale_bits=scale, underlay=underlay,
                anchor=[trunc_half(x0+x1), trunc_half(y0+y1)],
                draw='normal' if known else 'silhouette', flags=6,
                silhouette_index=None if known else 185,
                label_offset=(0x204 if mode < 3 else 0x200) if known else None,
                value=('Psi' if power == -1 else
                       str((power if mode < 3 else stat_byte) & 255)) if known else None,
                selectable=known)
