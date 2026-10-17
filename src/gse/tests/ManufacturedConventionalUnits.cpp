#include "ManufacturedConventionalUnits.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/unit/FormerMarkerRender.h"
#include "game/backend/unit/ColonyMarkerRender.h"
#include "game/backend/unit/SmacGroundEdgeCost.h"
#include "game/frontend/unit/UnitDef.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "scene/Scene.h"
#include "types/texture/Texture.h"
#include "gse/callable/Native.h"
#include "gse/value/Undefined.h"
#include "util/FinallyGuard.h"

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::unit;
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
struct World { base::Base* home; Unit* scout; Def* former; };
World Populate( GSE_CALLABLE, ConventionalUnitFixture& fixture ) {
	auto* game = fixture.Game(); auto* um = game->GetUM(); auto* bm = game->GetBM();
	for ( size_t y=0; y<game->GetMap()->GetHeight(); ++y ) { for ( size_t x=y&1; x<game->GetMap()->GetWidth(); x+=2 ) {
		auto* tile=game->GetMap()->GetTile(x,y); for(auto* corner:tile->elevation.corners){*corner=100;}
		tile->features=0; tile->terraforming=0; tile->rockiness=map::tile::ROCKINESS_FLAT; tile->Update();
	} }
	MoraleSet::morale_values_t names; for(int i=0;i<7;++i){names.emplace_back("level"+std::to_string(i));}
	um->DefineMoraleSet(new MoraleSet("manufactured-morale",names));
	auto* scout_def=new StaticDef("manufactured-scout",um->GetMoraleSet("manufactured-morale"),"Scout",MT_LAND,999,
		new CvrScoutRender(2,0,100,100,{1,1},{128,128}));
	auto* former_def=new StaticDef("manufactured-former",um->GetMoraleSet("manufactured-morale"),"Former (port marker)",MT_LAND,999,new FormerMarkerRender());
	um->DefineUnit(scout_def); um->DefineUnit(former_def);
	base::Base::SmacAllocationRoles roles; roles.worker="factory-worker";
	bm->DefinePop(new base::PopDef(roles.worker,"Worker",{},{},base::PopDef::PF_TILE_WORKER));
	for(uint8_t i=0;i<7;++i){roles.specialists[i]="factory-specialist"+std::to_string(i);bm->DefinePop(new base::PopDef(roles.specialists[i],"Specialist",{},{},base::PopDef::PF_NONE,i));}
	bm->DefineFacility(new base::FacilityDef("factory-hq","HQ","HQ",1,{"fixture",0,0,1,1},1));
	auto* owner=&game->GetState()->m_slots->GetSlot(0); auto* center=game->GetMap()->GetTile(16,16);
	base::Base::pops_t pops; pops.emplace(1,base::Pop(nullptr,1,bm->GetPopDef(roles.specialists[1])));
	auto* home=new base::Base(game,901,owner,owner->GetPlayer()->GetFaction(),center,"Factory",{{"factory-hq",bm->GetFacilityDef("factory-hq")}},pops,2,0,{},base::SmacBaseResourceContext{1,false,0,0,0,false});
	bm->SpawnBase(GSE_CALL,home); base::SmacBaseAllocationState allocation{1,{{},1}}; allocation.specialists.types[0]=1;
	home->PublishSmacAllocation(GSE_CALL,allocation,roles);
	auto* scout=um->SpawnConventionalUnit(GSE_CALL,71,scout_def,owner,center,{2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
	map::SmacWorkerKnowledge::Data knowledge{128,64,true,true,0,{},{},{}}; knowledge.tiles.resize(4096,{2,std::nullopt});
	knowledge.observers[1]=map::SmacWorkerKnowledge::Observer{}; knowledge.observers[1]->pact_with=0; knowledge.units={{71,0,false}};
	game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(knowledge));
	fixture.RetainedMorale(-1,5); fixture.ResourceState([](auto& state){state.social.factions.front().retained->at(2)=0;});
	fixture.Updates(); return {home,scout,former_def};
}
Unit* Manufacture( GSE_CALLABLE, ConventionalUnitFixture& fixture, const World& world, bool notify=true ) {
	auto candidate=fixture.Game()->GetUM()->PrepareManufacturedConventionalUnit(GSE_CALL,*world.home,1,world.former,SmacMoraleMode::Mode0);
	auto* unit=candidate.Commit(); if(notify){candidate.Notify(GSE_CALL);} return unit;
}
}

void AddManufacturedConventionalUnitTests( task::gsetests::GSETests* task ) {
	task->AddTest("Manufactured land training uses current morale and exact facility roles",GT(){
		for (const bool low_morale : {false,true}) {
			ConventionalUnitFixture fixture; fixture.ConfigureMovementCampaign(false);
			const auto error=fixture.Run([&](GSE_CALLABLE)->std::string{
				const auto world=Populate(GSE_CALL,fixture); auto* game=fixture.Game(); auto* bm=game->GetBM();
				for (const int32_t role : {3,4,8,27,30}) {
					const auto id="training-role"+std::to_string(role);
					bm->DefineFacility(new base::FacilityDef(id,"Unrelated display label","",1,{"fixture",0,0,1,1},role));
				}
				fixture.RetainedMorale(5,low_morale?-2:0);
				for (const int32_t role : {3,4,8}) {
					const auto id="training-role"+std::to_string(role); world.home->m_facilities.emplace(id,bm->GetFacilityDef(id));
				}
				GT_ASSERT(Manufacture(GSE_CALL,fixture,world)->Conventional().stored_morale==1,"unrelated starting facilities changed production training");
				world.home->m_facilities.emplace("training-role27",bm->GetFacilityDef("training-role27"));
				GT_ASSERT(Manufacture(GSE_CALL,fixture,world)->Conventional().stored_morale==(low_morale?2:3),"Command Center did not use current morale");
				world.home->m_facilities.emplace("training-role30",bm->GetFacilityDef("training-role30"));
				GT_ASSERT(Manufacture(GSE_CALL,fixture,world)->Conventional().stored_morale==(low_morale?3:5),"combined land training was not halved together");
				GT_OK();
			});
			GT_ASSERT(error.empty(),"fixture failed: " + error);
		}
		GT_OK();
	});
	task->AddTest("Manufactured conventional preparation preserves authority and defers its hook",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);
		return fixture.Run([&](GSE_CALLABLE)->std::string{
			const auto world=Populate(GSE_CALL,fixture);auto* game=fixture.Game();auto* um=game->GetUM();
			auto encoded=Def::Serialize(world.former);std::unique_ptr<Def> decoded(Def::Deserialize(encoded,um));
			GT_ASSERT(Def::Serialize(decoded.get()).ToString()==Def::Serialize(world.former).ToString(),"Former descriptor did not restore exactly");
			types::Buffer bad;bad.WriteString("wrong-version");bad.WriteString("manufactured-morale");bad.WriteString("Former");bad.WriteInt(DT_STATIC);
			bad.WriteInt(MT_LAND);bad.WriteFloat(1);bad.WriteInt(Render::RT_PORT_FORMER_MARKER);bad.WriteInt(2);
			GT_ASSERT(Rejects([&]{std::unique_ptr<Def> rejected(Def::Deserialize(bad,um));}),"unknown Former descriptor version admitted");
			const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto members=world.home->GetTile()->units;
			size_t hooks=0;bool caller_committed=false;
			um->On(GSE_CALL,"unit_spawn",NATIVE_CALL(&hooks,&caller_committed){if(!caller_committed){throw std::runtime_error("hook before caller commit");}++hooks;return VALUE(value::Undefined);}));
			{auto candidate=um->PrepareManufacturedConventionalUnit(GSE_CALL,*world.home,1,world.former,SmacMoraleMode::Mode0);}
			GT_ASSERT(fixture.Snapshot()==before&&Unit::GetNextId()==next&&world.home->GetTile()->units==members&&hooks==0&&fixture.Updates().empty(),"abandoned prepared Unit published authority");
			GT_ASSERT(Rejects([&]{um->PrepareManufacturedConventionalUnit(GSE_CALL,*world.home,1,world.scout->m_def,SmacMoraleMode::Mode0);})
				&&Rejects([&]{um->PrepareManufacturedConventionalUnit(GSE_CALL,*world.home,2,world.former,SmacMoraleMode::Mode0);}),"Former/Scout renderer mismatch admitted");
			GT_ASSERT(fixture.Snapshot()==before&&Unit::GetNextId()==next,"failed admission changed world");
			auto candidate=um->PrepareManufacturedConventionalUnit(GSE_CALL,*world.home,1,world.former,SmacMoraleMode::Mode0);auto* unit=candidate.Commit();
			GT_ASSERT(unit&&hooks==0&&um->GetUnit(unit->m_id)==unit&&unit->GetTile()->units.size()==2,"prepared commit lost stack membership or invoked hook");
			caller_committed=true;candidate.Notify(GSE_CALL);GT_ASSERT(hooks==1&&Rejects([&]{candidate.Notify(GSE_CALL);}),"notification was not exactly once");
			GT_OK();
		});
	});
	task->AddTest("Manufactured Former actual support stack departure and upkeep preserve original fields",GT(){
		ConventionalUnitFixture fixture;fixture.ConfigureMovementCampaign(false);
		return fixture.Run([&](GSE_CALLABLE)->std::string{
			const auto world=Populate(GSE_CALL,fixture);auto* game=fixture.Game();Unit* first=nullptr;
			for(int i=0;i<3;++i){auto* unit=Manufacture(GSE_CALL,fixture,world);if(!first){first=unit;}
				GT_ASSERT(unit->Conventional().design_id==1&&unit->Conventional().home_base_id==901&&unit->Conventional().stored_morale==1
					&&unit->Conventional().damage==0&&unit->Conventional().spent==0&&!unit->Conventional().failed_move
					&&unit->QueryStats(SmacMoraleMode::Mode0).effective_morale==0&&unit->QueryStats(SmacMoraleMode::Mode0).full_allowance==3,"original untrained Former creator fields differ");
				GT_ASSERT(game->GetCanonicalMineralSupport(*world.home)==(i==2?1:0),"Former was exempted from ordinary support");
			}
			auto* source=first->GetTile();auto* target=game->GetMap()->GetTile(18,16);auto* wrapper=first->Wrap(GSE_CALL);
			const auto random=fixture.GameplayRandom();const auto home_bytes=base::Base::Serialize(world.home).ToString();
			fixture.MoveEvent(GSE_CALL,first,target);
			GT_ASSERT(first->GetTile()==target&&first->Conventional().spent==3&&source->units.size()==3&&source->units.at(71)==world.scout
				&&target->units.at(first->m_id)==first&&Unit::Unwrap(wrapper)==first&&fixture.GameplayRandom()==random,"actual Former departure lost stable stack identity or movement state");
			game->GetUM()->ResetConventionalSpent(GSE_CALL,first);
			GT_ASSERT(first->Conventional().spent==0&&first->Conventional().home_base_id==901&&first->QueryStats(SmacMoraleMode::Mode0).full_allowance==3
				&&base::Base::Serialize(world.home).ToString()==home_bytes&&game->GetCanonicalMineralSupport(*world.home)==1,"upkeep reset changed home/Base or support");
			const auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
			GT_ASSERT(knowledge.units.size()==4&&knowledge.remembered[1].at(16*64+8)->unit_present,"departure erased remaining friendly occupation");
			GT_OK();
		});
	});
	task->AddTest("Manufactured Former actual deferred restore binds home without creation replay",GT(){
		ConventionalUnitFixture source;source.ConfigureMovementCampaign(false);std::string snapshot,record;size_t id=0;
		const auto error=source.Run([&](GSE_CALLABLE)->std::string{const auto world=Populate(GSE_CALL,source);auto* unit=Manufacture(GSE_CALL,source,world);
			id=unit->m_id;record=Unit::Serialize(unit).ToString();snapshot=source.Snapshot();GT_OK();});if(!error.empty()){return error;}
		ConventionalUnitFixture destination;destination.ConfigureMovementCampaign(false);
		return destination.Run([&](GSE_CALLABLE)->std::string{
			size_t hooks=0;destination.Game()->GetUM()->On(GSE_CALL,"unit_spawn",NATIVE_CALL(&hooks){++hooks;return VALUE(value::Undefined);}));
			destination.Restore(GSE_CALL,snapshot,true);auto* unit=destination.Game()->GetUM()->GetUnit(id);
			GT_ASSERT(unit&&Unit::Serialize(unit).ToString()==record&&unit->Conventional().home_base_id==901&&hooks==0
				&&unit->QueryStats(SmacMoraleMode::Mode0).full_allowance==3&&unit->GetTile()->units.size()==2
				&&static_cast<StaticDef*>(unit->m_def)->m_render->m_type==Render::RT_PORT_FORMER_MARKER,"restore lost Former package/home/stack or replayed creation");GT_OK();
		});
	});
	task->AddTest("Manufactured Former marker is distinct owned presentation and bounded descriptor",GT(){
		scene::Scene scene("Former marker fixture",scene::SCENE_TYPE_NONE);std::shared_ptr<types::texture::Texture> retained;
		{
			game::frontend::sprite::InstancedSpriteManager sprites(&scene);MoraleSet::morale_values_t names;for(int i=0;i<7;++i){names.emplace_back("level");}MoraleSet morale("marker",names);
			StaticDef def("marker",&morale,"Former (port marker)",MT_LAND,1,new FormerMarkerRender());game::frontend::unit::UnitDef view(&sprites,&def);
			auto* sprite=view.GetSprite(1)->instanced_sprite;retained=sprite->texture_owner;
			GT_ASSERT(sprite==view.GetSprite(6)->instanced_sprite&&sprite->key.find("PortFormerMarker:")==0&&retained&&retained->GetWidth()==64
				&&retained->GetPixel(0,0)==0&&retained->GetPixel(20,20)!=0,"distinct generated Former marker lost ownership or contents");
		}
		GT_ASSERT(retained&&retained->GetPixel(20,20)!=0,"retained preview texture died with sprite registration");
		{
			game::frontend::sprite::InstancedSpriteManager sprites(&scene); MoraleSet::morale_values_t names; for(int i=0;i<7;++i){names.emplace_back("green");} MoraleSet morale("colony",names);
			StaticDef def("colony",&morale,"Colony marker",MT_LAND,0,new ColonyMarkerRender());game::frontend::unit::UnitDef view(&sprites,&def);
			auto* sprite=view.GetSprite(1)->instanced_sprite;
			GT_ASSERT(sprite->key.find("PortColonyMarker:")==0&&sprite->texture_owner->GetPixel(20,20)!=retained->GetPixel(20,20),"Colony marker reused Former identity or pixels");
		} GT_OK();
	});
	task->AddTest("Manufactured Former edge pricing preserves package terrain order",GT(){
		using namespace smac_edge;Query q{{1,Chassis::Infantry,0,0,9,9,0,3},3,1,0,false,false,128,64,true,{16,16,3,0,Base},{18,16,3,0,0}};
		GT_ASSERT(QueryGroundEdgeCost(q)==3,"observed ordinary Former edge differs");q.destination.features=Forest;q.destination.rockiness=2;
		GT_ASSERT(QueryGroundEdgeCost(q)==9,"Former terrain ordering differs");q.destination.features=Fungus;q.destination.rockiness=0;q.retained_planet=-4;
		GT_ASSERT(QueryGroundEdgeCost(q)==3,"plan9 must use basic-max fungus cost independent of retained PLANET");
		q.design.weapon_mode=0;GT_ASSERT(Rejects([&]{QueryGroundEdgeCost(q);}),"zero-offense Combat was mistaken for Former");GT_OK();
	});
}
}
