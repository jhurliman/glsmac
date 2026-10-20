#include "SmacPredefinedDesigns.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t Checked(int64_t value) {if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Design arithmetic exceeds measured signed32");}return int32_t(value);}
void Put(SmacOriginalDesignPool& pool,size_t index,uint32_t off,uint8_t size,uint32_t value,uint32_t pc,const SmacDesignObserver& observer) {
    for(unsigned k=0;k<size;++k) {pool.at(index).at(off+k)=uint8_t(value>>(8*k));}
    if(observer) {observer({SmacDesignStore::Space::Designs,uint32_t(index*52)+off,size,value&uint32_t(size==4?UINT32_MAX:(1u<<(8*size))-1),pc});}
}
int32_t Cost(const SmacDesignComponents& c,int32_t ch,int32_t we,int32_t ar,uint32_t mask) {
    const auto& chassis=c.chassis.at(ch);const auto& weapon=c.weapons.at(we);const auto& armor=c.armor.at(ar);
    int32_t w=weapon.cost,a=armor.cost,speed=chassis.cost,penalty=0;
    if(w<1 || a<1 || speed<1 || chassis.domain<0 || chassis.domain>2 || mask>>24) {throw std::invalid_argument("Design cost requires admitted components");}
    for(unsigned j=0;j<24;++j) {if(!(mask&(1u<<j))) {continue;}if(penalty) {penalty=Checked(int64_t(penalty)+1);}
        const auto cost=c.abilities[j].cost;int64_t extra;
        if(cost>=0) {extra=cost;}else {switch(cost) {
            case -1:extra=std::clamp(w/a,0,2);break;case -2:extra=int64_t(w)-1;break;case -3:extra=int64_t(a)-1;break;
            case -4:extra=int64_t(speed)-1;break;case -5:extra=int64_t(w)+a-2;break;case -6:extra=int64_t(w)+speed-2;break;
            case -7:extra=int64_t(a)+speed-2;break;default:throw std::invalid_argument("Unclosed design ability cost");}}
        penalty=Checked(int64_t(penalty)+extra);
    }
    if(chassis.domain==1) {a/=2;speed=Checked(int64_t(speed)+1);}
    else if(chassis.domain==2) {speed=Checked(int64_t(speed)+2);if(a>1) {a=Checked(int64_t(a)*2);}}
    else {for(unsigned j=0;j<24;++j) {if((mask&(1u<<j)) && (c.abilities[j].flags&1024)) {penalty=Checked(int64_t(penalty)+1);}}}
    if(chassis.missile && weapon.power>=99) {return weapon.cost;}
    w=std::max(w,a/2+1);auto cost=Checked(Checked((int64_t(speed)+a)*w)+int64_t(2))/4;
    if(speed==1) {cost=cost/2+1;}if(weapon.cost>1 && armor.cost>1) {cost=Checked(int64_t(cost)+1);}
    if(chassis.domain==0 && weapon.cost>1 && armor.cost>1 && speed>1) {cost=Checked(int64_t(cost)+1);}
    if(chassis.domain==1 && weapon.mode!=11) {cost=Checked(int64_t(cost)+1)/2;}
    if(chassis.domain==2) {cost/=weapon.mode>2?2:4;}
    cost=std::max(cost,2);if(w==1 && a==1 && speed==1) {cost=1;}
    return Checked(Checked((int64_t(penalty)+4)*cost)+int64_t(2))/4;
}
}
SmacDesignComponents SmacEarlyFissionComponents() {
    SmacDesignComponents c{};c.chassis[0]={1,0,false,1,1};c.chassis[1]={2,0,false,1,2};
    c.weapons[0]={1,0,1};c.armor[0]={1,1};c.armor[1]={2,2};return c;
}
void BuildSmacFissionDesign(SmacOriginalDesignPool& pool,size_t index,const SmacDesignComponents& c,int32_t ch,int32_t we,int32_t ar,
    uint32_t mask,uint8_t owner_mask,const SmacDesignObserver& observer) {
    const auto& chassis=c.chassis.at(ch);const auto& weapon=c.weapons.at(we);const auto& armor=c.armor.at(ar);
    const auto cost=Cost(c,ch,we,ar,mask);const auto put=[&](uint32_t off,uint8_t size,uint32_t value,uint32_t pc){Put(pool,index,off,size,value,pc,observer);};
    put(39,1,1,0x5bd6cb);put(36,1,ch,0x5bd6d1);put(37,1,we,0x5bd6d7);put(38,1,ar,0x5bd6de);put(32,4,mask,0x5bd6e5);put(41,1,cost,0x5bd6f0);
    int32_t carry=0;if(weapon.mode==7) {carry=chassis.cargo;if(mask&0x800000) {carry/=2;}if(mask&0x80000) {carry=Checked(3*int64_t(carry)+1)/2;}}put(40,1,carry,0x5bd729);
    if(chassis.missile) {put(42,1,weapon.power>=99?5:0,weapon.power>=99?0x5bd747:0x5bd75a);put(43,1,weapon.power>=99?14:13,weapon.power>=99?0x5bd74e:0x5bd761);}
    else if(weapon.mode>=7) {put(42,1,weapon.mode,0x5bd777);put(43,1,uint32_t(Checked(int64_t(weapon.mode)+32)),0x5bd77f);}
    else if(chassis.domain==1) {put(42,1,6,0x5bd7a4);put(43,1,6,0x5bd7aa);}
    else if(chassis.domain==2) {put(42,1,(mask&32)?4:0,(mask&32)?0x5bd7c8:0x5bd7db);put(43,1,(mask&32)?9:8,(mask&32)?0x5bd7cf:0x5bd7e2);}
    else {
        int plan=0;put(42,1,0,0x5bd7f0);if(armor.rating>1) {plan=2;put(42,1,2,0x5bd809);}
        if(weapon.power>=armor.rating && plan==2) {plan=1;put(42,1,1,0x5bd83b);}
        if(chassis.speed>1 && weapon.power>armor.rating) {put(42,1,0,0x5bd87d);}if(mask&0x8018) {put(42,1,0,0x5bd88e);}
        put(43,1,3,0x5bd897);if(armor.rating>weapon.power) {put(43,1,2,0x5bd8c0);}
        if(int64_t(weapon.power)>2*int64_t(armor.rating)) {put(43,1,1,0x5bd8f0);}if(chassis.speed>=2) {put(43,1,4,0x5bd90d);}
        if(chassis.speed>=3) {put(43,1,5,0x5bd92b);}if(weapon.power==1 && armor.rating==1) {
            put(43,1,10,0x5bd95c);if(chassis.speed>1) {put(42,1,3,0x5bd978);}if(mask&0x400000) {put(42,1,2,0x5bd991);}}
    }
    put(48,2,1,0x5bd99b);put(44,1,0,0x5bd9c1);put(45,1,owner_mask,0x5bd9e5);put(46,1,255,0x5bd9eb);
}
void LoadSmacPredefinedDesigns(SmacOriginalDesignPool& pool,const SmacDesignComponents& c,const std::vector<SmacPredefinedDesign>& rows,const SmacDesignObserver& observer) {
    if(rows.size()!=14) {throw std::invalid_argument("Predefined startup requires the admitted fourteen-row catalog");}
    for(size_t i=0;i<rows.size();++i) {const auto& r=rows[i];if(r.name.size()>31 || r.name.find(char(0))!=std::string::npos || r.prerequisite<-2 || r.prerequisite>88) {throw std::invalid_argument("Invalid predefined design source row");}
        std::copy(r.name.begin(),r.name.end(),pool[i].begin());pool[i][r.name.size()]=0;
        Put(pool,i,50,2,uint32_t(r.prerequisite),0x59e09e,observer);BuildSmacFissionDesign(pool,i,c,r.chassis,r.weapon,r.armor,r.abilities,255,observer);
        if(r.plan>=0) {Put(pool,i,42,1,uint32_t(r.plan),0x59e0cb,observer);}if(r.cost) {Put(pool,i,41,1,uint32_t(r.cost),0x59e0d6,observer);}
        if(r.capacity) {Put(pool,i,40,1,uint32_t(r.capacity),0x59e0e1,observer);}Put(pool,i,46,1,uint32_t(r.icon),0x59e0ec,observer);
    }
}
std::string NameSmacOpeningDesign(const SmacEarlyDesignNames& n,int32_t owner,int32_t chassis,int32_t armor) {
    if(owner<1 || owner>7 || !((chassis==0 && armor==1) || (chassis==1 && armor==0))) {throw std::invalid_argument("Localized opening name requires admitted early components");}
    if(chassis==1) {return n.scout+" "+n.rover;}
    const auto variant=(uint32_t(owner)&1u)^(uint32_t(n.armor_rating)&1u);return n.armor_short+" "+(variant?n.infantry_odd:n.infantry_even);
}
int32_t GenerateSmacInstalledOpeningDesigns(std::array<faction::SmacStartupFaction,8>& actors,SmacOriginalDesignPool& designs,
    const faction::SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const faction::SmacCapabilityCatalog& catalog,const SmacDesignComponents& components,const SmacEarlyDesignNames& names,
    int32_t owner,int32_t difficulty,int32_t& first,const SmacDesignObserver& observer) {
    auto effective=catalog;const auto& worm=designs[8];
    effective.mind_worm_flags=uint16_t(worm[48])|(uint16_t(worm[49])<<8);effective.mind_worm_exclusion=worm[44];effective.mind_worm_plan=worm[42];
    const auto prerequisite=uint32_t(worm[50])|(uint32_t(worm[51])<<8);effective.mind_worm_prerequisite=prerequisite<32768?int32_t(prerequisite):int32_t(prerequisite)-65536;
    return GenerateSmacOpeningDesigns(actors,designs,ownership,prerequisites,effective,owner,difficulty,first,
        [&](int32_t f,int32_t chassis,int32_t armor){return NameSmacOpeningDesign(names,f,chassis,armor);},observer,&components);
}
}
