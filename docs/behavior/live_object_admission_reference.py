"""Finite ordinary main-map actor/name admission; explicit native state inputs.

The supplied control square is(22,38), viewport contains it, conventional
designs, no network inhibitors, no transported-unit predicate, allowance3.
"""
def predict(s):
 display=s.get('display',3);owner=s.get('owner',3);base_owner=s.get('base_owner',owner);seen=s.get('seen',0);flags16=s.get('flags16',0);status=s.get('status',0)
 flags=s.get('view_flags',0x60010ee3);editor=s.get('editor',False);viewer=s.get('direct_viewer',display);knowledge=bool((8 if s.get('explored',True) else 0)&(1<<viewer)) or s.get('reveal',False) or editor
 live=bool(s.get('lifecycle',1) or s.get('lifecycle_alt',0) or s.get('rules80'))
 initial=live and not flags&0x1000000 and bool(flags&0x802000c0) and not flags&0x1000
 bases=bool(s.get('base') and s.get('base_count',1)>0 and s.get('tile_base',s.get('base')))
 base_body=bool(initial and flags&0x200080 and knowledge and bases and (editor or base_owner==display or s.get('base_seen',seen)&(1<<display)))
 name=bool('direct_viewer' not in s and live and flags&0x800 and s.get('names',True) and s.get('zoom',0)>-11 and s.get('base') and s.get('base_count',1)>0 and (editor or base_owner==display or s.get('base_seen',seen)&(1<<display)))
 def done(units=()):return dict(units=[dict(unit=i,flags=f,overlay=not bool(f&0x440)) for i,f in units],base_body=base_body,base_name=name)
 if not initial or flags&0x80000000:return done()
 d4=s.get('override_d4',-1);cc=s.get('override_cc',-1);d0=s.get('override_d0',-1);selected=0 if s.get('selected') else -1
 control=s.get('control',1);pending=s.get('pending',1);order=s.get('order',0)
 exceptional=flags16&0x440==0x440
 if d4>=0:index=d4;drawflags=3
 elif cc>=0:
  index=1 if s.get('chassis',0)==8 and s.get('stack') else -1 if s.get('chassis',0)==8 else cc;drawflags=2
 elif d0>=0:index=d0;drawflags=2
 else:
  if not knowledge:return done()
  valid=bool(selected==0 and owner==display and not exceptional and not status&0x4200)
  if valid:
   index=0;drawflags=3
   if control and pending and s.get('blink',0) and order==0 and not s.get('suppress',0):
    mouse=s.get('mouse',[40,40]);distance=(min(abs(mouse[0]-22),80-abs(mouse[0]-22))+abs(mouse[1]-38))&~1
    if s.get('whole') or distance<=2:return done()
    drawflags=0x403
  else:
   if not flags&0x200040 or not s.get('unit',True) or s.get('unit_count',1)<=0:return done()
   index=0;drawflags=3
  if not editor and not (owner==display and not exceptional) and not seen&(1<<display):return done()
  tag=s.get('tag',owner if s.get('base') else 15)
  if s.get('tile_base',s.get('base')) and 0<=tag<8:
   actionable=owner==display and not exceptional and (bool(status&0x4200) or order not in [1,2,3])
   if not (control and selected==index and actionable and not status&0x4200 and pending):return done()
 if index<0:return done()
 if drawflags&1:
  result=index;best=-99
  for candidate in ([0,1] if s.get('stack') else [0]):
   cowner=owner if candidate==0 else s.get('second_owner',owner)
   if candidate==d4:continue
   if candidate==selected and cowner==display and not status&0x4200 and control and pending:result=candidate;break
   rank=(0 if candidate==0 else s.get('second_domain',1))+(4 if cowner==display else 0)
   if rank>best:best=rank;result=candidate
  index=result
  if d4>=0 and index==d4:return done()
 return done([(index,drawflags)])
