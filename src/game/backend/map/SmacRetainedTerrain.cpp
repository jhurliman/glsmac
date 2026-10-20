#include "SmacRetainedTerrain.h"
#include "SmacClimate.h"
#include "SmacGlobalSpread.h"
#include "SmacStartingBalance.h"
#include "game/backend/base/SmacEcology.h"
#include <set>
#include <stdexcept>

namespace game::backend::map::retained_terrain {
namespace {
uint32_t Read32(const uint8_t* p) {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
void Validate(const Cells& cells,smac_site::Geometry g) {
    if(g.width<16 || g.width>512 || (g.width&1) || g.height<1 || g.height>256 || cells.size()!=size_t(g.width/2)*size_t(g.height)) {
        throw std::invalid_argument("Retained terrain requires a complete bounded parity grid");
    }
}
const Cell& At(const Cells& cells,smac_site::Geometry g,int32_t x,int32_t y) {return cells.at(size_t(y)*size_t(g.width/2)+size_t(x/2));}
}
Cells WithLiveGlobalSpread(const Cells& cells,smac_site::Geometry g,const smac_spread::Input& before,const smac_spread::Result& plan) {
    Validate(cells,g);
    if(before.geometry.wrap!=g.wrap) {throw std::invalid_argument("Spread topology differs from retained terrain");}
    if(before.geometry.width!=int32_t(g.width) || before.geometry.height!=int32_t(g.height)
        || before.tiles.size()!=(size_t(g.width/2)*size_t(g.height)) || cells.size()!=(size_t(g.width/2)*size_t(g.height))) {
        throw std::invalid_argument("Live flora projection requires the bound retained plane");
    }
    std::array<uint8_t,44> masks{};masks[0]=0x18;masks[4]=0xff;masks[5]=0xc0;
    for(size_t k=0;k<4;++k) {
        masks[8+k]=uint8_t(smac_spread::LIVE_MATERIAL_MASK>>(8*k));
        for(size_t owner=1;owner<=7;++owner) {masks[12+4*owner+k]=masks[8+k];}
    }
    auto projected=cells;
    for(size_t i=0;i<(size_t(g.width/2)*size_t(g.height));++i) {
        auto& tile=projected[i];const auto& current=before.tiles[i];
        for(size_t k=0;k<44;++k) {
            if((tile.bytes[k]^current.bytes[k])&uint8_t(~masks[k])) {
                throw std::invalid_argument("Live flora projection changed unowned retained terrain authority");
            }
            tile.bytes[k]=current.bytes[k];
        }
    }
    return WithGlobalSpread(projected,g,before,plan);
}

Cells WithGlobalSpread(const Cells& cells,smac_site::Geometry g,const smac_spread::Input& before,const smac_spread::Result& plan) {
    Validate(cells,g);
    if(before.geometry.wrap!=g.wrap) {throw std::invalid_argument("Spread topology differs from retained terrain");}
    if(before.geometry.width!=int32_t(g.width) || before.geometry.height!=int32_t(g.height)
        || before.tiles.size()!=(size_t(g.width/2)*size_t(g.height)) || plan.tiles.size()!=(size_t(g.width/2)*size_t(g.height)) || cells.size()!=(size_t(g.width/2)*size_t(g.height))) {
        throw std::invalid_argument("Global spread publication requires the bound retained terrain");
    }
    for(size_t i=0;i<(size_t(g.width/2)*size_t(g.height));++i) {
        if(before.tiles[i].bytes!=cells[i].bytes) {throw std::invalid_argument("Stale global spread terrain input");}
    }
    const auto valid=[&](const auto& p) {return p[0]>=0 && p[0]<int32_t(g.width) && p[1]>=0 && p[1]<int32_t(g.height) && !((p[0]^p[1])&1);};
    const auto put=[](Cell& tile,size_t offset,uint32_t value) {for(size_t i=0;i<4;++i) {tile.bytes[offset+i]=uint8_t(value>>(8*i));}};
    auto result=cells;int32_t previous=-1;
    for(const auto& change:plan.changes) {
        if(change.iteration<=previous || size_t(change.iteration)>=plan.samples.size() || !valid(change.source) || !valid(change.target)
            || plan.samples[change.iteration]!=change.source) {throw std::invalid_argument("Invalid global spread journal order or coordinates");}
        previous=change.iteration;
        const auto& source=At(result,g,change.source[0],change.source[1]);
        auto& target=result[size_t(change.target[1])*(g.width/2)+change.target[0]/2];
        const uint32_t feature=(source.Climate()>>5)<3 ? 0x8000u:0x200000u;
        if(change.feature!=feature || !(source.Items()&feature) || (target.Items()&feature)
            || change.source==change.target || target.bytes!=change.before.bytes) {
            throw std::invalid_argument("Stale global spread source or target journal");
        }
        auto expected=target;put(expected,8,(target.Items()|feature)&~0x20u);
        for(size_t owner=1;owner<=7;++owner) {
            if(!(before.observer_status[owner]&0x200u) && !(source.bytes[4]&(1u<<owner))) {continue;}
            const auto offset=12+4*owner;
            if(Read32(source.bytes.data()+offset)&feature) {put(expected,offset,(Read32(target.bytes.data()+offset)|feature)&~0x20u);}
        }
        if(expected.bytes!=change.after.bytes) {throw std::invalid_argument("Global spread journal changed unrelated terrain authority");}
        target=expected;
    }
    for(size_t i=0;i<(size_t(g.width/2)*size_t(g.height));++i) {
        if(result[i].bytes!=plan.tiles[i].bytes) {throw std::invalid_argument("Global spread terrain differs from its ordered journal");}
    }
    return result;
}

Cells WithStartingBalance(const Cells& cells,smac_site::Geometry g,const smac_balance::Result& plan) {
    Validate(cells,g);
    if(cells.size()!=(size_t(g.width/2)*size_t(g.height))) {throw std::logic_error("Incomplete retained terrain source");}
    auto result=cells;
    for(const auto& w:plan.writes) {
        const auto x=w.point[0],y=w.point[1];
        if(x<0 || x>=int32_t(g.width) || y<0 || y>=int32_t(g.height) || ((x^y)&1)
            || (w.mask!=0x04000000u && w.mask!=0x08000000u) || w.after!=(w.before|w.mask)) {
            throw std::invalid_argument("Invalid startup compensation terrain write");
        }
        auto& tile=result.at(size_t(y)*(g.width/2)+x/2);
        if(tile.Items()!=w.before) {throw std::invalid_argument("Stale startup compensation terrain authority");}
        for(size_t k=0;k<4;++k) {tile.bytes[8+k]=uint8_t(w.after>>(8*k));}
    }
    return result;
}

Cells WithBloomTerrain(const Cells& cells,smac_site::Geometry g,const base::SmacBloomTerrainPlan& plan) {
    Validate(cells,g);
    if (cells.size() != (size_t(g.width/2)*size_t(g.height))) { throw std::logic_error("Retained terrain is incomplete"); }
    auto result = cells;
    std::set<std::array<int32_t,2>> visited;
    constexpr uint32_t event_items = 0x00288070u; // fungus plus five removable improvements
    const auto write32 = [](uint8_t* bytes, uint32_t value) {
        for (size_t byte = 0; byte < 4; ++byte) { bytes[byte] = static_cast<uint8_t>(value >> (8 * byte)); }
    };
    for (const auto& delta : plan.Changes()) {
        const auto x = delta.coordinate[0], y = delta.coordinate[1];
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(g.width) || y >= static_cast<int32_t>(g.height)
            || ((x ^ y) & 1) || !visited.insert(delta.coordinate).second) {
            throw std::invalid_argument("Invalid or duplicate Retained bloom coordinate");
        }
        const auto& before = delta.before;
        const auto& after = delta.after;
        auto& tile = result.at(size_t(y) * (g.width / 2) + size_t(x) / 2);
        if ((tile.Climate() >> 5) != before.altitude_class || (tile.bytes[5] >> 6) != before.rockiness
            || tile.Landmarks() != before.landmarks) {
            throw std::invalid_argument("Retained bloom terrain authority is stale");
        }
        tile.bytes[0] = static_cast<uint8_t>((tile.bytes[0] & 0x1f) | (after.altitude_class << 5));
        tile.bytes[5] = static_cast<uint8_t>((tile.bytes[5] & 0x3f) | (after.rockiness << 6));
        // Preserve retained flags independently of the event and live registries.
        // Copy only the event-owned terrain bits.
        write32(tile.bytes.data() + 8, (tile.Items() & ~event_items) | (after.items & event_items));
        write32(tile.bytes.data() + 12, after.landmarks);
    }
    return result;
}

smac_climate::World ExportClimateTerrain(const Cells& cells,smac_site::Geometry g) {
    Validate(cells,g);
    if(cells.size()!=(size_t(g.width/2)*size_t(g.height))) {throw std::logic_error("Retained terrain is incomplete");}
    smac_climate::World world{g.width,g.height,g.wrap,{}};
    world.tiles.reserve((size_t(g.width/2)*size_t(g.height)));
    for(const auto& record:cells) {
        const auto& raw=record.bytes;
        smac_climate::Tile tile{raw[0]>>5,raw[0]&7,(raw[0]>>3)&3,raw[1],raw[6],record.Items(),record.Landmarks(),record.CachedOccupant(),raw[5]>>6,{}};
        for(size_t owner=0;owner<7;++owner) {tile.remembered_items[owner]=Read32(raw.data()+16+4*owner);}
        world.tiles.push_back(tile);
    }
    world.Validate();return world;
}
Cells WithClimateTerrain(const Cells& cells,smac_site::Geometry g,const smac_climate::World& world) {
    Validate(cells,g);
    if(cells.size()!=(size_t(g.width/2)*size_t(g.height))) {throw std::logic_error("Retained terrain is incomplete");}
    world.Validate();
    if(world.horizontal_wrap!=g.wrap) {throw std::invalid_argument("Climate topology differs from retained terrain");}
    if(world.width!=static_cast<int32_t>(g.width) || world.height!=static_cast<int32_t>(g.height)) {throw std::invalid_argument("Climate plane differs from retained geometry");}
    auto result=cells;
    const auto write=[](uint8_t* raw,uint32_t value) {for(size_t i=0;i<4;++i) {raw[i]=static_cast<uint8_t>(value>>(8*i));}};
    for(size_t i=0;i<(size_t(g.width/2)*size_t(g.height));++i) {
        const auto& tile=world.tiles[i];auto& raw=result[i].bytes;
        raw[0]=static_cast<uint8_t>((tile.elevation<<5)|(tile.moisture<<3)|tile.temperature);
        raw[1]=static_cast<uint8_t>(tile.contour);raw[6]=static_cast<uint8_t>(tile.aux);
        raw[2]=static_cast<uint8_t>((raw[2]&0xf0u)|tile.cached_occupant);
        raw[5]=static_cast<uint8_t>((raw[5]&0x3fu)|(tile.rockiness<<6));
        write(raw.data()+8,tile.items);write(raw.data()+12,tile.landmarks);
        for(size_t owner=0;owner<7;++owner) {write(raw.data()+16+4*owner,tile.remembered_items[owner]);}
    }
    return result;
}


}
