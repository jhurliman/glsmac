#include "Map.h"
#include "SmacPlanetMap.h"
#include "tile/Tile.h"
#include <stdexcept>

namespace game::backend::map {
GlsmacTerritoryTopology Map::PrepareTerritorySummary(const GlsmacTerritoryTopology& topology,const SmacWorkerKnowledge& knowledge) const {
    // Absence in older saves is unavailable producer/override history. Only
    // the explicit fresh local world admission initializes that authority.
    if(!topology.Summary()) {return topology;}
    const auto data=knowledge.CopyData();
    if(!m_resource_policy || !topology.SiteRatings() || topology.Width()!=GetWidth() || topology.Height()!=GetHeight()
        || !topology.Wraps() || data.width!=topology.Width() || data.height!=topology.Height() || !data.horizontal_wrap
        || data.tiles.size()!=topology.Regions().size()) {
        throw std::runtime_error("Territory summary lacks owned map, claims or site-cache inputs");
    }
    const auto* original=m_resource_policy->OriginalPlanet();
    std::vector<smac_territory::SummaryTile> tiles;tiles.reserve(data.tiles.size());
    for(int32_t y=0;y<topology.Height();++y) {for(int32_t x=y&1;x<topology.Width();x+=2) {
        const auto index=size_t(y)*(topology.Width()/2)+x/2;const auto* square=GetTile(x,y);
        const auto owner=data.tiles[index].claimant.value_or(0);
        if(owner<0 || owner>7) {throw std::runtime_error("Territory summary raw claimant is invalid");}
        uint8_t moisture=0,rockiness=0;bool special=false;
        if(original) {
            const auto& source=original->At(x,y);moisture=(source.Climate()>>3)&3;rockiness=source.bytes[5]>>6;
            special=(source.Items()&0x100000)!=0;
        } else if(!square->is_water_tile) {
            if(square->moisture<tile::MOISTURE_ARID || square->moisture>tile::MOISTURE_RAINY
                || square->rockiness<tile::ROCKINESS_FLAT || square->rockiness>tile::ROCKINESS_ROCKY) {
                throw std::runtime_error("Territory summary lacks decoded land terrain");
            }
            moisture=square->moisture-tile::MOISTURE_ARID;rockiness=square->rockiness-tile::ROCKINESS_FLAT;
        }
        // DisplayedTerrainV1 does not own original water climate bits or the
        // special marker. Its explicit projection uses zero, like resources.
        // InstalledPlanetV1 reads the saved original water climate as well.
        const smac_site::Coordinate point{{x,y}};
        tiles.push_back({topology.Regions()[index],static_cast<uint8_t>(owner),data.tiles[index].explored_by,moisture,rockiness,
            bool(square->features&tile::FEATURE_XENOFUNGUS),topology.SiteRatings()->Excluded(point),special,
            topology.SiteRatings()->Rating(point),m_resource_policy->SiteSignal(*square)});
    }}
    return topology.WithTerritorySummary(smac_territory::Summarize(tiles,data.all_map_revealed,topology.Summary()->search_objectives));
}
}
