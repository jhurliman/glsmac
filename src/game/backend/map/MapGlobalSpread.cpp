#include "Map.h"
#include "SmacGlobalSpread.h"
#include "SmacNativeTerrainItems.h"
#include "tile/Tile.h"
#include <stdexcept>

namespace game::backend::map {
smac_spread::Input Map::CopyGlobalSpreadInput(int32_t turn,const std::array<uint32_t,8>& status) const {
    const auto* planet=m_resource_policy?m_resource_policy->OriginalPlanet():nullptr;
    if(!planet || GetWidth()!=80 || GetHeight()!=80 || !m_territory_topology
        || !m_worker_knowledge || !m_worker_knowledge_ready || turn<0) {
        throw std::runtime_error("Global flora requires current installed Planet terrain and ready knowledge");
    }
    const auto knowledge=m_worker_knowledge->CopyData();
    const auto wrap=m_territory_topology->Wraps();
    if(knowledge.width!=80 || knowledge.height!=80 || knowledge.horizontal_wrap!=wrap
        || knowledge.tiles.size()!=3200) {
        throw std::runtime_error("Global flora knowledge geometry differs from current map");
    }
    smac_spread::Input result{{80,80,wrap},3200,turn,status,{}};
    result.tiles.reserve(3200);
    // Only flora's material inputs are projected. This is not an authoritative
    // 44-byte gameplay plane: site, occupancy and other retained words below
    // must not be used for wildlife or published as current entity authority.
    constexpr uint32_t material=4u|8u|0x10u|0x20u|0x40u|0x80u|0x800u|0x2000u|
        0x8000u|0x40000u|0x80000u|0x200000u|0x400000u|0x800000u|0x1000000u|0x80000000u;
    const auto items=[](uint16_t features,uint16_t work) {
        return NativeTerrainItems(work,features)|((features&tile::FEATURE_MONOLITH)?0x2000u:0u);
    };
    const auto put=[](SmacPlanetMap::Tile& tile,size_t offset,uint32_t value) {
        for(size_t k=0;k<4;++k) {tile.bytes[offset+k]=uint8_t(value>>(8*k));}
    };
    const auto word=[](const SmacPlanetMap::Tile& tile,size_t offset) {
        uint32_t value=0;for(size_t k=0;k<4;++k) {value|=uint32_t(tile.bytes[offset+k])<<(8*k);}return value;
    };
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
        const size_t index=size_t(y)*40+x/2;
        auto output=planet->At(x,y);const auto* live=GetTile(x,y);
        if(!live || live->moisture<1 || live->moisture>4 || live->rockiness<1 || live->rockiness>4
            || (live->terraforming&~uint16_t(0x1fff))) {
            throw std::runtime_error("Global flora requires admitted live moisture, rockiness and improvements");
        }
        output.bytes[0]=uint8_t((output.bytes[0]&~0x18u)|((live->moisture-1)<<3));
        output.bytes[5]=uint8_t((output.bytes[5]&0x3fu)|((live->rockiness-1)<<6));
        output.bytes[4]=knowledge.tiles[index].explored_by;
        put(output,8,(output.Items()&~material)|items(live->features,live->terraforming));
        for(size_t owner=1;owner<=7;++owner) {
            const auto& row=knowledge.remembered[owner];
            const bool needed=(status[owner]&0x200u) || (output.bytes[4]&(1u<<owner));
            if(row.empty() || !row.at(index)) {
                if(needed) {throw std::runtime_error("Global flora cannot substitute current terrain for missing observer memory");}
                continue;
            }
            const auto& memory=*row[index];const auto offset=12+4*owner;
            put(output,offset,(word(output,offset)&~material)|items(memory.features,memory.terraforming));
        }
        result.tiles.push_back(output);
    }}
    return result;
}
}
