#include "SmacRawSiteQuality.h"
#include "game/backend/resource/SmacResourceBonus.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include <stdexcept>

namespace game::backend::map::smac_site {
Score RawQuality(const std::vector<std::array<uint8_t,44>>& records,const Areas& areas,const RawQualityContext& c,Coordinate point) {
    const auto g=c.geometry;
    if(g.width<2 || g.width>512 || (g.width&1) || g.height<2 || g.height>256
        || records.size()!=size_t(g.width/2)*size_t(g.height)) {throw std::invalid_argument("Raw site quality requires the original parity grid");}
    const auto word=[](const auto& r,size_t off){return uint32_t(r[off])|(uint32_t(r[off+1])<<8)|(uint32_t(r[off+2])<<16)|(uint32_t(r[off+3])<<24);};
    const Query query=[&](auto p) {
        const auto& r=records.at(size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2));
        const auto items=word(r,8),marks=word(r,12);const auto band=resource::smac::AltitudeBand(r[0]>>5);const bool automatic=c.rules&0x2000;
        const bool bonus=resource::smac::ResolveResourceBonus({p[0],p[1],band,c.pattern,automatic,bool(items&0x400),
            bool(items&0x20000000),bool(items&0x10000),bool(items&0x20000)})!=resource::smac::ResourceBonus::None;
        const bool site=resource::smac::ResolveSiteSignal({p[0],p[1],band,items,automatic,c.pattern,c.coefficient_u,c.coefficient_v})!=0;
        return Tile{uint8_t(r[0]>>5),uint8_t((r[0]>>3)&3),uint8_t(r[5]>>6),r[3],
            bool(items&0x20),bool(items&0x2000),bool(items&0x200000),bool(items&0x80),bool(items&0x1000),bool(items&1),
            bonus,site,bool(items&0x40004000),bool(marks&2),bool(marks&1),bool(marks&8),bool(marks&4),r[15]};
    };
    return Quality(g,point,0,areas,query);
}
}
