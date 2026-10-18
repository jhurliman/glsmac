#include "SmacLiveTerritory.h"
#include "ConventionalUnits.h"
#include "fixtures/GlsmacTerritoryTopology.h"
#include "fixtures/SmacCoastalGeography.h"
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
		auto value=Topology::Build(64,32,true,sea);GT_ASSERT(value.Areas()->at(1)==1024 && value.Areas()->at(63)==64 && !value.Postpass(),"fresh region fill invented fertility or omitted enqueued polar area");
		value=value.WithTerrainPostpass(sea,fertile);
		GT_ASSERT(value.Postpass()->fertility.at(1)==960 && value.Postpass()->fertility.at(63)==64,"postpass did not count final polar labels separately");
		sea[Index(64,4,4)]=true;fertile[Index(64,4,4)]=false;
		auto changed=value.Rebuild(sea);changed.ValidateTerrain(sea);
		GT_ASSERT(changed.Areas()->at(1)==1023 && changed.Areas()->at(63)==128 && changed.Areas()->at(65)==1 && changed.Postpass()==value.Postpass(),"fill refreshed postpass data or repaired polar accumulation");
		changed=changed.WithTerrainPostpass(sea,fertile);
		GT_ASSERT(changed.Postpass()->fertility.at(1)==959 && changed.Postpass()->fertility.at(63)==64,"postpass did not replace old F");
		const auto wire=changed.Serialize();GT_ASSERT(Topology::Deserialize(wire)==changed,"stale areas changed on restore");
		for(int variant=0;variant<3;++variant) {auto bad=wire;if(variant==0) {bad[13]=0;}if(variant==1) {bad.back()=4;}if(variant==2) {bad.pop_back();}
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"invalid geography metadata accepted");}
		for(const size_t word:{size_t(0),size_t(127),size_t(128),size_t(255)}) {for(const int32_t negative:{-1,INT32_MIN}) {
			auto bad=wire;WriteArea(bad,16+1024+4*word,negative);
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"negative saved area or fertility counter accepted");
		}}
		for(const auto region:{1,63,65}) {auto bad=wire;WriteArea(bad,16+1024+4*region,0);
			GT_ASSERT(Rejects([&]{Topology::Deserialize(bad);}),"zero area accepted for a referenced land water or polar region");}
		auto retained=wire;WriteArea(retained,16+1024+4*127,INT32_MAX);
		GT_ASSERT(Topology::Deserialize(retained).Serialize()==retained,"valid retained positive history was recounted or clamped");
		auto legacy=value.Serialize().substr(0,1040);legacy.replace(0,8,"GLSTERR1");legacy[13]=0;legacy[14]=0;
		const auto old=Topology::Deserialize(legacy);old.ValidateTerrain(std::vector<bool>(1024,false));
		GT_ASSERT(!old.Areas() && !old.Postpass() && Topology::Deserialize(old.Serialize())==old,"V1 load invented retained geography");
		GT_ASSERT(Rejects([&]{old.Rebuild(sea);}) && Rejects([&]{old.WithTerrainPostpass(sea,fertile);}),"missing polar history was fabricated");GT_OK();
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
			GT_ASSERT(map->GetTerritoryTopology()->Serialize()==geography,"knowledge publication silently refreshed retained geography");saved=fixture.Snapshot();GT_OK();
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
