"""Original page0 checkbox geometry and owned-PCX extraction reference.

No game pixels or font payloads are embedded. This is the observed 1024x768,
MainFontSize16 contract, not a general responsive layout engine.
"""

IDS = tuple(range(7, 15))
ATLAS_RECTS = {
    "unused_first": (227, 1, 18, 18),
    "unchecked": (227, 20, 18, 18),
    "checked": (227, 39, 18, 18),
}


def decode_single_plane_pcx(data):
    import struct

    if data[0] != 10 or data[2:4] != bytes((1, 8)) or data[65] != 1:
        raise ValueError("Expected original single-plane 8-bit RLE PCX")
    x0, y0, x1, y1 = struct.unpack_from("<4H", data, 4)
    width, height = x1 - x0 + 1, y1 - y0 + 1
    stride = struct.unpack_from("<H", data, 66)[0]
    out, pos = bytearray(), 128
    while len(out) < stride * height:
        value, pos = data[pos], pos + 1
        count = 1
        if value & 0xC0 == 0xC0:
            count, value, pos = value & 63, data[pos], pos + 1
        out.extend((value,) * count)
    if len(out) != stride * height:
        raise ValueError("PCX run exceeds decoded size")
    return [bytes(out[y * stride:y * stride + width]) for y in range(height)]


def checkbox_pixels(interface_pcx, state):
    """Native-loaded indices: PCX +10, then replace loaded6/7 with key9."""
    rows = decode_single_plane_pcx(interface_pcx)
    x, y, width, height = ATLAS_RECTS[state]
    pixels = bytes((v + 10) & 255 for row in rows[y:y + height]
                   for v in row[x:x + width])
    return bytes(9 if v in (6, 7) else v for v in pixels)


def row_geometry(ordinal):
    if ordinal not in range(8):
        raise ValueError("Only the eight main Preferences rows")
    top = 24 * ordinal
    return {
        "hit_rect": (0, top, 457, top + 24),
        "sprite_rect": (0, top + 3, 18, top + 21),
        "text_request": (18, top + 7),
        "text_baseline": top + 19,
        "first_label_character_origin_x": 23,  # leading space advances5
    }


def native_click_id(x, y, current_id=14):
    """Direct629a50 behavior within the child Window's actual client bounds.

    Outside those bounds is rejected by this wrapper. Inside blank rows the
    original624010 leaves its linked-list cursor unchanged; this reproduces
    that observed fallback, rather than recommending it for new interfaces.
    """
    if not (0 <= x < 457 and 0 <= y < 376):
        return None
    ordinal = y // 24  # one visible column
    return IDS[ordinal] if ordinal < len(IDS) else current_id
