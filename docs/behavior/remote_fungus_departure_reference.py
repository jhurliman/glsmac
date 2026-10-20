"""Admitted ordinary land Scout pipeline; execute emitted effects canonically.

No coordinate-keyed choices. Supply owning site/cost queries and constructor.
Broader combat, transport, ability and diplomacy admission is caller-owned.
"""
from global_ecology_reference import distance
PROTECTION=[(0,0),(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2),(2,-2),(2,2),(-2,2),(-2,-2),(1,-3),(3,-1),(3,1),(1,3),(-1,3),(-3,1),(-3,-1),(-1,-3),(4,0),(-4,0),(0,4),(0,-4)]
def crt(seed):
 seed=(214013*seed+2531011)&0xffffffff;return seed,(seed>>16)&32767
def nearest_any(p,bases,width,flat,retained):
 chosen=-1;best=9999
 for b in bases:
  d=distance(*p,*b['position'],width,flat)
  if d<=best:chosen=b['id'];best=d
 return chosen,best if chosen>=0 else retained
def step(*,destination,tile,bases,width,height,flat,retained_distance,life,
         resolved_site,encounter_enabled=True,allowance,spent,edge_cost,
         failure_marker=False,quantum=3,crt_seed,shared_seed,crt_movement=True,
         factory=lambda:True):
 """Land/land empty destination, healthy non-Psi Scout, no exemptions.

 ZOC and prior profile admission must already pass. tile returns numeric band,
 flags, affiliation. factory() must execute canonical birth or report failure.
 resolved_site is the owning typed Supply Pod/site query, not a bit guess.
 """
 t=tile(destination);draws=[];actions=[];nearest=None;threshold=None
 def draw_crt(reason,mod):
  nonlocal crt_seed
  before=crt_seed;crt_seed,raw=crt(crt_seed);value=raw%mod
  draws.append(dict(stream='CRT',reason=reason,before=before,after=crt_seed,raw=raw,value=value));return value
 reason='disabled' if not encounter_enabled else 'destination_not_fungus' if not t['flags']&0x20 or t['band']<2 else 'destination_tube' if t['flags']&8 else 'resolved_site' if resolved_site(destination) else None
 if reason is None:
  nearest,retained_distance=nearest_any(destination,bases,width,flat,retained_distance)
  if retained_distance<=1:reason='near_any_Base'
  else:
   threshold=min(retained_distance,32)+10*life
   if t['flags']&0x100000:
    threshold//=2
    if life==0:threshold//=2
   if t['flags']&4:threshold//=2
   for i,(dx,dy) in enumerate(PROTECTION):
    x,y=destination[0]+dx,destination[1]+dy
    if not flat:x%=width
    if not (0<=x<width and 0<=y<height):continue
    q=tile((x,y))
    if q['flags']&0x80000000 or q['flags']&1 and 0<=q['affiliation']<8:threshold//=4 if i<9 else 2
   if draw_crt('native_encounter',100)<threshold:
    actions.append('canonical_native_factory')
    if factory():
     actions+=['mark_destination_history_0x100000','return_without_movement']
     return dict(action='encounter_birth',native_return=0,spent=spent,nearest=nearest,distance=retained_distance,threshold=threshold,skip_reason=None,draws=draws,actions=actions,crt=crt_seed,shared=shared_seed)
    actions.append('factory_failed_continue_without_rollback')
 remaining=max(0,min(999,allowance-spent))
 if not remaining:action='no_allowance';answer=0
 else:
  actions.append('canonical_edge_cost_query');cost=edge_cost()
  lottery=cost>remaining and (remaining<quantum or t['flags']&0x20 and t['band']>=2)
  admitted=True
  if lottery:
   if crt_movement:roll=draw_crt('movement_lottery',cost)
   else:
    before=shared_seed;shared_seed=(1664525*shared_seed+1013904223)&0xffffffff;roll=((shared_seed&65535)*cost)>>16
    draws.append(dict(stream='shared',reason='movement_lottery',before=before,after=shared_seed,value=roll))
   admitted=roll<remaining+(quantum if failure_marker else 0)
  answer=1
  if admitted:action='relocate';spent=(spent+cost)&255;actions+=['spend_raw_cost','clear_failure_and_0x40','canonical_unlink_placement_sight']
  else:action='lottery_refusal';spent=allowance;actions+=['canonical_endturn','set_failure_0x8000']
 return dict(action=action,native_return=answer,spent=spent,nearest=nearest,distance=retained_distance,threshold=threshold,skip_reason=reason,draws=draws,actions=actions,crt=crt_seed,shared=shared_seed)
