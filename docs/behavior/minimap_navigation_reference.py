"""Named-field reference for the bounded original minimap navigation receipts.

No binary addresses or art. This models request geometry, not framebuffer pixels.
Integer division follows native signed truncation toward zero.
"""


def trunc_div(n, d):
    assert d > 0
    return (-1 if n < 0 else 1) * (abs(n) // d)


def overview_layout(world_size, window_size):
    w, h = world_size
    vw, vh = window_size
    assert w > 0 and h > 4 and w % 2 == h % 2 == 0
    source = [w * 4, (h // 2 - 2) * 4]  # Native zoom -14, no scale override.
    fits = vw >= source[0] and vh >= source[1]
    offset = [(vw - source[0]) // 2, (vh - source[1]) // 2] if fits else [0, 0]
    return dict(world_size=list(world_size), window_size=list(window_size),
                source_size=source, image_size=source[:] if fits else [vw, vh],
                offset=offset, scaled=not fits)


def pixel_to_tile(pixel, layout):
    x, y = pixel
    w, h = layout['world_size']
    px, py = layout['offset']
    if px or py:
        iw, ih = layout['image_size']
        if not (px <= x < px + iw and py <= y < py + ih):
            return None
        x, y = x - px, y - py
    else:
        iw, ih = layout['window_size']
    tx = trunc_div(w * x, iw)
    ty = trunc_div(h * y, ih)
    ty = (ty & ~1) | (tx & 1)
    return [tx, ty] if 0 <= tx < w and 0 <= ty < h else None


def tile_to_pixel(tile, layout, flat_map=False):
    x, y = tile
    w, h = layout['world_size']
    if not flat_map:
        if x < 0:
            x += w
        elif x >= w:
            x -= w
    y = max(0, min(h - 1, y))
    if not 0 <= x < w:
        return None
    px, py = layout['offset']
    iw, ih = layout['image_size'] if px or py else layout['window_size']
    return [px + trunc_div(iw * x, w), py + trunc_div(ih * y, h)]


def view_geometry(world_size, window_size, camera_center, zoom, flat_map=False):
    """Ordinary first-coordinate/span geometry in the tested overview/detail domain.

    Includes flat-map edge/pole clamps; excludes tiny-map centering and scale override.
    """
    w, h = world_size
    vw, vh = window_size
    cx, cy = camera_center
    unit = max(1, trunc_div(trunc_div(50 * (zoom + 16), 16) + 1, 4))
    dw, dh = 8 * unit, 4 * unit
    ceil_div = lambda n, d: (n + d - 1) // d
    sx, sy = ceil_div(vw, dw), ceil_div(vh + 2 * unit, dh)
    ox, oy = ceil_div(vw + 4 * unit, dw), sy
    assert sx <= w // 2 and sy <= h // 2, 'Tiny-map centering is outside this helper.'
    fx, fy = cx - sx, cy - sy
    fy = max(-2, min(h + 2 * (2 - sy), fy))
    if flat_map:
        fx = max(-4, min(w + 2 * (2 - sx), fx))
    elif fx < 0:
        fx += w
    elif fx >= w:
        fx -= w
    fx = (fx & ~1) | (fy & 1)
    return dict(first=[fx, fy], span=[sx, sy, ox, oy], diamond_size=[dw, dh])


def viewport_outline(first, span, layout, flat_map=False, enabled=True):
    """Native line/frame requests; line endpoints are inclusive, frame rect is half-open."""
    if not enabled:
        return []
    x, y = first
    sx, sy, ox, oy = span
    start = tile_to_pixel([x, y], layout, flat_map)
    end = tile_to_pixel([x + sx + ox, y + sy + oy], layout, flat_map)
    if start is None or end is None:
        return []
    x1, y1 = start
    x2, y2 = end
    if x1 <= x2:
        return [dict(kind='frame', rect=[x1, y1, x2, y2], colors=[251, 251])]
    left = layout['offset'][0]
    right = (left + layout['image_size'][0] if left else layout['window_size'][0]) - 1
    return [
        dict(kind='horizontal', args=[left, x2 - 1, y1, 251]),
        dict(kind='horizontal', args=[left, x2 - 2, y2 - 1, 251]),
        dict(kind='vertical', args=[x2 - 1, y1 + 1, y2 - 1, 251]),
        dict(kind='horizontal', args=[x1 + 1, right, y1, 251]),
        dict(kind='horizontal', args=[x1, right, y2 - 1, 251]),
        dict(kind='vertical', args=[x1, y1, y2 - 2, 251]),
    ]
