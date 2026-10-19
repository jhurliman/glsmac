#include "SmacStartingBalance.h"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <stdexcept>

namespace game::backend::map::smac_balance {
namespace {
#include "SmacRestartProximityOffsets.h"
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
int32_t Distance(Geometry g,Coordinate a,Coordinate b) {
    auto dx=std::abs(a[0]-b[0]);const auto dy=std::abs(a[1]-b[1]);
    if(g.wrap) {dx=std::min(dx,g.width-dx);}
    return std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
}
}
Result Prepare(const Input& input,const SmacGameplayRandomState& initial,const Query& supplied,const Site& site) {
    const auto g=input.geometry;
    Require(g.width>=2 && g.width<=512 && !(g.width&1) && g.height>=2 && g.height<=256
        && input.difficulty>=0 && input.difficulty<=5 && bool(supplied) && bool(site),"Unsupported startup balancing inputs");
    const auto valid=[&](Coordinate p){return p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height && !((p[0]^p[1])&1);};
    const auto offset=[&](Coordinate p,Coordinate d)->std::optional<Coordinate> {
        p[0]+=d[0];p[1]+=d[1];if(g.wrap) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        return valid(p) ? std::optional<Coordinate>(p):std::nullopt;
    };
    auto units=input.units;std::sort(units.begin(),units.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    std::set<int64_t> ids;
    for(const auto& u:units) {Require(u.id>=0 && u.owner>=0 && u.owner<=7 && valid(u.point) && ids.insert(u.id).second,"Invalid balancing Unit identity");}
    for(const auto a:input.areas) {Require(a>=0 && a<=g.width/2*g.height,"Invalid balancing region area");}
    std::map<Coordinate,uint32_t> changed;
    const auto query=[&](Coordinate p) {
        Require(valid(p),"Balancing query outside parity grid");auto t=supplied(p);
        Require(t.elevation<=7 && t.rain<=3 && t.rockiness<=3 && t.region<128,"Invalid balancing tile");
        const auto i=changed.find(p);if(i!=changed.end()) {t.items=i->second;}return t;
    };
    const auto nearest=[&](int32_t owner,Coordinate p,int64_t excluded)->const Unit* {
        const Unit* best=nullptr;int32_t distance=9999;
        for(const auto& u:units) {if(u.owner!=owner || u.id==excluded) {continue;}
            const auto d=Distance(g,p,u.point);if(d<=distance) {best=&u;distance=d;}}
        return best;
    };
    std::array<const Unit*,8> anchors{};
    for(int32_t f=1;f<=7;++f) {
        auto* u=nearest(f,{{0,0}},-1);
        if(u && query(u->point).region>=64) {u=nearest(f,u->point,u->id);}anchors[f]=u;
    }
    Result result{initial,{},{},{},{}};std::array<std::optional<int32_t>,8> scores{};int32_t maximum=0;
    for(int32_t f=1;f<=7;++f) {const auto* u=anchors[f];if(!u) {continue;}
        const auto region=query(u->point).region;const auto area=input.areas[region];int32_t spacing=9999,count=0;
        for(int32_t other=1;other<=7;++other) {if(other!=f && anchors[other] && query(anchors[other]->point).region==region) {
            spacing=std::min(spacing,3200*Distance(g,u->point,anchors[other]->point)/(g.width/2*g.height));}}
        const auto space=spacing>=1000 ? std::clamp((area+31)/32,1,8):spacing/5-3;
        for(size_t i=0;i<21;++i) {const auto p=offset(u->point,proximity[i]);if(!p) {continue;}const auto t=query(*p);
            if(!t.rain) {continue;}const auto height=int32_t(t.elevation)-2;
            if((height>0 || (t.items&0x80)) && t.rockiness<2 && (t.rockiness==1 || height>1 || t.rain>1)) {++count;}
            if(t.rockiness==1 && height>1) {++count;}
        }
        const auto terrain=std::clamp(count/2,0,8),region_term=std::clamp(area/64,0,3),value=space+terrain+region_term;
        result.scores.push_back({f,u->point,region,area,spacing,space,count,terrain,region_term,value});scores[f]=value;maximum=std::max(maximum,value);
    }
    for(int32_t f=1;f<=7;++f) {const auto* u=anchors[f];if(!u) {continue;}
        // This draw occurs even when the deficit is zero or a Pod grant suffices.
        const auto orientation=result.random.NextCrtRaw15()&15;int32_t credit=0;
        Require(input.configured_unit_roles[f].size()<=8,"Too many configured startup UNIT bonuses");
        for(const auto role:input.configured_unit_roles[f]) {credit+=role==8 ? 3:1;}
        auto deficit=maximum-*scores[f]-credit;result.deficits.push_back({f,maximum,credit,deficit,scores});
        if(input.difficulty>1 && (deficit>=3 || input.difficulty>=((input.human_mask&(1u<<f)) ? 4:5))) {
            result.pods.push_back({f,u->point});deficit-=3;
        }
        if(input.difficulty<2) {deficit=(deficit+1)/2;}
        int32_t special=0;
        for(int32_t i=0;i<deficit;++i) {
            const auto p=offset(u->point,proximity[9+((7*(orientation+i+1))&15)]);if(!p) {continue;}
            const auto before=query(*p).items;
            if(site(*p,before) && !(before&0x0c000000)) {--*scores[f];}
            uint32_t mask=0x08000000;
            if(special<2 && deficit-i>=3 && !((i^special)&1)) {mask=0x04000000;--deficit;++special;}
            const auto after=before|mask;changed[*p]=after;result.writes.push_back({f,*p,before,mask,after});
        }
    }
    return result;
}
}
