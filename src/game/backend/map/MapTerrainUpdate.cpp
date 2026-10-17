#include "Map.h"

#include "Consts.h"
#include "MapState.h"
#include "game/backend/Random.h"
#include "module/LandSurface.h"
#include "tile/Tile.h"
#include "tile/TileState.h"
#include "types/texture/Texture.h"
#include <limits>
#include <set>
#include <stdexcept>

namespace game::backend::map {
namespace {
using Texture = types::texture::Texture;
constexpr uint16_t ORDINARY = tile::TERRAFORMING_FARM | tile::TERRAFORMING_SOLAR |
	tile::TERRAFORMING_MINE | tile::TERRAFORMING_FOREST | tile::TERRAFORMING_ROAD;

bool Ordinary( const tile::Tile& t, uint16_t improvements ) {
	return !t.is_water_tile && t.features == tile::FEATURE_NONE &&
		( t.rockiness == tile::ROCKINESS_NONE || t.rockiness == tile::ROCKINESS_FLAT ) &&
		t.moisture >= tile::MOISTURE_ARID && t.moisture <= tile::MOISTURE_RAINY && !( improvements & ~ORDINARY );
}
std::shared_ptr< Texture > Copy( const Texture& source, size_t x, size_t y ) {
	const auto dim = s_consts.tc.texture_pcx.dimensions;
	auto pixels = std::make_shared< Texture >( dim.x, dim.y, types::texture::TF_CPU_ONLY );
	pixels->AddFrom( &source, types::texture::AM_DEFAULT, x, y, x + dim.x - 1, y + dim.y - 1 );
	return pixels;
}
bool ImprovementSprite( const std::string& name ) {
	return name == "FarmLand" || name == "SolarLand" || name == "MineLand";
}
}

struct Map::PreparedTerrainUpdate::Data {
	TerrainRenderUpdate payload;
	TerrainCoastRecipes::Land land;
	tile::TileState* state = nullptr;
	tile::TileState::sprites_t sprites;
	std::unordered_map< std::string, sprite_actor_t > actors;
	std::unordered_map< size_t, std::pair< std::string, types::Vec3 > > instances;
	size_t next_id = 0;
};
Map::PreparedTerrainUpdate::PreparedTerrainUpdate() : data( std::make_unique< Data >() ) {}
Map::PreparedTerrainUpdate::~PreparedTerrainUpdate() = default;

void Map::RecordWaterPaint( const Texture* source, size_t x, size_t y, size_t dx, size_t dy,
	types::texture::add_flag_t flags, uint8_t rotate, float alpha, util::Perlin* perlin,
	std::optional< TerrainCoastRecipes::Key > land ) {
	if ( !m_capture_coast ) return;
	const auto random = GetRandom()->GetState();
	m_coast_recipes.Record( { dx, dy }, { Copy( *source, x, y ), land, flags, rotate, alpha, random, perlin } );
}

std::unique_ptr< Map::PreparedTerrainUpdate > Map::PrepareOrdinaryTerrainUpdate( const tile::Tile* changed, uint16_t terraforming ) {
	// The native headless fixture has no renderer. A loaded render dump is distinct:
	// it has source/state but no replay recipe and must not silently corrupt coasts.
	if ( !m_map_state || !m_textures.source.texture_pcx ) return {};
	if ( !m_terrain_updates_ready || m_land_canvas || m_current_ts )
		throw std::runtime_error( "terrain update requires initialized idle presentation" );
	if ( !changed || GetTile( changed->coord.x, changed->coord.y ) != changed || !Ordinary( *changed, changed->terraforming ) || !Ordinary( *changed, terraforming ) )
		throw std::runtime_error( "ordinary terrain presentation is unavailable" );
	auto prepared = std::unique_ptr< PreparedTerrainUpdate >( new PreparedTerrainUpdate );
	auto& out = *prepared->data;
	out.state = GetTileState( changed );
	out.next_id = m_next_sprite_instance_id;
	std::map< std::pair< size_t, size_t >, const tile::Tile* > affected;
	affected.emplace( std::make_pair( changed->coord.y, changed->coord.x ), changed );
	const auto connections = ( changed->terraforming ^ terraforming ) & ( tile::TERRAFORMING_ROAD | tile::TERRAFORMING_FOREST );
	for ( const auto* t : changed->neighbours ) {
		if ( !t->is_water_tile && ( t->terraforming & connections ) ) affected.emplace( std::make_pair( t->coord.y, t->coord.x ), t );
	}
	for ( const auto& [coord, t] : affected ) {
		const auto improvements = t == changed ? terraforming : t->terraforming;
		if ( !Ordinary( *t, improvements ) ) throw std::runtime_error( "connected terrain presentation is unavailable" );
		auto* ts = GetTileState( t );
		if ( !ts->moisture_original ) throw std::runtime_error( "missing terrain moisture canvas" );
		const auto dim = s_consts.tc.texture_pcx.dimensions;
		auto pixels = std::make_shared< Texture >( dim.x, dim.y, types::texture::TF_CPU_ONLY );
		// Presentation policy, not either gameplay stream. Stable across repeated
		// preparation, cancellation and changed neighbor connections.
		const uint32_t seed = 0x6c616e64u ^ static_cast< uint32_t >( t->coord.x * 73856093u ) ^ static_cast< uint32_t >( t->coord.y * 19349663u );
		Random random( nullptr, seed ? seed : 1 );
		m_current_ts = ts; m_land_canvas = pixels.get(); m_land_random = &random;
		m_land_changed = changed; m_land_terraforming = terraforming;
		try {
			module::LandSurface painter( this );
			painter.GenerateTile( t, ts, m_map_state );
		} catch ( ... ) {
			m_current_ts = nullptr; m_land_canvas = nullptr; m_land_random = nullptr; m_land_changed = nullptr;
			throw;
		}
		m_current_ts = nullptr; m_land_canvas = nullptr; m_land_random = nullptr; m_land_changed = nullptr;
		const TerrainCoastRecipes::Key key{ static_cast< size_t >( ts->tex_coord.x1 ), static_cast< size_t >( ts->tex_coord.y1 ) };
		out.land.emplace( key, pixels );
		out.payload.patches.push_back( { key.first, key.second, std::move( pixels ) } );
	}
	const auto coast = m_coast_recipes.Prepare( out.land );
	out.payload.patches.insert( out.payload.patches.end(), coast.begin(), coast.end() );

	// Keep bonus, pod and landmark instances untouched. Only the three ordinary
	// improvement sprites are replaced; Forest/Road are painted in the atlas.
	for ( const auto& sprite : out.state->sprites ) {
		if ( ImprovementSprite( sprite.name ) ) out.payload.removed.emplace_back( sprite.instance, sprite.actor );
		else out.sprites.push_back( sprite );
	}
	const auto& layer = out.state->layers[ tile::LAYER_LAND ].coords;
	const types::Vec3 center{ ( layer.left.x + layer.right.x ) / 2, -( layer.top.y + layer.bottom.y ) / 2,
		( layer.left.z + layer.top.z + layer.right.z + layer.bottom.z ) / 4 };
	const auto add = [&]( const std::string& name, pcx_texture_coordinates_t coords, float z ) {
		if ( out.next_id == std::numeric_limits< size_t >::max() ) throw std::runtime_error( "terrain sprite identity overflow" );
		const auto key = "Terrain_" + name + " " + coords.ToString() + " " + s_consts.tc.ter1_pcx.dimensions.ToString();
		const sprite_actor_t actor{ name, coords, z };
		out.payload.actors.emplace( key, actor );
		if ( !m_sprite_actors.count( key ) ) out.actors.emplace( key, actor );
		const auto id = out.next_id++;
		out.instances.emplace( id, std::make_pair( key, center ) );
		out.payload.added.push_back( { id, key, center } );
		out.sprites.push_back( { key, id, name, coords } );
	};
	if ( terraforming & tile::TERRAFORMING_SOLAR ) add( "SolarLand", s_consts.tc.ter1_pcx.solar_land[ 0 ], 0.002f * 11 );
	if ( terraforming & tile::TERRAFORMING_FARM ) add( "FarmLand", s_consts.tc.ter1_pcx.farm_land[ changed->moisture ], 0.002f * 9 );
	if ( terraforming & tile::TERRAFORMING_MINE ) add( "MineLand", s_consts.tc.ter1_pcx.mine_land[ 0 ], 0.002f * 13 );
	// Reserve before the caller's authority commit. Node transfers below allocate
	// nothing; capacity changes here do not publish a new actor or instance.
	m_sprite_actors.reserve( m_sprite_actors.size() + out.actors.size() );
	m_sprite_instances.reserve( m_sprite_instances.size() + out.instances.size() );
	return prepared;
}

const TerrainRenderUpdate& Map::GetTerrainUpdatePayload( const PreparedTerrainUpdate& prepared ) const { return prepared.data->payload; }
void Map::CommitOrdinaryTerrainUpdate( PreparedTerrainUpdate& prepared ) noexcept {
	auto& state = *prepared.data;
	for ( const auto& [id, actor] : state.payload.removed ) m_sprite_instances.erase( id );
	m_sprite_actors.merge( state.actors );
	m_sprite_instances.merge( state.instances );
	state.state->sprites.swap( state.sprites );
	m_next_sprite_instance_id = state.next_id;
	m_coast_recipes.Commit( state.land );
}

}
