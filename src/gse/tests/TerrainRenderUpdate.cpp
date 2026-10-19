#include "TerrainRenderUpdate.h"
#include "game/frontend/tile/Tile.h"
#include "game/backend/MapObject.h"
#include "game/frontend/unit/SelectionBlink.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/MapState.h"
#include "game/backend/map/Consts.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/module/Finalize.h"
#include "game/backend/map/module/Sprites.h"
#include "fixtures/SmacTerrainHeights.h"
#include "types/mesh/Render.h"
#include "types/mesh/Data.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/Random.h"
#include "game/FrontendRequest.h"
#include "types/texture/Texture.h"
#include "util/Perlin.h"
#include "task/gsetests/GSETests.h"
#include "graphics/opengl/TexturePaging.h"
#include "graphics/PresentationViewport.h"
#include <cstring>
#include <algorithm>
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
    void RecordEmptyWater(tile::Tile* t) {
        world.m_capture_coast=true;world.m_current_ts=world.GetTileState(t);
        world.EnsureWaterFungusRecipe();world.m_current_ts=nullptr;world.m_capture_coast=false;
    }
    size_t NextId() const { return world.m_next_sprite_instance_id; }
    bool InstancesEmpty() const { return world.m_sprite_instances.empty(); }
    bool ContextIdle() const { return !world.m_land_canvas && !world.m_current_ts; }
    bool CheckNativeFinalizer(bool coast) {
        using Planet=map::SmacPlanetMap;
        std::string wire(Planet::FILE_BYTES,'\0');
        wire.replace(0,11,"TERRANMAP\0\x1a",11);
        wire[11]=5;wire[15]=80;wire[19]=80;wire[15+0xa30]=4;
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {
            wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES]=char(4<<5);
        }
        // A sea contributor must flatten the whole shared west vertex.
        if(coast) {wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+80*Planet::RECORD_BYTES]=char(2<<5);}
        world.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
        types::mesh::Render render(25,20);
        types::mesh::Data picking(5,4);
        world.m_meshes.terrain=&render;world.m_meshes.terrain_data=&picking;
        world.m_land_random=&gameplay;
        auto* t=world.GetTile(2,2);auto* ts=world.GetTileState(t);
        ts->coord={10,20};ts->has_water=coast;ts->is_coastline_corner=coast;
        t->W->is_water_tile=coast;
        world.m_map_state->first_run=true;
        const auto state=gameplay.GetStateString();
        game::backend::Random expected(nullptr,741);
        if(!coast) {for(int i=0;i<2;++i) {expected.GetFloat(-2,2);}}
        map::module::Finalize finalizer(&world);
        finalizer.GenerateTile(t,ts,world.m_map_state);
        types::Vec3 center,west,pick_center,pick_west;
        render.GetVertexCoord(ts->layers[tile::LAYER_LAND].indices.center,&center);
        render.GetVertexCoord(ts->layers[tile::LAYER_LAND].indices.left,&west);
        picking.GetVertexCoord(ts->data_mesh.indices.center,&pick_center);
        picking.GetVertexCoord(ts->data_mesh.indices.left,&pick_west);
        const float center_z=map::s_consts.tile.scale.z+25.0f/48;
        const auto close_coord=[](const types::Vec3& a,const types::Vec3& b) {
            return std::abs(a.x-b.x)<0.000001f && std::abs(a.y-b.y)<0.000001f && std::abs(a.z-b.z)<0.000001f;
        };
        const bool result=close_coord(center,types::Vec3(10,20,center_z)) &&
            close_coord(west,types::Vec3(9.5f,20,coast?2.0f:center_z)) &&
            pick_center==center && pick_west==west &&
            ts->layers[tile::LAYER_LAND].coords.center==center &&
            gameplay.GetStateString()==expected.GetStateString() &&
            (coast?gameplay.GetStateString()==state:gameplay.GetStateString()!=state);
        const auto anchor=ts->GetProjectorCoords(tile::LAYER_LAND);
        const auto expected_height=coast?18:25; // floor((0+25+25+25)/4), not center25
        game::frontend::tile::Tile shown({2,2});shown.Update(*t,*ts);
        const bool projected=ts->smac_projector_height==std::optional<uint8_t>(expected_height) &&
            close_coord(anchor,types::Vec3(10,20,2.0f+expected_height/48.0f)) &&
            shown.GetRenderData().name_anchor==std::optional<types::Vec3>(anchor) &&
            shown.GetRenderData().coords==anchor && shown.GetRenderData().selection_coords.left==west &&
            shown.GetRenderData().selection_coords.center==center &&
            game::backend::MapObject(&world,t).GetRenderCoords()==anchor.InvertY();
        ts->smac_projector_height.reset();shown.Update(*t,*ts);
        const bool legacy=!shown.GetRenderData().name_anchor && ts->GetProjectorCoords(tile::LAYER_LAND)==center &&
            shown.GetRenderData().coords==center && game::backend::MapObject(&world,t).GetRenderCoords()==center.InvertY();
        world.m_meshes.terrain=nullptr;world.m_meshes.terrain_data=nullptr;
        return result && projected && legacy;
    }
    bool CheckSpriteAnchors(bool native) {
        auto* t=world.GetTile(2,2);auto* ts=world.GetTileState(t);
        auto& c=ts->layers[tile::LAYER_LAND].coords;
        c.center={10,20,9};c.left={9,20,2};c.top={10,19,2};
        c.right={11,20,2.125f};c.bottom={10,21,2.125f};
        if(native) ts->smac_projector_height=3;
        const types::Vec3 expected{10,-20,2.0625f}; // native floor3; legacy mean also3 here
        // Distinguish legacy float averaging from the native integer floor.
        c.bottom.z+=1.0f/48;
        const auto wanted=native?expected:types::Vec3(10,-20,(c.left.z+c.top.z+c.right.z+c.bottom.z)/4);
        t->bonus=tile::BONUS_NONE;
        t->terraforming=tile::TERRAFORMING_FARM|tile::TERRAFORMING_MINE|tile::TERRAFORMING_SOLAR|
            tile::TERRAFORMING_SENSOR|tile::TERRAFORMING_BUNKER;
        map::module::Sprites sprites(&world);sprites.GenerateTile(t,ts,world.m_map_state);
        if(ts->sprites.size()!=5) return false;
        for(const auto& s:ts->sprites) if(world.m_sprite_instances.at(s.instance).second!=wanted) return false;
        t->terraforming=tile::TERRAFORMING_FARM|tile::TERRAFORMING_MINE|tile::TERRAFORMING_SOLAR;
        auto ordinary=world.PrepareOrdinaryTerrainUpdate(t,tile::TERRAFORMING_FARM|tile::TERRAFORMING_MINE|tile::TERRAFORMING_SOLAR);
        const auto& a=world.GetTerrainUpdatePayload(*ordinary);
        if(a.added.size()!=3) return false;
        for(const auto& s:a.added) if(s.coords!=wanted) return false;
        auto appearance=world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_NONE,tile::TERRAFORMING_SOIL_ENRICHER,tile::ROCKINESS_FLAT}}});
        const auto& b=world.GetTerrainUpdatePayload(*appearance);
        return b.added.size()==1 && b.added.front().coords==wanted;
    }
    auto Replay(const map::TerrainCoastRecipes::Land& land) { return world.m_coast_recipes.Prepare(land); }
	void Commit( tile::Tile* t, uint16_t bits, map::Map::PreparedTerrainUpdate& update ) {
		t->terraforming = bits; world.CommitOrdinaryTerrainUpdate( update );
	}
};

void AddTerrainRenderUpdateTests( task::gsetests::GSETests* task ) {

    task->AddTest("SMAC terrain sprite anchors agree through initial ordinary and appearance callers",GT(){
        {TerrainRenderFixture f;GT_ASSERT(f.CheckSpriteAnchors(true),"Native sprite lost floored projector height");}
        {TerrainRenderFixture f;GT_ASSERT(f.CheckSpriteAnchors(false),"Legacy sprite lost floating outer mean");}
        GT_OK();
    });
    task->AddTest("SMAC terrain projector metadata survives tile transport and preserves legacy absence",GT(){
        tile::TileState source{};
        NEW(source.moisture_original,types::texture::Texture,map::s_consts.tc.texture_pcx.dimensions.x,map::s_consts.tc.texture_pcx.dimensions.y);
        source.layers[tile::LAYER_LAND].coords.center={10,20,2.0f+25.0f/48};
        source.smac_projector_height=18;
        const auto expected=source.GetProjectorCoords(tile::LAYER_LAND);
        tile::TileState restored{};restored.Deserialize(source.Serialize());
        GT_ASSERT(restored.smac_projector_height==source.smac_projector_height && restored.GetProjectorCoords(tile::LAYER_LAND)==expected,
            "Transport discarded native terrain-aware projector anchor");
        source.smac_projector_height.reset();const auto legacy=source.Serialize();
        tile::TileState old{};old.Deserialize(legacy);
        GT_ASSERT(!old.smac_projector_height && old.GetProjectorCoords(tile::LAYER_LAND)==source.layers[tile::LAYER_LAND].coords.center,
            "Legacy transport invented native projector displacement");
        for(const int height:{-1,128}) {
            auto bad=legacy;bad.WriteString("SMAC_PROJECTOR_HEIGHT_V1");bad.WriteInt(height);
            bool rejected=false;
            try {tile::TileState probe{};probe.Deserialize(bad);}catch(const std::exception&){rejected=true;}
            GT_ASSERT(rejected,"Out-of-range projector metadata was accepted");
        }
        auto unknown=legacy;unknown.WriteString("UNKNOWN_PROJECTOR");unknown.WriteInt(18);
        bool rejected=false;
        try {tile::TileState probe{};probe.Deserialize(unknown);}catch(const std::exception&){rejected=true;}
        GT_ASSERT(rejected,"Unknown projector extension was accepted");
        GT_OK();
    });
    task->AddTest("SMAC ordinary land heights match 18 original terrain contexts",GT(){
        for(const auto& c:terrain_cases::ALL) {
            const auto band_at=[&](int x,int y)->std::optional<uint8_t> {
                if(y<0 || y>=80) {return std::nullopt;}
                if(!c.flat) {x=(x+80)%80;}
                if(x<0 || x>=80) {return std::nullopt;}
                if(c.uniform_sea>=0) {return uint8_t(c.uniform_sea);}
                for(const auto& b:c.bands) {if(b[0]==x && b[1]==y) {return uint8_t(b[2]);}}
                return std::nullopt;
            };
            GT_ASSERT(map::smac_terrain::OrdinaryHeights(c.x,c.y,band_at,c.low)==c.expected,"Case: "+std::string(c.name));
        }
        GT_OK();
    });
    task->AddTest("SMAC ordinary land finalizer shares display picking and retained tile heights without jitter",GT(){
        {TerrainRenderFixture f;GT_ASSERT(f.CheckNativeFinalizer(false),"Interior fan or visual RNG schedule differs");}
        {TerrainRenderFixture f;GT_ASSERT(f.CheckNativeFinalizer(true),"Coastal picking replaced native shared heights");}
        GT_OK();
    });

    task->AddTest("Bloom terrain appearance stages land fungus rockiness sprites and neighbors atomically", GT() {
        TerrainRenderFixture f; auto* t=f.world.GetTile(2,2); auto* peer=f.world.GetTile(4,4);
        auto farm=f.world.PrepareOrdinaryTerrainUpdate(t,tile::TERRAFORMING_FARM);
        f.Commit(t,tile::TERRAFORMING_FARM,*farm);
        f.world.GetTileState(t)->sprites.push_back({"bonus",77,"NutrientBonusLand",{}});
        for (const auto coords : map::s_consts.tc.texture_pcx.fungus_land)
            f.source.Fill(coords.x,coords.y,coords.x+dim.x-1,coords.y+dim.y-1,Color(0.8f,0.1f,0.2f,1));
        const auto rng=f.gameplay.GetStateString();
        const map::Map::TerrainAppearances changes{{t,{tile::FEATURE_XENOFUNGUS,0,tile::ROCKINESS_ROLLING}},
            {peer,{tile::FEATURE_NONE,0,tile::ROCKINESS_ROCKY}}};
        auto first=f.world.PrepareTerrainAppearanceUpdate(changes);
        auto second=f.world.PrepareTerrainAppearanceUpdate(changes);
        const auto& a=f.world.GetTerrainUpdatePayload(*first); const auto& b=f.world.GetTerrainUpdatePayload(*second);
        GT_ASSERT(a.patches.size()>=9 && a.patches.size()==b.patches.size() && a.removed.size()==1 && a.added.empty(),
            "Batch omitted fungus edges, indirect rockiness or removed Farm");
        for (size_t i=0;i<a.patches.size();++i) GT_ASSERT(Equal(*a.patches[i].pixels,*b.patches[i].pixels),"Repeated preparation changed presentation RNG");
        GT_ASSERT(t->features==tile::FEATURE_NONE && t->terraforming==tile::TERRAFORMING_FARM
            && peer->rockiness==tile::ROCKINESS_FLAT && f.world.GetTileState(t)->sprites.size()==2,
            "Preparation published live terrain or sprites");
        second.reset();
        t->features=tile::FEATURE_XENOFUNGUS;t->terraforming=0;t->rockiness=tile::ROCKINESS_ROLLING;
        peer->rockiness=tile::ROCKINESS_ROCKY;f.world.CommitOrdinaryTerrainUpdate(*first);
        GT_ASSERT(f.world.GetTileState(t)->sprites.size()==1 && f.world.GetTileState(t)->sprites.front().instance==77
            && f.gameplay.GetStateString()==rng,"Commit replaced bonus identity or gameplay RNG");
        GT_OK();
    });
    task->AddTest("Bloom water fungus replaces final overlay and survives later coast replay", GT() {
        TerrainRenderFixture f; auto* t=f.world.GetTile(2,2);auto* ts=f.world.GetTileState(t);
        t->is_water_tile=true;ts->has_water=true;ts->layers[tile::LAYER_WATER].coords={};
        for (const auto coords : map::s_consts.tc.texture_pcx.fungus_water)
            f.source.Fill(coords.x,coords.y,coords.x+dim.x-1,coords.y+dim.y-1,Color(0.9f,0.1f,0.2f,1));
        const size_t water_y=tile::LAYER_WATER*8*dim.y+size_t(ts->tex_coord.y1);
        f.RecordCoast(t,size_t(ts->tex_coord.x1),water_y,types::texture::AM_MERGE | types::texture::AM_ROUND_LEFT,1);
        const auto rng=f.gameplay.GetStateString();
        auto bloom=f.world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_XENOFUNGUS,0,tile::ROCKINESS_FLAT}}});
        const auto& payload=f.world.GetTerrainUpdatePayload(*bloom);
        const auto water=std::find_if(payload.patches.begin(),payload.patches.end(),[&](const auto& patch){return patch.y==water_y;});
        GT_ASSERT(water!=payload.patches.end() && t->features==tile::FEATURE_NONE,"Water bloom missing or mutated live state");
        auto pixels=water->pixels;
        t->features=tile::FEATURE_XENOFUNGUS;f.world.CommitOrdinaryTerrainUpdate(*bloom);
        map::TerrainCoastRecipes::Land land;
        land.emplace(map::TerrainCoastRecipes::Key{size_t(ts->tex_coord.x1),size_t(ts->tex_coord.y1)},
            std::shared_ptr<const Texture>(ts->moisture_original,[](const Texture*){}));
        const auto replay=f.Replay(land);
        GT_ASSERT(replay.size()==1 && Equal(*replay.front().pixels,*pixels),"Later coast refresh erased committed water fungus");
        auto* ocean=f.world.GetTile(6,6);ocean->is_water_tile=true;
        auto* ocean_state=f.world.GetTileState(ocean);ocean_state->has_water=true;ocean_state->layers[tile::LAYER_WATER].coords={};
        f.RecordEmptyWater(ocean);
        auto ocean_bloom=f.world.PrepareTerrainAppearanceUpdate({{ocean,{tile::FEATURE_XENOFUNGUS,0,tile::ROCKINESS_FLAT}}});
        GT_ASSERT(ocean_bloom!=nullptr,"Open ocean without coast recipe could not bloom");
        auto clear=f.world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_NONE,0,tile::ROCKINESS_FLAT}}});
        const auto& cleared=f.world.GetTerrainUpdatePayload(*clear);
        const auto bare=std::find_if(cleared.patches.begin(),cleared.patches.end(),[&](const auto& patch){return patch.y==water_y;});
        GT_ASSERT(bare!=cleared.patches.end() && !Equal(*bare->pixels,*pixels) && f.gameplay.GetStateString()==rng,
            "Water overlay could not be replaced or changed gameplay RNG");
        GT_OK();
    });
    task->AddTest("Bloom terrain appearance failure and disposal retain caches and live identities", GT() {
        TerrainRenderFixture f;auto* t=f.world.GetTile(2,2);auto* peer=f.world.GetTile(4,4);
        const auto next=f.NextId();
        auto staged=f.world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_NONE,tile::TERRAFORMING_SOLAR,tile::ROCKINESS_FLAT}}});
        staged.reset();
        GT_ASSERT(f.NextId()==next && f.InstancesEmpty(),"Discarded batch published sprite identities");
        auto* canvas=f.world.GetTileState(peer)->moisture_original;f.world.GetTileState(peer)->moisture_original=nullptr;
        bool rejected=false;
        try {(void)f.world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_NONE,0,tile::ROCKINESS_FLAT}},
            {peer,{tile::FEATURE_NONE,0,tile::ROCKINESS_FLAT}}});}catch(const std::exception&){rejected=true;}
        f.world.GetTileState(peer)->moisture_original=canvas;
        GT_ASSERT(rejected && !f.world.IsPreparingTerrain() && f.ContextIdle()
            && f.NextId()==next && t->terraforming==0,"Late failure retained candidate context or partly published");
        t->features=tile::FEATURE_RIVER;
        auto* ts=f.world.GetTileState(t);ts->has_water=true;
        NEW(ts->river_original,Texture,dim.x,dim.y);
        ts->river_original->Fill(0,0,dim.x-1,dim.y-1,Color(0.1f,0.7f,0.2f,1));
        Texture river(dim.x,dim.y);river.AddFrom(ts->river_original,types::texture::AM_DEFAULT,0,0,dim.x-1,dim.y-1);
        f.RecordEmptyWater(t);
        auto river_bloom=f.world.PrepareTerrainAppearanceUpdate({{t,{uint16_t(tile::FEATURE_RIVER|tile::FEATURE_XENOFUNGUS),0,tile::ROCKINESS_ROLLING}}});
        GT_ASSERT(Equal(river,*ts->river_original) && t->features==tile::FEATURE_RIVER,"Detached preparation changed cached river-exit pixels");
        GT_ASSERT(f.world.PrepareTerrainAppearanceUpdate({{t,{tile::FEATURE_RIVER,0,tile::ROCKINESS_FLAT}}})!=nullptr,
            "Failure prevented a subsequent terrain preparation");
        GT_OK();
    });
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
        Blink cadence;
        cadence.Poll(epoch,true);
        for(int i=1;i<=8;++i) {
            GT_ASSERT(cadence.Poll(epoch+milliseconds(i*150+16),true),"late poll missed original cadence boundary");
            GT_ASSERT(!cadence.Poll(epoch+milliseconds(i*150+17),true),"late poll replayed a delivery");
        }
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
