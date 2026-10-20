"""Independent bounded Unity Survey main-map query/cache scheduler.

Empty rosters, normal zoom0 view, interior horizontal pans; all geographical
tiles admitted. Known land calls face-shade, unknown survey land does not.
"""
from viewport_geometry_cache_reference import select_tiles,horizontal_pan_regions
from minimap_navigation_reference import view_geometry
from unknown_view_geometry_reference import TRIANGLE_VERTICES,CORNER_PASS

def prepare_survey_frame(g,camera,knowledge,old_first=None,size=(1024,511),lighting=2):
 geo=view_geometry([g.width,g.rows],size,camera,0);first=geo['first'];span=geo['span'];unit=geo['diamond_size'][0]//8
 regions=horizontal_pan_regions(old_first,first,span) if old_first is not None else None
 queries=[];anchors=[];policies=[]
 def query(kind,args):
  if kind=='projector':value=g.projector_at(*args,first,span,unit)
  elif kind=='height':value=g.height(*args)
  else:value=g.shade(*args)
  queries.append(dict(kind=kind,args=list(args),result=value));return value
 if old_first is not None:
  for tile in [first,old_first]:
   query('projector',tile)
   for k in CORNER_PASS:query('height',[*tile,k])
 terrain=select_tiles(first,span,regions)
 for x,y in terrain:
  anchors.append(query('projector',[x,y]))
  for k in CORNER_PASS:query('height',[x,y,k])
  known=(x,y) in knowledge
  policies.append(dict(tile=[x,y],blank=0 if known else 1,full_light=1 if known else 0,global_lighting=lighting))
  if g.bands[x,y]>=3:
   for k in TRIANGLE_VERTICES:
    query('height',[x,y,k])
    if known:query('shade',[x,y,k])
 objects=select_tiles(first,span,regions,objects=True)
 for tile in objects:query('projector',tile)
 return dict(first=first,span=span,regions=regions,terrain_tiles=terrain,object_tiles=objects,anchors=anchors,queries=queries,policies=policies)
