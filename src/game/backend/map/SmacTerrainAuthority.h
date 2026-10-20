#pragma once
#include "SmacPlanetMap.h"
#include <memory>
#include <optional>

namespace game::backend::map {
namespace smac_generation {struct State;struct Geometry;}
// Immutable owned terrain view, independent of installed-container provenance.
// Cell is the existing format-neutral 44-byte record and its field readers.
class SmacTerrainAuthority {
public:
    using Cell=SmacPlanetMap::Tile;
    enum class Origin {Installed,Generated};
    struct Metadata {
        uint32_t width=0,height=0;
        bool horizontal_wrap=false;
        int32_t pattern=0,sea_offset=0,climate_extra=0;
        // Missing means not established by this source adapter. Do not invent
        // installed land/erosion categories from display terrain or defaults.
        std::array<std::optional<int32_t>,7> categories{};
        std::optional<uint32_t> generation_primary_rules;
    };
    static SmacTerrainAuthority Installed(SmacPlanetMap source);
    static SmacTerrainAuthority GeneratedPrestartup(const smac_generation::State& state,smac_generation::Geometry geometry);
    Origin SourceOrigin() const {return m_origin;}
    const Metadata& Info() const {return m_metadata;}
    const Cell& At(uint32_t x,uint32_t y) const;
    const std::shared_ptr<const SmacPlanetMap>& InstalledSource() const {return m_installed;}
    // Complete raw records only, not a TERRANMAP container or campaign codec.
    std::string RecordBytes() const;
private:
    SmacTerrainAuthority()=default;
    Origin m_origin=Origin::Installed;
    Metadata m_metadata;
    std::shared_ptr<const SmacPlanetMap> m_installed;
    std::vector<Cell> m_generated;
};
}
