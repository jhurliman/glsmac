#include "SmacRestartUnitPackage.h"
#include "game/backend/unit/SmacRestartUnitPackage.h"
#include "fixtures/SmacRestartUnitPackage.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend;
using Package=unit::SmacRestartUnitPackage;
struct Inputs {
    settings::SmacTechnologyRules technologies=smac_rules_fixture::Technologies();
    settings::SmacPredefinedUnitRules units=smac_startup_fixture::Units();
    faction::SmacFactionTechnologyState owner;
    unit::SmacStartupDesignState designs;
    explicit Inputs(const char* faction_id="GAIANS") {
        const auto tech=faction::SmacTechnologyState::NewWorld({{1,faction_id}},technologies,smac_startup_fixture::Factions());
        owner=tech.factions.front();designs=unit::SmacStartupDesignState::NewWorld(tech,technologies,units,3);
        // Exact numeric component inputs used by the captured custom controls.
        units.catalog.weapons.weapons[1].offense=2;units.catalog.weapons.weapons[5].offense=8;units.catalog.weapons.weapons[11].offense=24;
        units.catalog.defenses.defenses[2].rating=3;units.catalog.defenses.defenses[8].rating=12;
    }
    Package Select(int32_t turn=100) const {return Package::Select(turn,designs,units.catalog,owner,technologies);}
    void Add(int32_t id,uint32_t chassis,uint32_t weapon,uint32_t armor,uint8_t reactor=1,uint32_t abilities=0) {
        unit::SmacCustomDesign row;row.id=id;row.chassis=chassis;row.weapon=weapon;row.defense=armor;row.reactor=reactor;row.abilities=abilities;
        designs.owners.front().designs.push_back(row);
    }
};
bool Matches(const Package& result,const char* name) {
    for(const auto& row:restart_package_fixture::cases) {if(std::string(row.name)==name) {
        return result.defender==row.defender && result.balanced.value_or(-1)==row.balanced && result.mobile.value_or(-1)==row.mobile
            && result.creation_order==row.order;
    }}
    throw std::runtime_error("Missing literal restart case");
}
bool Rejects(const std::function<void()>& call) {try{call();}catch(const std::exception&){return true;}return false;}
}

void AddSmacRestartUnitPackageTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC restart selector honors shared intrinsic exclusions",GT(){
        Inputs in;auto& strongest=in.units.catalog.units[11];strongest.chassis_id=0;strongest.weapon_id=5;strongest.defense_id=8;
        strongest.prerequisite={settings::SmacTechnologyReference::Kind::NONE,std::nullopt};
        const auto ordinary=in.Select();GT_ASSERT(ordinary.defender==11,"restart overlay control lost eligible strong defender");
        in.designs.intrinsic_overrides[11]=unit::SmacIntrinsicDesignState{1,2};const auto retained=in.designs;
        GT_ASSERT(in.Select().defender!=11 && in.designs==retained,"restart ignored own intrinsic exclusion or changed state");GT_OK();
    });
    task->AddTest("SMAC restart package uses seven saved startup catalogs without regeneration",GT(){
        const std::array<std::pair<const char*,const char*>,7> names{{
            {"GAIANS","gaians_turn100"},{"HIVE","hive_turn100"},{"UNIVERSITY","univ_turn100"},{"MORGANITES","morgan_turn100"},
            {"SPARTANS","spartans_turn100"},{"BELIEVERS","believe_turn100"},{"PEACEKEEPERS","peace_turn100"}}};
        for(const auto& name:names) {
            Inputs in(name.first);const auto saved=in.designs;const auto tech=in.owner;
            GT_ASSERT(Matches(in.Select(),name.second),"saved startup package differs: "+std::string(name.first));
            GT_ASSERT(in.designs==saved && in.owner==tech,"restart regenerated designs or technologies");
        }
        Inputs in;GT_ASSERT(Matches(in.Select(1),"gaians_turn1"),"turn-one package differs");
        in.owner.owned.fill(false);GT_ASSERT(Matches(in.Select(),"no_owned_technology"),"Former grant was incorrectly filtered by ownership");
        in.owner.owned.fill(true);in.units.catalog.units[8].prerequisite={settings::SmacTechnologyReference::Kind::TECHNOLOGY_ID,20};
        GT_ASSERT(Matches(in.Select(),"all_owned_technology") && in.designs.owners.front().designs.empty(),"technology ownership manufactured advanced designs");
        GT_OK();
    });
    task->AddTest("SMAC restart package reinforcement thresholds are strict and ordered",GT(){
        const Inputs gaia,sparta("SPARTANS");
        for(const auto& row:restart_package_fixture::cases) {
            const std::string name=row.name;
            if(name.rfind("threshold_",0)==0) {GT_ASSERT(Matches(gaia.Select(row.turn),row.name),"strict reinforcement boundary differs: "+name);}
        }
        GT_ASSERT(Matches(sparta.Select(20),"spartans_turn20") && Matches(sparta.Select(21),"spartans_turn21"),"mobile threshold or factory order differs");
        // Mobile, balanced and defender have distinct IDs in the supplied roles
        // control, so equal IDs cannot conceal swapped factory ordering.
        Inputs roles;roles.Add(90,0,0,2);roles.Add(91,0,5,0);roles.Add(92,1,0,0);
        GT_ASSERT(Matches(roles.Select(),"saved_custom_three_roles"),"distinct-role creation order differs");GT_OK();
    });
    task->AddTest("SMAC restart package preserves strict ties eligibility and restart Psi evaluation",GT(){
        Inputs ties;ties.Add(90,0,1,2);ties.Add(91,0,1,2,4,0x10000);
        ties.designs.owners.front().designs[0].plan=12;ties.designs.owners.front().designs[0].prototyped=false;
        ties.designs.owners.front().designs[1].plan=0;ties.designs.owners.front().designs[1].prototyped=true;
        GT_ASSERT(Matches(ties.Select(),"saved_custom_equal_scores"),"reactor/ability/plan/prototype altered scores or later tie won");
        Inputs fast;fast.Add(90,1,11,8,4);GT_ASSERT(Matches(fast.Select(),"saved_fast_negative_score"),"negative mobile score replaced the absent winner");
        Inputs decoys;decoys.Add(90,0,11,8);decoys.Add(91,0,11,8);decoys.Add(92,3,11,8);decoys.Add(93,0,15,8);
        decoys.designs.owners.front().designs[0].active=false;decoys.designs.owners.front().designs[1].faction_exclusion=2;
        unit::SmacCustomDesign foreign;foreign.id=128;foreign.weapon=11;foreign.defense=8;
        decoys.designs.owners.push_back({2,{foreign},2});
        GT_ASSERT(Matches(decoys.Select(),"saved_ineligible_decoys"),"inactive/excluded/foreign/sea/zero-power design competed");
        Inputs psi;psi.Add(90,0,12,0);psi.Add(91,0,0,9);
        GT_ASSERT(Matches(psi.Select(),"mixed_psi_zero_cache"),"negative component was excluded or conventional mixed component became zero");
        // A predefined advanced record must pass its own prerequisite; custom
        // records above deliberately bypass their component prerequisites.
        Inputs predefined;auto& row=predefined.units.catalog.units[11];row.chassis_id=0;row.weapon_id=5;
        row.prerequisite={settings::SmacTechnologyReference::Kind::TECHNOLOGY_ID,20};
        GT_ASSERT(predefined.Select().balanced==2,"unowned predefined technology passed");
        predefined.owner.owned[20]=true;GT_ASSERT(predefined.Select().balanced==11,"enabled owned predefined design did not compete");
        predefined.technologies.technologies[20].prerequisites={"Disable","None"};
        GT_ASSERT(predefined.Select().balanced==2,"disabled owned technology passed");
        predefined.technologies.technologies[20].prerequisites={"None","Disable"};
        GT_ASSERT(predefined.Select().balanced==11,"asymmetric enabled None/Disable definition rejected");
        row.active=false;GT_ASSERT(predefined.Select().balanced==2,"inactive predefined design competed");
        row.active=true;row.faction_exclusion=2;GT_ASSERT(predefined.Select().balanced==2,"excluded predefined design competed");GT_OK();
    });
    task->AddTest("SMAC restart package rejects unsupported turn component and catalog domains",GT(){
        const Inputs original;
        for(const auto turn:{INT32_MIN,0,101,INT32_MAX}) {GT_ASSERT(Rejects([&]{original.Select(turn);}),"restart window widened");}
        for(int scenario=0;scenario<9;++scenario) {
            auto in=original;
            if(scenario==0){in.units.catalog.weapons.weapons[0].offense=INT32_MAX;}
            if(scenario==1){in.units.catalog.defenses.defenses[0].rating=128;}
            if(scenario==2){in.units.catalog.chassis.chassis[0].speed=-1;}
            if(scenario==3){in.units.catalog.units[2].source_id=1;}
            if(scenario==4){in.owner.admission.simulation_id=2;}
            if(scenario==5){in.Add(128,0,0,0);}
            if(scenario==6){in.units.catalog.source.ruleset_id="smax";}
            if(scenario==7){in.units.catalog.units[2].prerequisite={settings::SmacTechnologyReference::Kind::UNKNOWN,{}};}
            if(scenario==8){in.Add(90,9,0,0);}
            GT_ASSERT(Rejects([&]{in.Select();}),"unsupported restart input accepted: "+std::to_string(scenario));
        }GT_OK();
    });
}
}
