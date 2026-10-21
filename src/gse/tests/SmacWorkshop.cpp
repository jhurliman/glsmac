#include "SmacWorkshop.h"
#include "game/backend/unit/SmacWorkshop.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "fixtures/SmacStartupSource.h"
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
    task->AddTest("SMAC Workshop failed edits preserve previously applied design",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();auto owner=Owner();
        auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;
        owner.owned[1]=false;const auto empty=state;
        GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);}) && state==empty,"Unavailable armor mutated registry");
        owner.owned[1]=true;unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);const auto applied=state;
        for(unsigned change=0;change<7;++change) {
            auto bad=draft;
            switch(change) {case 0:bad.chassis=1;break;case 1:bad.reactor=2;break;case 2:bad.abilities=1;break;
                case 3:bad.name="Renamed";break;case 4:bad.name=std::string(32,'x');break;case 5:bad.name=std::string("a\0b",3);break;case 6:bad.selected_design=127;break;}
            GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,bad,catalog,tech,owner);}) && state==applied,"Unsupported edit changed applied design");
        }
        auto invalid=state;invalid.owners[0].designs[0].explicitly_named=true;invalid.owners[0].designs[0].name.reset();
        GT_ASSERT(Rejects([&]{invalid.Serialize(catalog);}),"Custom-name flag without a retained name accepted");GT_OK();
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
