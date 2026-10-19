#include "SmacFactionSummary.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/settings/SmacFactionSummaryLabels.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Int.h"
#include "gse/value/Undefined.h"

namespace gse::tests {
namespace {
using namespace game::backend;
using Labels=settings::SmacFactionSummaryLabels;
std::string Catalog() {
    return "\n#REPUTE\nNoble\nFaithful\nScrupulous\nDependable\nRuthless\nTreacherous\nWicked\nInfamous\n#MIGHT\nUnsurpassed, adjective\nPotent, adjective\nFormidable, adjective\nSufficient, adjective\nWanting, adjective\nFeeble, adjective\nPathetic, adjective\n";
}
std::string Source(const std::string& filename) {return smac_startup_fixture::OrdinarySource(filename)+(filename=="alpha.txt" ? Catalog() : "");}
Value* Query(GSE_CALLABLE,Game* game,const value::function_arguments_t& args={}) {
    return static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("get_smac_faction_summary"))->Run(GSE_CALL,args);
}
bool Rejects(const std::function<void()>& call) {try {call();}catch(const std::exception&) {return true;}return false;}
std::string Text(value::Object* summary,const std::string& key) {
    const auto* field=summary->Get(key);return field->type==VT_STRING ? static_cast<const value::String*>(field)->value : "<absent>";
}
}
void AddSmacFactionSummaryTests(task::gsetests::GSETests* task) {
    task->AddTest("faction summary labels preserve original index clamps and bounded retained catalog",GT(){
        const auto parsed=Labels::Read(Catalog());GT_ASSERT(parsed,"label catalog absent");
        GT_ASSERT(parsed->Integrity(0)=="Noble" && parsed->Integrity(7)=="Infamous" && parsed->Integrity(99)=="Infamous" && parsed->Integrity(UINT32_MAX)=="Noble","integrity signed clamp differs");
        GT_ASSERT(parsed->Might(0)=="Pathetic" && parsed->Might(4)=="Sufficient" && parsed->Might(99)=="Unsurpassed","Might rank index/clamp differs");
        GT_ASSERT(Labels::Deserialize(parsed->Serialize())==*parsed,"retained catalog changed");
        GT_ASSERT(!Labels::Read("#OTHER\nignored\n"),"missing catalog fabricated");
        GT_ASSERT(Rejects([&]{Labels::Read(Catalog()+Catalog());}) && Rejects([&]{Labels::Read("#REPUTE\nNoble\n");}),"partial/duplicate catalog accepted");
        auto oversized=*parsed;oversized.might.back()=std::string(257,'X');GT_ASSERT(Rejects([&]{oversized.Serialize();}),"oversized late label accepted");GT_OK();
    });
    task->AddTest("faction summary settings freeze labels and preserve old-save absence atomically",GT(){
        settings::GlobalSettings settings;settings.SelectRuleset("smac");settings.InitializeHostRules(Source);
        const auto saved=settings.Serialize().ToString();settings::GlobalSettings restored;restored.Deserialize(types::Buffer(saved));
        GT_ASSERT(restored.GetSmacFactionSummaryLabels() && *restored.GetSmacFactionSummaryLabels()==*settings.GetSmacFactionSummaryLabels(),"settings lost frozen labels");
        restored.InitializeHostRules([](const std::string&)->std::string {throw std::runtime_error("restore reread installed source");});
        types::Buffer prefix(saved);(void)prefix.ReadInt();types::Buffer old_header;old_header.WriteInt(20);
        types::Buffer tail;tail.WriteBool(true);tail.WriteString(settings.GetSmacFactionSummaryLabels()->Serialize().ToString());
        const auto old=old_header.ToString()+saved.substr(prefix.lenr,saved.size()-prefix.lenr-tail.lenw);
        settings::GlobalSettings migrated;migrated.Deserialize(types::Buffer(old));GT_ASSERT(!migrated.GetSmacFactionSummaryLabels(),"V20 acquired current installed labels");
        types::Buffer malformed;malformed.WriteBool(true);malformed.WriteString("bad catalog");
        GT_ASSERT(Rejects([&]{restored.Deserialize(types::Buffer(saved.substr(0,saved.size()-tail.lenw)+malformed.ToString()));}) && restored.Serialize().ToString()==saved,"bad late catalog partially changed settings");GT_OK();
    });
    task->AddTest("faction summary real local query and UI use retained rank integrity votes without mutation",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"PEACEKEEPERS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();fixture.ResourceState([](auto& state) {
                state.hostility.owners.at(1).integrity=3;state.rank_order=faction::SmacRankOrder{};state.rank_order->faction_at_rank[4]=1;
                state.ranking_scores=std::array<int32_t,8>{0,-1,-1,-1,-1,-1,-1,-1};
            });
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            auto* summary=static_cast<value::Object*>(Query(GSE_CALL,game));
            GT_ASSERT(Text(summary,"integrity")=="Dependable" && Text(summary,"might")=="Sufficient" && static_cast<value::Int*>(summary->Get("council_votes"))->value==0,"live summary used score or wrong owner/index");
            auto* init=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_faction_console_identity"));
            auto* check=static_cast<value::Callable*>(init->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
            auto* local=game->GetState()->m_slots->GetSlot(0).GetPlayer()->GetFaction();
            check->Run(GSE_CALL,{VALUE(value::String,,local->m_name),VALUE(value::String,,"Leader unavailable")});
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"read-only summary/UI mutated world or RNG");
            GT_ASSERT(Rejects([&]{Query(GSE_CALL,game,{VALUE(value::Int,,2)});}),"summary accepted foreign owner argument");
            fixture.Restore(GSE_CALL,before);summary=static_cast<value::Object*>(Query(GSE_CALL,game));
            GT_ASSERT(Text(summary,"integrity")=="Dependable" && Text(summary,"might")=="Sufficient","restore replaced retained rating authority");
            fixture.ResourceState([](auto& state){state.roster.front().active=false;});
            summary=static_cast<value::Object*>(Query(GSE_CALL,game));GT_ASSERT(summary->Get("might")->type==VT_UNDEFINED,"inactive inverse rank was invented");GT_OK();
        });
    });
    task->AddTest("faction summary council count uses current Base population and frozen VOTES modifier",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(Source,"PEACEKEEPERS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* bm=game->GetBM();
            bm->DefinePop(new base::PopDef("summary-citizen","Citizen",{}, {},base::PopDef::PF_NONE,0));
            base::Base::pops_t pops;for(size_t i=1;i<=3;++i) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(i),std::forward_as_tuple(nullptr,i,bm->GetPopDef("summary-citizen"),0));}
            auto* owner=&game->GetState()->m_slots->GetSlot(0);
            auto* base=new base::Base(game,701,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(2,2),"Summary",{},pops,4,0,std::nullopt,base::SmacBaseResourceContext{1,false,0,0,0x40});
            bm->SpawnBase(GSE_CALL,base);
            auto* summary=static_cast<value::Object*>(Query(GSE_CALL,game));GT_ASSERT(static_cast<value::Int*>(summary->Get("council_votes"))->value==6,"current population3 plus loaded VOTES2 did not yield6");
            fixture.MovementCapability(faction::SmacSimulationState::MovementCapability::Unavailable);
            summary=static_cast<value::Object*>(Query(GSE_CALL,game));GT_ASSERT(summary->Get("council_votes")->type==VT_UNDEFINED,"missing project authority was treated as no projects");GT_OK();
        });
    });
	 task->AddTest("faction summary unknown presentation catalog remains absent while genuine votes stay available",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* summary=static_cast<value::Object*>(Query(GSE_CALL,fixture.Game()));
            GT_ASSERT(summary->Get("integrity")->type==VT_UNDEFINED && summary->Get("might")->type==VT_UNDEFINED
                && static_cast<value::Int*>(summary->Get("council_votes"))->value==0,"missing labels acquired defaults or erased real empty-Base vote count");GT_OK();
        });
    });
}
}
