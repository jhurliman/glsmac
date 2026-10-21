#include "SmacWorkshop.h"
#include "game/backend/unit/SmacWorkshop.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacPrototypePrice.h"
#include "fixtures/WorkshopAlias.h"
#include "fixtures/WorkshopGeneral.h"
#include "fixtures/WorkshopSavedCommits.h"
#include "fixtures/WorkshopAllocation.h"
#include "fixtures/SmacIntrinsicOverlays.h"
#include "task/gsetests/GSETests.h"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>

namespace gse::tests {
namespace {
using namespace game::backend;
using Kind=unit::SmacWorkshopComponent;
bool Rejects(const std::function<void()>& fn) {try {fn();}catch(const std::exception&) {return true;}return false;}
faction::SmacFactionTechnologyState Owner() {faction::SmacFactionTechnologyState owner;owner.admission={1,"GAIANS"};owner.owned[1]=true;return owner;}
unit::SmacStartupDesignState Empty() {unit::SmacStartupDesignState state;state.global_difficulty=3;state.owners={{1,{},2},{2,{},2}};return state;}
}
void AddSmacWorkshopTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC Workshop retained common commits match seven complete native saved pools",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Workshop rules");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        auto owner=Owner();owner.owned.fill(true);faction::SmacDesignCounts counts{};
        auto pool=workshop_general_fixture::BasePool();pool[127]={};pool[128]={70,111,114,101,105,103,110,32,71,117,97,114,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,2,2,2,0,4,255,0,3,0,0,0};
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto original=state;
        for(const auto& test:workshop_commit_fixture::CASES) {
            const auto result=unit::PrepareSmacWorkshopCommit(state,test.draft,catalog,tech,owner,counts,counts);
            GT_ASSERT(result.success && !result.confirmation && !result.retirement && result.result.design_id==test.selected && state==original,"Native slot decision or detached authority changed");
            uint64_t hash=UINT64_C(14695981039346656037);for(const auto& row:*result.state.native_registry) {for(const auto byte:row) {hash=(hash^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(hash==test.digest,"Native saved pool differs: "+std::string(test.name)+" hash="+std::to_string(hash));
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(result.state.Serialize(catalog).ToString(),catalog)==result.state,"Prepared general commit lost state on restore");
        }
        // Full own bank: the cached live count, not queue references alone, gates retirement.
        for(size_t id=64;id<128;++id) {pool[id]=pool[128];}state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.RetainNativeRegistry(pool,catalog);
        unit::SmacWorkshopDraft draft;draft.selected_design=64;draft.armor=2;counts[64]=1;const auto full=state;
        const auto pending=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
        GT_ASSERT(!pending.success && pending.confirmation && !pending.retirement && pending.state==full && state==full,"Pending overwrite changed saved records/history");
        const auto accepted=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts,true);
        unit::SmacRetirementTuple old{};std::copy_n(pool[64].begin()+32,8,old.begin());
        GT_ASSERT(accepted.success && accepted.retirement==64 && accepted.state.retirement_history[1]->cursor==1 && accepted.state.retirement_history[1]->slots[0]==old
            && accepted.state.native_registry->at(128)==pool[128] && state==full,"Accepted retirement omitted old tuple or changed foreign/source authority");
        auto prototyped=state;prototyped.owners[0].designs[0].prototyped=true;prototyped.SynchronizeNativeRegistry();
        auto reactor_edit=draft;reactor_edit.armor=1;reactor_edit.reactor=2;
        const auto replacement=unit::PrepareSmacWorkshopCommit(prototyped,reactor_edit,catalog,tech,owner,counts,counts,true);
        GT_ASSERT(replacement.retirement==64 && !replacement.state.owners[0].designs[0].prototyped,
            "Retired source remained active during the new prototype-coverage scan");
        counts[64]=0;faction::SmacDesignCounts queued{};queued[64]=9;
        const auto silent=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,queued);
        GT_ASSERT(silent.success && !silent.confirmation && !silent.retirement && silent.state.retirement_history==state.retirement_history,"Queue-only silent edit incorrectly retired/history-recorded a design");
        // Matching a custom tuple copies only the source-controlled name and NUL.
        auto rename_pool=pool;rename_pool[65][38]=2;state=Empty();state.RetainNativeRegistry(rename_pool,catalog);draft.name="X";owner.owned.fill(false); // existing own aliases do not require component technologies
        const auto alias=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,queued);
        auto expected=rename_pool[65];expected[0]='X';expected[1]=0;
        GT_ASSERT(alias.success && alias.result.design_id==65 && alias.state.native_registry->at(65)==expected && alias.state.native_registry->at(64)==rename_pool[64],"Short alias rename erased its native name tail or source");
        // Fresh prototype construction has its own full-name clear policy.
        auto riot_pool=workshop_general_fixture::BasePool();riot_pool[127]={};riot_pool[128]=pool[128];riot_pool[128][48]=7;
        riot_pool[64].fill(0xa5);riot_pool[64][48]=0;riot_pool[64][49]=0;
        state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.RetainNativeRegistry(riot_pool,catalog);
        GT_ASSERT(state.RequestRiotPrototype(128,1,false,catalog,true)==64,"Fresh retained riot prototype selected another slot");
        std::array<uint8_t,32> cleared{};std::copy_n(riot_pool[128].begin(),13,cleared.begin());
        GT_ASSERT(std::equal(cleared.begin(),cleared.end(),state.native_registry->at(64).begin()) && state.native_registry->at(64)[47]==0xa5
            && state.native_registry->at(64)[50]==0xa5 && state.native_registry->at(64)[51]==0xa5,"Fresh constructor did not clear name or lost destination opaque/prerequisite bytes");GT_OK();
    });
    task->AddTest("SMAC Workshop intrinsic reactivation matches four native shared-record controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned alpha for intrinsic controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& test:intrinsic_overlay_fixture::CASES) {
            auto state=Empty();for(const auto& row:test.custom) {state.owners.at(row.id/64-1).designs.push_back(row);}
            for(size_t i=0;i<64;++i) {state.intrinsic_overrides[i]=test.before[i];}
            auto expected=state;for(size_t i=0;i<64;++i) {expected.intrinsic_overrides[i]=test.after[i];}
            auto owner=Owner();owner.admission.simulation_id=test.owner;unit::SmacWorkshopDraft draft;draft.selected_design=test.selected;
            const auto result=unit::ReuseSmacWorkshopDesign(state,draft,catalog,tech,owner);
            GT_ASSERT(result && result->design_id==2 && !result->intrinsic_name_warning && state==expected,"native global intrinsic overlay or unrelated bank changed");
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(state.Serialize(catalog).ToString(),catalog)==state,"intrinsic flags/exclusions lost on restore");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop picker keeps unknown cards and source identities",GT(){
        auto catalog=smac_startup_fixture::Units().catalog;auto tech=smac_rules_fixture::Technologies();auto owner=Owner();
        for(auto& row:catalog.chassis.chassis) {row.prerequisite="Tech5";}catalog.chassis.chassis[0].prerequisite="None";
        auto choices=unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);
        GT_ASSERT(choices.size()==9 && choices[0].technology_available && !choices[1].technology_available,"Unknown-tech cards were filtered out");
        catalog.chassis.chassis[1].prerequisite="Disable";
        choices=unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);
        GT_ASSERT(choices.size()==8 && choices[1].component_id==2,"Visible ordinal replaced component identity");
        catalog.reactors.reactors[1].prerequisite="Disable";catalog.reactors.reactors[2].prerequisite="Tech1";
        choices=unit::SmacWorkshopChoices(Kind::Reactor,catalog,tech,owner);
        GT_ASSERT(choices.size()==2 && choices[0].component_id==1 && choices[1].component_id==3 && choices[1].technology_available,"Reactor mapping lost its one-based identity");
        tech.technologies[1].prerequisites={"Disable","None"};
        GT_ASSERT(!unit::SmacWorkshopChoices(Kind::Reactor,catalog,tech,owner)[1].technology_available,"Raw ownership bypassed effective technology query");
        catalog.chassis.chassis[2].prerequisite="Unknown";
        GT_ASSERT(Rejects([&]{unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);}),"Unresolved source silently admitted");
        owner.admission.simulation_id=32;
        GT_ASSERT(Rejects([&]{unit::SmacWorkshopChoices(Kind::Armor,catalog,tech,owner);}),"Invalid owner reached bit shift");GT_OK();
    });
    task->AddTest("SMAC Workshop first commit reuses Scout or retains named Synthmetal in last slot",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();const auto untouched=state;unit::SmacWorkshopDraft draft;draft.name="Different intrinsic name";
        const auto reuse=unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
        GT_ASSERT(reuse.design_id==2 && reuse.intrinsic_name_warning && state==untouched,"Renaming an intrinsic tuple allocated a duplicate");
        for(const std::string name:{"","Test Guard"}) {
            state=Empty();draft.armor=1;draft.name=name;
            const auto result=unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
            const auto& design=state.Find(1)->designs.at(0);
            // Original 441e20 controls: ids 2/127, raw costs 1/2, plan 2,
            // flags 1/3 for automatic/supplied names. Production cost is separate.
            GT_ASSERT(result.design_id==127 && !result.intrinsic_name_warning && design.id==127 && design.cost==2 && design.plan==2
                && design.chassis==0 && design.weapon==0 && design.defense==1 && design.reactor==1 && !design.abilities
                && design.active && !design.prototyped && !design.faction_exclusion && !design.carry && !design.riot
                && design.name && design.explicitly_named==!name.empty() && state.Find(2)->designs.empty() && state.Find(1)->starting_scout==2,
                "Native first-path identity, cached fields or unrelated bank changed");
            const auto applied=state;
            GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==127 && state==applied,"Repeated Apply allocated another copy");
            const auto wire=state.Serialize(catalog).ToString();
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(wire,catalog)==state,"Committed name/flag lost on save");
            for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire.substr(0,n),catalog);}),"Truncated named design accepted");}
            GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire+"x",catalog);}),"Trailing named design accepted");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop reuses unnamed startup tuples and preserves supplied-name alias rules",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();unit::SmacCustomDesign existing;existing.id=64;existing.defense=1;existing.cost=2;existing.plan=2;existing.prototyped=true;
        state.owners[0].designs.push_back(existing);const auto before=state;
        unit::SmacWorkshopDraft draft;draft.armor=1;draft.name="Different direct draft";
        GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==64 && state==before,"Stored-name requirement duplicated a startup tuple or direct draft renamed it");
        const auto label=unit::SmacWorkshopDesignName(existing,catalog,1);
        GT_ASSERT(!label.empty() && state==before,"Read-only legacy name query mutated registry");
        state.owners[0].designs[0].name="Retained Guard";state.owners[0].designs[0].explicitly_named=true;draft.selected_design=64;
        GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==64
            && state.owners[0].designs[0].name==std::optional<std::string>("Different direct draft") && state.owners[0].designs[0].prototyped,"Named same-tuple alias rebuilt prototype metadata");
        const auto named=state;draft.selected_design=191;
        GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);}) && state==named,"Foreign selected design changed registry");GT_OK();
    });
    task->AddTest("SMAC Workshop V4 retains auxiliary prototype flags without changing V1 V2 V3 bytes",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
        const auto legacy=state.Serialize(catalog).ToString();state.owners[0].designs[0].auxiliary_flags=0xa508;
        const auto wire=state.Serialize(catalog).ToString();const auto restored=unit::SmacStartupDesignState::Deserialize(wire,catalog);
        GT_ASSERT(restored==state && wire!=legacy,"Auxiliary flags were lost or changed known prototype fields");
        auto clear=restored;clear.owners[0].designs[0].auxiliary_flags=0;
        GT_ASSERT(clear.Serialize(catalog).ToString()==legacy,"V4-only metadata changed earlier codec bytes");
        for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire.substr(0,n),catalog);}),"Truncated V4 accepted");}
        state.owners[0].designs[0].auxiliary_flags=1;
        GT_ASSERT(Rejects([&]{state.Serialize(catalog);}),"Auxiliary flags duplicated active/name/prototype authority");GT_OK();
    });
    task->AddTest("SMAC Workshop failed edits preserve previously applied design",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();auto owner=Owner();
        auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;
        owner.owned[1]=false;const auto empty=state;
        GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);}) && state==empty,"Unavailable armor mutated registry");
        owner.owned[1]=true;unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);const auto applied=state;
        for(unsigned change=0;change<7;++change) {
            auto bad=draft;
            switch(change) {case 0:bad.chassis=1;break;case 1:bad.reactor=2;break;case 2:bad.abilities=1;break;
                case 3:bad.armor=2;break;case 4:bad.name=std::string(32,'x');break;case 5:bad.name=std::string("a\0b",3);break;case 6:bad.selected_design=191;break;}
            GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,bad,catalog,tech,owner);}) && state==applied,"Unsupported edit changed applied design");
        }
        auto invalid=state;invalid.owners[0].designs[0].explicitly_named=true;invalid.owners[0].designs[0].name.reset();
        GT_ASSERT(Rejects([&]{invalid.Serialize(catalog);}),"Custom-name flag without a retained name accepted");GT_OK();
    });
    task->AddTest("SMAC Workshop free prototype uses presence beyond generic record budget",GT(){
        settings::SmacFactionStartupRule faction;
        const auto technologies=smac_rules_fixture::Technologies();
        GT_ASSERT(!faction.FreePrototype(),"Absent keyword granted prototype exemption");
        for(int n=0;n<8;++n) {faction.bonuses.push_back(settings::SmacFactionStartupBonus::Parse("FACILITY","17",technologies));}
        for(const std::string value:{"0","-4","1","not a number"}) {
            faction.bonuses.push_back(settings::SmacFactionStartupBonus::Parse("FREEPROTO",value,technologies));
            GT_ASSERT(faction.FreePrototype() && faction.Records().size()==8,"FREEPROTO used numeric value or consumed a generic slot");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop prototype price guards legacy metadata and ability-free basis",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;
        unit::SmacCustomDesign design;design.id=127;design.defense=1;design.cost=2;
        unit::SmacPrototypePriceContext context{3,false,false,std::array<int32_t,3>{{50,50,50}}};
        const auto ordinary=unit::SmacDesignProductionRows(design,catalog,context);
        GT_ASSERT(ordinary.rows==3 && ordinary.surcharge_present,"First Synthmetal prototype lost surcharge");
        design.abilities=0xffffffffu;design.cost=20;
        GT_ASSERT(unit::SmacDesignProductionRows(design,catalog,context).rows==21,"Surcharge used ability-inflated stored cost");
        context.percentages.reset();GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Old profile invented percentages");
        context.free_prototype=true;GT_ASSERT(unit::SmacDesignProductionRows(design,catalog,context).rows==20,"Free prototype used absent percentages");
        context.free_prototype=false;design.prototyped=true;
        GT_ASSERT(!unit::SmacDesignProductionRows(design,catalog,context).surcharge_present,"Completed prototype required new metadata");
        design.prototyped=false;context.percentages=std::array<int32_t,3>{{-1,50,50}};
        GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Negative percentage admitted");
        context.percentages=std::array<int32_t,3>{{INT32_MAX,50,50}};
        GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Overflowed native signed32 product admitted");GT_OK();
    });
    task->AddTest("SMAC Workshop owned allocation scans and overwrite prompts match original decisions",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned allocation rules");}
        const auto technologies=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(std::string(std::istreambuf_iterator<char>(input),{}));const auto owner=Owner();
        for(const auto& row:workshop_allocation_fixture::Cases()) {
            auto pool=workshop_allocation_fixture::BasePool();for(const auto& patch:row.patches) {pool.at(patch.first/52).at(patch.first%52)=patch.second;}
            faction::SmacDesignCounts units{},production{};for(const auto& item:row.units) {units.at(item.first)=item.second;}for(const auto& item:row.production) {production.at(item.first)=item.second;}
            const auto before=pool;const auto result=unit::ChooseSmacWorkshopSlot(pool,row.draft,units,production,technologies,owner);
            GT_ASSERT(result.selected==row.selected && result.kind==row.kind && result.confirmation==row.confirmation && pool==before,"Original slot selection differs: " + std::string(row.name));
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned general builder compatibility and cleanup match native controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned Workshop rules");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto technologies=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,technologies).catalog;
        std::ifstream labels_input(std::string(assets)+"/labels.txt",std::ios::binary);if(!labels_input) {throw std::runtime_error("Missing owned Workshop labels");}
        catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(std::string(std::istreambuf_iterator<char>(labels_input),{}));auto owner=Owner();owner.owned.fill(true);
        for(const auto& row:workshop_general_fixture::Builders()) {
            auto pool=workshop_general_fixture::BasePool();for(const auto& patch:row.patches) {pool.at(patch.first/52).at(patch.first%52)=patch.second;}
            unit::BuildSmacWorkshopRecord(pool,127,row.draft,catalog,technologies,owner);
            uint64_t digest=UINT64_C(14695981039346656037);for(const auto& record:pool) {for(const auto byte:record) {digest=(digest^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(unit::NameSmacWorkshopDesign(pool,127,row.draft,catalog,1)==row.native_name,"Original automatic name differs: " + std::string(row.name));
            GT_ASSERT(digest==row.digest,"Complete original design pool differs: " + std::string(row.name));
        }
        for(const auto& row:workshop_general_fixture::COMPATIBILITY) {
            unit::SmacWorkshopDraft draft;draft.chassis=row.chassis;draft.weapon=row.weapon;draft.armor=row.armor;
            GT_ASSERT(unit::SmacWorkshopAbilityCompatible(row.ability,draft,catalog)==row.compatible,"Original ability compatibility differs");
        }
        for(const auto& row:workshop_general_fixture::Cleanups()) {
            const auto actual=unit::CleanSmacWorkshopAbilities(row.draft,{22,23},row.second,catalog);
            GT_ASSERT(actual.mask==row.mask && actual.slots==row.slots,"Original ability cleanup differs: " + std::string(row.name));
        }
        auto pool=workshop_general_fixture::BasePool();const auto before=pool;unit::SmacWorkshopDraft invalid;invalid.weapon=999;
        GT_ASSERT(Rejects([&]{unit::BuildSmacWorkshopRecord(pool,127,invalid,catalog,technologies,owner);}) && pool==before,"Invalid builder input changed pool");
        GT_OK();
    });
    task->AddTest("SMAC Workshop owned original 927 prototype price receipts",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for prototype controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        size_t ordinal=0;
        for(const auto& row:smac_prototype_price_fixture::CASES) {
            unit::SmacCustomDesign design;design.id=row.id;design.chassis=row.chassis;design.weapon=row.weapon;
            design.defense=row.armor;design.reactor=row.reactor;design.cost=row.cost;design.prototyped=row.prototyped;
            const auto result=unit::SmacDesignProductionRows(design,catalog,{row.difficulty,row.free,row.skunkworks,row.percentages});
            GT_ASSERT(result.rows==row.rows && result.surcharge_present==row.surcharge,"Original prototype price differs at receipt "+std::to_string(ordinal));++ordinal;
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned original six complete custom alias and name transfers",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for Workshop alias controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        const auto owner=Owner();
        for(const auto& row:workshop_alias_fixture::Cases()) {
            auto state=Empty();state.owners[0].designs=row.before;auto expected=state;expected.owners[0].designs=row.after;
            const auto result=unit::ReuseSmacWorkshopDesign(state,row.draft,catalog,tech,owner);
            GT_ASSERT(result && result->design_id==row.selected && result->intrinsic_name_warning==row.warning && state==expected,"Complete native alias differs for "+std::string(row.id));
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(state.Serialize(catalog).ToString(),catalog)==state,"Native alias metadata lost on save");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned original component and commit controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for Workshop controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;const auto owner=Owner();
        const std::array<size_t,4> sizes{9,20,10,4};size_t kind=0;
        for(const auto picker:{Kind::Chassis,Kind::Weapon,Kind::Armor,Kind::Reactor}) {
            const auto choices=unit::SmacWorkshopChoices(picker,catalog,tech,owner);
            GT_ASSERT(choices.size()==sizes[kind],"Original visible-card count differs");
            for(size_t i=0;i<choices.size();++i) {
                const bool expected=i==0 || (kind==1 && i==14) || (kind==2 && i==1);
                GT_ASSERT(choices[i].technology_available==expected,"Original card admission differs");
            }++kind;
        }
        for(const std::string name:{"","Test Guard"}) {
            auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;draft.name=name;
            GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==127,"Original allocation differs");
            const auto& row=state.Find(1)->designs[0];
            GT_ASSERT(row.cost==2 && row.plan==2 && row.explicitly_named==!name.empty(),"Original Synthmetal record differs");
        }GT_OK();
    });
}
}
