#include "SmacGeneratedStartingSite.h"
#include "SmacSiteQuality.h"
#include "SmacRestartPreprocessing.h"
#include "game/backend/resource/SmacResourceBonus.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
uint32_t Read(const Record& r,size_t off) {return uint32_t(r[off])|(uint32_t(r[off+1])<<8)|(uint32_t(r[off+2])<<16)|(uint32_t(r[off+3])<<24);}
int32_t Signed32(const uint8_t* p) {const auto v=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
    return v<=INT32_MAX?int32_t(v):int32_t(int64_t(v)-4294967296LL);}
}
smac_start::Result SelectGeneratedStartingSite(State& s,Geometry g,const Regions& regions,
    const StartingContext& context,const StartingCacheObserver& stores,const smac_start::Observe& observe,const std::function<bool()>& cancelled) {
    if(s.stage!=Stage::Prestartup || s.generation_flags[0] || s.generation_flags[1] || s.topology<0 || s.topology>1
        || g.width<16 || g.width>512 || (g.width&1) || g.height<8 || g.height>256
        || s.records.size()!=size_t(g.width/2)*size_t(g.height)) {throw std::invalid_argument("Generated startup requires completed native terrain");}
    const smac_site::Geometry geometry{g.width,g.height,s.topology==0};
    smac_start::Input input{};input.geometry=geometry;input.scale=smac_restart::ScaleFromGeometry(geometry);
    input.ocean=s.categories[1];input.difficulty=context.difficulty;input.human=context.human;input.look_first=context.look_first;
    input.bases=context.bases;input.units=context.units;smac_site::Areas areas;
    for(size_t i=0;i<128;++i) {areas[i]=regions.Area(i);input.regions[i]={areas[i],Signed32(regions.records.data()+28*i+4),context.presence[i]};}
    const auto index=[&](auto p) {if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height || ((p[0]^p[1])&1)) {
            throw std::invalid_argument("Generated starting-site query outside parity grid");}
        return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);};
    const auto query=[&](auto p) {const auto& r=s.records[index(p)];const auto items=Read(r,8);
        return smac_start::Tile{uint8_t(r[0]>>5),uint8_t((r[0]>>3)&3),uint8_t(r[5]>>6),r[3],r[15],bool(items&0x20),bool((items&3) && (r[2]&15)<8)};};
    const smac_site::Query qualityQuery=[&](auto p) {
        const auto& r=s.records[index(p)];const auto items=Read(r,8),marks=Read(r,12);const auto band=resource::smac::AltitudeBand(r[0]>>5);
        const bool automatic=s.primary_rules&0x2000;
        const bool bonus=resource::smac::ResolveResourceBonus({p[0],p[1],band,s.resource_pattern,automatic,bool(items&0x400),
            bool(items&0x20000000),bool(items&0x10000),bool(items&0x20000)})!=resource::smac::ResourceBonus::None;
        const bool site=resource::smac::ResolveSiteSignal({p[0],p[1],band,items,automatic,s.resource_pattern,
            context.site_coefficient_u,context.site_coefficient_v})!=0;
        return smac_site::Tile{uint8_t(r[0]>>5),uint8_t((r[0]>>3)&3),uint8_t(r[5]>>6),r[3],
            bool(items&0x20),bool(items&0x2000),bool(items&0x200000),bool(items&0x80),bool(items&0x1000),bool(items&1),
            bonus,site,bool(items&0x40004000),bool(marks&2),bool(marks&1),bool(marks&8),bool(marks&4),r[15]};
    };
    const auto rating=[&](auto p) {
        const auto at=index(p);auto& r=s.records[at];if(Read(r,8)&0x1000) {return 0;}
        const auto cached=r[2]>>4;if(cached) {return int(cached);}
        const auto value=smac_site::Quality(geometry,p,0,areas,qualityQuery).rating;
        const auto before=r[2];r[2]=uint8_t((r[2]&15)|((value&15)<<4));
        if(stores) {stores({uint32_t(at*44+2),before,r[2]});}return value;
    };
    auto result=smac_start::Select(input,s.random,query,rating,observe,{},cancelled);s.random=result.random;return result;
}
}
