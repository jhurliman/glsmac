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
void Normalize(SmacCitizenCounts& c,int32_t capacity) {NormalizeSmacCitizenCounts(c,capacity);}
void Bound(SmacCitizenCounts& c,int32_t p,int32_t x) {c.talents=Clamp(c.talents,p);c.drones=Clamp(c.drones,p);Normalize(c,p-x);}
}
void NormalizeSmacCitizenCounts(SmacCitizenCounts& c,int32_t capacity,const std::function<void(uint8_t,int32_t)>& observer) {
    if(capacity<0 || c.talents<0 || c.drones<0 || c.extra_drone_steps<0) {throw std::invalid_argument("Citizen normalization requires nonnegative counts and capacity");}
    const auto excess=[&](){return int64_t(c.talents)+c.drones+c.extra_drone_steps-capacity;};
    const auto paired=[&](int32_t& burden,uint8_t branch) {
        if(excess()<=0) {return;}
        const auto n=int32_t(std::min({int64_t(c.talents),int64_t(burden),(excess()+1)/2}));
        if(observer) {for(int32_t i=0;i<n;++i) {--c.talents;observer(0,c.talents);--burden;observer(branch,burden);}}
        else {c.talents-=n;burden-=n;}
    };
    paired(c.extra_drone_steps,1);paired(c.drones,2);
    if(excess()>0 && c.talents) {
        const auto n=int32_t(std::min(int64_t(c.talents),excess()));
        if(observer) {for(int32_t i=0;i<n;++i) {--c.talents;observer(0,c.talents);}}else {c.talents-=n;}
    }
    if(excess()>0) {
        const auto n=std::max(0,c.drones-capacity);
        if(observer) {for(int32_t i=0;i<n;++i) {--c.drones;observer(3,c.drones);}}else {c.drones-=n;}
        if(excess()>0) {c.extra_drone_steps=std::min(c.extra_drone_steps,c.drones);if(observer) {observer(4,c.extra_drone_steps);}}
    }
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
    Population(p,x);if(pacifism<0 || c.talents<0 || c.drones<0 || c.extra_drone_steps<0) {throw std::invalid_argument("Police requires nonnegative citizen counts");}
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
bool SmacCitizenCalculation::operator==(const SmacCitizenCalculation& b) const {
    return population==b.population && excluded==b.excluded && bureaucracy==b.bureaucracy && social_talent==b.social_talent
        && psych_points==b.psych_points && police==b.police && facility_drones==b.facility_drones && paradise_talents==b.paradise_talents
        && prefix.content_allowance==b.prefix.content_allowance && prefix.ordinary==b.prefix.ordinary && prefix.assimilation==b.prefix.assimilation
        && prefix.after_faction==b.prefix.after_faction && prefix.faction_talents==b.prefix.faction_talents
        && early.initial==b.early.initial && early.psych==b.early.psych && early.facilities==b.early.facilities && after_police==b.after_police && final==b.final;
}
SmacCitizenStatus CalculateSmacCitizenStatus(int32_t turn,int32_t p,SmacCitizenCounts c,bool golden,bool riots,bool matrix,uint8_t nerve) {
    Population(p);if(turn<0 || c.talents<0 || c.drones<0) {throw std::invalid_argument("Invalid citizen status input");}
    SmacCitizenStatus r{};r.golden=golden;r.riots=riots;if(turn<2) {return r;}r.admitted=true;
    r.golden=p>=3 && c.drones==0 && c.talents>=(p+1)/2;r.golden_start=r.golden && !golden;r.golden_over=!r.golden && golden && c.drones<=c.talents;
    r.riot_dispatch=c.drones>c.talents && !matrix && !nerve;r.riot_new=r.riot_dispatch && !riots;r.riots=r.riot_dispatch;
    r.riots_over=riots && !r.riots && !r.golden;r.map_refresh=riots && !r.riots;return r;
}
SmacRiotFacilityResult CalculateSmacRiotFacilityLoss(bool ongoing,int32_t police,int32_t population,
    int32_t owner_bases,uint8_t assimilation,int32_t difficulty,const std::array<bool,39>& facilities,
    const std::array<bool,39>& grants,const std::function<int32_t()>& retained_rank,const std::function<int32_t()>& draw) {
    if(population<0 || population>127 || owner_bases<0 || difficulty<0 || difficulty>5 || !draw || !retained_rank) {
        throw std::invalid_argument("Riot search requires resolved ordinary Base inputs");
    }
    SmacRiotFacilityResult result;
    const auto next=[&]() {const auto value=draw();if(value<0 || value>32767) {throw std::runtime_error("Riot search requires raw CRT15 output");}++result.draws;return value;};
    if(!ongoing) {return result;}
    if(police>-6 && next()%(int64_t(police)+7)!=0) {return result;}
    if(facilities[1] || owner_bases<=1 || assimilation>=45) {return result;}
    if(population<4 && next()%3!=0) {return result;}
    const auto rank=retained_rank();
    if(rank<1 || rank>7) {throw std::invalid_argument("Riot search requires retained active faction rank");}
    const auto limit=population+20-difficulty-rank;
    for(int32_t i=0;i<=limit;++i) {
        const auto candidate=next()%39;++result.attempts;
        if(candidate>=2 && candidate<=33 && facilities[candidate] && !grants[candidate]) {
            result.removed=candidate;return result;
        }
    }
    result.exhausted=true;return result;
}

SmacRiotRecipient CalculateSmacRiotRecipient(int32_t police,bool human,int32_t owner,int32_t former,
    uint32_t stamp,int32_t x,int32_t y,int32_t width,bool wraps,const std::vector<SmacRiotBase>& bases,
    const std::array<SmacRiotRelationship,8>& relationships) {
    if(owner<1 || owner>7 || former<0 || former>7 || width<2 || width%2 || x<0 || x>=width || y<0) {
        throw std::invalid_argument("Riot recipient requires resolved faction and map inputs");
    }
    SmacRiotRecipient result;
    if(police>-3 || !human) {return result;}
    if(former!=owner) {result.selected=former;return result;}
    int32_t best=9999;
    for(const auto& base:bases) {
        if(base.owner<0 || base.owner>7 || base.x<0 || base.x>=width || base.y<0) {throw std::invalid_argument("Invalid riot recipient Base record");}
        if(base.owner==owner) {continue;}
        auto dx=std::abs(int64_t(x)-base.x);const auto dy=std::abs(int64_t(y)-base.y);
        if(wraps) {dx=std::min(dx,int64_t(width)-dx);}
        auto score=(3*std::max(dx,dy)+std::min(dx,dy)+1)/4;
        const auto relation=result.selected ? relationships.at(*result.selected):SmacRiotRelationship{bool(stamp&1u),bool(stamp&2u),bool(stamp&4u)};
        if(relation.pact) {score*=2;}
        if(relation.treaty) {score=score*3/2;}
        if(relation.half_distance) {score/=2;}
        const auto value=Checked(score);
        result.candidates.push_back({base.record_position,base.owner,result.selected,value});
        if(value<best) {best=value;result.selected=base.owner;}
    }
    return result;
}

}
