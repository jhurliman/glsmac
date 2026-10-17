#include "SmacLiveTerritory.h"
#include "ConventionalUnits.h"
#include "fixtures/GlsmacTerritoryTopology.h"
#include "fixtures/SmacRulesSource.h"
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
			auto bad=wire;if(i==0){bad.pop_back();}if(i==1){bad+='x';}if(i==2){bad[12]=2;}if(i==3){bad[13]=1;}
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
				const auto start=text.find("#RULES\n"),end=text.find("#",start+1);text.replace(start,end-start,Rules(raw));}return text;});smac_rules_fixture::AcceptWorldSize(target);
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
