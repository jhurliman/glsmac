#include "SmacLiveEconomy.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/map/Map.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/backend/slot/Slots.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
template<typename F> bool Rejects(F&& f) { try { f(); } catch(const std::exception&) { return true; } return false; }
std::string Source(const std::string& filename) {
	const auto original=smac_startup_fixture::OrdinarySource(filename);
	if(filename!="alpha.txt") { return original; }
	std::istringstream input(original);std::string line,section,result;size_t row=0;
	while(std::getline(input,line)) {
		if(!line.empty()&&line[0]=='#') {section=line;row=0;}
		else {
			++row;
			if(section=="#RULES"&&(row==2||row==7||row==21)) {line.replace(0,line.find(','),row==2?"2":"10");}
			// Actual Headquarters adds one energy to this supplied center value.
			if(section=="#RESOURCEINFO") {line=row==2?"Center,2,1,2":"Other,0,0,0";}
			if(section=="#FACILITIES") {
				const auto first=line.find(',');const auto second=line.find(',',first+1);const auto third=line.find(',',second+1);
				line.replace(second+1,third-second-1,row==1?"0":"2");
			}
			if(section=="#CITIZENS"&&row<=7) {line="Role,Roles,None,Disable,1,2,0,opaque";}
		}
		result+=line+'\n';
	}
	return result;
}
Base* Create(GSE_CALLABLE,Game* game) {
	auto* bm=game->GetBM();auto* owner=&game->GetState()->m_slots->GetSlot(0);
	for(const auto* id:{"NUTRIENTS","MINERALS","ENERGY"}) {game->GetRM()->DefineResource(new game::backend::resource::Resource(id,id,{}));}
	bm->DefinePop(new PopDef("energy-role","Not a name identity",{},{},PopDef::PF_NONE,1));
	for(int role:{1,2,4,8,9,27}) {bm->DefineFacility(new FacilityDef("role"+std::to_string(role),"Name", "",0,{},role));}
	auto* tile=game->GetMap()->GetTile(2,2);
	for(auto* corner:tile->elevation.corners){*corner=0;}tile->Update();tile->moisture=map::tile::MOISTURE_ARID;tile->rockiness=map::tile::ROCKINESS_FLAT;
	tile->features=0;tile->terraforming=0;tile->bonus=map::tile::BONUS_NONE;
	const auto item=game->MakeSmacProductionItem(owner,ProductionItemKind::Unit,2);
	const ProductionState production{item,0,item,0,std::vector<ProductionItem>{}};
	const SmacBaseAllocationState allocation{1,{{1},1}};
	auto* base=new Base(game,701,owner,owner->GetPlayer()->GetFaction(),tile,"Energy",{{"role1",bm->GetFacilityDef("role1")}},
		{{1,Pop(nullptr,1,bm->GetPopDef("energy-role"),0)}},2,0,production,
		SmacBaseResourceContext{1,false,0,0,0x00dfbe41u,false},allocation);
	bm->SpawnBase(GSE_CALL,base);return base;
}
}
void AddSmacLiveEconomyTests(task::gsetests::GSETests* task) {
	task->AddTest("SMAC minimap treasury observes in-turn Base publication without posting on read",[](GSE*,context::Context*)->std::string {
		ConventionalUnitFixture fixture;fixture.ConfigureResources(Source,"UNIVERSITY");
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* base=Create(GSE_CALL,game);
			auto* factory=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_restored_minimap"));
			auto* read=static_cast<value::Callable*>(factory->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
			const auto energy=[&]() {
				return static_cast<value::String*>(static_cast<value::Object*>(read->Run(GSE_CALL,{}))->Get("energy"))->value;
			};
			const auto initial=game->GetCanonicalEconomy(base->m_owner).credits;
			GT_ASSERT(energy()=="Energy: "+std::to_string(initial),"actual minimap lost initial treasury");
			fixture.PostEconomy(*base);
			const auto posted=game->GetCanonicalEconomy(base->m_owner).credits;
			GT_ASSERT(posted!=initial,"fixture did not exercise a real in-turn credit change");
			const auto saved=fixture.Snapshot();const auto random=fixture.GameplayRandom();
			game->GetBM()->AddUpdateTrigger(base);game->GetBM()->TriggerUpdates(GSE_CALL);
			GT_ASSERT(energy()=="Energy: "+std::to_string(posted),"actual Base publication left minimap treasury stale until next turn");
			GT_ASSERT(fixture.Snapshot()==saved && fixture.GameplayRandom()==random,"minimap refresh posted economy or changed game state");
			GT_OK();
		});
	});
	task->AddTest("SMAC private economy posting stages research and credits before one native commit",[](GSE*,context::Context*)->std::string {
		ConventionalUnitFixture fixture;fixture.ConfigureResources(Source,"UNIVERSITY");
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* base=Create(GSE_CALL,game);
			// Exercise positive first-turn research above the Citizen/Specialist gate.
			base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
			fixture.ResourceState([](auto& state){auto& economy=state.economy.at(1);economy.credits=100;economy.pending_debit=7;economy.research_fraction=90;
				state.social.factions[0].current[1]=0;state.social.factions[0].current[10]=2;
				auto& tech=state.technology.factions[0];tech.target=5;tech.cached_threshold=1000;tech.labs=40;});
			fixture.PostEconomy(*base);auto state=game->GetCanonicalEconomy(base->m_owner);
			GT_ASSERT(state.credits==102&&state.pending_debit==7&&state.research_fraction==30,"research carry or per-Base economy credit differs");
			bool progress=false;fixture.ResourceState([&](auto& simulation){progress=simulation.technology.factions[0].labs==43;simulation.technology.factions[0].cached_threshold=44;});
			GT_ASSERT(progress,"research was not submitted before the balance commit");const auto before=fixture.Snapshot();
			GT_ASSERT(Rejects([&]{fixture.PostEconomy(*base);})&&fixture.Snapshot()==before,"unconnected completion partly changed progress/fraction/credits");
			fixture.SettleEconomy(1);state=game->GetCanonicalEconomy(base->m_owner);
			GT_ASSERT(state.credits==95&&state.pending_debit==0,"faction debit did not settle after Base credit");
			fixture.ResourceState([](auto& simulation){simulation.economy.at(1).rates={10,0};simulation.technology.factions[0].target.reset();simulation.technology.factions[0].cached_threshold.reset();});
			fixture.PostEconomy(*base);GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).credits==96,"zero research submission incorrectly entered target selection");GT_OK();
		});
	});
	task->AddTest("SMAC live economy initializes seven source profiles and keeps reads detached",[](GSE*,context::Context*)->std::string {
		for(const auto& binding:settings::SmacFactionStartupRules::Bindings()) {
			ConventionalUnitFixture fixture;fixture.ConfigureResources(Source,binding.port_id);
			const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
				auto* game=fixture.Game();auto* owner=&game->GetState()->m_slots->GetSlot(0);
				const auto value=game->GetCanonicalEconomy(owner);
				GT_ASSERT(value.credits==(std::string(binding.port_id)=="MORGANITES"?110:10)&&value.pending_debit==0
					&&value.rates.psych==0&&value.rates.labs==5&&value.treaty_partners==0,"original startup rates/treasury/faction bonus differ");
				auto copy=value;copy.credits=99;GT_ASSERT(game->GetCanonicalEconomy(owner)==value,"query aliases mutable economy");
				auto* wrapped=static_cast<value::Object*>(game->Wrap(GSE_CALL));
				auto* credits=static_cast<value::Callable*>(wrapped->Get("get_smac_credits"));
				const auto before=fixture.Snapshot();const auto rng=fixture.GameplayRandom();
				for(int i=0;i<3;++i) {const auto* result=credits->Run(GSE_CALL,{});
					GT_ASSERT(result->type==VT_INT&&static_cast<const value::Int*>(result)->value==value.credits,"local treasury wrapper lost actual saved credits");}
				GT_ASSERT(Rejects([&]{credits->Run(GSE_CALL,{VALUE(value::Int,,1)});}),"treasury query accepted a caller-selected owner");
				GT_ASSERT(fixture.Snapshot()==before&&fixture.GameplayRandom()==rng,"treasury UI read posted or changed state");GT_OK();
			});if(!error.empty()){return error;}
		}GT_OK();
	});
	task->AddTest("SMAC live Base energy uses saved rates current efficiency exact facilities and restored authority",[](GSE*,context::Context*)->std::string {
		std::string snapshot;SmacBaseEnergyCalculation expected{};
		ConventionalUnitFixture fixture;fixture.ConfigureResources(Source,"UNIVERSITY");
		const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* base=Create(GSE_CALL,game);
			fixture.ResourceState([](auto& state){state.social.factions[0].current[1]=0;state.social.factions[0].retained->at(1)=-4;state.economy.at(1).credits=137;state.economy.at(1).pending_debit=7;state.economy.at(1).research_fraction=90;});
			const auto rng=fixture.GameplayRandom();
			auto result=game->GetCanonicalBaseEnergy(*base);
			GT_ASSERT(result.raw_intake==3&&result.waste==0&&result.tail.final==(SmacEnergyCategories{2,2,2})&&result.maintenance==0,"actual energy/specialist/rate allocation differs");
			base->m_facilities.emplace("role8",game->GetBM()->GetFacilityDef("role8"));result=game->GetCanonicalBaseEnergy(*base);
			GT_ASSERT(result.tail.final.labs==3&&result.maintenance==0,"University Node multiplier or actual free-facility waiver differs");
			base->m_facilities.emplace("role9",game->GetBM()->GetFacilityDef("role9"));result=game->GetCanonicalBaseEnergy(*base);
			GT_ASSERT(result.tail.final.labs==6&&result.maintenance==2,"Bio labs must precede Node multiplier; unrelated maintenance is not free");
			const auto before_display=fixture.Snapshot();
			auto* wrapper=static_cast<value::Object*>(base->Wrap(GSE_CALL));
			const auto display=[&](){return static_cast<value::Object*>(static_cast<value::Callable*>(wrapper->Get("get_smac_energy_calculation"))->Run(GSE_CALL,{}));};
			const auto number=[](value::Object* object,const std::string& key){return static_cast<value::Int*>(object->Get(key))->value;};
			auto* copied=display();
			GT_ASSERT(number(copied,"raw_intake")==3&&number(copied,"waste")==0&&number(copied,"net")==3
				&&number(copied,"credits")==137&&number(copied,"maintenance")==2,"Base UI projection substituted energy or treasury");
			GT_ASSERT(number(static_cast<value::Object*>(copied->Get("rates")),"labs")==5
				&&number(static_cast<value::Object*>(copied->Get("allocated")),"labs")==2
				&&number(static_cast<value::Object*>(copied->Get("final")),"labs")==6,"Base UI projection lost rates, specialists or facility output");
			static_cast<value::Int*>(copied->Get("credits"))->value=-999;
			GT_ASSERT(number(display(),"credits")==137&&fixture.Snapshot()==before_display&&fixture.GameplayRandom()==rng,
				"Base UI read posted economy or returned mutable authority");
			const auto stable=fixture.Snapshot();fixture.ResourceState([](auto& state){state.economy.at(1).treaty_partners=4;});
			GT_ASSERT(Rejects([&]{game->GetCanonicalBaseEnergy(*base);}),"missing commerce producer became zero for a real treaty");
			fixture.ResourceState([](auto& state){state.economy.at(1).treaty_partners=0;});
			GT_ASSERT(fixture.Snapshot()==stable&&fixture.GameplayRandom()==rng,"queries/rejection changed saved state or RNG");
			expected=result;snapshot=fixture.Snapshot();GT_OK();
		});GT_ASSERT(error.empty(),"energy source: "+error);
		ConventionalUnitFixture destination;destination.ConfigureResources(Source,"UNIVERSITY");
		return destination.Run([&](GSE_CALLABLE)->std::string {
			destination.Restore(GSE_CALL,snapshot,true);auto* game=destination.Game();auto* base=game->GetBM()->GetBase(701);
			const auto actual=game->GetCanonicalBaseEnergy(*base);const auto saved=game->GetCanonicalEconomy(base->m_owner);
			auto* wrapped=static_cast<value::Object*>(game->Wrap(GSE_CALL));
			auto* credits=static_cast<value::Callable*>(wrapped->Get("get_smac_credits"));
			GT_ASSERT(static_cast<value::Int*>(credits->Run(GSE_CALL,{}))->value==137,"restored local treasury wrapper replayed defaults");
			GT_ASSERT(actual.tail.final==expected.tail.final&&actual.maintenance==expected.maintenance&&saved.credits==137&&saved.pending_debit==7&&saved.research_fraction==90,"restore replayed defaults or lost final energy inputs");GT_OK();
		});
	});
}
}
