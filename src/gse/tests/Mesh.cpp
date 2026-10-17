#include <algorithm>
#include <array>
#include <memory>

#include "Mesh.h"

#include "task/gsetests/GSETests.h"
#include "types/mesh/Rectangle.h"
#include "config/Config.h"
#include "engine/Engine.h"
#include "graphics/Null.h"
#include "game/frontend/widget/Widget.h"
#include "gc/Space.h"
#include "gc/GC.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/context/GlobalContext.h"
#include "scene/Scene.h"
#include "scene/actor/Mesh.h"
#include "types/texture/Texture.h"
#include "ui/UI.h"
#include "ui/dom/Root.h"
#include "ui/dom/Widget.h"
#include "ui/geometry/Geometry.h"
#include "util/FinallyGuard.h"

namespace gse::tests {

// The headless engine's default viewport is 0x0. Only provide dimensions; all
// rendering remains disabled. This fixture outlives the local UI's GC runtime.
class WidgetGraphicsFixture {
public:
	WidgetGraphicsFixture() {
		// The headless GC worker also reads GetGraphics during collection.
		std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex );
		m_previous = g_engine->m_graphics;
		g_engine->m_graphics = &m_graphics;
	}
	~WidgetGraphicsFixture() {
		std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex );
		g_engine->m_graphics = m_previous;
	}
private:
	class SizedNull : public graphics::Null {
	public:
		SizedNull() { OnWindowResize(); }
		const unsigned short GetWindowWidth() const override { return 64; }
		const unsigned short GetWindowHeight() const override { return 64; }
		const unsigned short GetViewportWidth() const override { return 64; }
		const unsigned short GetViewportHeight() const override { return 64; }
	} m_graphics;
	graphics::Graphics* m_previous = nullptr;
};

static std::string CheckMeshCoordinates() {
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

static std::string CheckRectangleCoordinates() {
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

static std::string CheckConsumingRectangleConversion() {
	try {
		const auto destroy_rectangle = []( types::mesh::Rectangle* mesh ) { DELETE( mesh ); };
		const auto destroy_render = []( types::mesh::Render* mesh ) { DELETE( mesh ); };
		NEWV( rectangle, types::mesh::Rectangle );
		std::unique_ptr< types::mesh::Rectangle, decltype( destroy_rectangle ) > source( rectangle, destroy_rectangle );
		const types::Vec3 expected_coord( 3.0f, -7.0f, 0.5f );
		const types::Vec2< float > expected_tex( 0.25f, 0.75f );
		source->SetVertex( 2, expected_coord, expected_tex );
		const void* consumed_address = source.get();
		std::unique_ptr< types::mesh::Render, decltype( destroy_render ) > converted( source->ToRenderMesh(), destroy_render );
		source.release(); // ToRenderMesh consumes its tracked source object.
#ifdef DEBUG
		if ( g_engine->GetConfig()->HasDebugFlag( config::Config::DF_MEMORYDEBUG ) ) {
			// Delete checks/removes metadata only; it never frees or dereferences
			// the address. A second removal must fail. This also cleans the stale
			// record in the negative control, without relying on allocator reuse.
			bool removed = false;
			try { debug::g_memory_watcher->Delete( consumed_address, __FILE__, __LINE__ ); }
			catch ( const std::runtime_error& error ) {
				removed = std::string( error.what() ).find( "delete on non-allocated object" ) != std::string::npos;
			}
			GT_ASSERT( removed, "consuming rectangle conversion left its allocation tracked" );
		}
#endif
		types::Vec3 coord;
		types::Vec2< float > tex_coord;
		converted->GetVertexCoord( 2, &coord );
		converted->GetVertexTexCoord( 2, &tex_coord );
		GT_ASSERT( coord == expected_coord.InvertY() && tex_coord == expected_tex, "consuming conversion changed geometry or texture coordinates" );
	}
	catch ( const std::exception& error ) {
		GT_FAIL( std::string( "consuming rectangle conversion failed: " ) + error.what() );
	}
	GT_OK();
}

static std::string CheckWidgetMeshOwnership() {
	class WidgetPresenter : public game::frontend::widget::Widget {
	public:
		explicit WidgetPresenter( ::ui::UI* ui )
			: Widget( nullptr, ui, ::ui::WT_TILE_PREVIEW, "ownership-test", {} ) {}
		using Widget::AddMeshAndTexture;
		void Update( ::ui::dom::Widget*, const void* ) override {}
	private:
		void Register( ::ui::dom::Widget* ) override {}
	};
	std::string result;
	try {
		WidgetGraphicsFixture graphics;
		GSE runtime;
		auto* gc_space = runtime.GetGCSpace();
		gc_space->Accumulate( &runtime, [ & ]() {
			auto* ctx = runtime.CreateGlobalContext();
			const si_t si;
			ExecutionPointer ep;
			auto* ui = new ::ui::UI( GSE_CALL );
			auto* widget = new ::ui::dom::Widget( GSE_CALL, ui, nullptr, {} );
			util::FinallyGuard cleanup( [ & ]() {
				widget->Clear();
				widget->Destroy( GSE_CALL );
				// Keep the test root attached while releasing its scene actors.
				// Application-wide detached-root teardown is outside this fixture.
				ui->GetRoot()->Container::Destroy( GSE_CALL );
			} );
			WidgetPresenter presenter( ui );
			widget->GetGeometry()->SetWidth( 16 );
			widget->GetGeometry()->SetHeight( 16 );
			result = [ & ]() -> std::string {
				for ( size_t iteration = 0 ; iteration < 2 ; ++iteration ) {
					// Use the actual producer and DOM owner, including a second update.
					auto* actor = presenter.AddMeshAndTexture( widget, 0 );
					auto* scene = actor->GetScene();
					const void* actor_address = actor;
					const void* mesh_address = actor->GetMesh();
					const void* texture_address = widget->GetTexture();
					std::vector< scene::actor::Actor* > remaining_actors;
					scene->WithActors( [ & ]( const auto& actors ) { remaining_actors = actors; } );
					const auto registered = std::find( remaining_actors.begin(), remaining_actors.end(), actor );
					GT_ASSERT( registered != remaining_actors.end(), "widget actor was not registered with the scene" );
					remaining_actors.erase( registered );
					widget->Clear();
					bool others_preserved = false;
					scene->WithActors( [ & ]( const auto& actors ) { others_preserved = actors == remaining_actors; } );
#ifdef DEBUG
					if ( g_engine->GetConfig()->HasDebugFlag( config::Config::DF_MEMORYDEBUG ) ) {
						std::string stale;
						for ( const auto& entry : { std::pair< const void*, const char* >{ actor_address, "actor" }, { mesh_address, "mesh" }, { texture_address, "texture" } } ) {
							bool absent = false;
							try { debug::g_memory_watcher->Delete( entry.first, __FILE__, __LINE__ ); }
							catch ( const std::runtime_error& error ) {
								absent = std::string( error.what() ).find( "delete on non-allocated object" ) != std::string::npos;
							}
							// Metadata-only removal also cleans the negative control's stale record.
							if ( !absent ) { stale += std::string( " " ) + entry.second; }
						}
						GT_ASSERT( stale.empty(), "widget clear left tracked allocations:" + stale );
					}
#endif
					GT_ASSERT( others_preserved, "widget clear did not remove exactly its own scene actor" );
				}
				GT_OK();
			}();
		} );
	}
	catch ( const std::exception& error ) {
		GT_FAIL( std::string( "widget mesh ownership check failed: " ) + error.what() );
	}
	return result;
}

std::string CheckMeshCoordinateOutputs() {
	std::string errors;
	// Script callbacks hold their Space accumulation lock. UI fixtures create a
	// runtime and swap graphics under the global GC spaces lock, so they must
	// only run through the native test harness, outside script accumulation.
	for ( const auto& error : { CheckMeshCoordinates(), CheckRectangleCoordinates(), CheckConsumingRectangleConversion() } ) {
		if ( !error.empty() ) {
			if ( !errors.empty() ) { errors += "\n"; }
			errors += error;
		}
	}
	return errors;
}

void AddMeshTests( task::gsetests::GSETests* task ) {
	task->AddTest(
		"test mesh coordinates can be read into caller-owned stack storage",
		GT() { return CheckMeshCoordinates(); }
	);
	task->AddTest(
		"test rectangle getters and render copies preserve caller-owned coordinates",
		GT() { return CheckRectangleCoordinates(); }
	);
	task->AddTest(
		"test consuming rectangle conversion removes source allocation tracking",
		GT() { return CheckConsumingRectangleConversion(); }
	);
	task->AddTest(
		"test widget clear releases its actor, mesh and texture ownership",
		GT() { return CheckWidgetMeshOwnership(); }
	);
}

}
