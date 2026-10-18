#include "SmacCitizens.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <stdexcept>
namespace game::backend::base {
namespace {
int32_t Checked(int64_t n) {if(n<INT32_MIN || n>INT32_MAX) {throw std::overflow_error("Citizen calculation exceeds signed32");}return static_cast<int32_t>(n);}
void Population(int32_t p,int32_t x=0) {if(p<0 || p>127 || x<0 || x>p) {throw std::invalid_argument("Citizen population or excluded count is outside measured bounds");}}
void Difficulty(int32_t d) {if(d<0 || d>5) {throw std::invalid_argument("Citizen difficulty is outside ordinary rules");}}
int32_t Clamp(int64_t n,int32_t p) {return static_cast<int32_t>(std::clamp<int64_t>(n,0,p));}
void Normalize(SmacCitizenCounts& c,int32_t capacity) {
    if(c.talents<0 || c.drones<0 || c.extra_drone_steps<0) {throw std::invalid_argument("Citizen normalization requires nonnegative counts");}
    const auto paired=[&](int32_t& burden) {
        const int64_t excess=int64_t(c.talents)+c.drones+c.extra_drone_steps-capacity;
        if(excess>0) {const auto n=static_cast<int32_t>(std::min({int64_t(c.talents),int64_t(burden),(excess+1)/2}));c.talents-=n;burden-=n;}
    };
    paired(c.extra_drone_steps);paired(c.drones);
    const auto excess=int64_t(c.talents)+c.drones+c.extra_drone_steps-capacity;
    if(excess>0 && c.talents) {c.talents-=static_cast<int32_t>(std::min(int64_t(c.talents),excess));}
    if(int64_t(c.talents)+c.drones+c.extra_drone_steps>capacity) {
        c.drones=std::min(c.drones,capacity);
        c.extra_drone_steps=std::min(c.extra_drone_steps,c.drones);
    }
}
void Bound(SmacCitizenCounts& c,int32_t p,int32_t x) {c.talents=Clamp(c.talents,p);c.drones=Clamp(c.drones,p);Normalize(c,p-x);}
}
SmacCitizenPrefix CalculateSmacCitizenPrefix(int32_t p,bool human,int32_t difficulty,int32_t drone,int32_t talent,uint8_t timer) {
    Population(p);Difficulty(difficulty);const auto d=human?difficulty:3;
    SmacCitizenPrefix r{6-d,Clamp(int64_t(p)-(6-d),p),0,0,0};
    r.assimilation=std::min(std::max((p+d-2)/4,0),(int32_t(timer)+9)/10);
    r.after_faction=Checked(int64_t(r.ordinary)+r.assimilation+(drone?int64_t(p)/drone:0));
    r.faction_talents=talent?Checked((int64_t(p)+talent-1)/talent):0;return r;
}
SmacBureaucracyInput CalculateSmacCitizenBureaucracy(int32_t p,bool human,int32_t difficulty,int32_t efficiency,int32_t width,int32_t height,int32_t bases,int32_t position) {
    Population(p);Difficulty(difficulty);if(width<0 || height<0 || bases<0 || position<0) {throw std::invalid_argument("Invalid citizen bureaucracy map or list input");}
    const auto tiles=Checked(int64_t(width/2)*height);int32_t root=static_cast<int32_t>(std::sqrt(tiles));
    while(int64_t(root+1)*(root+1)<=tiles) {++root;}while(int64_t(root)*root>tiles) {--root;}
    const auto product=Checked(int64_t(8-(human?difficulty:3))*(4LL+std::max(efficiency,0))*root);
    SmacBureaucracyInput r{product/112,0};if(r.interval) {r.drones=Clamp((int64_t(bases)+position%r.interval-r.interval)/r.interval,p);}return r;
}
SmacCitizenFacilityInputs CalculateSmacCitizenFacilityInputs(int32_t p,const std::array<bool,34>& f,int32_t genejack,bool virtual_world,bool transit,bool genome,bool immortality) {
    Population(p);if(f[0]) {throw std::invalid_argument("Citizen facility index zero is invalid");}
    const auto drones=Checked((f[22]?int64_t(genejack):0)-(f[6]?2:0)-((f[11] || (f[8] && virtual_world))?2:0)
        -((transit && p<=3)?1:0)-(f[17]?1:0)-(f[18]?1:0));
    return {drones,f[12]?2:0,(genome?1:0)+(immortality?2:0)};
}
SmacCitizenEarlyStages ConvertSmacCitizens(const SmacCitizenConversion& i) {
    Population(i.population,i.excluded);if(i.ordinary<0 || i.bureaucracy<0 || i.faction_talents<0) {throw std::invalid_argument("Negative custom citizen inputs need separate conversion authority");}
    const auto q=Checked(int64_t(Clamp(int64_t(i.ordinary)-std::min(i.social_talent,0),i.population))+i.bureaucracy);
    SmacCitizenCounts c{Checked(int64_t(i.faction_talents)+std::max(i.social_talent,0)),Clamp(q,i.population),Clamp(int64_t(q)-i.population,i.population)};
    Normalize(c,i.population-i.excluded);SmacCitizenEarlyStages r;r.initial=c;
    c.talents=Clamp(int64_t(c.talents)+i.psych/2,i.population);c.drones=Clamp(c.drones,i.population);Normalize(c,i.population-i.excluded);r.psych=c;
    c.drones=Clamp(int64_t(c.drones)+i.facility_drones,i.population);c.talents=Clamp(int64_t(c.talents)+i.paradise_talents,i.population);Normalize(c,i.population-i.excluded);r.facilities=c;return r;
}
SmacCitizenPoliceStages PoliceSmacCitizens(SmacCitizenCounts c,int32_t p,int32_t x,int32_t police,int32_t pacifism,bool self_aware,const std::vector<SmacCitizenPoliceUnit>& units) {
    Population(p,x);if(pacifism<0) {throw std::invalid_argument("Negative pacifism count");}
    int slots=police<=-2?0:police<=0?1:police==1?2:3;const int strength=police>=3?2:1;
    const auto apply=[&](int reduction) {c.drones=std::max(0,c.drones-reduction);--slots;};
    if(self_aware && slots) {apply(strength);}
    for(bool nonlethal:{true,false}) {for(const auto& u:units) {if(slots && u.package_mode<=3 && u.nonlethal==nonlethal) {apply(strength+(nonlethal?1:0));}}}
    SmacCitizenPoliceStages r;r.garrison=c;
    const int64_t demand=police>=-2?0:police==-3?std::max(0,pacifism-1):police==-4?pacifism:2LL*pacifism;
    c.drones=Checked(int64_t(c.drones)+demand);r.pacifism=c;Bound(c,p,x);r.normalized=c;return r;
}
SmacCitizenFinalStages FinishSmacCitizens(SmacCitizenCounts c,int32_t p,int32_t x,int32_t talents,bool longevity,int32_t economics,bool sphere,uint8_t nerve) {
    Population(p,x);c.talents=Checked(int64_t(c.talents)+talents);if(longevity) {c.drones=Checked(int64_t(c.drones)-(economics==1?0:economics==2?2:1));}
    SmacCitizenFinalStages r;r.projects=c;if(sphere || nerve) {c.talents=c.drones=0;}r.suppressed=c;Bound(c,p,x);r.final=c;return r;
}
SmacCitizenStatus CalculateSmacCitizenStatus(int32_t turn,int32_t p,SmacCitizenCounts c,bool golden,bool riots,bool matrix,uint8_t nerve) {
    Population(p);if(turn<0 || c.talents<0 || c.drones<0) {throw std::invalid_argument("Invalid citizen status input");}
    SmacCitizenStatus r{};r.golden=golden;r.riots=riots;if(turn<2) {return r;}r.admitted=true;
    r.golden=p>=3 && c.drones==0 && c.talents>=(p+1)/2;r.golden_start=r.golden && !golden;r.golden_over=!r.golden && golden && c.drones<=c.talents;
    r.riot_dispatch=c.drones>c.talents && !matrix && !nerve;r.riot_new=r.riot_dispatch && !riots;r.riots=r.riot_dispatch;
    r.riots_over=riots && !r.riots && !r.golden;r.map_refresh=riots && !r.riots;return r;
}
}
