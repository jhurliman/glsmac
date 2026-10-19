#include "Finalize.h"

#include "game/backend/map/Map.h"
#include "game/backend/map/MapState.h"
#include "game/backend/map/Consts.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/map/tile/TileState.h"
#include "game/backend/Random.h"
#include "types/mesh/Render.h"
#include "types/mesh/Data.h"

namespace game {
namespace backend {
namespace map {
namespace module {

void Finalize::GenerateTile( const tile::Tile* tile, tile::TileState* ts, MapState* ms ) {

	tile::tile_vertices_t vertices;
	tile::tile_tex_coords_t tex_coords;
	tile::tile_colors_t tint;

    // Installed/retained Planet carries original altitude bands separately
    // from the gameplay elevation adapter. Only replace the land mesh here;
    // contour-derived sea depth and maps without that provenance stay intact.
    const auto& policy=m_map->GetResourcePolicy();
    const auto* planet=policy?policy->OriginalPlanet():nullptr;
    const bool native_land=planet && !tile->is_water_tile &&
        tile->coord.x<SmacPlanetMap::WIDTH && tile->coord.y<SmacPlanetMap::HEIGHT &&
        (planet->At(tile->coord.x,tile->coord.y).Climate()>>5)>=3;
    ts->smac_projector_height.reset();
    if(native_land) {
        const auto band_at=[&](int x,int y)->std::optional<uint8_t> {
            if(y<0 || y>=int(SmacPlanetMap::HEIGHT)) {return std::nullopt;}
            x=(x+int(SmacPlanetMap::WIDTH))%int(SmacPlanetMap::WIDTH);
            return planet->At(x,y).Climate()>>5;
        };
        const auto heights=smac_terrain::OrdinaryHeights(int(tile->coord.x),int(tile->coord.y),band_at);
        ts->smac_projector_height=static_cast<uint8_t>((heights[1]+heights[2]+heights[3]+heights[4])/4);
        ts->layers[tile::LAYER_LAND].coords=smac_terrain::LandCoordinates(heights,ts->coord,s_consts.tile.radius);
    }

#define do_x() \
        x( center ); \
        x( left ); \
        x( top ); \
        x( right ); \
        x( bottom )

#define do_xs() \
        x( center, left, top ); \
        x( center, top, right ); \
        x( center, right, bottom ); \
        x( center, bottom, left )


	// fix some rare elevation glitches (slightly positive elevations on water tiles, slightly negative on land tiles)
	// TODO: investigate why it happens
	const uint8_t em = 1; // for elevations
	const float emf = 0.00000;//1f; // for vertices z
	if ( tile->is_water_tile && *tile->elevation.center > -em ) {
		*tile->elevation.center = -em;
	}
	else if ( !tile->is_water_tile && *tile->elevation.center < em ) {
		*tile->elevation.center = em;
	}

	for ( auto lt = 0 ; lt < tile::LAYER_MAX ; lt++ ) {

		// raise everything on z axis to prevent negative z values ( camera doesn't like it when zoomed in )
#define x( _k ) ts->layers[ lt ].coords._k.z += s_consts.tile.scale.z;
		do_x();
#undef x

		vertices = ts->layers[ lt ].coords;
		if ( lt == tile::LAYER_LAND && !tile->is_water_tile && !ts->has_water ) {

            if(native_land) {
                // Preserve the existing visual RNG schedule without moving the
                // native center off its exact fan position. Later art consumers
                // must not receive different random values because of heights.
                for(int draw=0;draw<2;++draw) {
                    m_map->GetRandom()->GetFloat(-s_consts.tile.random.center_coordinates_shift,
                        s_consts.tile.random.center_coordinates_shift);
                }
            }
            else {

			// smooth center vertices a bit and add some randomness

			auto& cw = ts->W->layers[ lt ].coords;
			auto& cn = ts->N->layers[ lt ].coords;
			auto& ce = ts->E->layers[ lt ].coords;
			auto& cs = ts->S->layers[ lt ].coords;
			vertices.center.z += (
				( cw.right.z - cw.left.z ) +
					( cn.bottom.z - cn.top.z ) +
					( ce.left.z - ce.right.z ) +
					( cs.top.z - cs.bottom.z )
			) / 12; // TODO: fix black lines when texture is perpendicular to camera

			if ( tile->is_water_tile && vertices.center.z > s_consts.tile.scale.z - emf ) {
				vertices.center.z = s_consts.tile.scale.z - emf;
			}
			if ( !tile->is_water_tile && vertices.center.z < s_consts.tile.scale.z + emf ) {
				vertices.center.z = s_consts.tile.scale.z + emf;
			}

#define xx( _k ) vertices.center._k += m_map->GetRandom()->GetFloat( -s_consts.tile.random.center_coordinates_shift, s_consts.tile.random.center_coordinates_shift ) * 0.05f
			xx( x );
			xx( y );
#undef xx

            }

		}
		else if ( lt != tile::LAYER_LAND ) {
#define x( _k ) vertices._k.z = s_consts.tile.water_level_z + s_consts.tile.scale.z
			do_x();
#undef x
		}

#define x( _k ) tex_coords._k = ts->layers[ lt ].tex_coords._k = { \
                ts->layers[ lt ].tex_coords._k.x * ms->variables.texture_scaling.x, \
                ( ts->layers[ lt ].tex_coords._k.y + lt * ms->dimensions.y * s_consts.tc.texture_pcx.dimensions.y ) * ms->variables.texture_scaling.y \
            }
		do_x();
#undef x

		tint.Set( ts->layers[ lt ].colors );

		// fix some rare elevation glitches (slightly positive elevations on water tiles, slightly negative on land tiles)
		// TODO: investigate why it happens
		{
#define x( _k ) \
                if ( !tile->is_water_tile ) { \
                    if ( lt == tile::LAYER_LAND && vertices._k.z < s_consts.tile.scale.z + emf ) { \
                        vertices._k.z = s_consts.tile.scale.z + emf; \
                    } \
                    else if ( lt == tile::LAYER_WATER_SURFACE && vertices._k.z > s_consts.tile.scale.z - emf ) { \
                        vertices._k.z = s_consts.tile.scale.z - emf; \
                    } \
                }
			do_x();
#undef x
		}

		if ( ms->first_run ) {
#define x( _k ) ts->layers[ lt ].indices._k = m_map->m_meshes.terrain->AddEmptyVertex()
			do_x();
#undef x
#define x( _a, _b, _c ) ts->layers[ lt ].surfaces._b##_##_c = m_map->m_meshes.terrain->AddSurface( { ts->layers[ lt ].indices._a, ts->layers[ lt ].indices._b, ts->layers[ lt ].indices._c } )
			do_xs();
#undef x
		}

#define x( _k ) m_map->m_meshes.terrain->SetVertex( ts->layers[ lt ].indices._k, vertices._k, tex_coords._k, tint._k )
		do_x();
#undef x

		if ( tile->coord.x == 0 && lt == tile::LAYER_LAND ) {

			// also copy tile to overdraw column

			// auto* layer = &ts->layers[ lt ]; // ?

#define x( _k ) { \
                ts->overdraw_column.coords._k.x = vertices._k.x + ms->dimensions.x * 0.5f; /* TODO: why 0.5? */ \
                ts->overdraw_column.coords._k.y = vertices._k.y; \
                ts->overdraw_column.coords._k.z = vertices._k.z; \
            }
			do_x();
#undef x

			/*Log( (string) "Copying land layer for overdraw from " +
				"[ ~" + to_string( vertices.center.x ) + ", ~" + to_string( vertices.center.y ) + " ]" +
				" to " +
				"[ ~" + to_string( ts->overdraw_column.coords.center.x ) + ", ~" + to_string( ts->overdraw_column.coords.center.y ) + " ]"
			);*/

			if ( ms->first_run ) {
#define x( _k ) ts->overdraw_column.indices._k = m_map->m_meshes.terrain->AddEmptyVertex()
				do_x();
#undef x
#define x( _a, _b, _c ) ts->overdraw_column.surfaces._b##_##_c = m_map->m_meshes.terrain->AddSurface( { ts->overdraw_column.indices._a, ts->overdraw_column.indices._b, ts->overdraw_column.indices._c } )
				do_xs();
#undef x
			}

#define x( _k ) m_map->m_meshes.terrain->SetVertex( ts->overdraw_column.indices._k, ts->overdraw_column.coords._k, tex_coords._k, tint._k )
			do_x();
#undef x

		}

	}

	// also add to data mesh for click lookups

	if ( tile->is_water_tile ) {
		vertices = ts->layers[ tile::LAYER_WATER ].coords;
	}
	else {
		vertices = ts->layers[ tile::LAYER_LAND ].coords;
		if ( ts->is_coastline_corner && !native_land ) {
			if ( tile->W->is_water_tile ) {
				vertices.left = ts->layers[ tile::LAYER_WATER ].coords.left;
			}
			if ( tile->N->is_water_tile ) {
				vertices.top = ts->layers[ tile::LAYER_WATER ].coords.top;
			}
			if ( tile->E->is_water_tile ) {
				vertices.right = ts->layers[ tile::LAYER_WATER ].coords.right;
			}
			if ( tile->S->is_water_tile ) {
				vertices.bottom = ts->layers[ tile::LAYER_WATER ].coords.bottom;
			}
			vertices.center.z = ( vertices.left.z + vertices.top.z + vertices.right.z + vertices.bottom.z ) / 4;
		}
	}

	// store tile coordinates
	types::mesh::data_t data = tile->coord.y * ms->dimensions.x + tile->coord.x + 1; // +1 because we need to differentiate 'tile at 0,0' from 'no tiles'

	if ( ms->first_run ) {
#define x( _k ) ts->data_mesh.indices._k = m_map->m_meshes.terrain_data->AddEmptyVertex()
		do_x();
#undef x
#define x( _a, _b, _c ) m_map->m_meshes.terrain_data->AddSurface( { ts->data_mesh.indices._a, ts->data_mesh.indices._b, ts->data_mesh.indices._c } )
		do_xs();
#undef x
	}

#define x( _k ) m_map->m_meshes.terrain_data->SetVertex( ts->data_mesh.indices._k, vertices._k, data )
	do_x();
#undef x

#undef do_x
#undef do_xs

}

}
}
}
}
