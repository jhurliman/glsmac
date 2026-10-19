#include "SmacGlobalSpread.h"
#include <stdexcept>
#include <optional>

namespace game::backend::map::smac_spread {
namespace {
uint32_t Word(const SmacPlanetMap::Tile& tile,size_t offset) {
    const auto* b=tile.bytes.data()+offset;
    return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
}
void Put(SmacPlanetMap::Tile& tile,size_t offset,uint32_t value) {
    for(size_t i=0;i<4;++i) {tile.bytes[offset+i]=uint8_t(value>>(8*i));}
}
}
Result Prepare(const Input& input,const SmacGameplayRandomState& initial) {
    const auto g=input.geometry;
    if(g.width<2 || g.width>512 || (g.width&1) || g.height<2 || g.height>256
        || input.turn<0 || input.tile_count<0 || input.tile_count>65536
        || input.tiles.size()!=size_t(g.width/2)*g.height) {
        throw std::invalid_argument("Global spread requires bounded parity terrain, turn and tile count");
    }
    Result result{initial,input.tiles,{},{}};
    const auto at=[&](Coordinate p)->SmacPlanetMap::Tile& {return result.tiles.at(size_t(p[1])*(g.width/2)+p[0]/2);};
    constexpr Coordinate neighbors[]{{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}};
    const auto attempts=input.tile_count/(32+input.turn/4);
    for(int32_t iteration=0;iteration<attempts;++iteration) {
        auto x=int32_t(result.random.NextCrtRaw15())%g.width;
        const auto y=int32_t(result.random.NextCrtRaw15())%g.height;
        x+=(y&1)-(x&1);const Coordinate point{{x,y}};result.samples.push_back(point);
        const auto source=at(point);const bool sea=(source.Climate()>>5)<3;
        const uint32_t feature=sea ? 0x8000u:0x200000u;
        if(!(source.Items()&feature)) {continue;}
        int32_t best=0;std::optional<Coordinate> selected;
        for(const auto& delta:neighbors) {
            Coordinate p{{x+delta[0],y+delta[1]}};
            if(g.wrap) {p[0]=(p[0]%g.width+g.width)%g.width;}
            if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {continue;}
            const auto& target=at(p);const auto band=target.Climate()>>5,rain=(target.Climate()>>3)&3;
            if((band<3)!=sea || band<2 || (band>4 && !rain)) {continue;}
            const auto landmarks=target.Landmarks();
            if((landmarks&0x80000002u)==2 || (landmarks&0x80000040u)==0x40) {continue;}
            const auto rock=target.bytes[5]>>6;
            if(rock>1 || (target.Items()&0x208051u)) {continue;}
            const int32_t score=4*(3-rain)-rock;
            if(score>best) {best=score;selected=p;}
        }
        if(!selected) {continue;}
        auto& target=at(*selected);if(target.Items()&feature) {continue;}
        const auto before=target;
        Put(target,8,(target.Items()|feature)&~0x20u);
        for(size_t owner=1;owner<=7;++owner) {
            if(!(input.observer_status[owner]&0x200u) && !(source.bytes[4]&(1u<<owner))) {continue;}
            const auto offset=12+4*owner;
            if(Word(source,offset)&feature) {Put(target,offset,(Word(target,offset)|feature)&~0x20u);}
        }
        result.changes.push_back({iteration,point,*selected,feature,best,before,target});
    }
    return result;
}
}
