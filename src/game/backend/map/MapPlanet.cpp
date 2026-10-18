#include "Map.h"
#include "SmacPlanetMap.h"
#include "tile/Tiles.h"
#include "game/backend/Game.h"
#include "util/FinallyGuard.h"
#include <fstream>
#include <algorithm>
#include <utility>

namespace game::backend::map {

const Map::error_code_t Map::LoadInstalledPlanet(const std::string& path, bool automatic_land_resources, MT_CANCELABLE) {
    m_game->RequireWorkerKnowledgeMutation();
    if(m_tiles || m_resource_policy || m_territory_topology) {throw std::runtime_error("Installed Planet requires an empty new-world map");}
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file || file.tellg()!=static_cast<std::streamoff>(SmacPlanetMap::FILE_BYTES)) {
        throw std::runtime_error("Installed standard maps/planet.MP is missing or has an unsupported size");
    }
    std::string bytes(SmacPlanetMap::FILE_BYTES,'\0');file.seekg(0);
    if(!file.read(bytes.data(),bytes.size())) {throw std::runtime_error("Could not read installed standard Planet map");}
    auto source=SmacPlanetMap::Decode(bytes);
    const auto policy=GlsmacResourceMapPolicy::InstalledPlanetV1(source,automatic_land_resources);
    if(canceled) {return EC_ABORTED;}
    tile::Tiles* staged=nullptr;NEW(staged,tile::Tiles,this,SmacPlanetMap::WIDTH,SmacPlanetMap::HEIGHT);
    util::FinallyGuard discard([&]() {if(staged) {DELETE(staged);} });
    const auto height=[&](int x,int y) {
        y=std::clamp(y,0,79);x=(x%80+80)%80;
        if((x&1)!=(y&1)) {x=(x+1)%80;}
        // Explicit GLSMAC display projection. Resource altitude remains the
        // original climate band, never this interpolated mesh height.
        return (int(source.At(x,y).Contour())-60)*50+25;
    };
    const auto corner=[&](int x,int y,int dx,int dy) {
        if(dx!=0) {return (height(x,y)+height(x+dx*2,y)+height(x+dx,y-1)+height(x+dx,y+1))/4;}
        return (height(x,y)+height(x,y+dy*2)+height(x-1,y+dy)+height(x+1,y+dy))/4;
    };
    for(uint32_t y=0;y<80;++y) {for(uint32_t x=y&1;x<80;x+=2) {
        const auto& original=source.At(x,y);auto& square=staged->At(x,y);const auto items=original.Items(),landmarks=original.Landmarks();
        square.moisture=1+((original.Climate()>>3)&3);square.rockiness=1+(original.bytes[5]>>6);
        square.features=0;square.terraforming=0;
        if(items&0x80) {square.features|=tile::FEATURE_RIVER;}
        if(items&0x2000) {square.features|=tile::FEATURE_MONOLITH;}
        if(items&0x20) {square.features|=tile::FEATURE_XENOFUNGUS;}
        if(!(landmarks&0x80000000u)) {
            if(landmarks&4) {square.features|=tile::FEATURE_JUNGLE;}
            if(landmarks&8) {square.features|=tile::FEATURE_URANIUM;}
            if(landmarks&0x400) {square.features|=tile::FEATURE_GEOTHERMAL;}
            if(landmarks&2) {square.features|=tile::FEATURE_VOLCANO;}
            if(landmarks&1) {square.features|=tile::FEATURE_GARLAND_CRATER;}
            if(landmarks&0x800) {square.features|=tile::FEATURE_FOSSIL_FIELD_RIDGE;}
        }
        for(const auto& bit : {std::pair<uint32_t,tile::terraforming_t>{4,tile::TERRAFORMING_ROAD},{8,tile::TERRAFORMING_MAG_TUBE},
            {0x10,tile::TERRAFORMING_MINE},{0x40,tile::TERRAFORMING_SOLAR},{0x8000,tile::TERRAFORMING_FARM},
            {0x80000,tile::TERRAFORMING_SOIL_ENRICHER},{0x200000,tile::TERRAFORMING_FOREST},
            {0x400000,tile::TERRAFORMING_CONDENSER},{0x800000,tile::TERRAFORMING_MIRROR},{0x1000000,tile::TERRAFORMING_BOREHOLE}}) {
            if(items&bit.first) {square.terraforming|=bit.second;}
        }
        const auto bonus=policy.Bonus(square);
        square.bonus=bonus==resource::smac::ResourceBonus::Nutrients ? tile::BONUS_NUTRIENT
            : bonus==resource::smac::ResourceBonus::Minerals ? tile::BONUS_MINERALS
            : bonus==resource::smac::ResourceBonus::Energy ? tile::BONUS_ENERGY : tile::BONUS_NONE;
        *square.elevation.left=corner(x,y,-1,0);*square.elevation.top=corner(x,y,0,-1);
        *square.elevation.right=corner(x,y,1,0);*square.elevation.bottom=corner(x,y,0,1);
    } if(canceled) {return EC_ABORTED;} }
    // The candidate Map is not published by Game until this entire import and
    // topology preparation succeeds. Guard rolls it back on any exception.
    util::FinallyGuard rollback([&]() {if(staged) {m_tiles=nullptr;m_resource_policy.reset();m_territory_topology.reset();} });
    m_tiles=staged;m_resource_policy=policy;
    for(uint32_t y=0;y<80;++y) {for(uint32_t x=y&1;x<80;x+=2) {staged->At(x,y).Update();}}
    m_territory_topology=PrepareFreshTerritoryTopology();
    if(canceled) {return EC_ABORTED;}
    staged=nullptr;
    return EC_NONE;
}
}
