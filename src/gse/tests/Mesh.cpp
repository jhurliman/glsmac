#include <array>
#include <memory>

#include "Mesh.h"

#include "task/gsetests/GSETests.h"
#include "types/mesh/Rectangle.h"

namespace gse::tests {

void AddMeshTests( task::gsetests::GSETests* task ) {

	task->AddTest(
		"test mesh coordinates can be read into caller-owned stack storage",
		GT() {
			try {
				types::mesh::Render mesh( 3, 1 );
				const std::array< types::Vec3, 3 > expected = { {
					{ -3.0f, 4.5f, 7.0f }, { 9.0f, -2.0f, 0.25f }, { 1.5f, 6.0f, -8.0f },
				} };
				for ( const auto& coord : expected ) { mesh.AddVertex( coord ); }
				mesh.AddSurface( { 0, 1, 2 } );
				mesh.Finalize();
				for ( size_t i = 0 ; i < expected.size() ; ++i ) {
					// The tile preview passes an ordinary local Vec3, not a tracked allocation.
					types::Vec3 output;
					mesh.GetVertexCoord( i, &output );
					GT_ASSERT( output == expected[ i ], "mesh coordinate changed" );
				}
			}
			catch ( const std::exception& error ) {
				GT_FAIL( std::string( "mesh coordinate read failed: " ) + error.what() );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test rectangle getters and render copies preserve caller-owned coordinates",
		GT() {
			try {
				types::mesh::Rectangle rectangle;
				const std::array< types::Vec3, 4 > coords = { {
					{ -2.0f, 5.0f, 0.5f }, { 3.0f, 5.0f, 0.5f },
					{ 3.0f, -7.0f, 0.5f }, { -2.0f, -7.0f, 0.5f },
				} };
				const std::array< types::Vec2< float >, 4 > tex_coords = { {
					{ 0.125f, 0.25f }, { 0.75f, 0.25f }, { 0.75f, 0.875f }, { 0.125f, 0.875f },
				} };
				for ( size_t i = 0 ; i < coords.size() ; ++i ) {
					rectangle.SetVertex( i, coords[ i ], tex_coords[ i ] );
					types::Vec3 coord;
					types::Vec2< float > tex_coord;
					rectangle.GetVertex( i, &coord, &tex_coord );
					GT_ASSERT( coord == coords[ i ], "rectangle coordinate changed" );
					GT_ASSERT( tex_coord == tex_coords[ i ], "rectangle texture coordinate changed" );
				}
				// CopyAsRenderMesh also uses stack outputs internally, then flips Y.
				const auto destroy_copy = []( types::mesh::Render* mesh ) { DELETE( mesh ); };
				std::unique_ptr< types::mesh::Render, decltype( destroy_copy ) > copy( rectangle.CopyAsRenderMesh(), destroy_copy );
				GT_ASSERT( copy->GetVertexCount() == 4 && copy->GetSurfaceCount() == 2, "render copy dimensions changed" );
				for ( size_t i = 0 ; i < coords.size() ; ++i ) {
					types::Vec3 coord;
					types::Vec2< float > tex_coord;
					copy->GetVertexCoord( i, &coord );
					copy->GetVertexTexCoord( i, &tex_coord );
					GT_ASSERT( coord == coords[ i ].InvertY(), "render copy coordinate changed" );
					GT_ASSERT( tex_coord == tex_coords[ i ], "render copy texture coordinate changed" );
				}
			}
			catch ( const std::exception& error ) {
				GT_FAIL( std::string( "rectangle coordinate read failed: " ) + error.what() );
			}
			GT_OK();
		}
	);
}

}
