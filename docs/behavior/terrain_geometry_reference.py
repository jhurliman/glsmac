"""Stock ordinary terrain geometry at zoom0; excludes landmark deformation/art.

Inputs are named retained altitude bands and map topology. Integer floor shifts
and signed truncating division are distinct. The renderer's palette/raster stage
is deliberately outside this geometry and illumination reference.
"""
from math import isqrt

CORNER_CENTERS = (
    ((-2, 0), (-1, -1), (0, 0), (-1, 1)),  # west
    ((-1, -1), (0, -2), (1, -1), (0, 0)),  # north
    ((0, 0), (1, -1), (2, 0), (1, 1)),    # east
    ((-1, 1), (0, 0), (1, 1), (0, 2)),    # south
)


def trunc_div(n, d):
    return (abs(n) // abs(d)) * (-1 if (n < 0) != (d < 0) else 1)


def planet_view_origin(camera_x, camera_y, wrap_x=True):
    """Measured80×80 world,1024×511 map viewport, zoom0; not general UI sizing."""
    # ceil(1024/96)=11 horizontal half-span; ceil((511+24)/48)=12 rows.
    first_y = max(-2, min(camera_y-12, 60))
    first_x = camera_x-11
    if wrap_x:
        if first_x < 0:
            first_x += 80
        elif first_x >= 80:
            first_x -= 80
    else:
        first_x = max(-4, min(first_x, 62))
    first_x = (first_x & ~1) | (first_y & 1)
    return first_x, first_y


def planet_flat_center(x, y, camera_x, camera_y, wrap_x=True):
    """In-view valid tile centers; offscreen closest-copy picking is outside scope."""
    first_x, first_y = planet_view_origin(camera_x, camera_y, wrap_x)
    dx = x-first_x
    if wrap_x:
        if dx < 0:
            dx += 80
        elif dx >= 80:
            dx -= 80
    return dx*48, (y-first_y)*24


def face_light(first_delta, second_delta, face):
    """Generate the signed lookup entry, not an approximate Lambert normal."""
    if not (-3 <= first_delta <= 3 and -3 <= second_delta <= 3):
        raise ValueError('slope lookup outside the generated ordinary range')
    a, b = ((first_delta, second_delta), (-second_delta, first_delta),
            (-first_delta, -second_delta), (second_delta, -first_delta))[face]
    a *= 100
    b *= 100
    length = isqrt(a*a + b*b)
    return trunc_div(-3*(a+b), length) if length else 0


def water_quads(center_x, center_y):
    """Zoom0 sea-level geometry; contour samples are separate depth inputs."""
    offsets = (((0, 0), (-24, 12), (-48, 0), (-24, -12)),
               ((0, 0), (-24, -12), (0, -24), (24, -12)),
               ((0, 0), (24, -12), (48, 0), (24, 12)),
               ((0, 0), (24, 12), (0, 24), (-24, 12)))
    return [[[center_x+dx, center_y+dy] for dx, dy in q] for q in offsets]


class TerrainGeometry:
    def __init__(self, width, height, bands, wrap_x=True, height_step=25):
        self.width, self.rows, self.bands = width, height, bands
        self.wrap_x, self.height_step = wrap_x, height_step

    def coord(self, x, y):
        if not 0 <= y < self.rows:
            return None
        if self.wrap_x:
            if x < 0:
                x += self.width
            elif x >= self.width:
                x -= self.width
        return (x, y) if 0 <= x < self.width else None

    def band(self, x, y):
        q = self.coord(x, y)
        return self.bands[q] if q is not None else None

    def corner_cells(self, x, y, corner):
        return [self.coord(x+dx, y+dy) for dx, dy in CORNER_CENTERS[corner-1]]

    def height(self, x, y, vertex):
        band = self.band(x, y)
        if band is None or band < 3:
            return 0
        if vertex == 0:
            return min(127, self.height_step*(band-3))
        cells = self.corner_cells(x, y, vertex)
        if any(q is None or self.bands[q] < 3 for q in cells):
            return 0
        return sum(self.height(*q, 0) for q in cells) >> 2

    def corner_slope_code(self, x, y, corner):
        cells = self.corner_cells(x, y, corner)
        if any(q is None or self.bands[q] < 3 for q in cells):
            return 4
        return (sum(self.bands[q] for q in cells)-4*self.band(x, y)+4) & 15

    def face_shade(self, x, y, face):
        band = self.band(x, y)
        if band is None or band < 3:
            return 0
        first = self.corner_slope_code(x, y, face+1)-4
        second = self.corner_slope_code(x, y, (face+1) % 4+1)-4
        return face_light(first, second, face)

    def shade(self, x, y, vertex):
        if vertex == 0:
            return sum(self.face_shade(x, y, face) for face in range(4)) >> 2
        total = 0
        for i, q in enumerate(self.corner_cells(x, y, vertex)):
            if q is not None:
                total += self.face_shade(*q, (i+1) & 3)
                total += self.face_shade(*q, (i+2) & 3)
        return total >> 3

    def vertices(self, x, y, center_x, center_y):
        """Flat projected tile center before vertical terrain displacement."""
        xy = ((0, 0), (-48, 0), (0, -24), (48, 0), (0, 24))
        return [[center_x+dx, center_y+dy-self.height(x, y, i)]
                for i, (dx, dy) in enumerate(xy)]

    def triangles(self, x, y, center_x, center_y):
        xy = self.vertices(x, y, center_x, center_y)
        shade = [self.shade(x, y, v) for v in range(5)]
        return [dict(vertices=[xy[i] for i in ids], shade=[shade[i] for i in ids])
                for ids in ((0, 1, 2), (0, 2, 3), (0, 3, 4), (0, 4, 1))]
