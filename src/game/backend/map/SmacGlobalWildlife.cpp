#include "SmacGlobalWildlife.h"
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <stdexcept>

namespace game::backend::map::smac_wildlife {
namespace {
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
// Explicit original DWORD wrapping followed by signed division.
int32_t Signed(uint32_t value) {return value<=INT32_MAX ? int32_t(value):int32_t(int64_t(value)-(int64_t(1)<<32));}
constexpr Coordinate neighbors[]{{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}};
constexpr Coordinate ambush[]{{2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},
    {-1,3},{-3,1},{-3,-1},{-1,-3},{4,0},{-4,0},{0,4},{0,-4}};
}
Result Produce(const Input& input,const SmacGameplayRandomState& initial,const Hooks& hooks) {
    const auto g=input.geometry;
    Require(g.width>=2 && g.width<=512 && !(g.width&1) && g.height>=2 && g.height<=256
        && input.turn>=0 && input.life>=0 && input.life<=2 && input.difficulty>=0 && input.difficulty<=5
        && input.unit_count>=0,"Unsupported global wildlife geometry, turn, life, difficulty or count");
    Result result{initial,input.unit_count,{},{},{}};
    if((input.rules&0x200000u) || input.voice_marker!=-1 || input.unit_count>=1792) {return result;}
    Require(bool(hooks.tile) && bool(hooks.site) && bool(hooks.create) && bool(hooks.capacity)
        && bool(hooks.sentry) && bool(hooks.promote_stack),"Global wildlife requires the staged helper contracts");
    const auto valid=[&](Coordinate p) {return p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height && !((p[0]^p[1])&1);};
    std::set<int32_t> ids;
    for(const auto& base:input.bases) {Require(base.id>=0 && valid(base.point) && ids.insert(base.id).second,"Invalid wildlife Base record");}
    const auto offset=[&](Coordinate p,Coordinate delta)->std::optional<Coordinate> {
        p[0]+=delta[0];p[1]+=delta[1];if(g.wrap) {p[0]=(p[0]%g.width+g.width)%g.width;}
        return valid(p) ? std::optional<Coordinate>(p):std::nullopt;
    };
    const auto sample=[&] {
        auto x=int32_t(result.random.NextCrtRaw15())%g.width;
        const auto y=int32_t(result.random.NextCrtRaw15())%g.height;x+=(y&1)-(x&1);return Coordinate{{x,y}};
    };
    const auto nearest=[&](Coordinate point,uint8_t region) {
        std::pair<int32_t,int32_t> best{-1,9999};
        for(const auto& base:input.bases) {
            if(hooks.tile(base.point).Region()!=region) {continue;}
            auto dx=std::abs(point[0]-base.point[0]);const auto dy=std::abs(point[1]-base.point[1]);
            if(g.wrap) {dx=std::min(dx,g.width-dx);}
            const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
            if(distance<=best.second) {best={base.id,distance};}
        }
        return best;
    };
    const auto create=[&](int32_t design,Coordinate point) {
        const auto id=hooks.create(design,point,result.random);
        Require(id>=-1 && id<2048,"Wildlife constructor returned an invalid native index");
        result.actions.push_back({Action::Kind::Create,id,design,point});
        if(id>=0) {++result.unit_count;}return id;
    };
    const bool early=input.turn<2*(10*(3-input.life)-input.difficulty);
    if(input.turn%(2*(5-input.life))==0) {
        std::optional<Coordinate> chosen;
        for(int attempt=1;attempt<=1000;++attempt) {
            const auto point=sample();result.sea_samples.push_back(point);
            if(attempt==1000) {break;} // consume the pair, but do not test candidate1000
            const auto tile=hooks.tile(point);const auto flags=tile.Items();const auto band=tile.Climate()>>5;
            if((flags&0x20u) && band>=2) {chosen=point;break;}
            if(band>=3 || ((flags&3u) && tile.CachedOccupant()<8)) {continue;}
            const auto region=tile.Region();
            if(!(region&63) || (region&63)==63) {continue;}
            Require(region<input.region_areas.size(),"Wildlife sea region exceeds retained area table");
            if(input.region_areas[region]<50) {continue;}chosen=point;break;
        }
        if(chosen) {
            const auto multiplier=(input.secondary&0x100000u) ? 2u:1u;
            const auto requested=early ? 1:Signed(multiplier*uint32_t(input.turn))/(4*(4*(8-input.life)-input.difficulty))+input.life+1;
            const auto carrier=create(9,*chosen);
            if(carrier>=0) {
                const auto capacity=hooks.capacity(carrier);Require(capacity>=0 && capacity<=256,"Wildlife capacity exceeds native byte-derived domain");
                for(int i=0;i<std::min(requested,capacity);++i) {
                    const auto passenger=create(8,*chosen);
                    Require(passenger>=0,"Wildlife passenger factory failed before its required sentry operation");
                    hooks.sentry(passenger,result.random);result.actions.push_back({Action::Kind::Sentry,passenger,-1,*chosen});
                }
                hooks.promote_stack(carrier,result.random);result.actions.push_back({Action::Kind::PromoteStack,carrier,-1,*chosen});
                if(input.locust_technology_masks[0] || input.locust_technology_masks[1]) {create(10,*chosen);}
            }
        }
    }
    const auto blocked=[&](Coordinate point) {
        for(const auto& delta:neighbors) {
            const auto next=offset(point,delta);if(!next) {continue;}const auto tile=hooks.tile(*next);
            const auto owner=tile.CachedOccupant();
            if((tile.Items()&3u) && owner>0 && owner<8 && !(input.native_diplomacy[owner]&1u)) {return true;}
        }return false;
    };
    for(int iteration=0;iteration<2*input.life+2;++iteration) {
        const auto point=sample();result.land_samples.push_back(point);const auto tile=hooks.tile(point);
        const auto flags=tile.Items();const auto region=tile.Region();const auto owner=tile.CachedOccupant();
        if((tile.Climate()>>5)<3 || !(flags&0x20u) || !(region&63) || (region&63)==63 || ((flags&1u) && owner<8)) {continue;}
        const auto base=nearest(point,region);
        if((flags&2u) && owner<8) {
            if(owner==0 || (base.first>=0 && base.second<=2)) {continue;}
            for(const auto& delta:ambush) {
                const auto candidate=offset(point,delta);if(!candidate) {continue;}const auto target=hooks.tile(*candidate);
                if((target.Climate()>>5)<3 || (input.observer_status[owner]&0x200u) || (target.bytes[4]&(1u<<owner))
                    || ((target.Items()&3u) && target.CachedOccupant()<8) || blocked(*candidate)) {continue;}
                create(8,*candidate);break;
            }
            continue;
        }
        if(base.second>1 && base.second<6 && !hooks.site(point)) {create(8,point);}
    }
    return result; // native repeats its exhausted loop guard; cluster block is unreachable
}
}
