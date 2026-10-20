#include "SmacGeneratedStartup.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include "game/backend/map/SmacGeneratedStartingSite.h"
#include "game/backend/map/SmacStartupTerritory.h"
#include "game/backend/map/SmacStartingBalance.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include "game/backend/unit/SmacUnitInitialization.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
template<class R> uint32_t Read(const R& r,size_t off) {uint32_t v=0;for(unsigned k=0;k<4;++k) {v|=uint32_t(r.at(off+k))<<(8*k);}return v;}
template<class R> void Write(R& r,size_t off,uint32_t value) {for(unsigned k=0;k<4;++k) {r.at(off+k)=uint8_t(value>>(8*k));}}
map::smac_site::Coordinate Position(const base::SmacStartupBaseRecord& b) {return {{int32_t(b[0])+(int32_t(b[1])<<8),int32_t(b[2])+(int32_t(b[3])<<8)}};}
}
SmacGeneratedStartup CompleteSmacGeneratedStartup(const map::smac_generation::State& input,
    const map::smac_generation::Regions& input_regions,map::smac_generation::Geometry geometry,
    const SmacGeneratedStartupRules& rules,const std::string& channel_prefix,const SmacGeneratedStartupObservers& observer) {
    namespace generation=map::smac_generation;
    constexpr std::array<const char*,7> identities{{"GAIANS","HIVE","UNIV","MORGAN","SPARTANS","BELIEVE","PEACE"}};
    if(input.stage!=generation::Stage::Prestartup || input.primary_rules!=0x1a0e || input.topology!=0
        || geometry.width!=80 || geometry.height!=80 || input.records.size()!=3200
        || input.categories!=generation::Categories{{2,1,1,1,1,1,1}}) {
        throw std::invalid_argument("Generated startup requires the admitted standard local SMAC geography");
    }
    for(size_t i=0;i<7;++i) {if(rules.factions[i].identity!=identities[i]) {throw std::invalid_argument("Generated startup requires stock faction identities in their original slots");}}
    SmacGeneratedStartup world{input,input_regions};auto& terrain=world.terrain;auto& regions=world.regions;
    const map::smac_site::Geometry g{geometry.width,geometry.height,true};
    unit::LoadSmacPredefinedDesigns(world.designs,rules.components,rules.predefined);
    unit::SmacStartupUnitFactoryState factory;factory.dirty=terrain.dirty_flags;
    const auto prefix=[&](int owner,const SmacStartupPersonality& definition) {
        InitializeSmacFactionPrefix(world.actors[owner],{owner,3,2,255,terrain.primary_rules,0,false,10,definition,channel_prefix," "},terrain.random,observer.random);
    };
    for(int owner=1;owner<=7;++owner) {
        const auto& definition=rules.factions[owner-1];auto& actor=world.actors[owner];prefix(owner,definition.personality);
        generation::StartingContext context{3,owner==1,false};
        for(size_t r=0;r<128;++r) {for(int f=1;f<8;++f) {if(world.actors[f].record[0x95e320-0x95d1a8+r]) {context.presence[r]|=uint8_t(1u<<f);}}}
        for(size_t i=0;i<world.bases.size();++i) {context.bases.push_back({int64_t(i),Position(world.bases[i])});}
        for(const auto& u:world.units) {context.units.push_back({u.owner,{{u.x,u.y}}});}
        const auto site=generation::SelectGeneratedStartingSite(terrain,geometry,regions,context,{},{},{},
            [&](uint32_t pc,uint32_t before,uint32_t after,uint16_t value){if(observer.random) {observer.random({pc,before,after,value});}});
        if(!site.site) {throw std::runtime_error("Generated faction site search exhausted");}
        const auto position=*site.site;const auto at=size_t(position[1])*40+size_t(position[0]/2);
        PrepareSmacStartingSite(actor,terrain.records,{g,position,owner,regions.Area(terrain.records[at][3]),3,terrain.primary_rules},terrain.random,observer.random);
        for(const auto technology:definition.named_technologies) {GrantSmacStartupNamedTechnology(actor,world.ownership,owner,technology);}
        int32_t first_new=-1;
        unit::GenerateSmacInstalledOpeningDesigns(world.actors,world.designs,world.ownership,rules.yields.prerequisites,
            rules.capabilities,rules.components,rules.design_names,owner,3,first_new);
        const auto scout_design=unit::MarkAndSelectSmacStartingScout(world.designs,owner);
        base::SmacBaseSeedCatalog catalog{};catalog.ownership=world.ownership;catalog.prerequisites=rules.yields.prerequisites;
        catalog.formers_prerequisite=rules.formers_technology;catalog.free_facility_technologies.fill(-2);catalog.facility_bonuses=definition.facilities;
        for(size_t i=0;i<3;++i) {catalog.design_costs[i]=world.designs[i][41];}
        std::array<uint8_t,128> name_buffer{};
        const auto name=base::GenerateSmacBaseName(actor,name_buffer,definition.names,owner,terrain.resource_pattern,true,false);
        const auto index=int32_t(world.bases.size());world.bases.emplace_back();
        const auto seeded=base::SeedSmacStartupBase(world.bases.back(),terrain.records,actor,
            {g,position,owner,index,3,2,2,10,0,false,false,false,false,name},catalog);
        if(seeded.index!=index) {throw std::runtime_error("Generated Base allocation failed");}
        base::PrepareSmacStartupBaseRadius(terrain.records,world.actors,g,position,owner);
        const auto territory=map::RebuildSmacStartupTerritory(terrain.records,world.actors,world.bases,regions,
            {g,8,terrain.resource_pattern,11,61,terrain.primary_rules,2,3,factory.dirty});
        std::vector<unit::OriginalUnitBytes> raw_units;for(const auto& u:world.units) {raw_units.push_back(unit::OriginalSmacUnitBytes(u));}
        base::SmacFirstBaseScratch scratch{};
        base::CalculateSmacFirstBase(terrain.records,world.bases,world.actors,raw_units,world.ownership,rules.yields,
            {{80,80,true,terrain.resource_pattern,terrain.primary_rules,2},{2,definition.drone,definition.talent,index},3,0,5,1,0,0},scratch,
            [&](const auto& store){if(observer.worker_stores && store.caller>=0x4fc000 && store.caller<=0x4feaff) {observer.worker_stores(store);}},
            observer.worker_queries,observer.worker_scores);
        factory.dirty=territory.dirty|2;factory.cached_base=index;
        const auto scout=unit::CreateSmacStartupGroundUnit(terrain.records,world.units,actor,world.bases,factory,
            {g,position,owner,scout_design,world.designs[scout_design][42],0,1});
        if(scout<0) {throw std::runtime_error("Generated Scout allocation failed");}world.units.at(scout).home_base=-1;
        terrain.dirty_flags=factory.dirty;
        if(observer.founded) {observer.founded(owner,world);}
        for(size_t i=0;i<11;++i) {Write(actor.record,0x95d478-0x95d1a8+4*i,uint32_t(definition.social[i]));}
        if(owner==1) {prefix(0,{0,0,0,0,0,0});}
    }
    // Local pairing scratch is discarded, but its rejection draws are gameplay
    // state. The final unmatched faction may reject 399 counted partners.
    std::array<int32_t,8> pairs{};
    for(int owner=1;owner<=7;++owner) {if(pairs[owner]) {continue;}int attempts=0,other;
        do {const auto before=terrain.random.Crt();const auto value=terrain.random.NextCrtRaw15();++attempts;other=value%7+1;
            if(observer.random) {observer.random({0x5cb1a8,before,terrain.random.Crt(),value});}
        }while(other==owner || (pairs[other] && attempts<400));
        ++pairs[owner];++pairs[other];
    }
    constexpr std::array<std::array<int,2>,4> rivalries{{{{1,4}},{{2,6}},{{3,6}},{{7,5}}}};
    for(const auto& pair:rivalries) {const auto left=pair[0],right=pair[1];
        Write(world.actors[left].record,0x38+4*right,Read(world.actors[left].record,0x38+4*right)|0x200u);
        Write(world.actors[right].record,0x38+4*left,Read(world.actors[right].record,0x38+4*left)|0x200u);
    }
    map::smac_balance::Input balance{};balance.geometry=g;balance.difficulty=3;balance.human_mask=2;
    for(size_t i=0;i<128;++i) {balance.areas[i]=regions.Area(i);}
    for(const auto& u:world.units) {balance.units.push_back({u.id,u.owner,{{u.x,u.y}}});}
    const auto tile=[&](auto p)->const auto& {return terrain.records.at(size_t(p[1])*40+size_t(p[0]/2));};
    const auto compensation=map::smac_balance::Prepare(balance,terrain.random,[&](auto p){const auto& t=tile(p);
        return map::smac_balance::Tile{uint8_t(t[0]>>5),uint8_t((t[0]>>3)&3),uint8_t(t[5]>>6),t[3],Read(t,8)};
    },[&](auto p,uint32_t items){return int32_t(resource::smac::ResolveSiteSignal({p[0],p[1],resource::smac::AltitudeBand(tile(p)[0]>>5),items,
        bool(terrain.primary_rules&0x2000),terrain.resource_pattern,11,61}));},
        [&](uint32_t before,uint32_t after,uint16_t value){if(observer.random) {observer.random({0x5c805c,before,after,value});}});
    for(int owner=1;owner<=7;++owner) {
        for(const auto& pod:compensation.pods) {if(pod.owner!=owner) {continue;}
            factory.cached_base=-1;
            const auto id=unit::CreateSmacStartupGroundUnit(terrain.records,world.units,world.actors[owner],world.bases,factory,{g,pod.point,owner,0,world.designs[0][42],0,1});
            if(id<0) {throw std::runtime_error("Generated compensation Pod allocation failed");}world.units.at(id).home_base=-1;
        }
        for(const auto& store:compensation.writes) {if(store.owner!=owner) {continue;}auto& record=terrain.records.at(size_t(store.point[1])*40+size_t(store.point[0]/2));Write(record,8,Read(record,8)|store.mask);}
    }
    terrain.random=compensation.random;terrain.dirty_flags=factory.dirty;return world;
}
}
