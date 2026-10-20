#include "SmacOpeningDesigns.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::unit {
namespace {
void Owner(int32_t owner) {if(owner<1 || owner>7) {throw std::invalid_argument("Opening designs require positive owner");}}
uint16_t Flags(const SmacOriginalDesign& d) {return uint16_t(d[48])|(uint16_t(d[49])<<8);}
void Store(SmacOriginalDesignPool& pool,size_t index,uint32_t offset,uint8_t size,uint32_t value,uint32_t pc,const SmacDesignObserver& observer) {
    for(unsigned k=0;k<size;++k) {pool[index][offset+k]=uint8_t(value>>(8*k));}
    if(observer) {observer({SmacDesignStore::Space::Designs,uint32_t(52*index)+offset,size,value,pc});}
}
}
int32_t ConstructSmacOpeningDesign(SmacOriginalDesignPool& designs,int32_t owner,int32_t chassis,int32_t armor,
    int32_t difficulty,const std::string& name,int32_t& first,bool allow_first_defender,const SmacDesignObserver& observer) {
    Owner(owner);
    if(!((chassis==0 && (armor==0 || armor==1)) || (chassis==1 && armor==0))) {throw std::invalid_argument("Opening constructor requires measured early components");}
    size_t count=0;for(size_t i=64*owner;i<size_t(64*owner+64);++i) {if(Flags(designs[i])&1) {++count;}}
    const auto& prior=designs[64*owner];
    if(count && !(allow_first_defender && count==1 && (Flags(prior)&1) && chassis==1 && armor==0
        && prior[36]==0 && prior[37]==0 && prior[38]==1 && prior[39]==1 && prior[42]==2)) {
        throw std::invalid_argument("Opening constructor excludes existing own designs");
    }
    if(chassis==0 && armor==0) {return 2;}
    const size_t index=64*owner+count;
    const auto put=[&](uint32_t off,uint8_t size,uint32_t value,uint32_t pc){Store(designs,index,off,size,value,pc,observer);};
    put(39,1,1,0x5bd6cb);put(36,1,chassis,0x5bd6d1);put(37,1,0,0x5bd6d7);put(38,1,armor,0x5bd6de);
    put(32,4,0,0x5bd6e5);put(41,1,2,0x5bd6f0);put(40,1,0,0x5bd729);put(42,1,0,0x5bd7f0);
    if(armor) {put(42,1,2,0x5bd809);}put(43,1,3,0x5bd897);
    if(armor) {put(43,1,2,0x5bd8c0);}else {put(43,1,4,0x5bd90d);put(43,1,10,0x5bd95c);put(42,1,3,0x5bd978);}
    put(48,2,1,0x5bd99b);put(44,1,0,0x5bd9c1);put(45,1,1u<<owner,0x5bd9e5);put(46,1,255,0x5bd9eb);
    if(difficulty<=1) {put(48,2,5,0x597b17);}
    put(48,1,designs[index][48]|0x40,0x597b22);put(48,1,designs[index][48]|0x80,0x597b29);put(43,1,armor?2:4,0x597b37);
    std::fill_n(designs[index].begin(),32,0);const auto length=std::min<size_t>(31,name.find(char(0)));
    std::copy_n(name.begin(),std::min(length,name.size()),designs[index].begin());
    put(48,1,designs[index][48]&0xfd,0x597c5a);
    if(first<0) {first=int32_t(index);if(observer) {observer({SmacDesignStore::Space::Global,0,4,uint32_t(first),0x597d58});}}
    return int32_t(index);
}
int32_t MarkAndSelectSmacStartingScout(SmacOriginalDesignPool& designs,int32_t owner,const SmacDesignObserver& observer) {
    Owner(owner);int32_t scout=2;
    for(size_t i=64*owner;i<size_t(64*owner+64);++i) {
        const auto flags=Flags(designs[i]);if(!(flags&1)) {continue;}
        Store(designs,i,48,2,flags|4,0x5c9eb8,observer);if(designs[i][42]==3) {scout=int32_t(i);}
    }
    return scout;
}
int32_t GenerateSmacOpeningDesigns(std::array<faction::SmacStartupFaction,8>& actors,SmacOriginalDesignPool& designs,
    const faction::SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const faction::SmacCapabilityCatalog& catalog,int32_t owner,int32_t difficulty,int32_t& first,
    const SmacOpeningDesignName& name,const SmacDesignObserver& observer) {
    Owner(owner);
    for(size_t t=0;t<ownership.size();++t) {if((ownership[t]&(1u<<owner)) && !((t<=6 && t!=3) || t==43)) {throw std::invalid_argument("Opening designs exclude acquired advanced technologies");}}
    for(size_t i=64*owner;i<size_t(64*owner+64);++i) {if(Flags(designs[i])&1) {throw std::invalid_argument("Opening generator requires pristine own design block");}}
    const auto tech=[&](int32_t t){return faction::QuerySmacTechnology(owner,t,t>=0 && t<89?ownership[t]:0,t>=0 && t<89?prerequisites[t]:std::array<int32_t,2>{{-1,-1}});};
    const bool defender=tech(catalog.armor[1].prerequisite),recon=tech(catalog.chassis[1].prerequisite);
    if((defender || recon) && !name) {throw std::invalid_argument("Opening generator requires localized design names");}
    first=-1;if(observer) {observer({SmacDesignStore::Space::Global,0,4,UINT32_MAX,0x597df8});}
    faction::RefreshSmacStartupCapabilities(actors,ownership,prerequisites,catalog,{owner,0x1a0e,0,3,2,-1,-1},[&](const auto& s){
        if(observer) {observer({SmacDesignStore::Space::Factions,s.offset,s.size,s.value,s.caller});}});
    if(defender) {ConstructSmacOpeningDesign(designs,owner,0,1,difficulty,name(owner,0,1),first,true,observer);}
    if(recon) {ConstructSmacOpeningDesign(designs,owner,1,0,difficulty,name(owner,1,0),first,true,observer);}
    return first;
}
}
