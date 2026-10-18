#include "TerrainRenderUpdate.h"
#include "game/frontend/unit/SelectionBlink.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/MapState.h"
#include "game/backend/map/Consts.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/Random.h"
#include "game/FrontendRequest.h"
#include "types/texture/Texture.h"
#include "util/Perlin.h"
#include "task/gsetests/GSETests.h"
#include "graphics/opengl/TexturePaging.h"
#include "graphics/PresentationViewport.h"
#include <cstring>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace map = game::backend::map;
namespace tile = map::tile;
using Texture = types::texture::Texture;
using Update = map::TerrainRenderUpdate;
using Color = types::Color;
const auto dim = map::s_consts.tc.texture_pcx.dimensions;
bool Equal( const Texture& a, const Texture& b ) {
	return a.GetBitmapSize() == b.GetBitmapSize() && !std::memcmp( a.GetBitmap(), b.GetBitmap(), a.GetBitmapSize() );
}
}

class TerrainRenderFixture {
public:
	map::Map world{ nullptr, 8, 8 };
	Texture source{ 1024, 1024 };
	Texture atlas{ 9 * dim.x, 8 * tile::LAYER_MAX * dim.y };
	game::backend::Random gameplay{ nullptr, 741 };
	util::Perlin coast{ 819 };

	TerrainRenderFixture() {
		world.InitTextureVariants();
		NEW( world.m_map_state, map::MapState );
		world.m_map_state->dimensions = { 8, 8 };
		const common::mt_flag_t canceled = false;
		world.m_map_state->LinkTileStates( canceled );
		world.m_textures.source.texture_pcx = &source;
		world.m_terrain_updates_ready = true;
		const auto fill = [&]( auto coords, uint8_t value ) {
			source.Fill( coords.x, coords.y, coords.x + dim.x - 1, coords.y + dim.y - 1, Color( value / 255.0f, 0.2f, 0.3f, 1 ) );
		};
		for ( const auto coords : map::s_consts.tc.texture_pcx.farm ) fill( coords, 80 );
		for ( size_t i = 0; i < 16; ++i ) fill( map::s_consts.tc.texture_pcx.forest[ i ], 100 + i );
		for ( size_t i = 0; i < 9; ++i ) fill( map::s_consts.tc.texture_pcx.road[ i ], 150 + i );
		for ( size_t y = 0; y < 8; ++y ) for ( size_t x = y & 1; x < 8; x += 2 ) {
			auto* t = world.GetTile( x, y ); t->features = tile::FEATURE_NONE; t->terraforming = 0;
			t->moisture = tile::MOISTURE_MOIST; t->rockiness = tile::ROCKINESS_FLAT; t->is_water_tile = false;
			auto* ts = world.GetTileState( t ); ts->tex_coord.x1 = x * dim.x; ts->tex_coord.y1 = y * dim.y;
			ts->has_water = false; ts->is_coastline_corner = false;
			ts->layers[ tile::LAYER_LAND ].coords = {};
			NEW( ts->moisture_original, Texture, dim.x, dim.y );
			ts->moisture_original->Fill( 0, 0, dim.x - 1, dim.y - 1, Color( 0.1f, 0.2f, 0.3f, 1 ) );
		}
		// No backend alias to this frontend-owned atlas is installed.
	}

	void RecordCoast( tile::Tile* t, size_t dx, size_t dy, uint32_t flags, float alpha ) {
		auto* ts = world.GetTileState( t );
		world.m_capture_coast = true; world.m_land_random = &gameplay;
		world.RecordWaterPaint( ts->moisture_original, 0, 0, dx, dy, flags, 0, alpha, &coast,
			map::TerrainCoastRecipes::Key{ size_t( ts->tex_coord.x1 ), size_t( ts->tex_coord.y1 ) } );
		world.m_capture_coast = false; world.m_land_random = nullptr;
	}
	void Commit( tile::Tile* t, uint16_t bits, map::Map::PreparedTerrainUpdate& update ) {
		t->terraforming = bits; world.CommitOrdinaryTerrainUpdate( update );
	}
};

void AddTerrainRenderUpdateTests( task::gsetests::GSETests* task ) {
	task->AddTest("Selected Unit badge shared phase coalesces150ms deliveries and retains gates",GT(){
        using Blink=game::frontend::unit::SelectionBlink;using namespace std::chrono;
        Blink phase;const auto epoch=Blink::Clock::time_point{}+seconds(10);
        GT_ASSERT(phase.Visible() && !phase.Poll(epoch,true),"shared phase did not initialize visible");
        GT_ASSERT(!phase.Poll(epoch+milliseconds(149),true) && phase.Visible(),"timer fired before150ms");
        GT_ASSERT(phase.Poll(epoch+milliseconds(150),true) && !phase.Visible(),"first callback did not toggle once");
        GT_ASSERT(!phase.Poll(epoch+milliseconds(150),true),"same delivery toggled twice");
        GT_ASSERT(phase.Poll(epoch+milliseconds(2000),true) && phase.Visible(),"late callbacks replayed elapsed parity");
        GT_ASSERT(!phase.Poll(epoch+milliseconds(2150),false) && phase.Visible(),"gate changed retained phase");
        GT_ASSERT(phase.Poll(epoch+milliseconds(2300),true) && !phase.Visible(),"next admitted callback did not toggle once");
        phase.EnsureVisible();
        GT_ASSERT(phase.Visible() && phase.Poll(epoch+milliseconds(2450),true) && !phase.Visible(),"ensure-visible rearmed shared timer");
        GT_OK();
    });
	task->AddTest("4:3 presentation fits windows and maps only the rendered area",GT(){
		using graphics::PresentationViewport;
		const auto wide=PresentationViewport::Fit(1920,1080);
		GT_ASSERT(wide.x==240 && wide.y==0 && wide.width==1440 && wide.height==1080,"wide window stretched or cropped");
		const auto square=PresentationViewport::Fit(1000,1000);
		GT_ASSERT(square.x==0 && square.y==125 && square.width==1000 && square.height==750,"square window lost letterboxing");
		const auto original=PresentationViewport::Fit(1024,768);
		GT_ASSERT(original.x==0 && original.y==0 && original.width==1024 && original.height==768,"native 4:3 changed");
		for(int w=0;w<=4096;w+=17) {for(int h=0;h<=2160;h+=13) {
			const auto a=PresentationViewport::Fit(w,h);
			GT_ASSERT(a.width<=w && a.height<=h && a.width*3==a.height*4 && a.width%2==0 && a.height%2==0,
				"aspect, containment or even texture dimensions violated");
			GT_ASSERT(w-a.width-2*a.x<=1 && h-a.height-2*a.y<=1,"bars are not centered");
			if(!a.width) {continue;}
			int x=a.x,y=a.y;
			GT_ASSERT(a.MapPoint(x,y) && x==0 && y==0,"first pixel mapped incorrectly");
			x=a.x+a.width-1;y=a.y+a.height-1;
			GT_ASSERT(a.MapPoint(x,y) && x==a.width-1 && y==a.height-1,"last pixel mapped incorrectly");
			x=a.x-1;y=a.y;
			GT_ASSERT(!a.MapPoint(x,y),"left bar accepted input");
			x=a.x+a.width;y=a.y;
			GT_ASSERT(!a.MapPoint(x,y),"right bar accepted input");
			x=a.x;y=a.y-1;
			GT_ASSERT(!a.MapPoint(x,y),"top bar accepted input");
			x=a.x;y=a.y+a.height;
			GT_ASSERT(!a.MapPoint(x,y),"bottom bar accepted input");
		}}
		const auto retina=PresentationViewport::Fit(3840,2160);
		GT_ASSERT(retina.x==wide.x*2 && retina.width==wide.width*2 && retina.height==wide.height*2,"Retina drawable distorted aspect");GT_OK();
	});

	task->AddTest("GPU texture paging preserves full Planet dimensions and split update pixels",GT(){
		using graphics::opengl::TexturePaging;
		const auto planet=TexturePaging::Make(4536,17920,16384,2048);
		GT_ASSERT(planet.Paged() && planet.page_width==4536 && planet.page_height==8960 && planet.Layers()==2,
			"standard Planet was reduced or exceeds GPU page limits");
		const auto huge=TexturePaging::Make(7224,28672,16384,2048);
		const auto custom=TexturePaging::Make(28728,57344,16384,2048);
		GT_ASSERT(huge.Layers()==2 && custom.Layers()==8 && custom.page_width==14364 && custom.page_height==14336,
			"large map atlas paging lost full dimensions");
		const auto paged=TexturePaging::Make(7,9,4,8);
		std::array<unsigned,63> visits{};size_t count=0;
		for(const auto& patch:paged.Updates(1,2,6,8)) {
			++count;GT_ASSERT(patch.x+patch.right-patch.left<=4 && patch.y+patch.bottom-patch.top<=3 && patch.layer<6,
				"update escaped a GPU page");
			for(size_t y=patch.top;y<patch.bottom;++y) {for(size_t x=patch.left;x<patch.right;++x) {++visits[y*7+x];}}
		}
		GT_ASSERT(count==6,"cross-page update omitted a slice");
		for(size_t y=0;y<9;++y) {for(size_t x=0;x<7;++x) {
			GT_ASSERT(visits[y*7+x]==unsigned(x>=1 && x<6 && y>=2 && y<8),"update repeated or omitted a logical pixel");
		}}
		const auto rejects=[](auto call) {try{call();return false;}catch(const std::exception&){return true;}};
		GT_ASSERT(rejects([]{TexturePaging::Make(9,9,4,8);}) && rejects([&]{paged.Updates(0,0,8,9);})
			&& !TexturePaging::Make(7,9,16,8).Paged() && TexturePaging::Make(0,0,16,8).Updates(0,0,0,0).empty(),
			"texture paging accepted invalid bounds or changed ordinary/empty textures");GT_OK();
	});
	task->AddTest( "Former terrain render patches own pixels and preserve coast replay", GT() {
		TerrainRenderFixture f;
		auto* t = f.world.GetTile( 2, 2 ); auto* neighbor = f.world.GetTile( 4, 2 );
		const auto before_rng = f.gameplay.GetStateString();
		const auto paint_rng = f.gameplay.GetState();
		const auto corner_flags = types::texture::AM_MERGE | types::texture::AM_COASTLINE_BORDER | types::texture::AM_ROUND_LEFT;
		const auto neighbor_flags = types::texture::AM_MERGE | types::texture::AM_COASTLINE_BORDER | types::texture::AM_PERLIN_TOP | types::texture::AM_MIRROR_Y;
		const size_t water_y = 8 * dim.y * tile::LAYER_WATER;
		f.RecordCoast( t, 0, water_y, corner_flags, 1 );
		f.RecordCoast( neighbor, 0, water_y, neighbor_flags, 0.37f );
		f.RecordCoast( t, dim.x, water_y, neighbor_flags, 0.63f );
		auto first = f.world.PrepareOrdinaryTerrainUpdate( t, tile::TERRAFORMING_FARM );
		const auto& update = f.world.GetTerrainUpdatePayload( *first );
		GT_ASSERT( update.patches.size() == 3 && update.added.size() == 1, "missing interior/corner/neighbor patches or Farm sprite" );
		GT_ASSERT( t->terraforming == 0 && f.world.GetTileState( t )->sprites.empty(), "preparation published terrain or sprite authority" );
		Texture expected( dim.x, dim.y );
		// Recording does not advance gameplay: each paint captured this same state.
		// The independent direct composition resets its own RNG for each operation.
		util::random::Random expected_rng( 1 );
		expected_rng.SetState( paint_rng );
		expected.AddFrom( update.patches[ 0 ].pixels.get(), corner_flags, 0, 0, dim.x - 1, dim.y - 1, 0, 0, 0, 1, &expected_rng, &f.coast );
		expected_rng.SetState( paint_rng );
		expected.AddFrom( f.world.GetTileState( neighbor )->moisture_original, neighbor_flags, 0, 0, dim.x - 1, dim.y - 1, 0, 0, 0, 0.37f, &expected_rng, &f.coast );
		GT_ASSERT( Equal( expected, *update.patches[ 1 ].pixels ), "coastal ordered masks/corner copy differ" );
		std::unique_ptr< game::FrontendRequest > queued;
		{
			game::FrontendRequest request( game::FrontendRequest::FR_UPDATE_TERRAIN );
			NEW( request.data.update_terrain.update, Update, update );
			queued = std::make_unique< game::FrontendRequest >( request );
		}
		f.Commit( t, tile::TERRAFORMING_FARM, *first ); first.reset();
		queued->data.update_terrain.update->ApplyTexture( f.atlas );
		GT_ASSERT( f.atlas.UpdatedCount() >= 3, "frontend patches did not report GPU dirty areas" );
		auto second = f.world.PrepareOrdinaryTerrainUpdate( neighbor, tile::TERRAFORMING_FOREST );
		const auto& again = f.world.GetTerrainUpdatePayload( *second );
		GT_ASSERT( again.patches.size() == 2, "second change did not produce land and coast patches" );
		Texture second_expected( dim.x, dim.y );
		expected_rng.SetState( paint_rng );
		second_expected.AddFrom( queued->data.update_terrain.update->patches[ 0 ].pixels.get(), corner_flags, 0, 0, dim.x - 1, dim.y - 1, 0, 0, 0, 1, &expected_rng, &f.coast );
		expected_rng.SetState( paint_rng );
		second_expected.AddFrom( again.patches[ 0 ].pixels.get(), neighbor_flags, 0, 0, dim.x - 1, dim.y - 1, 0, 0, 0, 0.37f, &expected_rng, &f.coast );
		GT_ASSERT( Equal( second_expected, *again.patches[ 1 ].pixels ), "second coast replay lost previously committed source" );
		GT_ASSERT( f.gameplay.GetStateString() == before_rng, "presentation changed gameplay RNG" );
		GT_OK();
	} );
	task->AddTest( "Former terrain render changes all five improvements and connected neighbors", GT() {
		TerrainRenderFixture f; auto* t = f.world.GetTile( 2, 2 ); auto* east = f.world.GetTile( 4, 2 );
		for ( const uint16_t bit : { tile::TERRAFORMING_FARM, tile::TERRAFORMING_SOLAR, tile::TERRAFORMING_MINE, tile::TERRAFORMING_FOREST, tile::TERRAFORMING_ROAD } ) {
			auto update = f.world.PrepareOrdinaryTerrainUpdate( t, bit );
			const auto& payload = f.world.GetTerrainUpdatePayload( *update );
			GT_ASSERT( payload.patches.size() == 1, "isolated improvement changed unrelated atlas tiles" );
			const bool sprite = bit == tile::TERRAFORMING_FARM || bit == tile::TERRAFORMING_SOLAR || bit == tile::TERRAFORMING_MINE;
			GT_ASSERT( payload.added.size() == size_t( sprite ), "ordinary improvement sprite missing or fabricated" );
			if ( bit == tile::TERRAFORMING_FOREST ) GT_ASSERT( payload.removed.size() == 1, "forest retained prior Mine sprite" );
			f.Commit( t, bit, *update );
		}
		auto road = f.world.PrepareOrdinaryTerrainUpdate( east, tile::TERRAFORMING_ROAD );
		const auto& connected = f.world.GetTerrainUpdatePayload( *road );
		GT_ASSERT( connected.patches.size() == 2 && connected.patches[ 0 ].x < connected.patches[ 1 ].x, "road neighbor not rebuilt in coordinate order" );
		GT_ASSERT( connected.patches[ 0 ].pixels->GetPixel( 0, 0 ) != connected.patches[ 1 ].pixels->GetPixel( 0, 0 ), "opposite road connection variants collapsed" );
		f.Commit( east, tile::TERRAFORMING_ROAD, *road );
		auto forest = f.world.PrepareOrdinaryTerrainUpdate( t, tile::TERRAFORMING_FOREST ); f.Commit( t, tile::TERRAFORMING_FOREST, *forest );
		auto next = f.world.PrepareOrdinaryTerrainUpdate( east, tile::TERRAFORMING_FOREST );
		GT_ASSERT( f.world.GetTerrainUpdatePayload( *next ).patches.size() == 2, "forest neighbor not rebuilt" );
		GT_OK();
	} );
	task->AddTest( "Former terrain render rejection and queued payload disposal preserve authority", GT() {
		TerrainRenderFixture f; auto* t = f.world.GetTile( 2, 2 );
		f.world.GetTileState( t )->sprites.push_back( { "bonus", 77, "NutrientBonusLand", {} } );
		auto valid = f.world.PrepareOrdinaryTerrainUpdate( t, tile::TERRAFORMING_FARM );
		valid.reset();
		GT_ASSERT( f.world.GetTileState( t )->sprites.size() == 1 && t->terraforming == 0, "discarded preparation changed live state" );
		bool rejected = false; t->features = tile::FEATURE_RIVER;
		try { (void)f.world.PrepareOrdinaryTerrainUpdate( t, tile::TERRAFORMING_ROAD ); } catch ( const std::runtime_error& ) { rejected = true; }
		GT_ASSERT( rejected && t->terraforming == 0, "unsupported compositor mutated terrain" );
		t->features = tile::FEATURE_NONE;
		auto next = f.world.PrepareOrdinaryTerrainUpdate( t, tile::TERRAFORMING_SOLAR );
		f.Commit( t, tile::TERRAFORMING_SOLAR, *next );
		GT_ASSERT( f.world.GetTileState( t )->sprites.size() == 2 && f.world.GetTileState( t )->sprites[ 0 ].instance == 77, "unrelated bonus instance was replaced" );
		auto malformed = f.world.GetTerrainUpdatePayload( *next ); malformed.patches.push_back( { f.atlas.GetWidth(), 0, malformed.patches.front().pixels } );
		const auto before = f.atlas.UpdatedCount(); rejected = false;
		try { malformed.ApplyTexture( f.atlas ); } catch ( const std::runtime_error& ) { rejected = true; }
		GT_ASSERT( rejected && f.atlas.UpdatedCount() == before, "late bad patch partly changed atlas" );
		GT_OK();
	} );
}
}
