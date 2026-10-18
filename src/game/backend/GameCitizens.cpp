#include "Game.h"
#include "Player.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "faction/Faction.h"
#include "map/Map.h"
#include "slot/Slot.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include <algorithm>
#include <climits>
#include <stdexcept>
namespace game::backend {
std::optional<base::SmacCitizenCalculation> Game::CalculateCanonicalCitizens(const base::Base& target,
    const base::SmacBaseResourceCalculation& resources,const base::SmacBaseAllocationState* candidate,
    std::optional<size_t> population,const unit::Unit* prepared,const std::vector<size_t>& removed,
    const base::FacilityDef* granted,std::string* unavailable) const {
    const auto missing=[&](const char* reason)->std::optional<base::SmacCitizenCalculation> {if(unavailable) {*unavailable=reason;}return {};};
    if(unavailable) {unavailable->clear();}
    const auto context=target.GetSmacResourceContext();
    if(target.GetGame()!=this || !m_bm || m_bm->GetBase(target.m_id)!=&target || !m_map) {throw std::runtime_error("Citizens require a registered Base in this world");}
    if(!target.m_owner || target.m_owner->GetPlayer()!=GetPlayer()) {return missing("Citizen details are available for your own Bases");}
    RequireSmacLocalHumanOwner(target.m_owner);ValidateCurrentSimulation();
    const auto& simulation=m_simulation->value;
    if(simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {return missing("Citizen project ownership is unavailable for this campaign");}
    if(!simulation.retained_base_totals) {return missing("This development campaign has no retained faction Base totals for citizen processing");}
    const auto* social=simulation.social.Find(context.owner_simulation_id);
    const auto& profiles=m_state->m_settings.global.GetSmacFactionStartupRules();
    const auto* profile=profiles ? profiles->Find(target.m_faction->m_id):nullptr;
    if(!social || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed || !profile) {return missing("Citizens require the frozen faction and current social ratings");}
    const auto count=population.value_or(target.m_pops.size());
    if(count>127 || m_map->GetWidth()>INT32_MAX || m_map->GetHeight()>INT32_MAX) {throw std::runtime_error("Citizen population or map geometry exceeds its measured domain");}
    const auto allocation=candidate ? std::optional<base::SmacBaseAllocationState>(*candidate):target.GetSmacAllocation();
    if(!allocation) {return missing("Citizen specialist allocation is unavailable");}
    allocation->Validate();
    const auto p=static_cast<int32_t>(count),x=allocation->specialists.total_count;
    if(x<0 || x>p || x>16) {throw std::runtime_error("Citizen excluded count differs from its specialist slots");}
    int32_t drone=0,talent=0;bool seen_drone=false,seen_talent=false;
    for(const auto& record:profile->bonuses) {
        if(record.kind==settings::SmacFactionStartupBonus::Kind::DRONE) {
            if(seen_drone) {return missing("Repeated custom faction DRONE scalar needs its original loader authority");}seen_drone=true;drone=record.parameter;
        }
        if(record.kind==settings::SmacFactionStartupBonus::Kind::TALENT) {
            if(seen_talent) {return missing("Repeated custom faction TALENT scalar needs its original loader authority");}seen_talent=true;talent=record.parameter;
        }
    }
    const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(target.m_owner->GetPlayer()->GetDifficultyLevel());
    base::SmacCitizenCalculation result{};result.population=p;result.excluded=x;result.social_talent=social->current.at(3);result.police=social->current.at(5);
    result.prefix=base::CalculateSmacCitizenPrefix(p,true,difficulty,drone,talent,context.conquest ? context.conquest->assimilation:0);
    if(result.prefix.after_faction<0 || result.prefix.faction_talents<0) {return missing("Negative custom faction citizen conversion is not established");}
    const auto& order=m_bm->GetBaseRecordOrder();const auto position=std::find(order.begin(),order.end(),target.m_id);
    if(position==order.end() || order.size()>INT32_MAX) {throw std::runtime_error("Citizen Base has no global record position");}
    const auto total=simulation.retained_base_totals->at(context.owner_simulation_id);
    if(total<0) {return missing("Citizen bureaucracy requires a nonnegative retained faction Base total");}
    result.bureaucracy=base::CalculateSmacCitizenBureaucracy(p,true,difficulty,social->current.at(1),static_cast<int32_t>(m_map->GetWidth()),static_cast<int32_t>(m_map->GetHeight()),total,static_cast<int32_t>(position-order.begin())).drones;
    // This energy calculation validates every registered local facility and the
    // no-project admission. Candidate slots and a staged local grant are read-only.
    const auto energy=CalculateCanonicalBaseEnergy(target,resources,nullptr,&*allocation,granted);
    result.psych_points=energy.tail.final.psych;
    std::array<bool,34> local{};
    for(const auto& row:target.m_facilities) {local.at(*row.second->m_smac_source_id)=true;}
    if(granted) {local.at(*granted->m_smac_source_id)=true;}
    if(local[22]) {return missing("Genejack citizen effects require the owned configured drone rule");}
    const auto facilities=base::CalculateSmacCitizenFacilityInputs(p,local,0,false,false,false,false);
    result.facility_drones=facilities.drone_adjustment;result.paradise_talents=facilities.paradise_talents;
    result.early=base::ConvertSmacCitizens({p,x,result.prefix.after_faction,result.bureaucracy,result.prefix.faction_talents,result.social_talent,result.psych_points,result.facility_drones,result.paradise_talents});
    std::optional<map::SmacWorkerKnowledge::Data> terrain;
    if(result.police<-2) {
        const auto& knowledge=m_map->GetSmacWorkerKnowledge();
        if(!knowledge) {return missing("Citizen pacifism requires current territory authority");}
        terrain=knowledge->CopyData();
        if(terrain->width!=static_cast<int32_t>(m_map->GetWidth()) || terrain->height!=static_cast<int32_t>(m_map->GetHeight()) || terrain->tiles.size()!=size_t(terrain->width/2)*terrain->height) {throw std::runtime_error("Citizen territory geometry differs from the live map");}
    }
    m_um->RequireConventionalUnitsReady();std::vector<base::SmacCitizenPoliceUnit> garrison;int32_t pacifism=0;
    const auto append=[&](const unit::Unit* u,bool staged) {
        if(!u || !u->IsConventional()) {throw std::runtime_error("Citizens require authenticated conventional units");}
        const auto& state=u->Conventional();
        (void)ResolveConventionalUnit(state,u->m_owner);const auto design=ResolveConventionalDesign(state,*m_simulation);
        const auto* tile=staged ? target.GetTile():u->GetTile();
        if(staged) {
            if(u->GetTile() || m_um->GetUnit(u->m_id) || u->m_owner!=target.m_owner || state.home_base_id!=target.m_id) {throw std::runtime_error("Citizen staged unit identity differs");}
        } else if(!tile || tile->tiles!=m_map->GetTilesPtr() || !tile->units.count(u->m_id) || tile->units.at(u->m_id)!=u) {throw std::runtime_error("Citizen unit map membership differs");}
        if(std::find(removed.begin(),removed.end(),u->m_id)!=removed.end()) {return;}
        // The admitted resolver rejects all special abilities and nonland triads.
        // No owner/home filter belongs to the original garrison police scan.
        if(tile==target.GetTile()) {garrison.push_back({design.plan,false});}
        if(result.police>=-2) {return;} // Pacifism contributes zero in this domain.
        if(state.owner_simulation_id!=context.owner_simulation_id || state.home_base_id!=target.m_id || design.plan>9) {return;}
        const auto& catalog=m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
        if(catalog.weapons.weapons.at(design.weapon).offense==0) {return;}
        bool protected_tile=false;
        if(tile->base) {
            if(m_bm->GetBase(tile->base->m_id)!=tile->base || tile->base->GetTile()!=tile) {throw std::runtime_error("Citizen pacifism Base marker differs");}
            const auto owner=tile->base->GetSmacResourceContext().owner_simulation_id;protected_tile=owner>=0 && owner<=7;
        }
        if(!protected_tile) {
            const auto index=tile->coord.y*size_t(terrain->width/2)+tile->coord.x/2;
            protected_tile=terrain->tiles.at(index).claimant==context.owner_simulation_id;
        }
        if(!protected_tile) {if(pacifism==INT32_MAX) {throw std::overflow_error("Citizen pacifism count exceeds signed32");}++pacifism;}
    };
    for(const auto id:m_um->GetUnitRecordOrder()) {const auto* u=m_um->GetUnit(id);if(!u || u->m_id!=id) {throw std::runtime_error("Citizen Unit record order differs");}append(u,false);}
    if(prepared) {append(prepared,true);}
    result.after_police=base::PoliceSmacCitizens(result.early.facilities,p,x,result.police,pacifism,false,garrison).normalized;
    // The admitted campaign has no projects and no nerve-stapling command or
    // stored counter writer. Sphere presence is an actual local facility input.
    result.final=base::FinishSmacCitizens(result.after_police,p,x,facilities.project_talents,false,0,local[23],0).final;
    return result;
}
}
