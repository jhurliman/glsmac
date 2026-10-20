#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <array>
#include <memory>

namespace game::backend::base { class SmacBloomTerrainPlan; }

#include "types/Buffer.h"
#include "game/backend/resource/SmacResourceBonus.h"

namespace game::backend::map {
namespace tile { class Tile; }
namespace smac_climate { struct World; }
namespace smac_balance {struct Result;}
namespace smac_spread {struct Input;struct Result;}
class SmacPlanetMap;
class SmacTerrainAuthority;

// GLSMAC map semantics, not an original generator or height reconstruction.
// The raw Tile fields remain authoritative; this saved tag specifies how the
// admitted resource consumer interprets them. Absence is explicitly unavailable.
class GlsmacResourceMapPolicy {
public:
	static constexpr uint32_t MAX_WIRE_BYTES = 256 * 1024;
	static GlsmacResourceMapPolicy DisplayedTerrainV1();
	static GlsmacResourceMapPolicy InstalledPlanetV1( SmacPlanetMap source, bool automatic_land_resources );
	bool operator==( const GlsmacResourceMapPolicy& other ) const;
	const SmacPlanetMap* OriginalPlanet() const { return m_planet.get(); }
    const std::shared_ptr<const SmacPlanetMap>& OriginalPlanetSource() const { return m_planet; }
    const std::shared_ptr<const SmacTerrainAuthority>& NativeTerrainSource() const {return m_native;}
	bool AutomaticLandResources() const { return m_automatic_land_resources; }
    GlsmacResourceMapPolicy WithBloomTerrain(const base::SmacBloomTerrainPlan& plan) const;
    GlsmacResourceMapPolicy WithStartingBalance(const smac_balance::Result& plan) const;
    GlsmacResourceMapPolicy WithGlobalSpread(const smac_spread::Input& before,const smac_spread::Result& plan) const;
    GlsmacResourceMapPolicy WithLiveGlobalSpread(const smac_spread::Input& before,const smac_spread::Result& plan) const;
    GlsmacResourceMapPolicy WithClimateTerrain(const smac_climate::World& world,int32_t sea_level_offset,int32_t climate_extra) const;
	resource::smac::AltitudeBand Altitude( const tile::Tile& tile ) const;
	std::array<bool,3> LandmarkBonuses( const tile::Tile& tile ) const;

	// Water classification comes from Tile::Update's five-sample majority.
	// Depth labels use the same strict integer boundaries as tile_preview.
	resource::smac::AltitudeBand Altitude( bool water, int64_t center_elevation ) const;
	bool AllowsGeneratedBonus( bool water, int64_t center_elevation ) const;
	resource::smac::ResourceBonus Bonus( const tile::Tile& tile ) const;
	// Full original class on installed Planet; explicit display marker on legacy maps.
	uint8_t SiteSignal( const tile::Tile& tile ) const;

	types::Buffer Serialize() const;
	static GlsmacResourceMapPolicy Deserialize( const std::string& wire );
	static void WriteOptional( types::Buffer& output, const std::optional< GlsmacResourceMapPolicy >& policy, bool smac );
	static std::optional< GlsmacResourceMapPolicy > ReadOptional( types::Buffer& input, bool smac );

private:
    void SetOriginalSource(SmacPlanetMap source);
	GlsmacResourceMapPolicy() = default;
    static GlsmacResourceMapPolicy RetainedClimatePlanetV2(SmacPlanetMap source,bool automatic_land_resources);
    bool m_retained_climate=false;
	std::shared_ptr<const SmacPlanetMap> m_planet;
    std::shared_ptr<const SmacTerrainAuthority> m_native;
	bool m_automatic_land_resources = false;
};

}
