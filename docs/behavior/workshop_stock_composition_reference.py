"""Source-derived stock SMAC composer operations, before CVR assembly/raster.

Inputs are installed component definitions, owner, preview and phase. Offsets
are symbolic producer identities, never captured camera centers. phase is the
native nonnegative animation counter. Material rows are zero-based.
"""
def op(asset,kind='copy',material=None,frame=0,offset=None):
    return dict(asset=asset.lower(),kind=kind,material=material,frame=frame,offset=offset)

def body(owner,ch,we,ar,re,weapon_mode,armor_strength,phase=0):
    owner=owner if 0<=owner<=7 else 0
    tint='owner:'+str(owner)
    tier=1 if ar in [1,2,3,5,6,7] else 2 if ar in [4,8] else 0
    if we==18 and ch==1 and armor_strength<=1:ch=0
    C=lambda a:op(a)
    T=lambda a:op(a,material=tint)
    base=[C('VB.CVR'),T('VBP.CVR')]
    gun=[C('VGMC.CVR'),T('VGMCP.CVR')]
    reactor=[] if not re else [C('VRCP%02d.cvr'%(re-1))]
    if ch==0:
        if weapon_mode<3:return [] if we==14 else [op('VI.CVR','attach',frame=phase%15,offset='infantry')]
        if we==18:return [op('VPT.CVR','attach',frame=phase%15,offset='infantry')]
        return [] if we==19 else base+[C('VCL.cvr'),C('VCLT00.CVR'),C('VLIGHTS.CVR')]
    if ch==1:return reactor+gun+base+[C('VSP.cvr'),op('vsptb.cvr',frame=phase),op('vsptf.cvr',frame=phase),C('VLIGHTS.CVR')]
    if ch==2:return [C('VB.CVR'),T('vht-vbp.cvr'),T('vhtp.cvr'),T('vhttp.cvr')]+([] if tier else [C('vhta00.cvr'),C('vhtta00.cvr')])+[op('VLIGHTS.CVR','attach',offset='hover_lights')]
    if ch==3:return reactor+gun+base+[C('VFL.cvr')]
    if ch==4:return base+gun+[C('VCU.cvr'),T('VCUP.CVR'),C('VCUW.CVR')]+([] if tier else [C('VCUA00.cvr')])
    if ch==5:return reactor+base+gun+[T('VJTP.CVR')]+([] if tier else [C('VJT00.cvr')])
    if ch==6:return reactor+base+gun+[C('vct.cvr'),T('VCTP.CVR'),op('VCTB.CVR',frame=phase%15)]+([] if tier else [T('vct00.cvr')])
    if ch==7:return reactor+base+gun+[C('VGS.cvr'),T('VGSP.CVR')]
    if ch==8:return [C('vw13.cvr' if we==13 else 'VM.CVR')]+base
    return []

def weapon(owner,ch,we,power,mode):
    if ch==8:return []
    if we in [13,20]:return [op('vw13.cvr' if we==13 else 'VM.CVR')]
    if mode<3:
        lookup=[-1,-1,0,0,1,3,2,2,4,4,5,5,6,7,7,7,8,8,8,8,9,9,9,9,10]
        n=11 if power==-1 else 12 if power==99 else lookup[max(0,min(power,24))]
        if n<0:return []
        asset='VW%02d.CVR'%(n+1)
        if ch==2:return [op(asset,'attach',offset='hover_gun')]
        if ch!=0:return [op(asset)]
        owner=owner if 0<=owner<=7 else 0
        return [op(asset,'attach',offset='infantry_gun'),op('VGMT.CVR'),op('VGMTP.CVR',material='owner:'+str(owner))]
    if mode==8:return [op('drop.cvr'),op('droplet.cvr')] if ch==0 else [op('droplet.cvr','attach',offset='colony')]
    if mode==11:return [] if ch==0 else [op('ptmod.cvr')]
    if mode==9:return ([] if ch else [op('vt.cvr')])+[op('vwntu.cvr')]
    return [op({7:'VWNTT.CVR',10:'VWNST.CVR',12:'VWNAA.CVR'}[mode])] if mode in [7,10,12] else []

def armor(ch,we,ar,preview,weapon_mode,phase=0):
    if ar not in range(10):ar=0
    tint={2:0,3:1,6:2,7:3}.get(ar)
    ordinary=ar in [1,2,3,6,7] or ar==5 and preview!=0
    structure=ar in [4,8,9] or ar==5 and preview==0
    if ar==5 and preview!=0:tint=4
    material=None if tint is None else 'armor:'+str(tint)
    result=[]
    infantry=ch==0 and (weapon_mode<3 or weapon_mode==11)
    if ar in [4,8,9]:
        if infantry:result=[op({4:'viptawal.cvr',8:'viptasgn.cvr',9:'viptapsi.cvr'}[ar],'attach',frame=phase,offset='infantry')]
        else:result=[op('a.cvr'),op({4:'apwall.cvr',8:'asgen.cvr',9:'apsid.cvr'}[ar])]
    if ch==0:
        if ordinary:
            result += [op('vipta00.cvr','attach',material,phase%15,'infantry')] if we!=-1 and (weapon_mode<3 or weapon_mode==11) else [op('VA01.cvr',material=material)]
    elif ch==1 and ordinary:result += [op('VA01.cvr',material=material),op('VSPA01.CVR',material=material)]
    elif ch==2:
        result += [op('vhta00.cvr'),op('vhtta00.cvr')] if structure else [op('vhta01.cvr',material=material),op('vhtta01.cvr',material=material)] if ordinary else []
    elif ch in [-1,3,7] and ordinary:result += [op('VA01.cvr',material=material)]
    elif ch==4:
        result += [op('VCUA00.cvr')] if structure and ar!=5 else [op('VCUA01.cvr',material=material)] if ordinary else []
    elif ch==5:
        result += [op('VJT00.cvr')] if structure and ar!=5 else [op('VA01.cvr',material=material),op('VJT01.cvr',material=material)] if ordinary else []
    elif ch==6:
        result += [op('vct00.cvr'),op('VA01.cvr')] if structure and ar!=5 else [op('vct01.cvr',material=material),op('VA01.cvr',material=material)] if ordinary else []
    return result

def reactor(ch,we,re,abilities,weapon_mode):
    if not re:return []
    if ch==0 and (weapon_mode<3 or weapon_mode==11):return [op('viptr%02d.cvr'%(re-1),'attach',offset='infantry')]
    if ch==8:return [op('vpbr%02d.cvr'%(re-1))]
    return [op('VR%02d.cvr'%(re-1))]+([op('VHR.CVR')] if abilities&0x8000 else [])
