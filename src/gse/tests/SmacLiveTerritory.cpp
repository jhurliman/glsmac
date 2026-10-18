#include "SmacLiveTerritory.h"
#include "ConventionalUnits.h"
#include "fixtures/GlsmacTerritoryTopology.h"
#include "fixtures/SmacCoastalGeography.h"
#include "fixtures/SmacLandConcentration.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacSharedLandmarkDetails.h"
#include "fixtures/SmacTerritorySummary.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotTerritory.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacTerritoryRules.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slots.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "game/FrontendRequest.h"
#include <functional>
#include <algorithm>
#include <memory>

namespace gse::tests {
namespace {
using namespace game::backend;
using Topology = map::GlsmacTerritoryTopology;
using Knowledge = map::SmacWorkerKnowledge;
bool Rejects(const std::function<void()>& call) { try {call();} catch(const std::exception&) {return true;} return false; }
size_t Index(int32_t width,int32_t x,int32_t y) {return static_cast<size_t>(y)*(width/2)+x/2;}
void WriteArea(std::string& wire,size_t offset,int32_t value) {
	for(size_t i=0;i<4;++i) {wire[offset+i]=static_cast<char>((static_cast<uint32_t>(value)>>(8*i))&255);}
}
std::string Rules(int32_t border) {
	std::string text="#RULES\n";
	for(int row=1;row<=70;++row) {text+=std::to_string(row==48?border:0)+", ; explicit fixture\n";}
	return text+"#END\n";
}
void Land(Game* game) {
	for(size_t y=0;y<game->GetMap()->GetHeight();++y) {for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2) {
		for(auto* value:game->GetMap()->GetTile(x,y)->elevation.corners) {*value=100;}
	}}
	for(size_t y=0;y<game->GetMap()->GetHeight();++y) {for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2) {game->GetMap()->GetTile(x,y)->Update();}}
}
Knowledge Supplied(Game* game) {
	Knowledge::Data data{static_cast<int32_t>(game->GetMap()->GetWidth()),static_cast<int32_t>(game->GetMap()->GetHeight()),true,true,0,{},{},{}};
	data.tiles.resize(data.width*data.height/2,{6,std::nullopt});
	data.observers[1]=Knowledge::Observer{};data.observers[2]=Knowledge::Observer{};
	data.remembered[1].resize(data.tiles.size());data.remembered[1][0]=Knowledge::Improvements{1,2,true,false};
	return Knowledge::Create(data);
}
base::Base* Found(GSE_CALLABLE,Game* game,size_t id,size_t slot,int32_t simulation,size_t x,size_t y,bool complete=true) {
	auto* bm=game->GetBM();auto* owner=&game->GetState()->m_slots->GetSlot(slot);
	if(!bm->GetPopDef("territory-specialist")) {bm->DefinePop(new base::PopDef("territory-specialist","Specialist",{},{},base::PopDef::PF_NONE,uint8_t(0)));}
	base::Base::pops_t pops;pops.emplace(4,base::Pop(nullptr,4,bm->GetPopDef("territory-specialist"),0));
	base::SmacBaseAllocationState allocation{1,{{},1}};
	auto candidate=std::make_unique<base::Base>(game,id,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(x,y),"Territory Base",
		base::Base::facilities_t{},pops,5,0,std::nullopt,base::SmacBaseResourceContext{simulation,false,0,0,0,false},
		complete?std::optional<base::SmacBaseAllocationState>(allocation):std::nullopt);
	bm->SpawnBase(GSE_CALL,candidate.get());return candidate.release();
}
}

void AddSmacLiveTerritoryTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC land concentration matches8 native builders and6 retained area reductions",GT(){
        for(const auto& row:land_concentration_fixture::cases) {
            if(row.width) {
                std::vector<bool> sea(row.width*row.height/2,true);
                for(const auto& point:row.land) {sea.at(Index(row.width,point[0],point[1]))=false;}
                const auto topology=Topology::Build(row.width,row.height,row.wrap,sea,row.before);
                GT_ASSERT(topology.Areas() && *topology.Areas()==row.after,"Complete builder areas differ: "+std::string(row.id));
                GT_ASSERT(topology.LandRegionConcentrated()==row.expected,"Complete builder concentration differs: "+std::string(row.id));
            } else {
                GT_ASSERT(Topology::LandRegionConcentrated(row.before)==row.expected,"Area fragment differs: "+std::string(row.id));
            }
        }
        auto invalid=Topology::RegionValues{};invalid[42]=-1;
        GT_ASSERT(Rejects([&]{Topology::LandRegionConcentrated(invalid);}),"Negative normal region area was silently admitted");
        GT_OK();
    });
    task->AddTest("SMAC live land concentration clears at new world and survives Base events and actual Start",GT(){
        for(const bool retained:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);std::string saved;
            const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();Land(game);game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
                bool fresh=false;fixture.ResourceState([&](auto& state){fresh=state.retained_land_region_concentration==std::optional<bool>{false};});
                GT_ASSERT(fresh && game->GetMap()->GetTerritoryTopology()->LandRegionConcentrated(),"Whole-world reset followed terrain classification instead of clearing status");
                fixture.ResourceState([&](auto& state){state.retained_land_region_concentration=retained;});
                Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,902,0,1,24,16);
                game->GetBM()->DespawnBase(GSE_CALL,901);fixture.RefreshPopulation(1);
                bool preserved=false;fixture.ResourceState([&](auto& state){preserved=state.retained_land_region_concentration==std::optional<bool>{retained};});
                GT_ASSERT(preserved,"Base or owner-summary update rebuilt terrain status");
                saved=fixture.Snapshot();GT_OK();
            });if(!first.empty()) {return first;}
            ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
            const auto second=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.ResourceState([&](auto& state){state.retained_land_region_concentration=!retained;});
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});bool preserved=false;
                restored.ResourceState([&](auto& state){preserved=state.retained_land_region_concentration==std::optional<bool>{retained};state.retained_land_region_concentration.reset();});
                GT_ASSERT(preserved,"Actual Start discarded the saved status");
                Found(GSE_CALL,restored.Game(),903,0,1,32,16);bool absent=false;
                restored.ResourceState([&](auto& state){absent=!state.retained_land_region_concentration;});
                GT_ASSERT(absent,"Legacy absent status was reconstructed at a Base event");GT_OK();
            });if(!second.empty()) {return second;}
        }
        GT_OK();
    });
	task->AddTest("SMAC live victory history follows founding nonlast removal actual Start and legacy absence",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);std::string saved;faction::SmacVictoryHistory retained;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
			bool fresh=false;fixture.ResourceState([&](auto& state){fresh=state.victory_history && *state.victory_history==faction::SmacVictoryHistory{};
				auto& history=*state.victory_history;history.last_base_expansion_or_advice_turn.fill(777);
				for(size_t owner=0;owner<8;++owner) {history.economic_deadlines[owner]=1000+owner;history.economic_paid_costs[owner]=5000+owner;}
			});GT_ASSERT(fresh,"New world did not own fresh economic and Base history");
			Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,902,0,1,24,16);
			bool stamped=false;fixture.ResourceState([&](auto& state){const auto& history=*state.victory_history;
				stamped=history.last_base_expansion_or_advice_turn[1]==1 && history.last_base_expansion_or_advice_turn[2]==777
					&& history.economic_deadlines[1]==1001 && history.economic_paid_costs[1]==5001;
				state.victory_history->last_base_expansion_or_advice_turn[1]=333;retained=*state.victory_history;
			});GT_ASSERT(stamped,"Founding did not stamp only gaining owner or changed economic history");
			game->GetBM()->DespawnBase(GSE_CALL,901);
			bool preserved=false;fixture.ResourceState([&](auto& state){preserved=*state.victory_history==retained;});
			GT_ASSERT(preserved,"Nonlast Base removal rewrote history");saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});bool preserved=false;
			restored.ResourceState([&](auto& state){preserved=state.victory_history && *state.victory_history==retained;state.victory_history.reset();});
			GT_ASSERT(preserved,"Actual Start rebuilt saved expansion or economic history");
			Found(GSE_CALL,restored.Game(),903,0,1,32,16);
			bool absent=false;restored.ResourceState([&](auto& state){absent=!state.victory_history;});
			GT_ASSERT(absent,"Founding manufactured missing legacy history");GT_OK();
		});
	});
	task->AddTest("SMAC live regional workspace preserves Base events but retires at owner tail and Start",GT(){
		namespace routes=map::smac_territory;
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);std::string saved;
		const auto graph=[](Game* game) {
			routes::RegionalRouteMap value{static_cast<int32_t>(game->GetMap()->GetWidth()),static_cast<int32_t>(game->GetMap()->GetHeight()),true,{}, {},2};
			value.tiles.assign(value.width*value.height/2,{false,1,1});value.tiles[Index(value.width,20,16)].territory=2;return value;
		};
		const routes::RegionalRouteQuery query{{16,16},{20,16},1,2,0};
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
			auto& workspace=game->GetMap()->GetRegionalPathWorkspace();const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
			GT_ASSERT(workspace.Query(graph(game),query,0,[](int32_t){return 8;}),"Live workspace failed warm route");
			const auto distances=workspace.Distances();
			GT_ASSERT(before==fixture.Snapshot() && random==fixture.GameplayRandom(),"Transient workspace entered save or RNG authority");
			Found(GSE_CALL,game,901,0,1,16,16);fixture.RefreshPopulation(1);
			GT_ASSERT(workspace.Destination()==query.target && workspace.Distances()==distances,"Founding or summary eagerly retired the route");
			game->GetBM()->DespawnBase(GSE_CALL,901);
			GT_ASSERT(workspace.Destination()==query.target && workspace.Distances()==distances,"Base removal retired the route");
			saved=fixture.Snapshot();
			std::array<std::string,8> roles;
			for(size_t i=0;i<roles.size();++i) {
				roles[i]="route-role-"+std::to_string(i);
				game->GetBM()->DefinePop(new base::PopDef(roles[i],roles[i],{},{},i ? base::PopDef::PF_NONE : base::PopDef::PF_TILE_WORKER,
					i ? std::optional<uint8_t>(i-1) : std::nullopt));
			}
			fixture.BasePresentationRoles(roles);
			// This asset-free source deliberately has synthetic negative score
			// inputs. Enter the supported pre-ranking state; upkeep owns its
			// first aggregate publication independently of the transient route.
			fixture.ResourceState([](auto& state){state.economy.at(1).population_aggregate.reset();});
			game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
			GT_ASSERT((workspace.Destination()==routes::Coordinate{-1,-1}) && workspace.Distances()==distances,
				"Actual owner tail failed destination-only invalidation");GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		const auto second=restored.Run([&](GSE_CALLABLE)->std::string {
			auto& old=restored.Game()->GetMap()->GetRegionalPathWorkspace();
			GT_ASSERT(old.Query(graph(restored.Game()),query,0,[](int32_t){return 8;}),"Restore fixture failed warm route");
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			auto* game=restored.Game();auto& workspace=game->GetMap()->GetRegionalPathWorkspace();
			GT_ASSERT((workspace.Destination()==routes::Coordinate{-1,-1})
				&& std::all_of(workspace.Distances().begin(),workspace.Distances().end(),[](uint32_t value){return value==0;}),"Actual Start restored a warm route");
			GT_ASSERT(workspace.Query(graph(game),query,0,[](int32_t){return 8;}),"Fresh restored workspace cannot query");
			GT_OK();
		});
		if(!second.empty()) {return second;}
		ConventionalUnitFixture imported;imported.ConfigureMovementCampaign(false);
		return imported.Run([&](GSE_CALLABLE)->std::string {
			auto* world=imported.Game()->GetMap();
			GT_ASSERT(world->GetRegionalPathWorkspace().Query(graph(imported.Game()),query,0,[](int32_t){return 8;}),"Raw import fixture failed warm route");
			auto bytes=world->GetTilesPtr()->Serialize();
			GT_ASSERT(world->LoadFromBuffer(bytes)==map::Map::EC_NONE,"Raw map replacement failed");
			GT_ASSERT((world->GetRegionalPathWorkspace().Destination()==routes::Coordinate{-1,-1}),"Raw map replacement retained route key");GT_OK();
		});
	});
	task->AddTest("SMAC live Base-phase flags retain peers removal and actual Start history",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);std::string saved;
		const auto flags=[](ConventionalUnitFixture& value,int owner) {
			std::optional<faction::SmacRegionalMilitaryState::Flags> result;
			value.ResourceState([&](auto& state){result=state.regional_military->factions.at(owner-1).base_phase_flags;});return result;
		};
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);auto* square=game->GetMap()->GetTile(18,16);
			for(auto* corner:square->elevation.corners) {*corner=-1000;}square->Update();
			// Corner elevations are shared: publish a coherent complete map,
			// including every neighbor changed by this controlled sea tile.
			for(size_t y=0;y<game->GetMap()->GetHeight();++y) {for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2) {game->GetMap()->GetTile(x,y)->Update();}}
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));Found(GSE_CALL,game,901,0,1,16,16);
			fixture.ResourceState([&](auto& state){
				state.regional_military->factions[0].base_phase_flags.emplace();state.regional_military->factions[0].base_phase_flags->fill(8);
				state.regional_military->factions[6].base_phase_flags.emplace();state.regional_military->factions[6].base_phase_flags->fill(128);
			});
			const auto random=fixture.GameplayRandom();fixture.RefreshPopulation(1);
			GT_ASSERT(game->GetTurnId()==1 && flags(fixture,1)->at(1)==136 && flags(fixture,1)->at(14)==0 && flags(fixture,7)->at(1)==128,
				"Actual Base summary lost sea/expiry gates or recounted a peer");
			game->GetBM()->DespawnBase(GSE_CALL,901);
			GT_ASSERT(flags(fixture,1)->at(1)==136 && fixture.GameplayRandom()==random,"Base removal eagerly refreshed flag history or drew RNG");
			saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			GT_ASSERT(flags(restored,1)->at(1)==136 && flags(restored,7)->at(1)==128,"Actual Start reconstructed stale Base flags");
			restored.RefreshPopulation(1);
			GT_ASSERT(flags(restored,1)->at(1)==8 && flags(restored,7)->at(1)==128,"Next summary retained removed Base coast or refreshed peer history");
			restored.ResourceState([&](auto& state){state.regional_military->factions[0].base_phase_flags.reset();});
			restored.RefreshPopulation(1);GT_ASSERT(!flags(restored,1),"Owner summary invented missing legacy bit8 history");GT_OK();
		});
	});
    task->AddTest("SMAC regional routing preserves native peer marks across split source components",GT(){
        namespace routes=game::backend::map::smac_territory;
        routes::RegionalRouteMap graph{16,12,false,std::vector<routes::RegionalRouteTile>(96,{true,-1,1}),{},0};
        const std::array<std::array<int32_t,3>,8> land{{{{2,2,1}},{{4,2,1}},{{2,8,1}},{{4,8,1}},{{6,8,-1}},{{8,8,-1}},{{10,8,2}},{{12,8,2}}}};
        for(const auto& point:land) {graph.tiles.at(point[1]*8+point[0]/2)={false,static_cast<int8_t>(point[2]),1};}
        const std::vector<routes::Base> bases{{{2,2},1},{{2,8},1},{{10,8},2}};
        GT_ASSERT(!routes::RegionalBasesReachable(graph,bases,1,2,1,0,2,1),"Native failed first source lost the peer component mark");
        GT_ASSERT(routes::RegionalLandReachable(graph,{{2,8},{10,8},1,2,0}),"Native second source direct path was not reachable");
        auto reordered=bases;reordered[0]=bases[1];reordered[1]=bases[0];
        GT_ASSERT(routes::RegionalBasesReachable(graph,reordered,1,2,1,0,2,1),"Native reachable-first Base order did not change the result");GT_OK();
    });

	task->AddTest("SMAC live regional fresh routes use actual claims Base order and saved Start",GT(){
		namespace routes=map::smac_territory;
		const auto query=[](ConventionalUnitFixture& fixture,int mode)->bool {
			auto* game=fixture.Game();auto* world=game->GetMap();
			routes::RegionalRelations relations{};uint8_t humans=0,own_bases=0,peer_bases=0;
			fixture.ResourceState([&](auto& state){
				humans=state.human_membership;
				for(const auto& owner:state.hostility.owners) {for(size_t target=0;target<8;++target) {relations.at(owner.first).at(target)=owner.second.directions[target].primary;}}
				own_bases=state.economy.at(1).regional_base_counts->at(1);peer_bases=state.economy.at(2).regional_base_counts->at(1);
			});
			const auto graph=routes::ProjectRegionalRoutes(*world->GetSmacWorkerKnowledge(),world->TerritorySea(),
				world->GetTerritoryTopology()->Regions(),relations,humans);
			std::vector<routes::Base> bases;
			for(const auto id:game->GetBM()->GetBaseRecordOrder()) {
				const auto* base=game->GetBM()->GetBase(id);const auto* tile=base->GetTile();
				bases.push_back({{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},base->GetSmacResourceContext().owner_simulation_id});
			}
			return routes::RegionalBasesReachable(graph,bases,1,2,1,mode,own_bases,peer_bases);
		};
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);std::string saved;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
			Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,800,1,2,64,16);
			const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
			GT_ASSERT(query(fixture,0) && !query(fixture,1),"Actual unclaimed corridor did not distinguish regional route modes");
			GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Fresh route query changed saved world or RNG");saved=before;GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			GT_ASSERT(query(restored,0) && !query(restored,1),"Actual Start changed retained route inputs");
			restored.Game()->GetBM()->DespawnBase(GSE_CALL,800);
			GT_ASSERT(!query(restored,0) && !query(restored,1),"Actual Base removal left an invented route endpoint");GT_OK();
		});
	});
    task->AddTest("SMAC territory summary matches 12 complete original aggregate passes",GT(){
        namespace native=map::smac_territory;
        for(const auto& row:territory_summary_fixture::Cases()) {
            const auto actual=native::Summarize(row.tiles,row.reveal,row.search);
            GT_ASSERT(actual==row.expected,"native territory summary differs: "+std::string(row.id));
            GT_ASSERT(native::TerritorySummary::Deserialize(actual.Serialize())==actual,"summary codec changed native retained output");
        }
        for(size_t turn=0;turn<8;++turn) {
            GT_ASSERT(native::RefreshTerritoryAtGlobalTail(turn)==(turn==1 || turn==5),"native global-upkeep gate differs");
        }
        // Native byte/word storage wraps; a region can reach the map bound.
        std::vector<native::SummaryTile> wrap(65536,{1,1,2,2,0,false,false,false,13,2});
        const auto actual=native::Summarize(wrap,0,0);
        GT_ASSERT(actual.owners[1].tiles[1]==0 && actual.owners[1].site_signals[1]==0
            && actual.site_signals[1]==65536 && actual.maximum_rating[1]==13,"byte word and global counter widths differ");
        GT_OK();
    });
    task->AddTest("SMAC retained territory summary framing preserves stale and absent history",GT(){
        namespace native=map::smac_territory;
        const auto bare=Topology::Build(16,8,true,std::vector<bool>(64,false));
        auto summary=territory_summary_fixture::Cases().back().expected;summary.search_objectives=128;
        const auto value=bare.WithTerritorySummary(summary);const auto wire=value.Serialize();
        GT_ASSERT(Topology::Deserialize(wire)==value && value.Rebuild(std::vector<bool>(64,false)).Summary()==value.Summary(),"restore or region rebuild recounted retained summary");
        auto old=bare.Serialize();old.replace(0,8,"GLSTERR4");
        GT_ASSERT(!Topology::Deserialize(old).Summary(),"old save acquired invented territory or Search authority");
        for(int variant=0;variant<4;++variant) {
            auto bad=wire;if(variant==0) {bad.pop_back();}if(variant==1) {bad.back()=1;}
            if(variant==2) {bad[7]='4';}if(variant==3) {bad[16+64+512+1024+4]=16;}
            GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"malformed retained summary accepted");
        }
        GT_ASSERT(Rejects([&]{native::Summarize({{128,0,0,0,0,false,false,false,0,0}},0,0);}),"invalid summary tile admitted");
        GT_OK();
    });
    task->AddTest("SMAC live territory summaries follow founding removal and actual Start",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);std::string saved,retained;
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();Land(game);auto* world=game->GetMap();
            for(size_t y=0;y<world->GetHeight();++y) {for(size_t x=y&1;x<world->GetWidth();x+=2) {
                auto* square=world->GetTile(x,y);square->moisture=map::tile::MOISTURE_RAINY;square->rockiness=map::tile::ROCKINESS_FLAT;
                square->features=0;square->terraforming=map::tile::TERRAFORMING_FARM;square->bonus=map::tile::BONUS_NONE;
            }}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
            const auto rng=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
            Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,800,1,2,48,16);
            const auto& summary=*world->GetTerritoryTopology()->Summary();
            GT_ASSERT(summary.owners[1].covered_suitable[1]==21 && summary.owners[2].covered_suitable[1]==21,"covered suitable counts used Farm or missed Base radius");
            GT_ASSERT(summary.owners[1].known[1]==3968 && summary.owners[2].known[63]==128 && summary.owners[0].known[1]==0,"known counters filtered exploration by ownership");
            GT_ASSERT(summary.owners[0].tiles[1]>0 && summary.owners[0].maximum_rating[1]==0 && summary.maximum_rating[1]>0,"neutral claims acquired an owned maximum");
            // Mutation/query/save alone must leave the retained producer untouched.
            const auto before=world->GetTerritoryTopology()->Summary();world->GetTile(0,0)->moisture=map::tile::MOISTURE_ARID;
            const auto snapshot=fixture.Snapshot();(void)snapshot;
            GT_ASSERT(world->GetTerritoryTopology()->Summary()==before,"snapshot refreshed territory counters");
            game->GetBM()->DespawnBase(GSE_CALL,800);
            const auto& removed=*world->GetTerritoryTopology()->Summary();
            GT_ASSERT(removed.owners[2].tiles[1]==0 && removed.owners[2].covered_suitable[1]==0 && removed.owners[1].covered_suitable[1]==21,"removal retained stale claims or dropped survivor exclusion");
            GT_ASSERT(fixture.GameplayRandom()==rng && game->GetRandom()->GetStateString()==jsf,"territory pass consumed gameplay RNG");
            retained=world->GetTerritoryTopology()->Serialize();saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return first;}
        ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
            GT_ASSERT(restored.Game()->GetMap()->GetTerritoryTopology()->Serialize()==retained,"actual Start rebuilt retained territory summary");GT_OK();
        });
    });

	task->AddTest("SMAC shared landmark detail retains native saved bytes and independent geography lifetimes",GT(){
		for(const auto& row:shared_landmark_fixture::storage) {
			const std::vector<bool> sea(64,false),fertile(64,true);
			const auto bare=Topology::Build(16,8,true,sea);
			GT_ASSERT(!bare.LandmarkDetails(),"region allocation invented initialized shared bytes");
			const auto value=bare.WithLandmarkDetails({row.before.begin(),row.before.end()});
			const auto loaded=Topology::Deserialize(value.Serialize());
			GT_ASSERT(loaded.LandmarkDetails() && *loaded.LandmarkDetails()==std::vector<uint8_t>(row.loaded.begin(),row.loaded.end()),
				"native saved shared bytes changed: "+std::string(row.name));
			GT_ASSERT(loaded.Rebuild(sea).WithTerrainPostpass(sea,fertile).LandmarkDetails()==value.LandmarkDetails(),
				"region fill or terrain postpass rewrote shared bytes");
			const auto sites=value.WithTerrainPostpass(sea,fertile).WithSiteRatings(map::smac_site::RatingCache::Fresh({16,8,true}));
			GT_ASSERT(Topology::Deserialize(sites.Serialize())==sites,"site flags consumed opaque high shared bytes");
			for(int version=1;version<=3;++version) {
				auto old=bare.Serialize();old[7]=static_cast<char>('0'+version);
				if(version==1) {old.resize(16+64);old[13]=0;}
				const auto legacy=Topology::Deserialize(old);
				GT_ASSERT(!legacy.LandmarkDetails() && !Topology::Deserialize(legacy.Serialize()).LandmarkDetails(),"legacy restore manufactured raw shared bytes");
			}
			GT_ASSERT(!(bare==bare.WithLandmarkDetails(std::vector<uint8_t>(64,0))),"unknown detail was treated as known zero");
			for(int variant=0;variant<4;++variant) {
				auto bad=sites.Serialize();if(variant==0) {bad.pop_back();}if(variant==1) {bad+='x';}
				if(variant==2) {bad[15]=4;}if(variant==3) {bad[7]='3';}
				GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"invalid shared-detail frame admitted");
			}
			GT_ASSERT(Rejects([&]{bare.WithLandmarkDetails(std::vector<uint8_t>(63,0));}),"wrong shared detail extent admitted");
		}GT_OK();
	});
	task->AddTest("SMAC live shared landmark detail survives founding removal save and actual Start",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);std::string saved;
		std::vector<uint8_t> details;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);auto* map=game->GetMap();
			for(size_t y=0;y<map->GetHeight();++y) {for(size_t x=y&1;x<map->GetWidth();x+=2) {
				auto* square=map->GetTile(x,y);square->moisture=map::tile::MOISTURE_RAINY;square->rockiness=map::tile::ROCKINESS_FLAT;
				square->bonus=map::tile::BONUS_NONE;square->features=0;square->terraforming=0;
			}}
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));const auto& topology=*map->GetTerritoryTopology();
			GT_ASSERT(topology.LandmarkDetails() && *topology.LandmarkDetails()==std::vector<uint8_t>(4096,0),"fresh map initializer omitted raw zero detail");
			details.resize(4096);for(size_t i=0;i<details.size();++i) {details[i]=shared_landmark_fixture::mutation_pattern[i%5];}
			fixture.TerritoryTopology(topology.WithLandmarkDetails(details).Serialize());
			Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,800,1,2,24,16);
			GT_ASSERT(*map->GetTerritoryTopology()->LandmarkDetails()==details,"founding rewrote overlapping shared byte");
			game->GetBM()->DespawnBase(GSE_CALL,800);
			GT_ASSERT(*map->GetTerritoryTopology()->LandmarkDetails()==details,"removal rewrote overlapping shared byte");
			saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();
			GT_ASSERT(*game->GetMap()->GetTerritoryTopology()->LandmarkDetails()==details,"actual Start reconstructed shared bytes");
			game->GetBM()->DespawnBase(GSE_CALL,901);
			GT_ASSERT(*game->GetMap()->GetTerritoryTopology()->LandmarkDetails()==details,"restored removal discarded shared history");GT_OK();
		});
	});
	task->AddTest("SMAC live shared landmark detail controls actual uncached site quality suppression",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);auto* map=game->GetMap();
			for(size_t y=0;y<map->GetHeight();++y) {for(size_t x=y&1;x<map->GetWidth();x+=2) {
				auto* square=map->GetTile(x,y);square->moisture=map::tile::MOISTURE_RAINY;square->rockiness=map::tile::ROCKINESS_FLAT;
				square->bonus=map::tile::BONUS_NONE;square->features=0;square->terraforming=0;
			}}
			map->GetTile(33,15)->features=map::tile::FEATURE_JUNGLE;
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));const auto topology=*map->GetTerritoryTopology();
			const auto active=map->PrepareSiteRatingRefill(topology,{},0);
			auto details=*topology.LandmarkDetails();details[Index(128,33,15)]=128;
			const auto suppressed=map->PrepareSiteRatingRefill(topology.WithLandmarkDetails(details),{},0);
			GT_ASSERT(active.SiteRatings()->Rating({{32,16}})==3 && suppressed.SiteRatings()->Rating({{32,16}})==1,
				"live site query ignored owned landmark suppression");
			GT_ASSERT(*suppressed.LandmarkDetails()==details,"site refill modified shared metadata");GT_OK();
		});
	});
	task->AddTest("SMAC live site cache joins founding removal rollback and actual Start",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);std::string saved,retained;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);auto* map=game->GetMap();
			for(size_t y=0;y<map->GetHeight();++y) {for(size_t x=y&1;x<map->GetWidth();x+=2) {
				auto* square=map->GetTile(x,y);square->moisture=map::tile::MOISTURE_RAINY;square->rockiness=map::tile::ROCKINESS_FLAT;
				square->bonus=map::tile::BONUS_NONE;square->features=0;square->terraforming=0;
			}}
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
			// A V3 world has no shared-byte history. Preserve that absence rather
			// than treating missing metadata as an initialized fresh zero.
			auto legacy=map->GetTerritoryTopology()->Serialize();legacy.resize(legacy.size()-map->GetWidth()*map->GetHeight()/2-map::smac_territory::TerritorySummary::WIRE_BYTES);
			legacy.replace(0,8,"GLSTERR3");legacy[15]=static_cast<char>(static_cast<uint8_t>(legacy[15])&1);fixture.TerritoryTopology(legacy);
			std::string packed(map->GetWidth()*map->GetHeight()/2,char(13));packed[Index(128,96,48)]=29;fixture.SiteRatings(packed);
			// This distant cached tile must never enter the uncached decoder.
			map->GetTile(80,50)->features=map::tile::FEATURE_VOLCANO;
			const auto random=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
			Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,800,1,2,24,16);
			const auto& sites=*map->GetTerritoryTopology()->SiteRatings();
			GT_ASSERT(sites.Excluded({{16,16}}) && sites.Rating({{16,16}})==13 && sites.Rating({{20,16}})==1
				&& sites.Rating({{80,50}})==13 && sites.Excluded({{96,48}}),"live founding refreshed stale ratings or missed the radius/cache producer");
			map->GetTile(52,16)->features=map::tile::FEATURE_VOLCANO;
			const auto before=fixture.Snapshot();const auto id=base::Base::GetNextId();const auto requests=fixture.QueuedFrontendRequests().size();
			GT_ASSERT(Rejects([&]{Found(GSE_CALL,game,id,0,1,48,16);}),"missing uncached landmark detail was manufactured");
			GT_ASSERT(fixture.Snapshot()==before && base::Base::GetNextId()==id && !map->GetTile(48,16)->base
				&& fixture.QueuedFrontendRequests().size()==requests,"failed cache preparation published Base IDs tiles queues or history");
			map->GetTile(52,16)->features=0;game->GetBM()->DespawnBase(GSE_CALL,800);
			const auto& removed=*map->GetTerritoryTopology()->SiteRatings();
			GT_ASSERT(!removed.Excluded({{24,16}}) && removed.Rating({{24,16}})==1 && removed.Excluded({{16,16}})
				&& removed.Excluded({{96,48}}) && removed.Rating({{96,48}})==13,"live removal did not repair survivors or preserve a stray exclusion");
			GT_ASSERT(fixture.GameplayRandom()==random && game->GetRandom()->GetStateString()==jsf,"site history consumed either gameplay stream");
			retained=map->GetTerritoryTopology()->Serialize();saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();
			GT_ASSERT(game->GetMap()->GetTerritoryTopology()->Serialize()==retained,"actual Start reconstructed stale cache history");
			game->GetBM()->DespawnBase(GSE_CALL,901);
			GT_ASSERT(game->GetMap()->GetTerritoryTopology()->SiteRatings()->Excluded({{96,48}}),"restored removal globally cleared exclusions");GT_OK();
		});
	});
	task->AddTest("SMAC geography matches 11 original coastal and retained region area compositions",GT(){
		for(const auto& item:coastal_geography_fixture::cases) {
			const auto count=size_t(item.width)*item.height/2;std::vector<bool> sea(count);
			for(size_t i=0;i<count;++i) {sea[i]=item.land[i]=='0';}
			Topology::RegionValues before{},after{};for(const auto& row:item.before) {before.at(row.first)=row.second;}
			for(const auto& row:item.after) {after.at(row.first)=row.second;}
			auto actual=Topology::Build(item.width,item.height,item.wrap,sea,before);
			GT_ASSERT(actual.Areas() && *actual.Areas()==after,"stored areas differ: "+std::string(item.id));
			const auto hex=[](char digit){return digit<='9' ? digit-'0' : digit-'a'+10;};
			for(size_t i=0;i<count;++i) {GT_ASSERT(actual.Regions()[i]==16*hex(item.regions[2*i])+hex(item.regions[2*i+1]),"region labels differ: "+std::string(item.id));}
			if(item.postpass) {
				actual=actual.WithTerrainPostpass(sea,std::vector<bool>(count,false));
				for(size_t i=0;i<count;++i) {GT_ASSERT(actual.Postpass()->coastal[i]==item.coastal[i]-'0',"coastal flags differ: "+std::string(item.id)+" tile "+std::to_string(i));}
			}
			GT_ASSERT(Topology::Deserialize(actual.Serialize())==actual,"geography codec changed native history");
		}GT_OK();
	});
	task->AddTest("SMAC geography separates area fertility and restore lifetimes without inventing V1 history",GT(){
		std::vector<bool> sea(1024,false),fertile(1024,true);
		auto value=Topology::Build(64,32,true,sea);GT_ASSERT(value.Areas()->at(1)==1024 && value.Areas()->at(63)==64 && !value.Postpass() && !value.SiteRatings(),"region fill invented postpass/cache history or omitted enqueued polar area");
		value=value.WithSiteRatings(map::smac_site::RatingCache::Fresh({64,32,true}));
		value=value.WithTerrainPostpass(sea,fertile);
		GT_ASSERT(value.Postpass()->fertility.at(1)==960 && value.Postpass()->fertility.at(63)==64,"postpass did not count final polar labels separately");
		sea[Index(64,4,4)]=true;fertile[Index(64,4,4)]=false;
		auto changed=value.Rebuild(sea);changed.ValidateTerrain(sea);
		GT_ASSERT(changed.Areas()->at(1)==1023 && changed.Areas()->at(63)==128 && changed.Areas()->at(65)==1 && changed.Postpass()==value.Postpass(),"fill refreshed postpass data or repaired polar accumulation");
		changed=changed.WithTerrainPostpass(sea,fertile);
		GT_ASSERT(changed.Postpass()->fertility.at(1)==959 && changed.Postpass()->fertility.at(63)==64,"postpass did not replace old F");
		const auto wire=changed.Serialize();GT_ASSERT(Topology::Deserialize(wire)==changed,"stale areas changed on restore");
		for(int variant=0;variant<3;++variant) {auto bad=wire;if(variant==0) {bad[13]=0;}if(variant==1) {bad[16+1024+1024]=4;}if(variant==2) {bad.pop_back();}
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"invalid geography metadata accepted");}
		for(const size_t word:{size_t(0),size_t(127),size_t(128),size_t(255)}) {for(const int32_t negative:{-1,INT32_MIN}) {
			auto bad=wire;WriteArea(bad,16+1024+4*word,negative);
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"negative saved area or fertility counter accepted");
		}}
		for(const auto region:{1,63,65}) {auto bad=wire;WriteArea(bad,16+1024+4*region,0);
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"zero area accepted for a referenced land water or polar region");}
		auto retained=wire;WriteArea(retained,16+1024+4*127,INT32_MAX);
		GT_ASSERT(Topology::Deserialize(retained).Serialize()==retained,"valid retained positive history was recounted or clamped");
		auto legacy=value.Serialize().substr(0,1040);legacy.replace(0,8,"GLSTERR1");legacy[13]=0;legacy[14]=0;legacy[15]=0;
		const auto old=Topology::Deserialize(legacy);old.ValidateTerrain(std::vector<bool>(1024,false));
		GT_ASSERT(!old.Areas() && !old.Postpass() && !old.SiteRatings() && Topology::Deserialize(old.Serialize())==old,"V1 load invented retained geography");
		auto v2=value.Serialize();v2.resize(v2.size()-1024);v2.replace(0,8,"GLSTERR2");v2[15]=0;
		const auto previous=Topology::Deserialize(v2);
		GT_ASSERT(previous.Areas()==value.Areas() && previous.Postpass()==value.Postpass() && !previous.SiteRatings()
			&& Topology::Deserialize(previous.Serialize())==previous,"V2 load invented rating or exclusion history");
		GT_ASSERT(Rejects([&]{old.Rebuild(sea);}) && Rejects([&]{old.LandRegionConcentrated();}) && Rejects([&]{old.WithTerrainPostpass(sea,fertile);}),"missing polar history was fabricated");GT_OK();
	});
	task->AddTest("SMAC live geography admission and actual Start retain stale postpass data",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);fixture.ResourcePolicy(true);std::string saved,geography;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);auto* map=game->GetMap();
			GT_ASSERT(map->GetWidth()==128 && map->GetHeight()==64,"geography fixture dimensions changed");
			for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {auto* tile=map->GetTile(x,y);tile->moisture=map::tile::MOISTURE_RAINY;tile->rockiness=map::tile::ROCKINESS_FLAT;tile->features=0;}}
			map->GetTile(8,8)->moisture=map::tile::MOISTURE_ARID;map->GetTile(10,8)->rockiness=map::tile::ROCKINESS_ROCKY;map->GetTile(12,8)->features=map::tile::FEATURE_XENOFUNGUS;
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));const auto& topology=*map->GetTerritoryTopology();
			GT_ASSERT(topology.Areas()->at(1)==4096 && topology.Postpass() && topology.Postpass()->fertility.at(1)==3965 && topology.Postpass()->fertility.at(63)==128,"live terrain adapter confused port ordinals or ignored fertility exclusions");
			geography=topology.Serialize();map->GetTile(14,8)->moisture=map::tile::MOISTURE_ARID;
			game->PublishSmacWorkerKnowledge(GSE_CALL,*map->GetSmacWorkerKnowledge());
			GT_ASSERT(map->GetTerritoryTopology()->Serialize()==geography,"knowledge publication silently refreshed retained geography");
			GT_ASSERT(topology.SiteRatings(),"fresh map admission omitted rating-cache initialization");
			auto packed=topology.SiteRatings()->Serialize();packed[Index(128,20,20)]=29;packed[Index(128,22,20)]=5;
			fixture.SiteRatings(packed);geography=map->GetTerritoryTopology()->Serialize();saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			GT_ASSERT(restored.Game()->GetMap()->GetTerritoryTopology()->Serialize()==geography,"actual Start replayed a terrain postpass or derived area history");GT_OK();
		});
	});
	task->AddTest("SMAC territory topology matches 16 original seed and polar projections",GT(){
		for(const auto& item:territory_topology_fixture::cases) {
			std::vector<bool> sea(item.width*item.height/2,item.all_sea);
			for(const auto& at:item.sea) {sea[Index(item.width,at[0],at[1])]=true;}
			const auto actual=Topology::Build(item.width,item.height,item.wrap,sea);
			for(const auto& probe:item.probes) {GT_ASSERT(actual.Regions()[Index(item.width,probe.position[0],probe.position[1])]==probe.region,"seed/polar mismatch: "+std::string(item.id));}
			GT_ASSERT(Topology::Deserialize(actual.Serialize())==actual,"topology roundtrip changed owned regions");
		}
		GT_OK();
	});
	task->AddTest("SMAC territory topology rejects capacity malformed frames and stale terrain",GT(){
		std::vector<bool> sea(32,false);const auto value=Topology::Build(8,8,true,sea);const auto wire=value.Serialize();
		sea[10]=true;GT_ASSERT(Rejects([&]{value.ValidateTerrain(sea);}),"changed terrain retained region authority");
		for(int i=0;i<7;++i) {
			auto bad=wire;if(i==0){bad.pop_back();}if(i==1){bad+='x';}if(i==2){bad[12]=2;}if(i==3){bad[13]=2;}
			if(i==4){bad[16]=1;}if(i==5){bad[20]=64;}if(i==6){bad[8]=3;}
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"invalid topology frame admitted");
		}
		std::vector<bool> islands(128*64/2,false);
		for(int y=4;y<60;y+=4){for(int x=4;x<124;x+=4){islands[Index(128,x,y)]=true;}}
		GT_ASSERT(Rejects([&]{Topology::Build(128,64,true,islands);}),"unmeasured region-capacity merging silently admitted");
		types::Buffer framed;world_snapshot::WriteTerritoryTopology(framed,value,true);framed.WriteInt(17);
		GT_ASSERT(world_snapshot::ReadTerritoryTopology(framed,true)==value && framed.ReadInt()==17,"topology prefix consumed map bytes");
		types::Buffer oversized;oversized.WriteBool(true);const auto cursor=oversized.lenw;oversized.WriteString(std::string(Topology::MAX_WIRE_BYTES+1,'x'));
		GT_ASSERT(Rejects([&]{world_snapshot::ReadTerritoryTopology(oversized,true);}) && oversized.lenr==cursor,"oversized topology copied before bounded rejection");
		GT_OK();
	});
	task->AddTest("SMAC territory freezes raw row48 with clamped border and exact world equality",GT(){
		for(const auto raw:{INT32_MIN,-1,0,8,100,101,INT32_MAX}) {
			const auto rule=loader::txt::SmacRulesTXTLoader::ParseTerritoryRules(Rules(raw));
			GT_ASSERT(rule.RawBorderDistance()==raw && rule.BorderDistance()==std::max(0,std::min(100,raw)),"border source/clamp differs");
			GT_ASSERT(settings::SmacTerritoryRules::Deserialize(rule.Serialize())==rule,"raw border codec lost authority");
		}
		settings::GlobalSettings a,b;
		const auto initialize=[&](settings::GlobalSettings& target,int32_t raw) {
			target.SelectRuleset("smac");target.InitializeHostRules([&](const std::string& name){auto text=smac_rules_fixture::Source(name);if(name=="alpha.txt"){
				// Change only the border row; retain the full fixture's technology
				// tokens and other frozen profiles required by host initialization.
				auto row=text.find("#RULES\n")+7;
				for(int index=1;index<48;++index){row=text.find('\n',row)+1;}
				text.replace(row,text.find('\n',row)-row,std::to_string(raw)+", ; border fixture");}return text;});smac_rules_fixture::AcceptWorldSize(target);
		};
		initialize(a,100);initialize(b,101);const auto saved=a.Serialize().ToString();
		GT_ASSERT(Rejects([&]{a.Deserialize(b.Serialize());}) && a.Serialize().ToString()==saved,"clamp-equal raw border changed locked settings");
		types::Buffer header;world_snapshot::WriteHeader(header,b);
		GT_ASSERT(Rejects([&]{world_snapshot::ValidateHeader(header,a);}) && a.Serialize().ToString()==saved,"world replaced raw border authority");
		GT_OK();
	});
	task->AddTest("SMAC live territory founding removal and restore preserve actual Base record order",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);std::string snapshot,knowledge,topology;
		const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);const auto supplied=Supplied(game);game->PublishSmacWorkerKnowledge(GSE_CALL,supplied);
			const auto random=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
			Found(GSE_CALL,game,901,0,1,16,16);Found(GSE_CALL,game,800,1,2,24,16);
			GT_ASSERT(game->GetBM()->GetBaseRecordOrder()==std::vector<size_t>({901,800}),"record order inferred from ascending IDs");
			auto data=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
			GT_ASSERT(data.tiles[Index(128,20,16)].claimant==2,"later actual Base record did not win tie");
			GT_ASSERT(data.remembered==supplied.CopyData().remembered && data.tiles[0].explored_by==6,"territory wrote sight/exploration history");
			game->GetBM()->DespawnBase(GSE_CALL,800);
			GT_ASSERT(game->GetMap()->GetSmacWorkerKnowledge()->CopyData().tiles[Index(128,20,16)].claimant==1,"removal retained stale winning claim");
			Found(GSE_CALL,game,700,1,2,24,16);
			GT_ASSERT(game->GetBM()->GetBaseRecordOrder()==std::vector<size_t>({901,700}),"removal/insertion changed retained record order");
			GT_ASSERT(fixture.GameplayRandom()==random && game->GetRandom()->GetStateString()==jsf,"territory consumed RNG");
			snapshot=fixture.Snapshot();knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();topology=game->GetMap()->GetTerritoryTopology()->Serialize();GT_OK();
		});GT_ASSERT(result.empty(),"founding fixture: "+result);
		ConventionalUnitFixture restored;restored.ConfigureMovementCampaign(false);
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,snapshot,[](GSE_CALLABLE){});auto* game=restored.Game();
			GT_ASSERT(game->GetBM()->GetBaseRecordOrder()==std::vector<size_t>({901,700}) && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge
				&& game->GetMap()->GetTerritoryTopology()->Serialize()==topology,"restore replayed claims or sorted Base records");
			game->GetBM()->DespawnBase(GSE_CALL,700);
			GT_ASSERT(game->GetMap()->GetSmacWorkerKnowledge()->CopyData().tiles[Index(128,20,16)].claimant==1,"restored next removal used different order/topology");GT_OK();
		});
	});
	task->AddTest("SMAC live territory rejects incomplete founding before center ID or knowledge mutation",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();Land(game);
			game->GetBM()->DefinePop(new base::PopDef("territory-specialist","Specialist",{},{},base::PopDef::PF_NONE,uint8_t(0)));
			game->PublishSmacWorkerKnowledge(GSE_CALL,Supplied(game));
			const auto before=fixture.Snapshot();const auto id=base::Base::GetNextId();const auto requests=fixture.QueuedFrontendRequests().size();
			GT_ASSERT(Rejects([&]{Found(GSE_CALL,game,id+17,0,1,16,16,false);}),"incomplete founding adopted an implicit allocation");
			GT_ASSERT(fixture.Snapshot()==before && base::Base::GetNextId()==id && !game->GetMap()->GetTile(16,16)->base
				&& fixture.QueuedFrontendRequests().size()==requests,"rejected founding changed center IDs queue or saved authority");GT_OK();
		});
	});
}
}
