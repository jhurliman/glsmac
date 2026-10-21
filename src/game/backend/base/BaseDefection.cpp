#include "BaseDefection.h"
#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/State.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <set>
#include <type_traits>

namespace game::backend::base {
Base::SmacProductionResult Base::CompleteSmacRiotDefection(GSE_CALLABLE,SmacProductionResult result,SmacDefectionAttempt& attempt) {
    auto* um=m_game->GetUM();auto* bm=m_game->GetBM();auto& simulation=m_game->m_simulation->value;
    const auto old_owner=attempt.context.owner_simulation_id,new_owner=attempt.recipient;
    const auto participant=std::find_if(simulation.roster.begin(),simulation.roster.end(),[&](const auto& row){return row.simulation_id==new_owner;});
    if(new_owner==old_owner || new_owner<1 || new_owner>7 || participant==simulation.roster.end()) {
        THROW("Riot transfer requires an admitted positive recipient");
    }
    // The original transfers without activating or initializing the recipient.
    // This synchronous calculation permission changes no saved control state.
    if(m_game->m_smac_riot_query_owner) {THROW("Riot recipient calculation cannot nest");}
    util::FinallyGuard recipient_query([&]{m_game->m_smac_riot_query_owner.reset();});
    m_game->m_smac_riot_query_owner=new_owner;
    auto* recipient=&m_game->GetState()->m_slots->GetSlot(participant->slot);m_game->RequireSmacLocalHumanOwner(recipient);
    if(!attempt.context.FormerOwner() || !simulation.retained_base_totals || !simulation.victory_history) {
        THROW("Riot transfer requires retained former faction, Base totals and activity history");
    }
    auto totals=faction::AdjustSmacBaseTotal(faction::AdjustSmacBaseTotal(simulation.retained_base_totals,old_owner,-1),new_owner,1);
    auto victory=simulation.victory_history;victory->RecordBaseExpansion(new_owner,static_cast<int32_t>(m_game->GetTurnId()));
    auto targets=simulation.battle_plan_target;
    for(size_t owner=1;owner<8;++owner) {if(targets[owner]==static_cast<int64_t>(m_id)) {targets[owner]=-1;}}
    auto context=attempt.context;context.owner_simulation_id=new_owner;
    context.conquest=SmacConquestState{*context.FormerOwner(),uint8_t(new_owner==*context.FormerOwner() ? 0:50)};
    context.uncaptured_former_owner.reset();context.golden_age=false;context.drone_riots=false;context.recent_defender_loss=false;
    if(context.retained_status_word) {*context.retained_status_word&=0xffdef7b1u;}context.SyncKnownStatusBits();
    size_t old_bases=0;int64_t best=0;
    const auto facilities_for=[&](Base* base)->const facilities_t& {
        for(const auto& change:attempt.facility_changes) {if(change.first==base) {return change.second;}}return base->m_facilities;
    };
    for(const auto id:bm->GetBaseRecordOrder()) {auto* base=bm->GetBase(id);if(!base || base->m_owner!=m_owner) {continue;}++old_bases;
        if(base==this) {continue;}bool headquarters=false;
        for(const auto& facility:facilities_for(base)) {headquarters|=facility.second->m_smac_source_id==1;}
        const auto score=int64_t(base->GetPopulation())*(headquarters ? 2:1);
        if(score>best) {best=score;context.governor_options=base->GetSmacResourceContext().governor_options;}
    }
    if(old_bases<2) {THROW("Riot transfer needs a surviving old-owner Base; retained totals differ from live membership");}
    auto facilities=facilities_for(this);if(attempt.granted) {facilities[attempt.granted->m_id]=attempt.granted;}
    const auto& profiles=m_game->GetState()->m_settings.global.GetSmacFactionStartupRules();
    for(auto* owner:{m_owner,recipient}) {
        const auto* profile=profiles->Find(owner->GetPlayer()->GetFaction()->m_id);if(!profile) {THROW("Riot facility grants lack a faction profile");}
        for(const auto& bonus:profile->Records()) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::FACILITY) {
            const auto* facility=bm->GetSmacFacilityDef(std::clamp(bonus.parameter,1,38));
            if(!facility) {THROW("Riot facility grant has no registered definition");}facilities[facility->m_id]=facility;
        }}
    }
    auto production=attempt.production;const auto stockpile=m_game->MakeSmacProductionItem(recipient,ProductionItemKind::Stockpile,38);
    production.current_item=stockpile;production.baseline_item=stockpile;production.minerals=0;production.baseline_minerals=0;production.queue=std::vector<ProductionItem>{};
    std::vector<std::pair<size_t,unit::ConventionalUnitState>> changes;
    std::vector<Base*> refresh;
    auto all_removed=attempt.removed;
    const SmacWorkerRadiusMap radius{{static_cast<int32_t>(GetTile()->coord.x),static_cast<int32_t>(GetTile()->coord.y)},
        static_cast<int32_t>(m_game->GetMap()->GetWidth()),static_cast<int32_t>(m_game->GetMap()->GetHeight()),true};
    auto designs=simulation.designs;
    std::set<size_t> converted;
    auto initial=um->PrepareRemovalForQuery(all_removed,attempt.created);
    um->QueryDefectionUnits(attempt.created,initial,[&] {
        // Validate old homes while the Base still has its old owner.
        for(const auto id:um->GetUnitRecordOrder()) {const auto* unit=um->GetUnit(id);if(unit->IsNative()) {continue;}(void)um->ResolveConventional(unit->Conventional(),unit->m_owner);}
        for(int32_t index=0;index<=20;++index) {
            const auto xy=SmacWorkedSquareCollection::Resolve(radius,index);if(!xy) {continue;}
            auto* tile=m_game->GetMap()->GetTile((*xy)[0],(*xy)[1]);
            const auto& order=um->GetUnitRecordOrder();
            for(auto it=order.rbegin();it!=order.rend();++it) {
                const auto* unit=um->GetUnit(*it);if(unit->IsNative() || unit->GetTile()!=tile) {continue;}
                auto next=unit->Conventional();
                if(index<=8 && next.owner_simulation_id==old_owner) {
                    const auto resolved=m_game->ResolveConventionalDesign(next,*m_game->m_simulation);
                    if(next.design_id>64) {
                        const auto requested=designs.RequestRiotPrototype(next.design_id,new_owner,participant->faction_id=="MORGANITES",
                            m_game->GetState()->m_settings.global.GetSmacPredefinedUnitRules()->catalog,simulation.IsHuman(new_owner));
                        if(requested>=0) {next.design_id=requested;}
                    }
                    if(resolved.chassis!=0 && next.work_progress) {THROW("Riot work progress needs the measured chassis auxiliary byte");}
                    if(next.home_base_id && *next.home_base_id!=m_id) {refresh.push_back(bm->GetBase(*next.home_base_id));}
                    next.owner_simulation_id=new_owner;next.home=unit::ConventionalUnitState::Home::Base;next.home_base_id=m_id;
                    next.work_order=unit::ConventionalUnitState::WorkOrder::None;
                    if(next.retained_state_word) {*next.retained_state_word&=0xfcffbdffu;}
                    converted.insert(*it);changes.emplace_back(*it,next);
                } else if(next.owner_simulation_id==new_owner && next.retained_state_word && (*next.retained_state_word&0x40000u)) {
                    *next.retained_state_word&=~0x40000u;changes.emplace_back(*it,next);
                }
            }
        }
        for(const auto id:um->GetUnitRecordOrder()) {const auto* unit=um->GetUnit(id);if(unit->IsNative()) {continue;}
            const auto& state=unit->Conventional();
            if(state.owner_simulation_id==old_owner && state.home_base_id==m_id && !converted.count(id)) {all_removed.push_back(id);}
        }
    });
    auto removal=um->PrepareRemovalForQuery(all_removed,attempt.created);
    std::optional<unit::UnitManager::PreparedDefectionUnits> transfer,accounting;
    std::optional<SmacBaseUpkeepCache> cache;
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::optional<map::GlsmacTerritoryTopology> topology;
    auto allocation=attempt.allocation;
    auto* old_slot=m_owner;auto* old_faction=m_faction;
    std::optional<SmacBaseResourceContext> query_context=context;auto query_facilities=facilities;
    std::optional<ProductionState> query_production=production;
    std::vector<std::pair<Base*,facilities_t>> other_facilities;
    for(const auto& change:attempt.facility_changes) {if(change.first!=this) {other_facilities.push_back(change);}}
    auto query_totals=totals;auto query_victory=victory;auto query_designs=designs;
    // Ordinary staged queue changes followed by transfer's whole-queue debit
    // cancel to a debit of the original queue. The recipient gets no credit.
    auto references=simulation.economy.at(old_owner).production_reference_counts;
    if(references) {
        const auto targets=SmacProductionReferenceTargets(m_production);
        if(targets) {references=faction::ChangeSmacProductionReferences(*references,*targets,{});}
        else {references.reset();}
    }
    auto query_references=references;
    auto previous_topology=m_game->GetMap()->GetTerritoryTopology();
    bool active=false;
    util::FinallyGuard restore([&] {
        if(!active) {return;}
        m_game->CommitBaseTerritoryTopology(previous_topology);
        simulation.economy.at(old_owner).production_reference_counts.swap(query_references);
        simulation.retained_base_totals.swap(query_totals);simulation.victory_history.swap(query_victory);std::swap(simulation.designs,query_designs);
        m_resource_context.swap(query_context);m_facilities.swap(query_facilities);m_production.swap(query_production);
        for(auto& change:other_facilities) {change.first->m_facilities.swap(change.second);}
        m_owner=old_slot;m_faction=old_faction;
    });
    um->QueryDefectionUnits(attempt.created,removal,[&] {
        active=true;m_owner=recipient;m_faction=recipient->GetPlayer()->GetFaction();
        simulation.economy.at(old_owner).production_reference_counts.swap(query_references);
        m_resource_context.swap(query_context);m_facilities.swap(query_facilities);m_production.swap(query_production);
        for(auto& change:other_facilities) {change.first->m_facilities.swap(change.second);}
        simulation.retained_base_totals.swap(query_totals);simulation.victory_history.swap(query_victory);std::swap(simulation.designs,query_designs);
        transfer.emplace(um->PrepareDefectionUnitsForQuery(recipient,changes,attempt.created,&removal));
        transfer->Query([&] {
            auto view=m_game->GetMap()->GetSmacWorkerKnowledge()->CopyData();const auto bit=uint8_t(1u<<new_owner);
            for(auto& row:view.units) {if(converted.count(row.id)) {row.visible_to|=bit;}}
            if(view.occupancy) {
                for(const auto id:converted) {const auto* tile=um->GetUnit(id)->GetTile();view.occupancy->SetAffiliation({static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},new_owner);}
                const auto* tile=GetTile();view.occupancy->SetAffiliation({static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},new_owner);
            }
            const auto old_bit=uint8_t(1u<<old_owner);
            auto observed=std::find_if(view.bases.begin(),view.bases.end(),[&](const auto& row){return row.id==m_id;});
            if(observed==view.bases.end()) {view.bases.push_back({m_id,uint8_t(bit|old_bit),{}});observed=view.bases.end()-1;}
            observed->known_by|=uint8_t(bit|old_bit);observed->population[old_owner]=static_cast<uint8_t>(attempt.population);observed->population[new_owner]=static_cast<uint8_t>(attempt.population);

            for(int32_t index=0;index<=20;++index) {const auto xy=SmacWorkedSquareCollection::Resolve(radius,index);if(!xy) {continue;}
                const auto offset=size_t((*xy)[1])*(view.width/2)+(*xy)[0]/2;view.tiles.at(offset).explored_by|=bit;
            }
            auto radius_knowledge=std::optional<map::SmacWorkerKnowledge>(map::SmacWorkerKnowledge::Create(view));m_game->CommitBaseTerritory(radius_knowledge);
            auto territory=m_game->PrepareBaseTerritory(nullptr,nullptr,false,&topology);m_game->CommitBaseTerritory(territory,topology);
            const auto support=m_game->CalculateCanonicalUnitSupport(*this,attempt.population,nullptr);
            allocation=CalculateSmacWorkers(allocation,context,SmacBaseResourcePhase::Upkeep,attempt.population,nullptr,&production,support.mineral_support);
            cache=SmacBaseUpkeepCache{CalculateSmacResourceCalculation(SmacBaseResourcePhase::Upkeep,&allocation,attempt.population,attempt.food,nullptr,support.mineral_support),support};
            if(!cache->calculation.citizens) {THROW("Riot recipient final citizen pass is unavailable");}
            const auto markers=m_game->CalculateCanonicalUnitAccounting(*this,support);
            accounting.emplace(um->PrepareDefectionUnitsForQuery(recipient,markers,attempt.created,&removal));
            knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();topology=m_game->GetMap()->GetTerritoryTopology();
        });
    });
    // Restore the old authority before preparing its authenticated transition.
    m_game->CommitBaseTerritoryTopology(previous_topology);
    simulation.economy.at(old_owner).production_reference_counts.swap(query_references);
    simulation.retained_base_totals.swap(query_totals);simulation.victory_history.swap(query_victory);std::swap(simulation.designs,query_designs);
    for(auto& change:other_facilities) {change.first->m_facilities.swap(change.second);}
    m_resource_context.swap(query_context);m_facilities.swap(query_facilities);m_production.swap(query_production);m_owner=old_slot;m_faction=old_faction;active=false;
    auto facility_changes=attempt.facility_changes;
    const auto own=std::find_if(facility_changes.begin(),facility_changes.end(),[&](const auto& row){return row.first==this;});
    if(own==facility_changes.end()) {facility_changes.emplace_back(this,std::move(facilities));}else {own->second=std::move(facilities);}
    auto worker=PrepareSmacWorkerState(GSE_CALL,allocation,attempt.roles,context,production,attempt.food,attempt.population,std::move(facility_changes),recipient,std::move(refresh),false,nullptr,true);
    m_game->m_smac_riot_query_owner.reset(); // no privilege survives into publication/callbacks
    attempt.committed=true;
    if(attempt.publication) {*attempt.publication=true;}
    if(attempt.created) {result.completed_unit=attempt.created->Commit()->m_id;}
    removal.Commit();transfer->Commit();accounting->Commit();worker.Commit();m_smac_upkeep_cache.swap(cache);
    simulation.economy.at(old_owner).production_reference_counts.swap(references);
    simulation.retained_base_totals.swap(totals);simulation.victory_history.swap(victory);simulation.battle_plan_target.swap(targets);
    std::swap(simulation.designs,designs);
    simulation.gameplay_random=attempt.random;m_game->CommitBaseTerritory(knowledge,topology);worker.Finish();
    result.removed_units=std::move(all_removed);result.final_calculation=m_smac_upkeep_cache->calculation;result.retained_energy.reset();result.ownership_changed=true;
    m_game->Message(m_name+": joins "+m_faction->m_name+" after drone riots.");
    if(attempt.created && !removal.Removes(*result.completed_unit)) {attempt.created->Notify(GSE_CALL);}removal.Notify(GSE_CALL);
    return result;
}
}
