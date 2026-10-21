"""Typed source-derived inputs for the human Workshop air-summary and PSI boundary."""
def trunc(a,b):
    assert b!=0
    return (abs(a)//abs(b))*(-1 if (a<0)!=(b<0) else 1)
def s32(a):return (a+2**31)%2**32-2**31

def psi_bonus(value,percent,project_owned):
    if percent:value=trunc(s32(value*(100+percent)),100)
    return s32(value+trunc(value,2)) if project_owned else value

def attack_estimate(offense,peer_defense,ratio_attack=3,ratio_defense=2,psi_percent=0,dream_twister_owned=False):
    if offense>=0:return offense
    return psi_bonus(trunc(s32(peer_defense*ratio_attack),ratio_defense),psi_percent,dream_twister_owned)

def armor_score(armor,owner,peer_attack,ratio_attack=3,ratio_defense=2,psi_percent=0,neural_amplifier_owned=False):
    if armor>=0 or owner<0:return s32(armor*2)
    return s32(2*psi_bonus(trunc(s32(peer_attack*ratio_defense),ratio_attack),psi_percent,neural_amplifier_owned))

def counters(owner,units,designs,chassis,relations,peer_defense,psi_percent=0,dream_twister_owned=False,ratio_attack=3,ratio_defense=2):
    """Units are live dense-scan records; Native units use their real design records."""
    foreign=own0=own4=0
    for unit in units:
        d=designs[unit['design']];ch=chassis[d['chassis']]
        if unit['owner']==owner:
            if ch['domain']==2:
                own0+=d['plan']==0;own4+=d['plan']==4
        else:
            rel=relations[unit['owner']]
            admitted=bool(rel&8) and (not(rel&3) or bool(rel&0xc20))
            if admitted and ch['domain']==2 and d['plan']==0 and ch['special']==0:
                foreign+=attack_estimate(d['offense'],peer_defense,ratio_attack,ratio_defense,psi_percent,dream_twister_owned)>=peer_defense
    return dict(foreign_air=foreign,own_air_plan0=own0,own_air_plan4=own4)
