#include "TerrainRenderUpdate.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "fixtures/NativeTerrainView.h"
#include "Mesh.h"
#include "scene/Camera.h"
#include "game/frontend/MapProjection.h"
#include "game/frontend/tile/Tile.h"
#include "game/backend/MapObject.h"
#include "game/backend/Game.h"
#include "game/frontend/unit/SelectionBlink.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/MapState.h"
#include "game/backend/map/Consts.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/module/Finalize.h"
#include "game/backend/map/module/Sprites.h"
#include "fixtures/SmacTerrainHeights.h"
#include "fixtures/SmacTerrainRaster.h"
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
    bool CheckNativeTerrainSnapshot() {
        std::unique_ptr<::game::native_terrain::World> retained,claims;
        {
            map::Map local(nullptr,80,80);
            if(local.CopyNativeTerrain()) return false;
            using Planet=map::SmacPlanetMap;
            std::string wire(Planet::FILE_BYTES,'\0');wire.replace(0,11,"TERRANMAP\0\x1a",11);
            wire[11]=5;wire[15]=80;wire[19]=80;wire[15+0xa30]=4;
            for(size_t i=0;i<Planet::TILE_COUNT;++i) {
                wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES]=char(3<<5);
                wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES+1]=50;
            }
            local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
            local.m_territory_topology=map::GlsmacTerritoryTopology::Build(80,80,false,std::vector<bool>(3200,false));
            auto* live=local.GetTile(40,40);live->features=tile::FEATURE_XENOFUNGUS;
            live->terraforming=tile::TERRAFORMING_FOREST;live->moisture=tile::MOISTURE_RAINY;live->rockiness=tile::ROCKINESS_ROLLING;
            retained=local.CopyNativeTerrain();
            if(!retained || retained->horizontal_wrap) return false;
            const auto& before=retained->tiles[40*40+20];
            if(before.band!=3 || before.contour!=50 || before.moisture!=2 || before.rockiness!=1 || before.items!=0x200020 || !before.ordinary) return false;
            // The original landmark word combines profile memberships,
            // dirty bookkeeping and an upper detail byte. Metadata alone
            // must not create sea holes or exclude a neighboring land stencil.
            const size_t record=Planet::PREFIX_BYTES+Planet::HEADER_BYTES+(40*40+20)*Planet::RECORD_BYTES;
            for(const uint32_t word:{0x00400000u,0x75400000u,0x01400400u,0x80000001u}) {
                for(unsigned byte=0;byte<4;++byte) wire[record+12+byte]=static_cast<char>(word>>(8*byte));
                local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
                const auto metadata=local.CopyNativeTerrain();const auto& cell=metadata->tiles[40*40+20];
                if(cell.deformed!=bool(word&0xb03u) || cell.ordinary==bool(word&0xb03u)) return false;
            }
            // Source replacement must discard the shared cache; snapshots own
            // the resulting signed geometry after the backend is destroyed.
            for(const uint32_t word:{0x200u,0x800u}) {
                if(word==0x800u) {live->features|=tile::FEATURE_FOSSIL_FIELD_RIDGE;}
                for(unsigned byte=0;byte<4;++byte) wire[record+12+byte]=static_cast<char>(word>>(8*byte));
                local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
                auto* geometry=local.GetNativeGeometry();
                if(!geometry || geometry->Height(40,40,0)!=(word==0x200u?-12:20)) return false;
                const auto snapshot=local.CopyNativeTerrain();const auto& cell=snapshot->tiles[40*40+20];
                if(!cell.heights || !cell.lighting || cell.deformed || !cell.ordinary ||
                    (*cell.heights)[0]!=(word==0x200u?-12:20)) return false;
                if(snapshot->tiles[40*40+19].heights->at(3)!=cell.heights->at(1)) return false;
            }
            live->features&=~tile::FEATURE_FOSSIL_FIELD_RIDGE;
            for(unsigned byte=0;byte<4;++byte) wire[record+12+byte]=0;
            local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
            wire[record]=char(2<<5);
            for(const uint32_t word:{0x00400010u,0x75400080u,0xff400090u,0x00400040u,0x80000001u,0x80000100u}) {
                for(unsigned byte=0;byte<4;++byte) wire[record+12+byte]=static_cast<char>(word>>(8*byte));
                local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
                const auto metadata=local.CopyNativeTerrain();const auto& cell=metadata->tiles[40*40+20];
                // Sea heights remain zero and negative detail skips quads.
                if(cell.deformed || !cell.ordinary) return false;
            }
            wire[record]=char(3<<5);
            for(unsigned byte=0;byte<4;++byte) wire[record+12+byte]=0;
            local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
            live->terraforming=tile::TERRAFORMING_FARM;
            const auto farm=local.CopyNativeTerrain();
            if(!farm->tiles[40*40+20].ordinary || farm->tiles[40*40+20].items!=0x8020) return false;
            live->features|=tile::FEATURE_RIVER;wire[record+9]=2;
            local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
            const auto river=local.CopyNativeTerrain();
            if(!river->tiles[40*40+20].ordinary || river->tiles[40*40+20].items!=0x82a0) return false;
            live->features&=~tile::FEATURE_RIVER;wire[record+9]=0;
            local.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
            live->terraforming=tile::TERRAFORMING_ROAD|tile::TERRAFORMING_MAG_TUBE;
            if(!local.CopyNativeTerrain()->tiles[40*40+20].ordinary) return false;
            live->terraforming|=tile::TERRAFORMING_SOLAR;
            const auto updated=local.CopyNativeTerrain();
            if(!updated->tiles[40*40+20].ordinary || updated->tiles[40*40+20].items!=0x6c || !before.ordinary || before.owner!=-1) return false;
            map::SmacWorkerKnowledge::Data data{};data.width=80;data.height=80;data.ordinary_local_mode=true;data.tiles.resize(3200);data.observers[3]=map::SmacWorkerKnowledge::Observer{};
            data.tiles[40*40+20].claimant=3;
            local.m_worker_knowledge=map::SmacWorkerKnowledge::Create(data);local.m_worker_knowledge_ready=true;
            claims=local.CopyNativeTerrain();
            if(claims->tiles[40*40+20].owner!=3 || claims->tiles[40*40+20].region!=local.m_territory_topology->Regions()[40*40+20]) return false;
            data.tiles[40*40+20].claimant=0;local.m_worker_knowledge=map::SmacWorkerKnowledge::Create(data);
            if(local.CopyNativeTerrain()->tiles[40*40+20].owner!=0 || claims->tiles[40*40+20].owner!=3) return false;
            local.m_worker_knowledge_ready=false;
            if(local.CopyNativeTerrain()->tiles[40*40+20].owner!=-1) return false;
        }
        return retained->tiles[40*40+20].items==0x200020 && retained->tiles[40*40+20].ordinary
            && claims && claims->tiles[40*40+20].owner==3;
    }
    bool CheckProjectionProvenance() {
        game::backend::response_map_data_t response{};
        response.SetMapProjectionProvenance(world);
        if(response.native_projection) return false;
        using Planet=map::SmacPlanetMap;
        std::string wire(Planet::FILE_BYTES,'\0');wire.replace(0,11,"TERRANMAP\0\x1a",11);
        wire[11]=5;wire[15]=80;wire[19]=80;wire[15+0xa30]=4;
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {
            wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES]=char(2<<5);
        }
        world.m_resource_policy=map::GlsmacResourceMapPolicy::InstalledPlanetV1(Planet::Decode(wire),true);
        for(const auto& state:*world.GetMapState()->GetTileStatesPtr()) {
            if(state.smac_projector_height) return false;
        }
        response.SetMapProjectionProvenance(world);
        if(!response.native_projection) return false;
        world.m_resource_policy=map::GlsmacResourceMapPolicy::Deserialize(world.m_resource_policy->Serialize().ToString());
        response.SetMapProjectionProvenance(world);
        if(!response.native_projection) return false;
        world.m_resource_policy=map::GlsmacResourceMapPolicy::DisplayedTerrainV1();
        response.SetMapProjectionProvenance(world);
        return !response.native_projection;
    }
    auto Replay(const map::TerrainCoastRecipes::Land& land) { return world.m_coast_recipes.Prepare(land); }
	void Commit( tile::Tile* t, uint16_t bits, map::Map::PreparedTerrainUpdate& update ) {
		t->terraforming = bits; world.CommitOrdinaryTerrainUpdate( update );
	}
};

void AddTerrainRenderUpdateTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC native terrain snapshots current items and outlives its backend map",GT(){
        TerrainRenderFixture fixture;
        GT_ASSERT(fixture.CheckNativeTerrainSnapshot(),"Native terrain snapshot borrowed, went stale or admitted unsupported map");GT_OK();
    });
    task->AddTest("SMAC native terrain view preserves1597 original ordered submissions",GT(){
        namespace native=game::frontend::native_terrain;
        size_t total=0;
        for(const auto& c:native_view_fixture::cases) {
            const auto actual=native::View(c.x,c.y,c.wrap);uint64_t hash=14695981039346656037ULL;
            for(const auto& p:actual) {
                for(const auto value:{p.tile[0],p.tile[1],p.flat_center[0],p.flat_center[1],static_cast<int>(p.record_offset)}) {
                    const auto bits=static_cast<uint32_t>(value);
                    for(int shift=0;shift<32;shift+=8) {hash=(hash^static_cast<uint8_t>(bits>>shift))*1099511628211ULL;}
                }
            }
            GT_ASSERT(actual.size()==c.count && hash==c.hash,"Native view order, padding, wrap or seed offset differs");
            total+=actual.size();
        }
        GT_ASSERT(total==1597,"Missing native edge/interior view cases");GT_OK();
    });
    task->AddTest("SMAC native terrain coverage distinguishes keyed holes from shade-to-key stores",GT(){
        namespace raster=game::frontend::smac_terrain_raster;
        raster::Bitmap initial{8,8,std::vector<uint8_t>(64,9)},source{8,8,std::vector<uint8_t>(64,10)};
        raster::Polygon polygon{{{{{{0,0}},{{0,0}},-1},{{{8,0}},{{7,0}},-1},{{{8,8}},{{7,7}},-1},{{{0,8}},{{0,7}},-1}}},4};
        const raster::Command command{polygon,&source,raster::Shader::KeyedShaded};
        const auto stored=raster::DrawBatch(initial,{command},{{0,0,8,8}},true);
        GT_ASSERT(stored.writes==64 && std::all_of(stored.coverage.begin(),stored.coverage.end(),[](auto n){return n==1;}) &&
            std::all_of(stored.bitmap.pixels.begin(),stored.bitmap.pixels.end(),[](auto n){return n==9;}),"Opaque shade-to9 lost coverage");
        std::fill(source.pixels.begin(),source.pixels.end(),9);
        const auto skipped=raster::DrawBatch(initial,{command},{{0,0,8,8}},true);
        GT_ASSERT(skipped.writes==0 && std::all_of(skipped.coverage.begin(),skipped.coverage.end(),[](auto n){return !n;}),"Transparent key wrote coverage");GT_OK();
    });
    task->AddTest("SMAC native landmark quads compose before Forest and sea Fungus",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;
        for(auto& cell:world.tiles) {cell.band=3;cell.ordinary=true;}
        auto& cell=world.tiles[40*40+20];cell.landmarks=2;
        native::Source source;source.loaded={1024,768,std::vector<uint8_t>(1024*768,10)};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};
        for(unsigned i=0;i<256;++i) {source.palette[i]={uint8_t(i),0,0};}
        for(unsigned y=628;y<684;++y) {for(unsigned x=1;x<57;++x) {source.loaded.pixels[y*1024+x]=50;}}
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const auto ink=[&](){return native::Render(world,source,view,0).rgba[(288*1024+575)*4];};
        GT_ASSERT(ink()==50,"Mount detail0 did not draw its unlit quad after the land base");
        cell.items=0x200000;
        GT_ASSERT(ink()==10,"Landmark quad drew over later Forest");
        for(auto& water:world.tiles) {water.band=2;water.contour=40;}
        cell.items=0;
        GT_ASSERT(ink()==50,"Sea landmark quad did not follow the sector base");
        cell.items=0x20;
        GT_ASSERT(ink()==10,"Landmark quad drew over later sea Fungus");
        cell.items=0;cell.landmarks=0x80000002u;
        GT_ASSERT(ink()!=50,"Negative art detail drew a named quad");GT_OK();
    });
    task->AddTest("SMAC native terrain connected painter owns frame and preserves unsupported holes",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=3;tile.ordinary=true;}
        native::Source source{{1024,768,std::vector<uint8_t>(1024*768,10)},{}};
        source.palette[10]={11,22,33};
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const auto first=native::Render(world,source,view,0);
        GT_ASSERT(first.tiles==1 && first.writes>2000 && first.rgba[(288*1024+575)*4+3]==255,"Connected land pipeline did not paint");
        std::fill(source.loaded.pixels.begin(),source.loaded.pixels.end(),11);
        GT_ASSERT(first.rgba[(288*1024+575)*4]==11,"Frame retained borrowed source data");
        world.tiles[40*40+20].ordinary=false;
        const auto hole=native::Render(world,source,view,0);
        GT_ASSERT(hole.tiles==0 && hole.writes==0 && hole.rgba[(288*1024+575)*4+3]==0,"Unsupported continuation covered legacy terrain");GT_OK();
    });


    task->AddTest("SMAC native terrain connects water and keyed sea Fungus through owned snapshots",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=2;tile.contour=50;tile.ordinary=true;}
        native::Source source{{1024,768,std::vector<uint8_t>(1024*768,9)},{}};
        for(size_t i=0;i<source.palette.size();++i) {source.palette[i]={static_cast<uint8_t>(i),0,0};}
        source.palette[42]={0,255,0};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const auto water=native::Render(world,source,view,0);
        GT_ASSERT(water.tiles==1 && water.writes>2000 && water.rgba[(288*1024+575)*4+3]==255,"Live water prefix did not paint opaque source9");
        auto& center=world.tiles[40*40+20];center.items=0x20;
        const auto holes=native::Render(world,source,view,0);
        GT_ASSERT(holes.rgba==water.rgba,"Sea Fungus key did not preserve composed water");
        for(int y=516;y<572;++y) {std::fill_n(source.loaded.pixels.begin()+y*1024+508,56,42);}
        const auto fungus=native::Render(world,source,view,0);
        GT_ASSERT(fungus.rgba[(288*1024+575)*4+1]==255 && fungus.writes>water.writes,"Live sea Fungus did not follow water or used shaded/wrong bank");
        GT_ASSERT(holes.rgba==water.rgba,"Later source mutation changed an owned frame");
        center.band=1;
        const auto ocean=native::Render(world,source,view,0);
        center.items=0;
        GT_ASSERT(ocean.rgba==native::Render(world,source,view,0).rgba,"Ocean Fungus bypassed shelf admission");GT_OK();
    });

    task->AddTest("SMAC native terrain connects River after land and preserves display flag and extra pass",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=3;tile.contour=70;tile.ordinary=true;}
        native::Source source{{1024,768,std::vector<uint8_t>(1024*768,9)},{}};
        source.palette[9]={1,2,3};source.palette[32]={77,88,99};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        auto& center=world.tiles[40*40+20];center.items=0x80;
        const auto disabled=native::Render(world,source,view,0,{{0,0}},1);
        const auto keyed=native::Render(world,source,view,0);
        GT_ASSERT(keyed.rgba==disabled.rgba && keyed.writes==disabled.writes,"River key erased underlying land");
        for(int y=259;y<315;++y) {std::fill_n(source.loaded.pixels.begin()+y*1024+280,56,32);}
        const auto river=native::Render(world,source,view,0);
        GT_ASSERT(river.writes>disabled.writes && river.rgba[(288*1024+575)*4]==77,"Connected River did not paint after land");
        GT_ASSERT(native::Render(world,source,view,0,{{0,0}},1).rgba==disabled.rgba,"Disabled River display flag still painted");
        center.items|=0x200;
        const auto extra=native::Render(world,source,view,0);
        GT_ASSERT(extra.rgba==river.rgba && extra.writes-river.writes==river.writes-disabled.writes,"Raw extra River pass was omitted or reordered");GT_OK();
    });

    task->AddTest("SMAC native terrain composes current territory before coast and River",GT(){
        namespace native=game::frontend::native_terrain;
        namespace raster=game::frontend::smac_terrain_raster;
        ::game::native_terrain::World world;world.viewer=3;
        for(auto& tile:world.tiles) {tile.band=3;tile.contour=70;tile.ordinary=true;tile.region=1;tile.owner=3;}
        auto& center=world.tiles[40*40+20];
        world.tiles[39*40+41/2].owner=-1;
        native::Source source{{1024,768,std::vector<uint8_t>(1024*768,10)},{}};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};source.boundary_mask={60,60,std::vector<uint8_t>(3600,1)};
        for(auto& row:source.owner_clut) {for(size_t i=0;i<256;++i) {row[i]=static_cast<uint8_t>(i);}}
        source.owner_clut[3][10]=20;source.palette[10]={1,2,3};source.palette[20]={100,101,102};source.palette[30]={200,201,202};
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const auto disabled=native::Render(world,source,view,0,{{0,0}},0x401);
        const auto boundary=native::Render(world,source,view,0);
        const size_t pixel=(288*1024+575)*4;
        GT_ASSERT(disabled.rgba[pixel]==1 && boundary.rgba[pixel]==100,"Connected territory did not remap prior land indices");
        center.items=0x80;
        for(int y=259;y<315;++y) {std::fill_n(source.loaded.pixels.begin()+y*1024+280,56,30);}
        const auto river=native::Render(world,source,view,0);
        GT_ASSERT(river.rgba[pixel]==200,"Territory compositor painted over a later River");
        world.tiles[39*40+41/2].owner=3;center.items=0;
        GT_ASSERT(native::Render(world,source,view,0).rgba==disabled.rgba,"Territory boundary did not refresh after claim change");GT_OK();
    });

    task->AddTest("SMAC native terrain composes remembered Road and Tube after River and territory",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;world.viewer=3;
        for(auto& tile:world.tiles) {tile.band=3;tile.ordinary=true;tile.region=1;tile.owner=3;}
        auto& center=world.tiles[40*40+20];center.items=0x80;
        native::Source source{{1024,768,std::vector<uint8_t>(1024*768,10)},{}};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};source.boundary_mask={60,60,std::vector<uint8_t>(3600,1)};
        for(auto& row:source.owner_clut) {for(size_t i=0;i<256;++i) {row[i]=static_cast<uint8_t>(i);}}
        source.owner_clut[3][10]=20;world.tiles[39*40+41/2].owner=-1;
        source.palette[10]={1,0,0};source.palette[20]={2,0,0};source.palette[30]={3,0,0};source.palette[40]={4,0,0};source.palette[50]={5,0,0};
        for(int y=259;y<315;++y) {std::fill_n(source.loaded.pixels.begin()+y*1024+280,56,30);}
        for(int index=0;index<9;++index) {for(int y=0;y<56;++y) {
            std::fill_n(source.loaded.pixels.begin()+(395+57*(index/3)+y)*1024+775+57*(index%3),56,40);
            std::fill_n(source.loaded.pixels.begin()+(566+57*(index/3)+y)*1024+775+57*(index%3),56,50);
        }}
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const size_t pixel=(288*1024+575)*4;
        const auto river=native::Render(world,source,view,0);
        GT_ASSERT(river.rgba[pixel]==3,"Live-only items unexpectedly selected a Road");
        center.selected_items=4;const auto road=native::Render(world,source,view,0);
        GT_ASSERT(road.rgba[pixel]==4 && road.writes>river.writes,"Remembered Road did not compose after River/territory");
        GT_ASSERT(native::Render(world,source,view,0,{{0,0}},0x10401).rgba==river.rgba,"Disabling Road flag removed the preceding River");
        center.selected_items=12;const auto tube=native::Render(world,source,view,0);
        GT_ASSERT(tube.rgba[pixel]==5,"Remembered isolated Tube used Road family");
        center.selected_items=8;GT_ASSERT(native::Render(world,source,view,0).rgba==river.rgba,"Tube-only bits incorrectly admitted a Road branch");
        center.selected_items=0;center.admitted_base=true;
        GT_ASSERT(native::Render(world,source,view,0).rgba==river.rgba,"Isolated Base emitted an isolated patch");
        world.tiles[39*40+41/2].admitted_base=true;
        GT_ASSERT(native::Render(world,source,view,0).rgba[pixel]==5,"Admitted Bases did not connect through the Tube family");GT_OK();
    });

    task->AddTest("SMAC indexed terrain generates759 actual native spans from vertices",GT(){
        namespace raster=game::frontend::smac_terrain_raster;
        size_t count=0;
        for(const auto& c:smac_terrain_fixture::Cases()) {
            const auto got=raster::GenerateSpans(c.triangle,c.clip);
            GT_ASSERT(got==c.spans,"Native span mismatch: "+std::string(c.name));
            count+=got.size();
        }
        GT_ASSERT(count==759,"Missing original terrain or edge-control spans");
        GT_OK();
    });
    task->AddTest("SMAC indexed terrain sampling wraps shade and rejects invalid batches",GT(){
        namespace raster=game::frontend::smac_terrain_raster;
        raster::Bitmap initial{32,32,std::vector<uint8_t>(32*32,7)},texture{56,56,std::vector<uint8_t>(56*56,1)};
        const auto unchanged=initial.pixels;
        auto triangle=smac_terrain_fixture::Cases()[16].triangle;
        for(auto& vertex:triangle) {vertex.uv={8,8};vertex.light=-2;}
        const auto dark=raster::Draw(initial,texture,{triangle},{0,0,32,32});
        GT_ASSERT(dark.writes==231 && std::count(dark.bitmap.pixels.begin(),dark.bitmap.pixels.end(),255)==231,"Negative shade did not wrap indexed pixels");
        GT_ASSERT(dark.bitmap.pixels.front()==7 && initial.pixels==unchanged,"Draw changed the caller or pixels outside coverage");
        std::fill(texture.pixels.begin(),texture.pixels.end(),255);
        for(auto& vertex:triangle) {vertex.light=2;}
        const auto bright=raster::Draw(initial,texture,{triangle},{0,0,32,32});
        GT_ASSERT(bright.writes==231 && std::count(bright.bitmap.pixels.begin(),bright.bitmap.pixels.end(),1)==231,"Positive shade was clamped or added as RGB");
        const auto rejects=[](const auto& f) {try {f();}catch(const std::invalid_argument&) {return true;}return false;};
        auto invalid=triangle;invalid[0].screen[0]=32767;
        GT_ASSERT(rejects([&]{raster::Draw(initial,texture,{triangle,invalid},{0,0,32,32});}),"Later invalid triangle returned a partial batch");
        invalid=triangle;invalid[0].uv[0]=8192;
        GT_ASSERT(rejects([&]{raster::GenerateSpans(invalid,{0,0,32,32});}),"Unsupported fixed-point UV accepted");
        invalid=triangle;for(auto& v:invalid) {v.uv={60,60};}
        GT_ASSERT(rejects([&]{raster::Draw(initial,texture,{invalid},{0,0,32,32});}),"Texture address outside source accepted");
        auto malformed=texture;malformed.pixels.pop_back();
        GT_ASSERT(rejects([&]{raster::Draw(initial,malformed,{triangle},{0,0,32,32});}),"Malformed source bitmap accepted");
        const auto empty=raster::Draw(initial,texture,{triangle},{12,0,12,32});
        GT_ASSERT(empty.writes==0 && empty.bitmap.pixels==unchanged && initial.pixels==unchanged,"Empty clip or failure mutated caller pixels");
        GT_OK();
    });

    task->AddTest("SMAC native camera provenance survives all ocean tiles without land metadata",GT(){
        TerrainRenderFixture f;GT_ASSERT(f.CheckProjectionProvenance(),"Projection inferred provenance from current land or retained stale mode");
        GT_OK();
    });
    task->AddTest("SMAC ordinary terrain actual camera projects native96x48 diamonds and pixel heights",GT(){
        return WithWidgetGraphics([]()->std::string {
            scene::Camera camera(scene::Camera::CT_ORTHOGRAPHIC);
            camera.SetCustomAspectRatio(768.0f/1024);
            camera.SetAngle({-float(M_PI)*0.5f,float(M_PI)*0.75f,0});
            camera.SetScale(game::frontend::map_projection::CameraScale(0.125f,true));
            camera.SetPosition({0.75f,0.5f,0.5f});
            const auto project=[&](types::Vec3 c) {
                c.y=-c.y;c.z+=2;
                const auto& m=camera.GetMatrix().m;
                const float x=m[0][0]*c.x+m[0][1]*c.y+m[0][2]*c.z+m[0][3];
                const float y=m[1][0]*c.x+m[1][1]*c.y+m[1][2]*c.z+m[1][3];
                return types::Vec2<float>{(x+1)*512,(1-y)*384};
            };
            const std::array<types::Vec2<float>,5> flat{{{576,288},{528,288},{576,264},{624,288},{576,312}}};
            for(const auto& fixture:terrain_cases::ALL) {
                const auto c=map::smac_terrain::LandCoordinates(fixture.expected,{0,0},{0.5f,0.5f});
                const std::array<types::Vec3,5> vertices{{c.center,c.left,c.top,c.right,c.bottom}};
                for(size_t i=0;i<vertices.size();++i) {
                    const auto p=project(vertices[i]);
                    GT_ASSERT(std::abs(p.x-flat[i].x)<0.001f &&
                        std::abs(p.y-(flat[i].y-fixture.expected[i]))<0.001f,
                        "Original pixel projection differs: "+std::string(fixture.name)+" vertex"+std::to_string(i)+" at"+p.ToString());
                }
            }
            GT_ASSERT(game::frontend::map_projection::CameraScale(0.125f,false)==types::Vec3(0.125f,0.125f,0.125f),"Legacy camera scale changed");
            GT_OK();
        });
    });
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
        for(const int height:{-127,-18,-1,0,127}) {
            source.smac_projector_height=static_cast<int16_t>(height);
            tile::TileState signed_state{};signed_state.Deserialize(source.Serialize());
            GT_ASSERT(signed_state.smac_projector_height==source.smac_projector_height &&
                signed_state.GetProjectorCoords(tile::LAYER_LAND)==source.GetProjectorCoords(tile::LAYER_LAND),
                "Signed landmark projector displacement changed in transport");
        }
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
        for(const int height:{-128,128}) {
            auto bad=legacy;bad.WriteString("SMAC_PROJECTOR_HEIGHT_V2");bad.WriteInt(height);
            bool rejected=false;
            try {tile::TileState probe{};probe.Deserialize(bad);}catch(const std::exception&){rejected=true;}
            GT_ASSERT(rejected,"Out-of-range signed projector metadata was accepted");
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
