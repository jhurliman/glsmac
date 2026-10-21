#include "SmacProductionDesigns.h"
#include "game/backend/faction/SmacAirSummary.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t Word(int64_t value) noexcept {const auto bits=uint32_t(value);return bits<=INT32_MAX ? int32_t(bits):int32_t(int64_t(bits)-INT64_C(4294967296));}
int32_t Byte(int32_t value) noexcept {const auto bits=uint32_t(value)&255u;return bits<128 ? int32_t(bits):int32_t(bits)-256;}
int32_t Prerequisite(const settings::SmacTechnologyReference& r) {
    using K=settings::SmacTechnologyReference::Kind;
    if(r.kind==K::NONE) {return -1;}if(r.kind==K::DISABLED) {return -2;}
    if(r.kind==K::TECHNOLOGY_ID && r.technology_id) {return *r.technology_id;}
    throw std::invalid_argument("Production technology definition is unresolved");
}
}
SmacCustomDesign ReadSmacProductionDesign(const SmacOriginalDesign& row,int32_t id) {
    if(id<0 || id>=512) {throw std::invalid_argument("Production record identity outside native pool");}
    SmacCustomDesign d;d.id=id;d.chassis=row[36];d.weapon=row[37];d.defense=row[38];d.reactor=row[39];
    d.carry=row[40];d.cost=row[41];d.plan=row[42];d.faction_exclusion=row[44];
    for(unsigned i=0;i<4;++i) {d.abilities|=uint32_t(row[32+i])<<(8*i);}
    const auto flags=uint16_t(row[48])|(uint16_t(row[49])<<8);d.active=(flags&1)!=0;d.explicitly_named=(flags&2)!=0;d.prototyped=(flags&4)!=0;
    d.auxiliary_flags=uint16_t(flags&~7u);d.name=std::string(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));return d;
}
bool HasSmacProductionDesignTechnology(const SmacOriginalDesign& row,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    const auto bits=uint32_t(row[50])|(uint32_t(row[51])<<8);const auto id=bits<32768 ? int32_t(bits):int32_t(bits)-65536;
    std::array<int32_t,2> parents{{-1,-1}};
    if(id>=0 && id<88) {const auto& definition=technologies.technologies.at(id);
        for(size_t i=0;i<2;++i) {parents[i]=Prerequisite(technologies.ResolveToken(definition.prerequisites[i]));}}
    return faction::QuerySmacTechnology(owner.admission.simulation_id,id,id>=0 && id<88 && owner.owned.at(id) ? uint8_t(1u<<owner.admission.simulation_id):0,parents);
}
int32_t SmacProductionArmorScore(int32_t armor,const SmacProductionDesignContext& context) {
    if(armor>=0 || context.owner<0) {return Word(int64_t(armor)*2);}
    if(!context.peer_attack || !context.land_psi_ratio) {throw std::invalid_argument("PSI production rank requires retained peer attack and land ratios");}
    const auto& ratio=*context.land_psi_ratio;
    const std::array<int32_t,2> reciprocal{{ratio[1],ratio[0]}};
    return Word(int64_t(faction::SmacAirAttackEstimate(armor,*context.peer_attack,reciprocal,context.psi_percent,context.neural_amplifier))*2);
}
SmacProductionDesignList BuildSmacProductionDesignList(const SmacOriginalDesignPool& pool,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const SmacProductionDesignContext& context) {
    if(context.owner<1 || context.owner>7 || context.owner!=owner.admission.simulation_id) {throw std::invalid_argument("Production rank requires its actual owner");}
    struct Entry {int32_t id,key;};std::array<Entry,128> entries;SmacProductionDesignList result;result.count=64;
    for(size_t i=0;i<128;++i) {
        const auto id=int32_t(i<64 ? i:context.owner*64+i-64);const auto& row=pool.at(id);
        const bool admitted=(row[48]&1) && (id>=64 || HasSmacProductionDesignTechnology(row,technologies,owner));
        if(id<64 && admitted) {++result.count;}
        int32_t key=id<64 ? 999999999:888888888;
        if(admitted) {
            const auto& chassis=catalog.chassis.chassis.at(row[36]);const auto attack=Byte(catalog.weapons.weapons.at(row[37]).offense);
            if(attack<0 && !context.peer_defense) {throw std::invalid_argument("PSI production rank requires retained peer defense");}
            const auto power=faction::SmacAirAttackEstimate(attack,context.peer_defense.value_or(0),context.land_psi_ratio,context.psi_percent,context.dream_twister);
            const auto armor=SmacProductionArmorScore(Byte(catalog.defenses.defenses.at(row[38]).rating),context);
            key=Word((int64_t(row[42])*16+int32_t(chassis.triad))*262144+(int64_t(32)-power)*512
                +(int64_t(8)-uint8_t(chassis.speed))*64+8-int64_t(armor));
        }
        entries[i]={id,key};
    }
    std::stable_sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.key<b.key;});
    for(size_t i=0;i<128;++i) {result.order[i]=entries[i].id;result.keys[i]=entries[i].key;}return result;
}
}
