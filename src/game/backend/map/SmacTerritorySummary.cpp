#include "SmacTerritorySummary.h"
#include <stdexcept>

namespace game::backend::map::smac_territory {
namespace {
void Require(bool value) {if(!value) {throw std::runtime_error("Invalid retained territory summary");}}
}
bool TerritorySummary::Owner::operator==(const Owner& other) const {
    return tiles==other.tiles && known==other.known && suitable==other.suitable
        && covered_suitable==other.covered_suitable && maximum_rating==other.maximum_rating && site_signals==other.site_signals;
}
bool TerritorySummary::operator==(const TerritorySummary& other) const {
    return owners==other.owners && maximum_rating==other.maximum_rating && site_signals==other.site_signals
        && search_objectives==other.search_objectives;
}
std::string TerritorySummary::Serialize() const {
    Require(!(search_objectives&1));
    std::string wire;wire.reserve(WIRE_BYTES);
    const auto write=[&](uint32_t value,size_t bytes) {for(size_t i=0;i<bytes;++i) {wire.push_back(static_cast<char>((value>>(8*i))&255));}};
    for(const auto& owner:owners) {
        for(const auto* values:{&owner.tiles,&owner.known,&owner.suitable,&owner.covered_suitable}) {for(const auto value:*values) {write(value,2);}}
        for(const auto* values:{&owner.maximum_rating,&owner.site_signals}) {for(const auto value:*values) {write(value,1);}}
    }
    for(const auto* values:{&maximum_rating,&site_signals}) {for(const auto value:*values) {Require(value>=0);write(static_cast<uint32_t>(value),4);}}
    write(search_objectives,1);return wire;
}
TerritorySummary TerritorySummary::Deserialize(const std::string& wire) {
    Require(wire.size()==WIRE_BYTES);
    size_t offset=0;const auto read=[&](size_t bytes) {uint32_t value=0;for(size_t i=0;i<bytes;++i) {value|=uint32_t(static_cast<uint8_t>(wire[offset++]))<<(8*i);}return value;};
    TerritorySummary result;
    for(auto& owner:result.owners) {
        for(auto* values:{&owner.tiles,&owner.known,&owner.suitable,&owner.covered_suitable}) {for(auto& value:*values) {value=static_cast<uint16_t>(read(2));}}
        for(auto* values:{&owner.maximum_rating,&owner.site_signals}) {for(auto& value:*values) {value=static_cast<uint8_t>(read(1));}}
        for(const auto value:owner.maximum_rating) {Require(value<=15);}
    }
    for(auto* values:{&result.maximum_rating,&result.site_signals}) {for(auto& value:*values) {const auto raw=read(4);Require(raw<=INT32_MAX);value=static_cast<int32_t>(raw);}}
    for(const auto value:result.maximum_rating) {Require(value<=15);}
    result.search_objectives=static_cast<uint8_t>(read(1));Require(!(result.search_objectives&1));return result;
}
TerritorySummary Summarize(const std::vector<SummaryTile>& tiles,uint8_t all_map_revealed,uint8_t search_objectives) {
    Require(tiles.size()<=512*256/2 && !(search_objectives&1));
    TerritorySummary result;result.search_objectives=search_objectives;
    for(const auto& tile:tiles) {
        Require(tile.region<128 && tile.owner<8 && tile.moisture<4 && tile.rockiness<4 && tile.effective_rating<=15 && tile.site_signal<=2);
        auto& owner=result.owners[tile.owner];const auto region=tile.region;
        ++owner.tiles[region];
        if(tile.moisture && tile.rockiness<2 && !tile.fungus) {
            ++owner.suitable[region];if(tile.excluded) {++owner.covered_suitable[region];}
        }
        const auto rating=tile.excluded ? uint8_t(0) : tile.effective_rating;
        if(result.maximum_rating[region]<rating) {result.maximum_rating[region]=rating;}
        if(tile.owner && owner.maximum_rating[region]<rating) {owner.maximum_rating[region]=rating;}
        if(tile.site_signal) {++result.site_signals[region];++owner.site_signals[region];}
        for(size_t observer=1;observer<8;++observer) {
            const auto mask=uint8_t(1<<observer);
            const bool known=search_objectives&mask ? tile.special_marker : ((all_map_revealed|tile.explored_by)&mask)!=0;
            if(known) {++result.owners[observer].known[region];}
        }
    }
    return result;
}
bool RefreshTerritoryAtGlobalTail(size_t turn) {return turn%4==1;}
}
