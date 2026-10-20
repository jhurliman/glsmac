#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <optional>

namespace game::native_terrain {
// Original Base-specific owner/visibility gate after a matching live record.
inline bool AdmitsBase(int viewer,int owner,uint8_t known_by) {
    return viewer>0 && viewer<8 && (owner==viewer || (known_by&(1u<<viewer)));
}
// Owned rendering inputs copied on the backend thread. No Tile/Map pointers,
// gameplay RNG state, assets or serialization lifetime cross this boundary.
struct Tile {
    uint8_t band=0,moisture=0,rockiness=0,contour=0;
    uint32_t items=0,selected_items=0,resource_items=0;
    bool resource_ready=false;
    bool admitted_base=false;
    bool developed_ready=false,selected_known=false,center_known=false,base_present=false;
    uint8_t explored_by=0; // persistent saved terrain exploration, not current sight
    uint8_t cached_affiliation=15;
    int64_t nutrients=0;
    uint32_t landmarks=0;
    uint8_t region=0;
    int8_t owner=-1;
    bool ordinary=false,deformed=false,material_admitted=false;
    std::optional<std::array<int,5>> heights,lighting;
};
struct World {
    static constexpr int WIDTH=80,HEIGHT=80;
    bool horizontal_wrap=true,retained_geometry=false;
    int viewer=0;
    bool exploration_ready=false;
    uint8_t observers_present=0,revealed_by=0;
    std::optional<bool> initial_survey;
    enum class Knowledge {Unknown,Survey,Known};
    std::optional<Knowledge> KnowledgeAt(size_t index) const {
        if(!exploration_ready || viewer<1 || viewer>7 || !(observers_present&(1u<<viewer))) {return {};}
        const auto bit=static_cast<uint8_t>(1u<<viewer);
        if((tiles.at(index).explored_by|revealed_by)&bit) {return Knowledge::Known;}
        if(!initial_survey) {return {};}
        return *initial_survey?Knowledge::Survey:Knowledge::Unknown;
    }
    int32_t resource_pattern=0;
    bool automatic_land_resources=false;
    std::array<uint32_t,8> relations{};
    std::array<std::string,8> faction_files{};
    std::array<Tile,WIDTH*HEIGHT/2> tiles{};
};
}
