#include "SmacStartingSite.h"
#include "SmacRestartPreprocessing.h"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::map::smac_start {
namespace {
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
#include "SmacRestartProximityOffsets.h"
std::optional<Coordinate> Offset(Geometry g,Coordinate p,Coordinate d) {
    p[0]+=d[0];p[1]+=d[1];
    if(g.wrap) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
    if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return {};}
    return p;
}
int32_t Distance(Geometry g,Coordinate a,Coordinate b) {
    auto dx=std::abs(a[0]-b[0]);const auto dy=std::abs(a[1]-b[1]);
    if(g.wrap) {dx=std::min(dx,g.width-dx);}
    return std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
}
int32_t Draw(SmacGameplayRandomState& random,int32_t span) {return span>1 ? random.NextCrtRaw15()%span:0;}
int32_t Margin(int32_t dimension,int32_t n) {return static_cast<int32_t>(std::clamp<int64_t>((int64_t(1024)-n)*dimension/8192,0,9999));}
int32_t Count(uint8_t mask) {int32_t result=0;while(mask) {result+=mask&1;mask>>=1;}return result;}
}
const char* OutcomeName(Outcome value) {
    switch(value) {
        case Outcome::Selected:return "admitted";case Outcome::Limit:return "exhausted";
        case Outcome::LowElevation:return "elevation_low";case Outcome::Fungus:return "fungus";case Outcome::Rocky:return "rocky";
        case Outcome::InitialDistance:return "initial_distance";case Outcome::HighElevation:return "elevation_high";
        case Outcome::Occupied:return "occupied";case Outcome::Polar:return "polar";case Outcome::GoodTiles:return "good_tiles";
        case Outcome::RelaxedDistance:return "relaxed_distance";case Outcome::EarlyRegionArea:return "early_region_area";
        case Outcome::RegionDensity:return "ocean";case Outcome::RegionArea:return "region_area";
        case Outcome::SharedRegionArea:return "shared_region_area";case Outcome::SiteRating:return "site_rating";
        case Outcome::Landmark:return "landmark_distance";
    }throw std::invalid_argument("Unknown startup site outcome");
}
Result Select(const Input& input,const SmacGameplayRandomState& initial,const Query& supplied,const Rating& rating,
    const Observe& observe,const ObserveRefinement& refinement,const std::function<bool()>& cancelled) {
    const auto g=input.geometry;
    Require(g.width>=2 && g.width<=512 && !(g.width&1) && g.height>=2 && g.height<=256
        && input.scale>0 && input.scale<=512 && input.ocean>=0 && input.ocean<=2
        && input.difficulty>=0 && input.difficulty<=5 && bool(supplied) && bool(rating),"Unsupported startup site inputs");
    const auto valid=[&](Coordinate p) {return p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height && !((p[0]^p[1])&1);};
    for(const auto& region:input.regions) {Require(region.area>=0 && region.wet>=0,"Invalid startup region metadata");}
    for(const auto& base:input.bases) {Require(base.id>=0 && valid(base.point),"Invalid startup Base position");}
    for(const auto& unit:input.units) {Require(unit.owner>=0 && unit.owner<=7 && valid(unit.point),"Invalid startup Unit position");}
    const auto query=[&](Coordinate p) {Require(valid(p),"Startup query is outside its parity grid");const auto t=supplied(p);
        Require(t.elevation<=7 && t.moisture<=3 && t.rockiness<=3 && t.region<128,"Invalid decoded startup tile");return t;};
    const auto nearest=[&](Coordinate p,std::optional<uint8_t> region) {
        std::pair<int64_t,int32_t> result{-1,9999};
        for(const auto& base:input.bases) {if(region && query(base.point).region!=*region) {continue;}
            const auto d=Distance(g,p,base.point);if(d<=result.second) {result={base.id,d};}}
        return result;
    };
    Result result{std::nullopt,std::nullopt,initial};
    const auto score=[&](Coordinate p) {const auto value=rating(p);Require(value>=0 && value<=15,"Startup effective rating exceeds its nibble");++result.rating_reads;return value;};
    Coordinate center{};uint8_t region=0;
    for(int32_t n=0;;++n) {
        if(cancelled && cancelled()) {throw std::runtime_error("Startup site search cancelled");}
        Require(n<INT32_MAX,"Startup search exceeds signed counter domain");
        const auto mx=g.wrap ? 0:Margin(g.width,n),my=Margin(g.height,n);
        const auto raw_x=mx+Draw(result.random,g.width-2*mx),y=my+Draw(result.random,g.height-2*my);
        const Coordinate p{{(raw_x&~1)+(y&1),y}};const auto tile=query(p);region=tile.region;
        const auto bucket=smac_restart::LandmarkBucket(g,p,[&](auto square){return query(square).landmark_detail;});
        int32_t good=0;
        for(size_t i=0;i<21;++i) {const auto q=Offset(g,p,proximity[i]);if(!q) {continue;}const auto t=query(*q);
            if(t.elevation>=3 && t.moisture && t.rockiness<=1 && !t.fungus) {++good;}}
        auto close=nearest(p,region);auto distance=close.first<0 ? 32:close.second;auto presence=input.regions[region].presence;
        for(const auto& unit:input.units) {if(query(unit.point).region==region) {presence|=uint8_t(1u<<unit.owner);distance=std::min(distance,Distance(g,p,unit.point));}}
        const auto k=n+1;const auto area=input.regions[region].area;std::optional<int32_t> effective;Outcome outcome=Outcome::Selected;
        if(k>1024 && !input.human) {outcome=Outcome::Limit;}
        else if(tile.elevation<3) {outcome=Outcome::LowElevation;}
        else if(tile.fungus) {outcome=Outcome::Fungus;}
        else if(tile.rockiness>1) {outcome=Outcome::Rocky;}
        else if(distance<int64_t(6)*input.scale/56) {outcome=Outcome::InitialDistance;}
        else if(tile.elevation>=5) {outcome=Outcome::HighElevation;}
        else if(tile.occupied) {outcome=Outcome::Occupied;}
        else if(y<2 || y>=g.height-2) {outcome=Outcome::Polar;}
        else if(good<12-k/80) {outcome=Outcome::GoodTiles;}
        else if(distance<int64_t(16-k/64)*input.scale/56) {outcome=Outcome::RelaxedDistance;}
        else if(k<900 && area<36-k/64) {outcome=Outcome::EarlyRegionArea;}
        else if(int64_t(input.regions[region].wet)*(input.ocean+1)/2<24+(input.human ? 8*(6-input.difficulty):8)-k/16) {outcome=Outcome::RegionDensity;}
        else if(area<48*(5-(input.human ? input.difficulty:3))-k/4) {outcome=Outcome::RegionArea;}
        else if(presence && area/(Count(presence)+1)<64-k/64) {outcome=Outcome::SharedRegionArea;}
        else {
            effective=score(p);
            if(*effective<15-k/64 && !input.look_first) {outcome=Outcome::SiteRating;}
            else if(bucket<(1024-k)/128 || !bucket) {outcome=Outcome::Landmark;}
        }
        ++result.attempts;
        if(observe) {observe({k,p,result.random.Crt(),good,bucket,distance,close.first,region,presence,effective,outcome});}
        if(outcome==Outcome::Limit) {return result;}
        if(outcome==Outcome::Selected) {center=p;result.center=p;break;}
    }
    int32_t best=-1;size_t selected=0;
    for(size_t i=0;i<21;++i) {
        const auto p=Offset(g,center,proximity[i]);if(!p) {continue;}const auto t=query(*p);
        if(t.elevation<3 || t.elevation>=5 || t.region!=region || t.fungus || t.occupied) {continue;}
        auto distance=nearest(*p,{}).second;
        // Native Unit distance uses the accepted center, not this candidate.
        for(const auto& unit:input.units) {distance=std::min(distance,Distance(g,center,unit.point));}
        if(distance<6) {continue;}
        auto value=score(*p);
        for(size_t j=1;j<9;++j) {const auto neighbor=Offset(g,*p,proximity[j]);if(neighbor && query(*neighbor).elevation<3) {++value;break;}}
        if(refinement) {refinement({i,*p,value,best});}
        if(value>best) {best=value;selected=i;}
    }
    result.site=Offset(g,center,proximity[selected]);return result;
}
}
