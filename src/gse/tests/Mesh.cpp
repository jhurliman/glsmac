#include <algorithm>
#include <array>
#include <memory>
#include <filesystem>

#include "Mesh.h"
#include "loader/texture/SmacPalette.h"

#include "task/gsetests/GSETests.h"
#include "types/mesh/Rectangle.h"
#include "config/Config.h"
#include "engine/Engine.h"
#include "graphics/Null.h"
#include "game/frontend/widget/Widget.h"
#include "game/frontend/widget/Minimap.h"
#include "gc/Space.h"
#include "gc/GC.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/Exception.h"
#include "gse/context/GlobalContext.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Int.h"
#include "rr/Capture.h"
#include "loader/texture/SDL2.h"
#include "resource/ResourceManager.h"
#include "scene/Scene.h"
#include "scene/actor/Cache.h"
#include "scene/actor/Mesh.h"
#include "scene/actor/Sprite.h"
#include "types/texture/Texture.h"
#include "ui/UI.h"
#include "ui/dom/Root.h"
#include "ui/dom/Widget.h"
#include "ui/dom/Surface.h"
#include "ui/geometry/Geometry.h"
#include "util/FinallyGuard.h"
#include "util/UUID.h"

namespace gse::tests {

// The headless engine's default viewport is 0x0. Only provide dimensions; all
// rendering remains disabled. This fixture outlives the local UI's GC runtime.
class WidgetGraphicsFixture {
public:
	explicit WidgetGraphicsFixture( const std::string& texture_path = {} ) {
		if ( !texture_path.empty() ) {
			m_resources = std::make_unique< resource::ResourceManager >( texture_path );
			m_textures = std::make_unique< loader::texture::SDL2 >();
		}
		// The headless GC worker also reads GetGraphics during collection.
		std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex );
		m_previous = g_engine->m_graphics;
		g_engine->m_graphics = &m_graphics;
		if ( m_textures ) {
			m_previous_resources = g_engine->m_resource_manager;
			m_previous_textures = g_engine->m_texture_loader;
			g_engine->m_resource_manager = m_resources.get();
			g_engine->m_texture_loader = m_textures.get();
		}
	}
	~WidgetGraphicsFixture() {
		std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex );
		if ( m_textures ) {
			g_engine->m_resource_manager = m_previous_resources;
			g_engine->m_texture_loader = m_previous_textures;
			m_textures.reset(); // release cached textures while the null graphics is alive
		}
		g_engine->m_graphics = m_previous;
	}
	size_t TextureUnloadCount( const types::texture::Texture* texture ) const {
		std::lock_guard guard( m_graphics.texture_mutex );
		const auto it = m_graphics.texture_unloads.find( texture );
		return it == m_graphics.texture_unloads.end() ? 0 : it->second;
	}
private:
	class SizedNull : public graphics::Null {
	public:
		SizedNull() { OnWindowResize(); }
		const unsigned short GetWindowWidth() const override { return 64; }
		const unsigned short GetWindowHeight() const override { return 64; }
		const unsigned short GetViewportWidth() const override { return 64; }
		const unsigned short GetViewportHeight() const override { return 64; }
		void UnloadTexture( const types::texture::Texture* texture ) override {
			std::lock_guard guard( texture_mutex );
			++texture_unloads[ texture ];
		}
		mutable std::mutex texture_mutex;
		std::unordered_map< const types::texture::Texture*, size_t > texture_unloads;
	} m_graphics;
	graphics::Graphics* m_previous = nullptr;
	std::unique_ptr< resource::ResourceManager > m_resources;
	std::unique_ptr< loader::texture::SDL2 > m_textures;
	resource::ResourceManager* m_previous_resources = nullptr;
	loader::texture::TextureLoader* m_previous_textures = nullptr;
};

std::string WithWidgetGraphics( const std::function< std::string() >& check ) {
	WidgetGraphicsFixture graphics;
	return check();
}

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

static std::string CheckDetachedUIOwnership( const bool detached ) {
	class CountedTexture : public types::texture::Texture {
	public:
		explicit CountedTexture( size_t& destroyed ) : m_destroyed( destroyed ) { Resize( 2, 2 ); }
		~CountedTexture() override { ++m_destroyed; }
	private:
		size_t& m_destroyed;
	};
	std::string result;
	try {
		WidgetGraphicsFixture graphics;
		size_t textures_destroyed = 0;
		GSE runtime;
		auto* gc_space = runtime.GetGCSpace();
		gc_space->Accumulate( &runtime, [ & ]() {
			auto* ctx = runtime.CreateGlobalContext();
			const si_t si;
			ExecutionPointer ep;
			auto* ui = new ::ui::UI( GSE_CALL );
			auto* root = static_cast< value::Object* >( ui->GetRoot()->Wrap( GSE_CALL ) );
			// The public factories register actual children with the real root.
			static_cast< value::Callable* >( root->Get( "surface" ) )->Run( GSE_CALL, {} );
			size_t deinit_calls = 0;
			bool deinit_saw_texture = false;
			types::texture::Texture* expected_texture = nullptr;
			ui->RegisterWidget( ::ui::WT_TILE_PREVIEW, { "ownership-test", {},
				[]( ::ui::dom::Widget* ) {},
				[ & ]( ::ui::dom::Widget* w ) {
					++deinit_calls;
					deinit_saw_texture = textures_destroyed == 0 && w->GetTexture() == expected_texture;
				} } );
			auto* properties = new value::Object( GSE_CALL, {
				{ "type", new value::String( gc_space, "ownership-test" ) },
				{ "data", new value::Object( GSE_CALL ) },
			} );
			auto* widget = static_cast< ::ui::dom::Widget* >( ::ui::dom::Object::Unwrap(
				static_cast< value::Callable* >( root->Get( "widget" ) )->Run( GSE_CALL, { properties } ) ) );
			NEWV( texture, CountedTexture, textures_destroyed );
			expected_texture = texture;
			widget->SetTexture( texture, 0 );
			NEWV( mesh, types::mesh::Rectangle );
			auto* actor = new scene::actor::Mesh( "detached-widget-test", mesh );
			widget->SetActor( actor, 0 );
			auto* scene = actor->GetScene();
			// An unrelated actor must remain registered and alive.
			auto unrelated = std::make_unique< scene::actor::Mesh >( "unrelated", nullptr );
			scene->AddActor( unrelated.get() );
			util::FinallyGuard unrelated_cleanup( [ & ]() { scene->RemoveActor( unrelated.get() ); } );
			if ( !detached ) {
				auto* wrapped = static_cast< value::Object* >( widget->Wrap( GSE_CALL ) );
				static_cast< value::Callable* >( wrapped->Get( "remove" ) )->Run( GSE_CALL, {} );
			}
			ui->Destroy( GSE_CALL );
			std::vector< scene::actor::Actor* > remaining;
			scene->WithActors( [ & ]( const auto& actors ) { remaining = actors; } );
			const bool exact_actors = remaining == std::vector< scene::actor::Actor* >{ unrelated.get() };
			const bool exact_textures = textures_destroyed == 1;
			// Clean the old-code negative control without hiding its observed result.
			for ( auto* retained : remaining ) {
				scene->RemoveActor( retained );
				if ( retained != unrelated.get() ) { delete retained; }
			}
			widget->Clear(); // repeated clearing is safe after destruction
			result = [ & ]() -> std::string {
				GT_ASSERT( exact_actors, "detached UI retained its scene actors" );
				GT_ASSERT( exact_textures && textures_destroyed == 1, "detached widget did not release its texture exactly once" );
				GT_ASSERT( detached ? deinit_calls == 0 : deinit_calls == 1 && deinit_saw_texture, "widget deinit resource visibility or call count changed" );
				GT_OK();
			}();
		} );
	}
	catch ( const std::exception& error ) {
		GT_FAIL( std::string( "detached UI ownership check failed: " ) + error.what() );
	}
	return result;
}

static std::string CheckGeneratedSpriteOwnership() {
	class WidgetPresenter : public game::frontend::widget::Widget {
	public:
		explicit WidgetPresenter( ::ui::UI* ui ) : Widget( nullptr, ui, ::ui::WT_TILE_PREVIEW, "sprite-test", {} ) {}
		using Widget::AddMeshAndTexture;
		void Update( ::ui::dom::Widget*, const void* ) override {}
		void Register( ::ui::dom::Widget* ) override {}
	};
	class CountedMesh : public types::mesh::Rectangle {
	public:
		explicit CountedMesh( size_t& destroyed ) : m_destroyed( destroyed ) {}
		~CountedMesh() override { ++m_destroyed; }
	private:
		size_t& m_destroyed;
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
			WidgetPresenter presenter( ui );
			util::FinallyGuard cleanup( [ & ]() {
				widget->Destroy( GSE_CALL );
				ui->GetRoot()->Container::Destroy( GSE_CALL );
			} );
			widget->GetGeometry()->SetWidth( 16 );
			widget->GetGeometry()->SetHeight( 16 );
			types::texture::Texture atlas( 2, 2 );
			const auto color = types::Color::RGBA( 31, 63, 127, 255 );
			atlas.SetPixel( 0, 0, color );
			scene::actor::Sprite sprite( "ownership", { 2, 3 }, &atlas, { { 0, 0 }, { 1, 1 } }, { 0, 0 } );
			result = [ & ]() -> std::string {
				for ( const bool unwinding : { false, true } ) {
					size_t destroyed = 0;
					NEWV( counted, CountedMesh, destroyed );
					try {
						scene::actor::Sprite::mesh_ptr_t owned( counted );
						if ( unwinding ) { throw std::runtime_error( "copy interrupted" ); }
					}
					catch ( const std::runtime_error& ) {}
					const bool released = destroyed == 1;
					if ( !released ) { DELETE( counted ); } // clean an intentionally disabled deleter control
					GT_ASSERT( released, "generated-mesh deleter did not release its owner exactly once" );
					const types::mesh::Render* source = nullptr;
					scene::actor::Mesh* actor = nullptr;
					try {
						auto mesh = sprite.GenerateMesh();
						source = mesh.get();
						actor = presenter.AddMeshAndTexture( widget, 0, mesh.get(), sprite.GetTexture(), true );
						GT_ASSERT( actor->GetMesh() != source, "widget unexpectedly consumed the source geometry" );
						if ( unwinding ) { throw std::runtime_error( "after widget copy" ); }
					}
					catch ( const std::runtime_error& ) {}
#ifdef DEBUG
					if ( g_engine->GetConfig()->HasDebugFlag( config::Config::DF_MEMORYDEBUG ) ) {
						bool absent = false;
						try { debug::g_memory_watcher->Delete( source, __FILE__, __LINE__ ); }
						catch ( const std::runtime_error& error ) {
							absent = std::string( error.what() ).find( "delete on non-allocated object" ) != std::string::npos;
						}
						if ( !absent ) { delete source; } // metadata was removed above
						GT_ASSERT( absent, "sprite source mesh survived widget copying" );
					}
#endif
					GT_ASSERT( actor->GetMesh()->GetVertexCount() == 4 && actor->GetMesh()->GetIndexCount() == 6, "copied sprite geometry changed" );
					GT_ASSERT( actor->GetTexture() == &atlas, "sprite atlas borrowing changed" );
					widget->Clear();
					GT_ASSERT( atlas.GetPixel( 0, 0 ) == color, "clearing sprite widget damaged the borrowed atlas" );
				}
				GT_OK();
			}();
		} );
	}
	catch ( const std::exception& error ) { GT_FAIL( std::string( "sprite ownership check failed: " ) + error.what() ); }
	return result;
}

static std::string CheckMinimapCaptureOwnership() {
	class CountedTexture : public types::texture::Texture {
	public:
		CountedTexture( size_t& destroyed, bool fail ) : Texture( 2, 2 ), m_destroyed( destroyed ), m_fail( fail ) {}
		~CountedTexture() override { ++m_destroyed; }
		const size_t GetWidth() const override {
			if ( m_fail ) { throw std::runtime_error( "capture read failed" ); }
			return Texture::GetWidth();
		}
	private:
		size_t& m_destroyed;
		bool m_fail;
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
			game::frontend::widget::Minimap minimap( nullptr, ui );
			util::FinallyGuard cleanup( [ & ]() { ui->GetRoot()->Container::Destroy( GSE_CALL ); } );
			auto* root = static_cast< value::Object* >( ui->GetRoot()->Wrap( GSE_CALL ) );
			auto create_widget = [ & ]() {
				auto* properties = new value::Object( GSE_CALL, {
					{ "type", new value::String( gc_space, "minimap" ) },
					{ "width", new value::Int( gc_space, 2 ) },
					{ "height", new value::Int( gc_space, 2 ) },
				} );
				return static_cast< ::ui::dom::Widget* >( ::ui::dom::Object::Unwrap(
					static_cast< value::Callable* >( root->Get( "widget" ) )->Run( GSE_CALL, { properties } ) ) );
			};
			auto* widget = create_widget();
			result = [ & ]() -> std::string {
				GT_ASSERT( widget->GetTexture( 0 )->GetWidth() == 2 && widget->GetTexture( 0 )->GetHeight() == 2,
					"minimap fixture size is " + std::to_string( widget->GetTexture( 0 )->GetWidth() ) + "x" + std::to_string( widget->GetTexture( 0 )->GetHeight() ) );
				for ( size_t iteration = 0 ; iteration < 2 ; ++iteration ) {
					size_t destroyed = 0;
					const auto color = types::Color::RGBA( 31 + iteration, 63, 127, 255 );
					NEWV( texture, CountedTexture, destroyed, false );
					for ( size_t y = 0 ; y < 2 ; ++y ) {
						for ( size_t x = 0 ; x < 2 ; ++x ) { texture->SetPixel( x, y, color ); }
					}
					// Model only the GPU producer; exercise real response transfer and consuming copy.
					scene::actor::Mesh terrain( "capture-test" );
					const auto id = terrain.CaptureToTexture( nullptr, { 2, 2 } );
					auto* response = terrain.RR_GetRequests< rr::Capture >().at( 0 );
					response->texture = texture;
					response->SetProcessed();
					minimap.UpdateCapture( terrain.GetCaptureToTextureResponse( id ) );
					const bool released = destroyed == 1;
					if ( !released ) { DELETE( texture ); }
					GT_ASSERT( released, "consumed framebuffer capture was not released exactly once" );
					GT_ASSERT( widget->GetTexture( 0 )->GetPixel( 0, 0 ) == color, "minimap pixels did not survive capture destruction" );
					// Reattach after layout: initial property sizing occurs after registration.
					// The new layers must redraw from the cache after the source is destroyed.
					widget->Hide();
					widget->Show();
					GT_ASSERT( widget->GetTexture( 0 )->GetPixel( 0, 0 ) == color, "minimap redraw retained the destroyed capture" );
				}
				size_t destroyed = 0;
				NEWV( failed, CountedTexture, destroyed, true );
				bool threw = false;
				try { minimap.UpdateCapture( failed ); }
				catch ( const std::runtime_error& error ) { threw = std::string( error.what() ) == "capture read failed"; }
				const bool released = destroyed == 1;
				if ( !released ) { DELETE( failed ); }
				GT_ASSERT( threw && released, "failed minimap copying did not release its capture" );
				GT_OK();
			}();
		} );
	}
	catch ( const std::exception& error ) { GT_FAIL( std::string( "minimap ownership check failed: " ) + error.what() ); }
	return result;
}

static std::string CheckSurfaceBackgroundOwnership() {
	std::string result;
	try {
		const auto directory = std::filesystem::temp_directory_path() / ( "glsmac-surface-" + util::UUID::Generate() );
		std::filesystem::create_directory( directory );
		const auto data_directory = directory / "default";
		const auto image_path = data_directory / "fixture.bmp";
		util::FinallyGuard remove_fixture( [ & ]() {
			std::filesystem::remove( image_path );
			std::filesystem::remove( data_directory );
			std::filesystem::remove( directory );
		} );
		std::filesystem::create_directory( data_directory );
		auto* image = SDL_CreateRGBSurfaceWithFormat( 0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32 );
		GT_ASSERT( image, "could not allocate synthetic image" );
		{
			util::FinallyGuard release_image( [ & ]() { SDL_FreeSurface( image ); } );
			GT_ASSERT( SDL_FillRect( image, nullptr, SDL_MapRGBA( image->format, 32, 64, 128, 255 ) ) == 0, "could not fill synthetic image" );
			GT_ASSERT( SDL_SaveBMP( image, image_path.string().c_str() ) == 0, "could not save synthetic image" );
		}
		WidgetGraphicsFixture graphics( directory.string() );
		const types::texture::Texture* final_texture = nullptr;
		size_t final_unloads = 0;
		types::texture::Texture* borrowed = nullptr;
		size_t borrowed_unloads = 0;
		{
			GSE runtime;
			auto* gc_space = runtime.GetGCSpace();
			gc_space->Accumulate( &runtime, [ & ]() {
				auto* ctx = runtime.CreateGlobalContext();
				const si_t si;
				ExecutionPointer ep;
				auto* ui = new ::ui::UI( GSE_CALL );
				util::FinallyGuard cleanup( [ & ]() { ui->GetRoot()->Container::Destroy( GSE_CALL ); } );
				auto* scene = ui->GetRoot()->m_cache->GetScene();
				std::vector< scene::actor::Actor* > before;
				scene->WithActors( [ & ]( const auto& actors ) { before = actors; } );
				auto* root = static_cast< value::Object* >( ui->GetRoot()->Wrap( GSE_CALL ) );
				auto* surface = static_cast< ::ui::dom::Surface* >( ::ui::dom::Object::Unwrap(
					static_cast< value::Callable* >( root->Get( "surface" ) )->Run( GSE_CALL, {} ) ) );
				scene::actor::Mesh* actor = nullptr;
				scene->WithActors( [ & ]( const auto& actors ) {
					for ( auto* candidate : actors ) {
						if ( std::find( before.begin(), before.end(), candidate ) == before.end() ) {
							if ( actor || candidate->GetType() != scene::actor::Actor::TYPE_MESH ) {
								throw std::runtime_error( "surface factory did not publish one mesh actor" );
							}
							actor = static_cast< scene::actor::Mesh* >( candidate );
						}
					}
				} );
				if ( !actor ) { throw std::runtime_error( "surface factory did not publish its mesh actor" ); }
				borrowed = g_engine->GetTextureLoader()->LoadCustomTexture( "fixture.bmp" );
				borrowed_unloads = graphics.TextureUnloadCount( borrowed );
				const auto image_color = types::Color::RGBA( 32, 64, 128, 255 );
				const auto change = [ & ]( const char* background ) -> std::string {
					auto* previous = actor->GetTexture();
					const bool owned = previous && previous != borrowed;
					const auto unloads = graphics.TextureUnloadCount( previous );
					surface->WrapSet( "background", background ? new value::String( gc_space, background ) : nullptr, GSE_CALL );
					const bool replaced_owned = owned && previous != actor->GetTexture();
					const auto after = graphics.TextureUnloadCount( previous );
					if ( replaced_owned && after == unloads ) {
						delete previous; // clean the original overwrite regression after observing it
					}
					GT_ASSERT( after == unloads + ( replaced_owned ? 1 : 0 ), "background replacement did not release exactly its previous owned texture" );
					GT_ASSERT( graphics.TextureUnloadCount( borrowed ) == borrowed_unloads && borrowed->GetPixel( 0, 0 ) == image_color,
						"background replacement damaged the borrowed loader texture" );
					GT_OK();
				};
				result = [ & ]() -> std::string {
					for ( const auto* background : { "#123456", "fixture.bmp:crop(0,0,0,0)", "fixture.bmp:crop(1,1,1,1)",
						"fixture.bmp:crop(0,0,0,0)", "fixture.bmp:crop(1,1,1,1)", "fixture.bmp", "#456789", "#abcdef" } ) {
						const auto error = change( background );
						if ( !error.empty() ) { return error; }
					}
					auto* const valid = actor->GetTexture();
					const auto pixel = valid->GetPixel( 0, 0 );
					const auto unloads = graphics.TextureUnloadCount( valid );
					bool rejected = false;
					try { surface->WrapSet( "background", new value::String( gc_space, "fixture.bmp:unknown()" ), GSE_CALL ); }
					catch ( const Exception& error ) { rejected = error.class_name == EC.UI_ERROR; }
					GT_ASSERT( rejected && valid != borrowed && actor->GetTexture() == valid &&
						valid->GetPixel( 0, 0 ) == pixel && graphics.TextureUnloadCount( valid ) == unloads,
						"invalid filter changed the native background owner or pixels" );
					for ( const auto* background : { "fixture.bmp", "fixture.bmp:crop(0,0,0,0)", static_cast< const char* >( nullptr ) } ) {
						const auto error = change( background );
						if ( !error.empty() ) { return error; }
					}
					GT_ASSERT( !actor->GetTexture(), "unset background retained a texture" );
					const auto error = change( "fixture.bmp:crop(1,1,1,1)" );
					if ( !error.empty() ) { return error; }
					final_texture = actor->GetTexture();
					final_unloads = graphics.TextureUnloadCount( final_texture );
					GT_ASSERT( final_texture != borrowed && final_texture->GetPixel( 0, 0 ) == image_color, "filtered background pixels changed" );
					GT_OK();
				}();
			} );
		}
		if ( !result.empty() ) { return result; }
		GT_ASSERT( final_texture && graphics.TextureUnloadCount( final_texture ) == final_unloads + 1, "surface destruction did not release its last owned texture" );
		GT_ASSERT( graphics.TextureUnloadCount( borrowed ) == borrowed_unloads && borrowed->GetPixel( 0, 0 ) == types::Color::RGBA( 32, 64, 128, 255 ),
			"surface destruction released its borrowed loader texture" );
	}
	catch ( const std::exception& error ) { GT_FAIL( std::string( "surface ownership check failed: " ) + error.what() ); }
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

static std::string CheckTextureCopyUpdates() {
	try {
		types::texture::Texture source( 3, 2, types::texture::TF_CPU_ONLY );
		types::texture::Texture target( 4, 3, types::texture::TF_CPU_ONLY );
		for ( size_t y = 0; y < 2; ++y ) {
			for ( size_t x = 0; x < 3; ++x ) { source.SetPixel( x, y, types::Color::RGB( 40 + x, 80 + y, 120 ) ); }
		}
		for ( const auto size : { std::pair< size_t, size_t >{ 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } ) {
			target.Clear();
			target.ClearUpdatedAreas();
			target.AddFrom( &source, types::texture::AM_DEFAULT, 0, 0, size.first - 1, size.second - 1, 1, 1 );
			GT_ASSERT( target.GetUpdatedAreas().size() == 1, "copy did not emit exactly one update" );
			const auto& dirty = target.GetUpdatedAreas().back();
			GT_ASSERT( dirty.left == 1 && dirty.top == 1 && dirty.right == 1 + size.first && dirty.bottom == 1 + size.second,
				"copy dirty rectangle omitted its last row or column" );
			GT_ASSERT( target.GetPixel( size.first, size.second ) == source.GetPixel( size.first - 1, size.second - 1 ) &&
				target.GetPixel( 0, 0 ) == 0, "copy pixels escaped their destination" );
		}
	}
	catch ( const std::exception& error ) { GT_FAIL( std::string( "texture copy update failed: " ) + error.what() ); }
	GT_OK();
}

static std::string CheckDisplayPalette() {
	try {
		{
			auto* image = SDL_CreateRGBSurfaceWithFormat( 0, 3, 2, 8, SDL_PIXELFORMAT_INDEX8 );
			GT_ASSERT( image, "could not allocate palette boundary fixture" );
			util::FinallyGuard release( [ & ] { SDL_FreeSurface( image ); } );
			SDL_Color colors[ 256 ] = {};
			colors[ 235 ] = { 255, 245, 240, 255 };
			SDL_Palette common = { 256, colors, 0, 1 };
			auto* pixels = static_cast< uint8_t* >( image->pixels );
			std::fill_n( pixels, image->pitch * image->h, uint8_t( 255 ) );
			for ( int y = 0; y < image->h; ++y ) { std::fill_n( pixels + y * image->pitch, image->w, uint8_t( 235 ) ); }
			loader::texture::ApplySmacPalette( image, &common );
			GT_ASSERT( image->format->palette->colors[ 235 ].r == 235, "last admitted PCX index was not gamma corrected" );
			for ( const uint8_t index : { 236, 255 } ) {
				pixels[ image->pitch + 2 ] = index;
				bool rejected = false;
				try { loader::texture::ApplySmacPalette( image, &common ); }
				catch ( const std::invalid_argument& ) { rejected = true; }
				GT_ASSERT( rejected, "unsupported system-reserved PCX index was accepted" );
			}
		}
		const auto directory = std::filesystem::temp_directory_path() / ( "glsmac-palette-" + util::UUID::Generate() );
		const auto data = directory / "default";
		std::filesystem::create_directories( data );
		util::FinallyGuard remove_fixture( [ & ] {
			std::filesystem::remove( data / "fixture.bmp" );
			std::filesystem::remove( data / "interface.pcx" );
			std::filesystem::remove( data / "after-rejection.bmp" );
			std::filesystem::remove( data / "palette.pcx" );
			std::filesystem::remove( data );
			std::filesystem::remove( directory );
		} );
		for ( const bool common : { false, true } ) {
			auto* image = SDL_CreateRGBSurfaceWithFormat( 0, 3, 2, 8, SDL_PIXELFORMAT_INDEX8 );
			GT_ASSERT( image, "could not allocate indexed fixture" );
			util::FinallyGuard release( [ & ] { SDL_FreeSurface( image ); } );
			SDL_Color colors[ 256 ] = {};
			for ( auto& color : colors ) { color.a = 255; }
			// Distinct indices deliberately have the same source RGB. RGB repainting
			// would lose the distinction that the display palette must preserve.
			colors[ 1 ] = common ? SDL_Color{ 250, 40, 80, 255 } : SDL_Color{ 10, 20, 30, 255 };
			colors[ 2 ] = common ? SDL_Color{ 5, 99, 248, 255 } : colors[ 1 ];
			GT_ASSERT( SDL_SetPaletteColors( image->format->palette, colors, 0, 256 ) == 0, "could not set indexed fixture palette" );
			auto* pixels = static_cast< uint8_t* >( image->pixels );
			for ( size_t row = 0; row < 2; ++row ) {
				pixels[ row * image->pitch ] = 1; pixels[ row * image->pitch + 1 ] = 2; pixels[ row * image->pitch + 2 ] = 1;
			}
			// SDL_image detects the synthetic BMP by its contents, including when
			// the common-palette fixture uses the installed asset's filename.
			const auto path = data / ( common ? "palette.pcx" : "fixture.bmp" );
			GT_ASSERT( SDL_SaveBMP( image, path.string().c_str() ) == 0, "could not save indexed fixture" );
			if ( !common ) {
				colors[ 1 ] = { 255, 0, 255, 255 };
				GT_ASSERT( SDL_SetPaletteColors( image->format->palette, colors, 0, 256 ) == 0, "could not set rejection fixture palette" );
				GT_ASSERT( SDL_SaveBMP( image, ( data / "interface.pcx" ).string().c_str() ) == 0 &&
					SDL_SaveBMP( image, ( data / "after-rejection.bmp" ).string().c_str() ) == 0, "could not save rejection fixtures" );
			}
		}
		WidgetGraphicsFixture graphics( directory.string() );
		auto* loader = g_engine->GetTextureLoader();
		bool rejected = false;
		try { loader->LoadCustomTexture( "interface.pcx", types::texture::TF_SMAC_PALETTE ); }
		catch ( const std::exception& ) { rejected = true; }
		GT_ASSERT( rejected, "registered transparency source admitted the opaque palette profile" );
		GT_ASSERT( loader->LoadCustomTexture( "after-rejection.bmp" )->GetPixel( 0, 0 ) == types::Color::RGB( 255, 0, 255 ),
			"rejected registered load leaked transparency into the next custom texture" );
		auto* raw = loader->LoadCustomTexture( "fixture.bmp" );
		auto* mapped = loader->LoadCustomTexture( "fixture.bmp", types::texture::TF_SMAC_PALETTE );
		GT_ASSERT( raw != mapped && loader->LoadCustomTexture( "fixture.bmp" ) == raw,
			"display-palette cache aliased the ordinary source" );
		GT_ASSERT( raw->GetPixel( 0, 0 ) == types::Color::RGB( 10, 20, 30 ) && raw->GetPixel( 1, 0 ) == raw->GetPixel( 0, 0 ),
			"ordinary source palette changed" );
		GT_ASSERT( mapped->GetPixel( 0, 0 ) == types::Color::RGB( 235, 40, 80 ) &&
			mapped->GetPixel( 1, 0 ) == types::Color::RGB( 5, 99, 235 ) && mapped->GetPixel( 2, 0 ) == mapped->GetPixel( 0, 0 ),
			"common palette did not preserve indices and clamp bright channels" );
		class FragmentLoader : public loader::texture::SDL2 {
		public:
			using SDL2::LoadTextureImpl;
		} fragments;
		const auto source = ( data / "fixture.bmp" ).string();
		auto* raw_fragment = fragments.LoadTextureImpl( source, 0, 0, 1, 0, types::texture::LT_NONE, 1.0f, types::texture::TF_NONE );
		auto* mapped_fragment = fragments.LoadTextureImpl( source, 0, 0, 1, 0, types::texture::LT_NONE, 1.0f, types::texture::TF_SMAC_PALETTE );
		GT_ASSERT( raw_fragment != mapped_fragment && raw_fragment->GetPixel( 0, 0 ) == raw->GetPixel( 0, 0 ) &&
			mapped_fragment->GetPixel( 1, 0 ) == mapped->GetPixel( 1, 0 ), "cropped cache aliased palette variants" );
		const auto& dirty = mapped_fragment->GetUpdatedAreas().back();
		GT_ASSERT( dirty.left == 0 && dirty.top == 0 && dirty.right == 2 && dirty.bottom == 1,
			"one-row copy did not notify the full exclusive dirty rectangle" );
	}
	catch ( const std::exception& error ) { GT_FAIL( std::string( "display palette check failed: " ) + error.what() ); }
	GT_OK();
}

static std::string CheckViewportEdges() {
	try {
		WidgetGraphicsFixture graphics;
		class Probe : public ::ui::geometry::Geometry {
		public:
			explicit Probe( Geometry* parent = nullptr ) : Geometry( nullptr, parent, GT_RECTANGLE ) {}
			void UpdateImpl() override {}
		} root;
		root.SetWidth( 64 ); root.SetHeight( 64 ); root.Show();
		const auto& full = root.GetEffectiveArea();
		GT_ASSERT( full.right == 64 && full.bottom == 64 && full.width == 64 && full.height == 64,
			"full viewport cache lost the last row or column" );
		Probe corner( &root );
		corner.SetLeft( 63 ); corner.SetTop( 63 ); corner.SetWidth( 1 ); corner.SetHeight( 1 ); corner.Show();
		const auto& pixel = corner.GetEffectiveArea();
		GT_ASSERT( pixel.left == 63 && pixel.top == 63 && pixel.right == 64 && pixel.bottom == 64 && pixel.width == 1 && pixel.height == 1,
			"bottom-right pixel was clipped to an empty cache" );
	}
	catch ( const std::exception& error ) { GT_FAIL( error.what() ); }
	GT_OK();
}

void AddMeshTests( task::gsetests::GSETests* task ) {
	task->AddTest( "UI viewport cache includes the bottom-right pixel", GT() { return CheckViewportEdges(); } );
	task->AddTest( "texture copy uploads the last row and column including one-pixel extents", GT() { return CheckTextureCopyUpdates(); } );
	task->AddTest( "SMAC display palette preserves indexed colors and independent cache entries", GT() { return CheckDisplayPalette(); } );
	task->AddTest(
		"test surface background replacement preserves texture ownership",
		GT() { return CheckSurfaceBackgroundOwnership(); }
	);
	task->AddTest(
		"test generated sprite ownership survives widget copies and exceptions",
		GT() { return CheckGeneratedSpriteOwnership(); }
	);
	task->AddTest(
		"test minimap capture ownership survives redraws and exceptions",
		GT() { return CheckMinimapCaptureOwnership(); }
	);
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
	task->AddTest(
		"test detached UI destruction releases actors and widget textures",
		GT() { return CheckDetachedUIOwnership( true ); }
	);
	task->AddTest(
		"test attached widget deinit observes resources before cleanup",
		GT() { return CheckDetachedUIOwnership( false ); }
	);
}

}
