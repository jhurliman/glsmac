"""Bounded ordinary unknown/known main-map scheduling at fixed zoom and relief.

Build queries, anchors and unknown lines from map/knowledge/cache inputs. No
captured line coordinates, returns, query lists or cache bytes are model inputs.
Empty object rosters; ordinary flags; authored Planet geometry profile domain.
"""
from viewport_geometry_cache_reference import select_tiles,horizontal_pan_regions
from minimap_navigation_reference import view_geometry

TRIANGLE_VERTICES=(0,1,2,0,2,3,0,3,4,0,4,1)
CORNER_PASS=(4,3,2,1)

def prepare_frame(g,camera,knowledge,old_first=None,size=(1024,511),zoom=0):
    geo=view_geometry([g.width,g.rows],size,camera,zoom);first=geo['first'];span=geo['span'];unit=geo['diamond_size'][0]//8
    regions=horizontal_pan_regions(old_first,first,span) if old_first is not None else None
    queries=[];lines=[];anchors=[];unknown=[];known=[]
    def query(kind,args):
        if kind=='projector':v=g.projector_at(*args,first,span,unit)
        elif kind=='height':v=g.height(*args)
        else:v=g.shade(*args)
        queries.append(dict(kind=kind,args=list(args),result=v));return v
    if old_first is not None:
        for tile in [first,old_first]:
            query('projector',tile)
            for k in CORNER_PASS:query('height',[*tile,k])
    terrain=select_tiles(first,span,regions)
    for tile in terrain:
        x,y=tile;anchor=query('projector',tile);anchors.append(anchor)
        average=sum(query('height',[x,y,k]) for k in CORNER_PASS)>>2
        if (x,y) in knowledge:
            known.append(tile)
            if g.bands[x,y]>=3:
                for k in TRIANGLE_VERTICES:
                    query('height',[x,y,k]);query('shade',[x,y,k])
        else:
            unknown.append(tile);corners=[]
            for j,k in enumerate(TRIANGLE_VERTICES):
                query('height',[x,y,k])
                # Native unknown path reads signed cache bytes after the query,
                # even when the function returned0 for sea and left byte FF.
                displacement=g.cache.get((x,y,k),-1)
                if j%3==1:
                    dx,dy=((0,0),(-4*unit,0),(0,-2*unit),(4*unit,0),(0,2*unit))[k]
                    corners.append([anchor[0]+4*unit+dx,anchor[1]+average+2*unit+dy-displacement])
            lines.extend([*corners[k],*corners[(k+1)&3],194] for k in range(4))
    objects=select_tiles(first,span,regions,objects=True)
    for tile in objects:query('projector',tile)
    return dict(first=first,span=span,regions=regions,terrain_tiles=terrain,object_tiles=objects,anchors=anchors,known_tiles=known,unknown_tiles=unknown,queries=queries,unknown_grid_lines=lines)

def cache_geometry_bytes(g):
    """The independently modeled9 semantic bytes per record, tile-major."""
    data=bytearray()
    for y in range(g.rows):
        for col in range(g.width//2):
            x=2*col+(y&1)
            data.extend((g.face_cache.get((x,y,k),127)+128)&255 for k in range(4))
            data.extend(g.cache.get((x,y,k),-1)&255 for k in range(5))
    return bytes(data)
