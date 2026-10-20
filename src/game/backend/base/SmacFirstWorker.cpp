#include "SmacFirstWorker.h"
#include "SmacWorkerAvailability.h"
#include "SmacTerraformingPoints.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include "game/backend/resource/SmacBaseSquareYield.h"
#include "game/backend/resource/SmacTileNutrients.h"
#include "game/backend/resource/SmacTileMinerals.h"
#include "game/backend/resource/SmacTileEnergy.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
template<class Record> uint32_t Read(const Record& r,size_t off) {uint32_t v=0;for(unsigned k=0;k<4;++k) {v|=uint32_t(r.at(off+k))<<(8*k);}return v;}
int32_t Signed16(const SmacStartupBaseRecord& b,size_t off) {const auto v=uint32_t(b[off])|(uint32_t(b[off+1])<<8);return v<32768?int32_t(v):int32_t(v)-65536;}
int32_t Narrow(int64_t value) {if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Opening yield exceeds measured signed32 domain");}return int32_t(value);}
}
int32_t CalculateSmacFirstWorker(const std::vector<std::array<uint8_t,44>>& tiles,SmacStartupBaseRecord& base,
    const faction::SmacStartupFaction& actor,const faction::SmacStartupTechnologyOwnership& ownership,
    const SmacOpeningYieldRules& rules,const SmacFirstWorkerOptions& in,SmacFirstWorkerScratch& scratch,
    const SmacBaseAccountingObserver& observer,const std::function<void(const SmacFirstWorkerQuery&)>& query_observer,
    const std::function<void(int32_t,int32_t)>& score_observer) {
    using namespace resource::smac;
    const auto owner=base[4];
    if(owner<1 || owner>7 || base[6]!=1 || Read(base,0x78)>1 || Read(base,0x7c) || Read(base,0x34)
        || Read(base,0x38)&0x80000000u || SmacWrap32(Read(base,0x50))<0 || in.width<8 || in.width>512
        || (in.width&1) || in.height<2 || in.height>256 || tiles.size()!=size_t(in.width/2)*size_t(in.height)
        || rules.food_per_citizen<=0) {throw std::invalid_argument("First worker requires the fresh unmanaged population-one profile");}
    const auto field=[&](uint32_t address){return SmacWrap32(Read(actor.record,address-0x95d1a8));};
    for(const auto address:{0x95d9ac,0x95d9b0,0x95d9b4}) {if(field(address)) {throw std::invalid_argument("First worker excludes orbital inputs");}}
    const SmacWorkerRadiusMap map{{{Signed16(base,0),Signed16(base,2)}},in.width,in.height,in.horizontal_wrap};
    std::array<std::optional<std::array<int32_t,2>>,21> positions;
    for(int i=0;i<21;++i) {positions[i]=SmacWorkedSquareCollection::Resolve(map,i);}
    const auto tile=[&](const std::array<int32_t,2>& p)->const auto& {return tiles.at(size_t(p[1])*size_t(in.width/2)+size_t(p[0]/2));};
    const auto facility=[&](unsigned id){return bool(base[0x8c+id/8]&(1u<<(id&7)));};
    const auto tech=[&](int32_t id){return faction::QuerySmacTechnology(owner,id,id>=0 && id<89?ownership[id]:0,id>=0 && id<89?rules.prerequisites[id]:std::array<int32_t,2>{{-1,-1}});};
    const auto put=[&](uint32_t off,int64_t value,uint32_t pc){const auto v=uint32_t(value);for(unsigned k=0;k<4;++k) {base.at(off+k)=uint8_t(v>>(8*k));}
        if(observer) {observer({SmacBaseAccountingStore::Space::Bases,off,4,v,pc});}};
    const auto global=[&](uint32_t address,int32_t value,uint32_t pc){
        if(address==0x918674) {scratch.food_sufficient=value;}else if(address==0x918678) {scratch.remaining=value;}
        else if(address==0x9186b0) {scratch.unavailable=value;}else {scratch.reasons.at((address-0x9186b4)/4)=value;}
        if(observer) {observer({SmacBaseAccountingStore::Space::Global,address,4,uint32_t(value),pc});}};
    const auto yield=[&](int resource,const std::array<int32_t,2>& p){
        if(resource==3) {return int32_t(0);}
        const auto& t=tile(p);const auto items=Read(t,8),marks=Read(t,12);const auto band=AltitudeBand(t[0]>>5);const bool land=(t[0]>>5)>=3;
        const auto bonus=ResolveResourceBonus({p[0],p[1],band,in.pattern,bool(in.primary_rules&0x2000),bool(items&0x400),bool(items&0x20000000),bool(items&0x10000),bool(items&0x20000)});
        const auto detail=t[15]<128?int32_t(t[15]):int32_t(t[15])-256;
        const ResourceFlags landmark{{(land && (marks&0x80000004u)==4) || (!land && bool(marks&0x80)),
            ((marks&0x80000001u)==1 && detail<9) || ((marks&0x80000002u)==2 && detail<9) || (marks&0x80000200u)==0x200,
            ((marks&0x80000002u)==2 && detail<9) || (marks&0x80000008u)==8 || (marks&0x80000400u)==0x400 || (marks&0x80000800u)==0x800}};
        const auto& r=rules.resources;const bool golden=Read(base,0x30)&4;
        if(p==map.center) {
            if(!(items&1) || (t[2]&15)!=owner) {throw std::invalid_argument("Opening yield requires a consistent founded center");}
            const auto values=CalculateBaseSquareYield({r[1],r[2],r[4]},{{land,bonus,landmark,bool(items&0x80)},
                {facility(1),facility(3),facility(26)},{field(0x95d3f4),golden,uint32_t(field(0x95e200)),false,false},
                {{{false,false,false}},{{false,false,false}},false,false}});return Narrow(values[resource]);
        }
        if(resource==0) {return Narrow(CalculateTileNutrients({r[0][0],r[2][0],r[3][0],r[5][0],r[6][0],r[7][0],r[8][0],rules.mine_nutrients},
            {{band,Moisture((t[0]>>3)&3),Rockiness(t[5]>>6),bonus,landmark[0],bool(items&0x200000),bool(items&0x20),bool(items&0x1000000),bool(items&0x2000)},
             {bool(items&0x8000),bool(items&0x10),bool(items&0x80000),bool(items&0x400000)},
             {facility(13),facility(14),tech(rules.limit_technologies[0]),field(0x95d4bc),field(0x95d410),false,false}}).nutrients);}
        if(resource==1) {return Narrow(CalculateTileMinerals({r[0][1],r[2][1],r[3][1],r[6][1],r[7][1],r[8][1],rules.unroaded_mine_limit},
            {{band,int32_t(t[5]>>6),bonus,landmark[1],bool(items&0x200000),bool(items&0x20),bool(items&0x1000000),bool(items&0x2000)},
             {bool(items&0x10),bool(items&4)},{tech(rules.limit_technologies[1]),tech(rules.platform_technology),field(0x95d4c0),field(0x95d410),false,false}}).minerals);}
        constexpr std::array<std::array<int32_t,2>,8> adjacent{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
        int32_t mirrors=0;
        for(const auto& d:adjacent) {auto x=p[0]+d[0];const auto y=p[1]+d[1];if(in.horizontal_wrap) {if(x<0) {x+=in.width;}else if(x>=in.width) {x-=in.width;}}
            if(x>=0 && x<in.width && y>=0 && y<in.height) {const auto& neighbor=tile({{x,y}});if((Read(neighbor,8)&0x800000) && neighbor[7]==owner) {++mirrors;}}}
        return Narrow(CalculateTileEnergy({r[0][2],r[2][2],r[3][2],r[6][2],r[7][2],r[8][2]},
            {{band,bonus,landmark[2],bool(items&0x200000),bool(items&0x20),bool(items&0x2000),bool(items&0x1000000),bool(items&0x80)},
             {bool(items&0x40),bool(items&0x800000),mirrors},{facility(14),tech(rules.limit_technologies[2]),field(0x95d4c4),field(0x95d410),field(0x95d3f4),golden,false,false,false,false,false}}).energy);
    };
    SmacWorkerAvailability::Squares squares;SmacWorkerAvailabilityContext context{0,owner,map,bool(actor.record[1]&2),{}, {}};
    for(size_t i=0;i<25;++i) {
        const auto address=uint32_t(0x9186b4+4*i);
        if(i>=21) {global(address,1,0x4fc16b);continue;}
        if(!positions[i]) {global(address,2,0x4fc1c7);continue;}
        const auto& t=tile(*positions[i]);const auto items=Read(t,8);const bool explored=t[4]&(1u<<owner),real_base=(items&1) && (t[2]&15)<8;
        if(items&2) {throw std::invalid_argument("First worker excludes radius Units");}
        const auto claim=t[7];if(claim>=1 && claim<=7 && claim!=owner && (Read(actor.record,0x18+4*claim)&0x8000008u)==0x8000008u) {throw std::invalid_argument("First worker excludes recognized foreign claims");}
        squares[i]=SmacWorkerAvailabilitySquare{explored,real_base,(t[0]>>5)<3,{}, {}};
        if(!context.all_map_revealed && !explored) {global(address,2,0x4fc295);}
        else {global(address,0,0x4fc2a9);if(real_base) {global(address,4,0x4fc2df);}}
    }
    const auto unavailable=SmacWorkerAvailability::Build(context,squares,{});global(0x9186b0,0,0x4fc68e);global(0x9186b0,int32_t(unavailable),0x4fc6b5);
    global(0x918678,2,0x4fd66d);put(0x30,Read(base,0x30)&~0x8000u,0x4fd6bb);put(0x80,0,0x4fd6c3);put(0x78,Read(base,0x78)&(~unavailable|1u),0x4fd6de);
    for(unsigned i=0;i<4;++i) {const auto off=0xbc+4*i;if(i<3) {put(off,0,0x4fd710);put(off,0,0x4fd785);put(off,0,0x4fd79b);}else {put(off,0,0x4fd7a6);}}
    const auto query=[&](bool dispatcher,int resource,const auto& p){const auto value=yield(resource,p);if(query_observer) {query_observer({dispatcher,resource,p[0],p[1],value});}return value;};
    const auto collect=[&](int index,uint32_t pc,uint32_t sea_pc){const auto p=*positions[index];if((tile(p)[0]>>5)<3) {put(0x30,Read(base,0x30)|0x8000,sea_pc);}
        for(int resource=0;resource<4;++resource) {put(0xbc+4*resource,int64_t(SmacWrap32(Read(base,0xbc+4*resource)))+query(true,resource,p),pc);}};
    collect(0,0x4fd97c,0x4fd938);global(0x918678,1,0x4fd99e);global(0x918674,0,0x4fdc43);SmacWorkerRanking ranking;
    for(int i=1;i<21;++i) {if(scratch.reasons[i]) {continue;}const auto p=*positions[i];std::array<int32_t,3> values;
        for(int resource=0;resource<3;++resource) {values[resource]=query(false,resource,p);}
        const auto scored=SmacWorkerAllocationRules::Score({1,1,rules.food_per_citizen,{{SmacWrap32(Read(base,0xbc)),SmacWrap32(Read(base,0xc0)),SmacWrap32(Read(base,0xc4))}},0,SmacWorkerProduction::Unit,{}},values);
        if(score_observer) {score_observer(i,scored.score);}if(scored.food_sufficient) {global(0x918674,1,0x4fdec4);}ranking=SmacWorkerAllocationRules::Rank(ranking,i,false,scored.score);
    }
    uint32_t visited=1;int32_t remaining=1;
    if(ranking.best_index) {visited|=1u<<ranking.best_index;collect(ranking.best_index,0x4fe35d,0x4fe317);remaining=0;global(0x918678,0,0x4fe379);
        if(SmacWrap32(Read(base,0xbc))<rules.food_per_citizen) {throw std::invalid_argument("First worker requires later underfed swap processing");}}
    else {put(0x84,Read(base,0x84)&~15u,0x4fe89a);put(0x84,Read(base,0x84)|1u,0x4fe8b0);}
    put(0x78,visited,0x4fe8d8);put(0x7c,remaining,0x4fe8e7);put(0x30,Read(base,0x30)&~0x100u,0x4fe8f5);
    for(unsigned i=0;i<4;++i) {put(0xac+4*i,Read(base,0xbc+4*i),0x4fe90f);}put(0x48,0,0x4fe91c);
    // Reuse the established ecology producer for each original radius write.
    for(int i=0;i<21;++i) {if(!positions[i]) {continue;}const auto p=*positions[i];const auto items=Read(tile(p),8);uint32_t improvements=0;
        constexpr std::array<uint32_t,10> masks{{4,8,0x10,0x40,0x8000,0x80000,0x400000,0x800000,0x1000000,0x200000}};
        for(unsigned k=0;k<10;++k) {if(items&masks[k]) {improvements|=1u<<k;}}
        const auto points=CalculateSmacTerraformingSquarePoints({improvements,bool(visited&(1u<<i))},facility(13),facility(14));
        put(0x48,int64_t(SmacWrap32(Read(base,0x48)))+points,0x4feafe);
    }
    return ranking.best_index;
}
int32_t CalculateSmacFirstBase(const std::vector<std::array<uint8_t,44>>& tiles,std::vector<SmacStartupBaseRecord>& bases,
    std::array<faction::SmacStartupFaction,8>& actors,const std::vector<unit::OriginalUnitBytes>& units,
    const faction::SmacStartupTechnologyOwnership& ownership,const SmacOpeningYieldRules& rules,
    const SmacFirstBaseOptions& in,SmacFirstBaseScratch& scratch,const SmacBaseAccountingObserver& observer,
    const std::function<void(const SmacFirstWorkerQuery&)>& query_observer,const std::function<void(int32_t,int32_t)>& score_observer) {
    if(bases.size()!=1 || !units.empty() || bases[0][4]<1 || bases[0][4]>7 || !(in.citizens.human_mask&(1u<<bases[0][4]))) {
        throw std::invalid_argument("First-Base composition requires one human Base and no Units");
    }
    PrepareSmacOpeningSupport(bases,0,units,scratch.support,observer);
    const auto selected=CalculateSmacFirstWorker(tiles,bases[0],actors[bases[0][4]],ownership,rules,in.worker,scratch.worker,observer,query_observer,score_observer);
    scratch.growth=AccountSmacBaseNutrients(bases,actors,{0,0,scratch.support[1],scratch.support[5],rules.food_per_citizen,0},observer);
    AccountSmacBaseMinerals(bases,actors,{0,0,scratch.support[2],scratch.support[6],scratch.support[0],in.citizens.human_mask,
        in.global_difficulty,in.sea_level,in.sea_rise_rule,in.native_life,in.worker.primary_rules,in.secondary_rules,{}},observer);
    AccountSmacOpeningEnergy(bases,actors,scratch.energy,{in.citizens,scratch.support[3],scratch.support[7],in.flat_labs},observer);
    return selected;
}
}
