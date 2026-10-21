"""Flat contained shadow kernels and clipped indexed copies for live ground units.

Extends the existing Scout reference with native one/two pixel stamp selection
and count-with/without-terminator flags. Compressed blocks and raster clipping
are outside this bounded reference. No original model or palette bytes here.
"""
from scout_shadow_reference import stamp


def stamp_contained(mask, matrix, location, origin, stream, count, flags):
    assert flags in (0, 1, 32, 33)
    # Existing Scout helper excludes one trailing record. A stream without that
    # marker consumes count records, using the same delta-before-store order.
    stores = stamp(mask, matrix, location, origin, stream,
                   count + (0 if flags & 32 else 1), 33)
    if not flags & 1:
        for offset, value in stores:
            assert offset % 256 < 255 and offset // 256 < 255
            for dx, dy in ((1, 0), (0, 1), (1, 1)):
                mask[offset + dy * 256 + dx] = value
    return mask


def copy_planes(before, pitch, clip, offset, body, shadow, lookup,
                shadow_enabled=True):
    """Supplied target bounds contain clip; source planes are 256 square."""
    target = bytearray(before)
    stores = []
    for stage, key, source in (("shadow", 0, shadow), ("body", 9, body)):
        if stage == "shadow" and not shadow_enabled:
            continue
        for y in range(256):
            ty = y + offset[1]
            for x in range(256):
                tx = x + offset[0]
                value = source[y * 256 + x]
                if (clip[0] <= tx < clip[2] and clip[1] <= ty < clip[3]
                        and value != key):
                    pos = ty * pitch + tx
                    target[pos] = lookup[target[pos]] if stage == "shadow" else value
                    stores.append((stage, pos, target[pos]))
    return bytes(target), stores
