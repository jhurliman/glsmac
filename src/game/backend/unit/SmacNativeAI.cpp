#include "SmacNativeAI.h"
#include <algorithm>
#include <cstdlib>
#include <set>
#include <stdexcept>

namespace game::backend::unit::smac_native_ai {
namespace {
constexpr Coordinate directions[]{{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}};
constexpr Coordinate directive_offsets[]{{0,0},{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2},
    {2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},{-1,3},{-3,1},{-3,-1},{-1,-3}};
void Require(bool condition,const char* message) {if(!condition) {throw std::invalid_argument(message);}}
void Check(Geometry g) {Require(g.width>=2 && g.width<=512 && !(g.width&1) && g.height>=2 && g.height<=256,"Invalid native AI geometry");}
bool Valid(Geometry g,Coordinate p) {return p[0]>=0 && p[0]<g.width && p[1]>=0 && p[1]<g.height && !((p[0]^p[1])&1);}
std::optional<Coordinate> Offset(Geometry g,Coordinate p,Coordinate d) {
    p[0]+=d[0];p[1]+=d[1];if(g.wrap) {p[0]=(p[0]%g.width+g.width)%g.width;}
    if(!Valid(g,p)) {return {}; }return p;
}
int32_t Dx(Geometry g,Coordinate a,Coordinate b) {auto dx=std::abs(a[0]-b[0]);return g.wrap?std::min(dx,g.width-dx):dx;}
int32_t EvenManhattan(Geometry g,Coordinate a,Coordinate b) {return (Dx(g,a,b)+std::abs(a[1]-b[1]))&~1;}
int32_t Life(int32_t turn) {return turn<45?0:turn<90?1:turn<170?2:turn<250?3:turn<330?4:6;}
}
Target ChooseBase(Geometry g,Coordinate point,uint8_t region,int32_t home,uint8_t tag,
    const std::array<int32_t,8>& planet,const std::vector<Base>& bases) {
    Check(g);Require(Valid(g,point),"Invalid native AI source coordinate");
    Target result;std::set<int32_t> ids;
    for(const auto& base:bases) {
        Require(base.id>=0 && ids.insert(base.id).second && Valid(g,base.point) && base.owner<8
            && base.field_c0>=0 && base.field_c4>=0 && base.role12_count>=0,"Invalid native AI Base authority");
        if(region<64 && base.region!=region) {continue;}
        const auto dx=Dx(g,point,base.point),dy=std::abs(point[1]-base.point[1]);
        const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
        const int64_t denominator=int64_t(base.field_c0)+base.field_c4+32;
        Require(denominator<=INT32_MAX,"Native AI Base denominator exceeds admitted stock arithmetic");
        auto metric=int32_t(32*distance/denominator);
        if(base.role12_count) {metric/=2;}if(base.id==home) {metric/=2;}
        if(tag!=base.owner && planet[base.owner]>0 && !base.field48) {metric*=2;}
        result.terms.push_back({base.id,distance,int32_t(denominator),metric});
        if(metric<=result.metric) {result.base=base.id;result.metric=metric;}
    }return result;
}
Competition ChooseNeighbors(bool current_sea,int32_t triad,uint8_t facing,int32_t turn,int32_t difficulty,
    uint32_t state,const SmacGameplayRandomState& initial,const std::vector<Candidate>& candidates) {
    Require(triad>=0 && triad<=2 && turn>=0 && difficulty>=0 && difficulty<=5 && candidates.size()<=8,"Invalid native AI competition");
    Competition result{initial,Life(turn),0,-1,{}};int32_t previous=-1;
    for(const auto& c:candidates) {
        Require(c.direction>previous && c.direction<8,"Native AI candidates must retain direction-table order");previous=c.direction;
        Score score{c.direction,Rejection::None};
        if(!c.in_bounds) {score.rejection=Rejection::Bounds;}
        else if(c.sea && triad==0) {score.rejection=Rejection::LandToWater;}
        else if(!c.sea && triad==1) {score.rejection=Rejection::SeaToLand;}
        else if(!c.sea && current_sea && triad==0 && (c.items&2u) && c.affiliation>0 && c.affiliation<8) {score.rejection=Rejection::BeachingIntoPositiveUnit;}
        if(score.rejection!=Rejection::None) {result.terms.push_back(score);continue;}
        if((c.items&1u) && c.affiliation>0 && c.affiliation<8) {score.base_bonus=9999;}
        else if(c.head && c.head->owner!=0) {
            if(c.head->design<64 && c.head->weapon_offense<0) {score.rejection=Rejection::NegativePredefinedWeapon;}
            else if((triad==2)!=(c.head->triad==2)) {score.rejection=Rejection::AirMismatch;}
            if(score.rejection!=Rejection::None) {result.terms.push_back(score);continue;}
            score.base_bonus=999;
        }
        for(auto bits=c.items&0x80208054u;bits;bits&=bits-1) {score.features+=4;}
        score.item_2000_bonus=difficulty>2 && (c.items&0x2000u) && !(state&0x100u)?8:0;
        score.jitter=result.random.NextCrtRaw15()%8;
        auto angle=((int32_t(facing)^4)-c.direction)&7;angle=std::min(angle,8-angle);
        score.heading=(result.level+1)*angle+(c.direction==facing?result.level*result.level:0);
        score.total=score.base_bonus+score.features+score.item_2000_bonus+score.jitter+score.heading;
        if(score.total>result.best) {result.best=score.total;result.direction=c.direction;}
        result.terms.push_back(score);
    }return result;
}
Upkeep PlanUpkeep(int32_t triad,bool sea,bool ordinary_base,uint32_t flags,int32_t order,
    uint32_t state,int32_t transports,bool human) {
    Require(triad>=0 && triad<=2 && order>=0 && order<=255 && transports>=0,"Invalid native upkeep inputs");
    Upkeep result{state&0xcfffffdbu,order,{}};
    if(!human && order==1 && (triad!=0 || !sea)) {result.order=0;result.actions.push_back(UpkeepAction::ClearSentry);}
    if(sea && triad==0 && !ordinary_base) {
        if(!transports) {if(!(flags&0x20u)) {result.actions.push_back(UpkeepAction::Delete);}}
        else if(result.order!=1) {result.order=1;result.actions.push_back(UpkeepAction::Sentry);}
    }
    if(!sea && triad==1 && !ordinary_base && !(flags&0x20u)) {result.actions.push_back(UpkeepAction::Delete);}
    return result;
}
bool CommandPassAdmits(int32_t priority,int32_t role,int32_t triad,int32_t damage,int32_t owner,
    bool first,int32_t land_count,int32_t weapon,bool ability16) {
    Require(priority>=0 && priority<10 && triad>=0 && triad<=2 && owner>=0 && owner<8 && land_count>=0,"Invalid native command pass inputs");
    if(priority==0 && !first) {return false;}
    if(priority==1 && !(role==2 || (role==7 && land_count))) {return false;}
    if(priority==2 && !(triad==2 && role!=4)) {return false;}
    if(priority==3 && role!=1) {return false;}
    if(priority==4 && damage!=0) {return false;}
    if(priority<7 && (ability16 || role==4 || weapon==0x15)) {return false;}
    if(priority==8 && !(role==7 && owner!=0)) {return false;}return true;
}
Repeat CommandRepeat(int32_t result,int32_t before,int32_t after,int32_t order,int32_t triad,int32_t retries) {
    Require(retries>=0 && retries<=34 && before>=0 && after>=0 && triad>=0 && triad<=2,"Invalid native command repeat inputs");
    if(result || before!=after) {return {true,retries,false};}
    retries+=order==24 || triad==2?1:2;return {false,retries,retries>32};
}
Directive CarrierDirective(Geometry g,Coordinate point,Coordinate target,const TileQuery& tile,
    int32_t source_base,const std::function<bool(int32_t,uint8_t)>& connected) {
    Check(g);Require(Valid(g,point) && Valid(g,target) && bool(tile),"Invalid native carrier query");
    const auto source=tile(point),goal=tile(target);Directive result;
    if(source.sea && source.region==goal.region) {result.waypoint=target;result.direct=true;return result;}
    if(source_base>=0) {Require(bool(connected),"Native carrier inside Base requires actual water connectivity");}
    int32_t best=goal.sea?-1:0;
    for(const auto& delta:directive_offsets) {
        const auto p=Offset(g,target,delta);if(!p) {continue;}const auto c=tile(*p);
        if(!c.sea || (source_base>=0?!connected(source_base,c.region):c.region!=source.region)) {continue;}
        int32_t count=0;
        for(const auto& d:directions) {
            const auto q=Offset(g,*p,d);if(!q) {continue;}const auto t=tile(*q);
            if(!t.sea && t.region==goal.region && !((t.items&1u) && t.affiliation<8)) {++count;}
        }
        result.terms.push_back({*p,count});if(count>best) {best=count;result.waypoint=*p;}
    }return result;
}
Carrier CarrierStep(Geometry g,Coordinate point,const Base& target,int32_t work,int32_t count,int32_t capacity,const TileQuery& tile) {
    Check(g);Require(Valid(g,point) && Valid(g,target.point) && bool(tile) && work>=0 && count>=0 && capacity>=0,"Invalid native carrier step");
    if(!capacity || !count) {return {CarrierAction::NormalCompetition};}
    if(work>24) {return {CarrierAction::Disband};}
    for(int32_t direction=0;direction<8;++direction) {
        const auto p=Offset(g,point,directions[direction]);if(!p) {continue;}const auto c=tile(*p);
        if((c.items&2u) && c.affiliation>0 && c.affiliation<8) {
            if(c.sea) {return {CarrierAction::SeaAttack,direction,*p};}continue;
        }
        if(!c.sea && c.region==target.region && EvenManhattan(g,*p,target.point)<=12) {
            return {CarrierAction::ActivateStack,-1,*p,c.explored};
        }
    }return {CarrierAction::Directive};
}
}
