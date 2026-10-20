#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <stdexcept>

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
    explicit World(int w=WIDTH,int h=HEIGHT):width(w),height(h) {
        if(w<=0 || h<=0 || (w&1) || (h&1) || w>65534 || h>65534 ||
            uint64_t(w/2)*uint64_t(h)>UINT32_MAX/44u) {throw std::invalid_argument("Invalid native terrain world dimensions");}
        tiles.resize(size_t(w/2)*size_t(h));
    }
    int width=WIDTH,height=HEIGHT;
    bool ValidDimensions() const {
        return width>0 && height>0 && width<=65534 && height<=65534 && !(width&1) && !(height&1) &&
            uint64_t(width/2)*uint64_t(height)<=UINT32_MAX/44u &&
            uint64_t(width/2)*uint64_t(height)==tiles.size();
    }
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
    std::vector<Tile> tiles;
};
}
