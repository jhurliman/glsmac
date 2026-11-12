"""Self-authored initialized stock button recipe using an owned interface.pcx.

No original asset or font pixels are embedded. This covers native20px-high
Preferences buttons with nonoverlapping3px caps; caption raster is separate.
"""
from preferences_checkbox_reference import decode_single_plane_pcx


def state_index(pressed=False, selected=False, hovered=False, override=-1):
    if 0 <= override < 3:
        return override
    return 2 if pressed or selected else int(bool(hovered))


def slices(interface_pcx, state):
    if state not in range(3):
        raise ValueError("State must be0,1,2")
    rows = decode_single_plane_pcx(interface_pcx)
    y = 127 + 21 * state
    def crop(x, width):
        # Original PCX load adds10 modulo256. Loaded6/7 become key9.
        return bytes(9 if (v + 10) % 256 in (6, 7) else (v + 10) % 256
                     for row in rows[y:y + 20] for v in row[x:x + width])
    return crop(64, 3), crop(68, 67), crop(136, 3)


def plane(interface_pcx, width=214, state=0):
    if width < 6:
        raise ValueError("Reference covers nonoverlapping native3px caps")
    left, middle, right = slices(interface_pcx, state)
    result = bytearray([9]) * (width * 20)
    for y in range(20):
        result[y * width:y * width + 3] = left[y * 3:y * 3 + 3]
        result[y * width + width - 3:(y + 1) * width] = right[y * 3:y * 3 + 3]
        for x in range(3, width - 3):
            result[y * width + x] = middle[y * 67 + (x - 3) % 67]
    return result


def composite(child, backing):
    if len(child) != len(backing):
        raise ValueError("Equal-sized indexed planes required")
    return bytes(bg if fg == 9 else fg for fg, bg in zip(child, backing))
