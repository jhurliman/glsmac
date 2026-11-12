#include "BaseManager.h"
#include "Base.h"
#include "FacilityDef.h"
#include "SmacFactionDisposition.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "game/backend/SmacMovementHistory.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/SmacGroundEdgeCost.h"
#include "game/backend/unit/SmacSelectedMovement.h"
#include "game/backend/unit/SmacUnitFacing.h"
#include "game/FrontendRequest.h"
#include "gc/Space.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <array>

namespace game::backend::base {

struct BaseManager::PreparedSmacCapture::Data {
	Data(SmacGameplayRandomState rng,faction::SmacHostileState history,faction::SmacEconomyState actor,faction::SmacEconomyState defender)
		:random(rng),history(std::move(history)),actor_economy(actor),defender_economy(defender) {}
	unit::Unit* actor=nullptr;
	Base* base=nullptr;
	map::tile::Tile* source=nullptr;
	slot::Slot* owner=nullptr;
	int32_t old_owner=0,new_owner=0,loot=0;
	size_t population=0;
	bool raze=false;
    bool movement_failed=false;
	SmacGameplayRandomState random;
	faction::SmacHostileState history;
	faction::SmacEconomyState actor_economy,defender_economy;
	std::optional<faction::SmacVictoryHistory> victory;
	std::optional<faction::SmacBaseTotals> totals;
	SmacBaseResourceContext context{};
	Base::facilities_t facilities;
	std::vector<std::pair<Base*,Base::facilities_t>> facility_changes;
	std::optional<ProductionState> production;
	SmacBaseAllocationState allocation{};
	std::optional<SmacBaseUpkeepCache> cache;
	std::optional<map::SmacWorkerKnowledge> knowledge;
    std::vector<uint64_t> disclosed_to_humans;
	std::optional<map::GlsmacTerritoryTopology> topology;
	std::vector<std::pair<unit::Unit*,unit::ConventionalUnitState>> units;
	std::vector<Base*> refresh;
	std::optional<SmacDispositionPlan> disposition;
	std::optional<unit::UnitManager::PreparedFactionUnits> faction_units;
	std::optional<std::array<int32_t,2>> tracked_center;
};
BaseManager::PreparedSmacCapture::PreparedSmacCapture(std::unique_ptr<Data> data):m_data(std::move(data)) {}
BaseManager::PreparedSmacCapture::~PreparedSmacCapture()=default;
BaseManager::PreparedSmacCapture::PreparedSmacCapture(PreparedSmacCapture&&) noexcept=default;

BaseManager::PreparedSmacCapture BaseManager::PrepareSmacCapture(unit::Unit* actor,map::tile::Tile* destination,size_t caller) const {
	m_game->RequireSmacGameplayActive();
	auto* um=m_game->GetUM();
	if(!actor || um->GetUnit(actor->m_id)!=actor || !actor->IsConventional()) {THROW("Capture requires a current conventional unit");}
	m_game->RequireConventionalSightWorld();actor->RequireConventionalReady();
	if(!m_game->GetGCSpace()->IsAccumulating() || m_dispatching_updates) {THROW("Capture requires the owning command outside Base update dispatch");}
	auto* state=m_game->GetState();const auto& simulation=m_game->m_simulation->value;
	const auto* participant=simulation.FindSlot(caller);
	if(state->GetConnection() || state->m_settings.local.game_mode!=settings::LocalSettings::GM_SINGLEPLAYER
		|| state->m_settings.local.network_type!=settings::LocalSettings::NT_NONE || state->m_settings.local.network_role!=settings::LocalSettings::NR_NONE
		|| !participant || !participant->active
        || (!m_game->IsSmacControllerOwner(actor->Conventional().owner_simulation_id)
            && (participant->role!=roster::Role::Single || caller!=m_game->GetSlotNum()))
		|| actor->m_owner!=&state->m_slots->GetSlot(caller)
		|| simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {THROW("Capture currently requires the local human controller");}
	const auto* map=m_game->GetMap();auto* source=actor->GetTile();
	const auto ordinary=[&](const map::tile::Tile* tile) {
		return tile && tile->tiles==map->GetTilesPtr() && tile->coord.x<map->GetWidth() && tile->coord.y<map->GetHeight()
			&& ((tile->coord.x^tile->coord.y)&1)==0 && map->GetTile(tile->coord.x,tile->coord.y)==tile
			&& !tile->is_water_tile && !tile->IsLocked() && tile->rockiness>=map::tile::ROCKINESS_FLAT && tile->rockiness<=map::tile::ROCKINESS_ROCKY
            && !(tile->features&~(map::tile::FEATURE_RIVER|map::tile::FEATURE_XENOFUNGUS))
            && !(tile->terraforming&~(map::tile::TERRAFORMING_ROAD|map::tile::TERRAFORMING_MAG_TUBE|map::tile::TERRAFORMING_FOREST
                |map::tile::TERRAFORMING_FARM|map::tile::TERRAFORMING_MINE|map::tile::TERRAFORMING_SOLAR|map::tile::TERRAFORMING_SENSOR));
	};
	if(!ordinary(source) || !ordinary(destination) || !source->IsAdjactentTo(destination) || source->base || source->units.size()!=1
		|| source->units.begin()->second!=actor || source->units.begin()->first!=actor->m_id || actor->m_animation_id) {THROW("Capture currently requires adjacent clear flat land");}
	if(!destination->base || !destination->units.empty()) {THROW("Capturing a defended Base is not supported yet");}
	const auto registered=std::find_if(m_bases.begin(),m_bases.end(),[&](const auto& row){return row.second==destination->base;});
	if(registered==m_bases.end() || registered->second->m_id!=registered->first) {THROW("Capture target is not a current Base");}
	auto* captured=registered->second;const auto before=captured->GetSmacResourceContext();
	if(captured->m_owner==actor->m_owner || captured->GetPopulation()<1 || before.golden_age || before.drone_riots || before.resource_events) {THROW("This Base requires a different capture rule");}
	m_game->RequireSmacLocalHumanOwner(actor->m_owner);
    const auto& defending=m_game->RequireSmacProductionOwner(captured->m_owner);
    if(defending.role!=roster::Role::Single && defending.role!=roster::Role::Player) {THROW("Capture defender requires a bound local participant");}
	const auto resolved=um->ResolveConventional(actor->Conventional(),actor->m_owner);
	const auto design=m_game->ResolveConventionalDesign(actor->Conventional(),*m_game->m_simulation);
	const auto& catalog=state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	const auto stats=actor->QueryStats(unit::SmacMoraleMode::Mode1);
	if(resolved.package!=unit::ConventionalUnitContext::Package::Combat || (design.chassis!=0 && design.chassis!=1)
        || (design.abilities&~0x4000u) || resolved.reactor!=1 || catalog.weapons.weapons.at(design.weapon).offense<=0 || catalog.defenses.defenses.at(design.defense).rating<=0
        || stats.remaining_allowance<=0 || actor->Conventional().IsHeld() || actor->Conventional().work_order!=unit::ConventionalUnitState::WorkOrder::None
		|| simulation.designs.global_difficulty!=3 || unit::SmacStartupDesignState::OriginalDifficulty(actor->m_owner->GetPlayer()->GetDifficultyLevel())!=3
		|| unit::SmacStartupDesignState::OriginalDifficulty(captured->m_owner->GetPlayer()->GetDifficultyLevel())!=3) {THROW("This capture requires different unit or difficulty rules");}
	const auto old_owner=before.owner_simulation_id,new_owner=actor->Conventional().owner_simulation_id;
	const bool liberation=before.FormerOwner()==new_owner,raze=captured->GetPopulation()==1 && !liberation;
	simulation.hostility.RequireOrdinaryHostility(new_owner,old_owner);
    const auto ordinary_war=[](uint32_t word){return (word&16u)!=0;};
	for(const auto& pair:{std::pair<int32_t,int32_t>{old_owner,new_owner},{new_owner,old_owner}}) {
		const auto& relation=simulation.hostility.owners.at(pair.first).directions.at(pair.second);
		if(relation.secondary || !ordinary_war(relation.primary)) {THROW("Declare Vendetta before capturing this Base");}
	}
    const auto* social=simulation.social.Find(new_owner);
    if(!social || !social->retained) {THROW("Capture movement requires retained PLANET");}
    const auto edge=[&](const map::tile::Tile* tile) {
        uint32_t flags=tile->base ? unit::smac_edge::Base:0;
        if(tile->features&map::tile::FEATURE_RIVER) {flags|=unit::smac_edge::River;}
        if(tile->features&map::tile::FEATURE_XENOFUNGUS) {flags|=unit::smac_edge::Fungus;}
        if(tile->terraforming&map::tile::TERRAFORMING_ROAD) {flags|=unit::smac_edge::Road;}
        if(tile->terraforming&map::tile::TERRAFORMING_MAG_TUBE) {flags|=unit::smac_edge::Tube;}
        if(tile->terraforming&map::tile::TERRAFORMING_FOREST) {flags|=unit::smac_edge::Forest;}
        return unit::smac_edge::Tile{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y),3,
            static_cast<int32_t>(tile->rockiness-map::tile::ROCKINESS_FLAT),flags};
    };
    const auto cost=unit::smac_edge::QueryGroundEdgeCost({{design.id,static_cast<unit::smac_edge::Chassis>(design.chassis),0,
        catalog.weapons.weapons.at(design.weapon).offense,static_cast<int32_t>(catalog.weapons.weapons.at(design.weapon).mode),design.plan,design.abilities,stats.basic_allowance},
        stats.quantum,new_owner,social->retained->at(7),false,false,static_cast<int32_t>(map->GetWidth()),static_cast<int32_t>(map->GetHeight()),true,edge(source),edge(destination)});
    const auto movement=unit::smac_move::ResolveSelectedGroundStep({actor->m_id,actor->Conventional().spent,actor->Conventional().failed_move},
        stats.quantum,stats.full_allowance,cost,bool(destination->features&map::tile::FEATURE_XENOFUNGUS),simulation.gameplay_random,unit::smac_move::LotteryStream::SharedLow16);
    auto moved_state=actor->Conventional();if(moved_state.retained_state_word) {*moved_state.retained_state_word|=4u;}moved_state.spent=static_cast<uint8_t>(movement.unit.spent);moved_state.failed_move=movement.unit.prior_failure_marker;
    if(movement.outcome!=unit::smac_move::Outcome::Moved) {
        if(moved_state.retained_state_word && moved_state.failed_move) {*moved_state.retained_state_word|=0x8000u;}
        auto failed=std::make_unique<PreparedSmacCapture::Data>(movement.random,simulation.hostility,simulation.economy.at(new_owner),simulation.economy.at(old_owner));
        failed->movement_failed=true;failed->actor=actor;failed->source=source;failed->base=captured;failed->units.emplace_back(actor,moved_state);
        return PreparedSmacCapture(std::move(failed));
    }
    moved_state.damage=0;moved_state.facing=unit::SmacFacingForStep(*source,*destination);
    if(moved_state.retained_state_word) {*moved_state.retained_state_word&=~0x8040u;}
	const auto technology=[&](int32_t owner)->const faction::SmacFactionTechnologyState& {
		for(const auto& row:simulation.technology.factions) {if(row.admission.simulation_id==owner) {return row;}}
		THROW("Capture owner has no technology state");
	};
	for(size_t id=0;id<89;++id) {if(technology(old_owner).owned[id] && !technology(new_owner).owned[id]) {THROW("This capture requires a technology choice that is not available yet");}}
	const auto& defender_economy=simulation.economy.at(old_owner);
	if(!defender_economy.population_aggregate || *defender_economy.population_aggregate<0 || *defender_economy.population_aggregate==INT32_MAX
		|| defender_economy.credits<0 || simulation.economy.at(new_owner).credits<0) {THROW("Capture needs the defender's saved population summary and ordinary treasuries");}
	auto data=std::make_unique<PreparedSmacCapture::Data>(movement.random,simulation.hostility,simulation.economy.at(new_owner),defender_economy);
	data->raze=raze;
	if(!raze && data->defender_economy.production_reference_counts) {
		const auto targets=SmacProductionReferenceTargets(captured->GetProductionState());
		if(targets) {data->defender_economy.production_reference_counts=faction::ChangeSmacProductionReferences(*data->defender_economy.production_reference_counts,*targets,{});}
		else {data->defender_economy.production_reference_counts.reset();}
	}
	data->totals=faction::AdjustSmacBaseTotal(simulation.retained_base_totals,old_owner,-1);
	if(!raze) {data->totals=faction::AdjustSmacBaseTotal(data->totals,new_owner,1);}
	data->victory=simulation.victory_history;
	if(data->victory && !raze) {
		if(m_game->GetTurnId()>INT32_MAX) {THROW("Capture history turn exceeds signed32");}
		data->victory->RecordBaseExpansion(new_owner,static_cast<int32_t>(m_game->GetTurnId()));
	}
	data->actor=actor;data->base=captured;data->source=source;data->owner=actor->m_owner;data->old_owner=old_owner;data->new_owner=new_owner;
	data->population=captured->GetPopulation()-(liberation ? 0:1);
	const int64_t numerator=int64_t(defender_economy.credits)*static_cast<int64_t>(captured->GetPopulation());
	if(numerator>INT32_MAX) {THROW("Capture treasury multiplication exceeds the admitted ordinary domain");}
	const int64_t booty=numerator/(*defender_economy.population_aggregate+1);
	if(booty>INT32_MAX || int64_t(data->actor_economy.credits)+booty>INT32_MAX) {THROW("Capture treasury arithmetic exceeds the admitted ordinary domain");}
	data->loot=static_cast<int32_t>(booty);data->actor_economy.credits+=data->loot;
	data->context=before;
	if(!raze) {data->context.owner_simulation_id=new_owner;data->context.conquest=liberation ? SmacConquestState{new_owner,0}:SmacConquestState{old_owner,50};data->context.uncaptured_former_owner.reset();data->context.former_owner_migration=false;
	data->context.recent_defender_loss=false;data->context.SyncKnownStatusBits();}
	data->facilities=captured->m_facilities;
	bool headquarters=false;
	for(const auto& entry:data->facilities) {
		if(!entry.second || GetFacilityDef(entry.first)!=entry.second) {THROW("Capture facility is not registered");}
		ValidateSmacFacilityDef(entry.second);
		if(!entry.second->m_smac_source_id || *entry.second->m_smac_source_id>33) {THROW("Advanced facility capture needs its separate rules");}
		headquarters=headquarters || *entry.second->m_smac_source_id==1;
	}
    if(!simulation.IsHuman(old_owner)) {
        const bool survives=std::any_of(m_bases.begin(),m_bases.end(),[&](const auto& entry) {
            return entry.second!=captured && entry.second->m_owner==captured->m_owner;
        });
        // Paired native controls establish retained HQ capture and failed
        // last-size-one disposition, not paid relocation or liberation.
        const bool ordinary_retained=!raze && headquarters && defender_economy.credits<1000 && survives;
        const bool last_size_one=raze && !survives && simulation.roster.size()==2;
        if((!ordinary_retained && !last_size_one) || before.FormerOwner()!=old_owner
            || (before.conquest && before.conquest->assimilation!=0)) {
            THROW("Computer defender capture requires measured retained HQ or last size-one disposition");
        }
    }
	// Original HQ relocation precedes loot transfer and random facility loss.
	// Booty retains the old treasury calculation even after the relocation fee.
	if(headquarters && defender_economy.credits>=1000) {
		const auto& topology=map->GetTerritoryTopology();
		if(!topology || !defender_economy.regional_base_counts) {THROW("HQ relocation needs retained regional Base counts");}
		const auto region=[&](const Base* base) {const auto* tile=base->GetTile();return topology->Regions().at(size_t(tile->coord.y)*(map->GetWidth()/2)+tile->coord.x/2);};
		Base* selected=nullptr;int64_t best=9999;
		for(const auto& row:m_bases) {
			auto* candidate=row.second;if(candidate==captured || candidate->m_owner!=captured->m_owner) {continue;}
			int64_t distance_sum=1;const auto* center=candidate->GetTile();const auto candidate_region=region(candidate);
			for(const auto& other:m_bases) {
				if(other.second==captured || other.second==candidate || other.second->m_owner!=captured->m_owner) {continue;}
				const auto* tile=other.second->GetTile();auto dx=std::abs(int64_t(center->coord.x)-int64_t(tile->coord.x));
				if(topology->Wraps()) {dx=std::min(dx,int64_t(map->GetWidth())-dx);}
				const auto dy=std::abs(int64_t(center->coord.y)-int64_t(tile->coord.y));
				const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
				distance_sum+=distance*(candidate_region==region(other.second) ? 1 : 2);
			}
			const auto divisor=int64_t(defender_economy.regional_base_counts->at(candidate_region))+2*int64_t(candidate->GetPopulation());
			if(divisor<=0) {THROW("HQ relocation has an invalid population/region denominator");}
			const auto score=256*distance_sum/divisor;
			if(score<best) {best=score;selected=candidate;}
		}
		// The admitted local capture keeps focus on the captor, so the original
		// losing-focus confirmation is not entered. ID zero is not retried.
		if(selected && selected->m_id>0) {
			const auto* facility=GetSmacFacilityDef(1);if(!facility) {THROW("HQ relocation has no registered Headquarters");}
			auto facilities=selected->m_facilities;facilities[facility->m_id]=facility;
			data->facility_changes.emplace_back(selected,std::move(facilities));data->defender_economy.credits-=1000;
		}
	}
	data->defender_economy.credits=static_cast<int32_t>(std::clamp(int64_t(data->defender_economy.credits)-data->loot,INT64_C(0),INT64_C(9999999)));
	if(raze) {data->defender_economy.regional_base_counts=m_game->PrepareCanonicalRegionalBaseDelta(*captured,-1,map->GetTerritoryTopology());}
	if(!raze) {
	const auto remove=[&](int32_t source_id) {for(auto it=data->facilities.begin();it!=data->facilities.end();) {if(it->second->m_smac_source_id==source_id) {it=data->facilities.erase(it);}else{++it;}}};
	// Return to the recorded former faction skips population loss and the
	// entire facility RNG loop; HQ removal and pre-loss loot still apply.
	if(!liberation) {
		for(int32_t id=1;id<=38;++id) {if(id!=26 && data->random.NextCrtRaw15()%3==0) {remove(id);}}
		remove(6);remove(3);
	}
	remove(1);
	const auto* profile=state->m_settings.global.GetSmacFactionStartupRules()->Find(actor->m_owner->GetPlayer()->GetFaction()->m_id);
	if(!profile) {THROW("Capture owner has no faction profile");}
	for(const auto& bonus:profile->Records()) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::FACILITY) {
		const auto source_id=std::clamp(bonus.parameter,1,38);const auto* facility=GetSmacFacilityDef(source_id);
		if(!facility || source_id==1) {THROW("Capture requires a registered ordinary faction facility grant");}data->facilities[facility->m_id]=facility;
	}}
	}
	Base* replacement=nullptr;int64_t nearest=INT64_MAX,best_governor=0;
	for(const auto id:m_base_order) {
		auto* candidate=GetBase(id);if(!candidate) {THROW("Capture Base order is incomplete");}candidate->GetSmacResourceContext();
		if(candidate!=captured && candidate->m_owner==captured->m_owner) {
			const auto* tile=candidate->GetTile();auto dx=std::abs(int64_t(tile->coord.x)-int64_t(destination->coord.x));const auto dy=std::abs(int64_t(tile->coord.y)-int64_t(destination->coord.y));
			dx=std::min(dx,int64_t(map->GetWidth())-dx);const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
			if(distance<=nearest) {nearest=distance;replacement=candidate;}
		}
		if(!raze && candidate!=captured && candidate->m_owner==actor->m_owner) {
			int64_t weight=candidate->GetPopulation();
			for(const auto& entry:candidate->m_facilities) {if(!entry.second || GetFacilityDef(entry.first)!=entry.second) {THROW("Governor donor has an invalid facility");}if(entry.second->m_smac_source_id==1) {weight*=2;}}
			if(weight>best_governor) {best_governor=weight;data->context.governor_options=candidate->GetSmacResourceContext().governor_options;}
		}
	}
	std::optional<unit::UnitManager::PreparedConventionalRemoval> leaving;
	if(!replacement) {
		std::vector<size_t> ids;for(const auto id:um->GetUnitRecordOrder()) {if(um->GetUnit(id)->m_owner==captured->m_owner) {ids.push_back(id);}}
		leaving.emplace(um->PrepareRemovalForQuery(ids));
	}
	auto actor_state=moved_state;data->units.emplace_back(actor,actor_state);
	for(const auto& entry:um->GetUnits()) {
		auto* unit=entry.second;if(unit->IsNative()) {continue;}auto next=unit->Conventional();
		if(replacement && unit->m_owner==captured->m_owner && next.home_base_id==captured->m_id) {
			next.home_base_id=replacement->m_id;next.home=unit::ConventionalUnitState::Home::Base;
			(void)um->ResolveConventional(next,unit->m_owner);data->units.emplace_back(unit,next);
		}
	}
	if(!raze && replacement) {data->refresh.push_back(replacement);}
	if(!raze && actor_state.home_base_id) {data->refresh.push_back(GetBase(*actor_state.home_base_id));}
	if(!raze) {
	const auto target=m_game->SelectSmacFoundingProduction(actor->m_owner,destination);
	data->production=ProductionState{target,0,m_game->MakeSmacProductionItem(actor->m_owner,ProductionItemKind::Stockpile,38),0,std::vector<ProductionItem>{}};
	// Successful ordinary survivor capture preserves the independent paid marker.
	data->production->hurry_partial_paid=captured->m_production->hurry_partial_paid;
	}
	for(const auto& pair:{std::pair<int32_t,int32_t>{old_owner,new_owner},{new_owner,old_owner}}) {data->history.owners.at(pair.first).directions.at(pair.second).primary|=0x8000a118u;}
	auto& counters=data->history.owners.at(new_owner).directions.at(old_owner);const auto encounters=static_cast<uint32_t>(captured->GetPopulation())*2u;counters.paired_a+=encounters;counters.paired_b+=encounters;
	data->knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();
	data->topology=m_game->GetMap()->GetTerritoryTopology();
	if(!data->knowledge) {THROW("Capture needs saved movement sight and territory");}
	// Queries below contain no script callbacks or yields. The owning Space
	// excludes other readers/collection. Install only a private future query
	// context, then restore every field before returning even on a query error.
	// Pop nodes, reverse assignments, IDs, queues and campaign RNG never change.
	auto* previous_owner=captured->m_owner;auto* previous_faction=captured->m_faction;
	std::optional<SmacBaseResourceContext> preview_context=data->context;
	auto preview_facilities=data->facilities;auto preview_production=data->production;auto preview_knowledge=data->knowledge;
	auto preview_topology=data->topology;
	auto preview_actor_economy=data->actor_economy,preview_defender_economy=data->defender_economy;
	auto preview_history=data->history;
	auto preview_victory=data->victory;
	auto preview_totals=data->totals;
	std::map<size_t,unit::Unit*> preview_source,preview_destination{{actor->m_id,actor}};
	auto& query_simulation=m_game->m_simulation->value;
	auto* next_faction=data->owner->GetPlayer()->GetFaction();
	// The owning query excludes readers/GC. Temporarily remove the actual
	// registry node only after deriving removal's site/territory update; sight
	// then observes the real baseless destination instead of invented memory.
	auto& query_manager=*const_cast<BaseManager*>(this);
	decltype(m_bases)::node_type removed_base;
	auto removed_order=raze ? m_base_order:decltype(m_base_order){};
	if(raze) {removed_order.erase(std::remove(removed_order.begin(),removed_order.end(),captured->m_id),removed_order.end());}
	bool preview_active=false;
	util::FinallyGuard restore([&]{
		if(!preview_active) {return;}preview_active=false;
		if(!removed_base.empty()) {query_manager.m_bases.insert(std::move(removed_base));query_manager.m_base_order.swap(removed_order);destination->base=captured;}
		m_game->CommitBaseTerritory(preview_knowledge,preview_topology);
		query_simulation.hostility.owners.swap(preview_history.owners);
		query_simulation.victory_history.swap(preview_victory);
		query_simulation.retained_base_totals.swap(preview_totals);
		std::swap(query_simulation.economy.at(new_owner),preview_actor_economy);std::swap(query_simulation.economy.at(old_owner),preview_defender_economy);
		actor->CommitConventionalMove(actor->Conventional(),source);source->units.swap(preview_source);destination->units.swap(preview_destination);
		for(auto& entry:data->units) {std::swap(std::get<unit::ConventionalUnitState>(entry.first->m_state),entry.second);}
		captured->m_resource_context.swap(preview_context);captured->m_facilities.swap(preview_facilities);captured->m_production.swap(preview_production);
		captured->m_owner=previous_owner;captured->m_faction=previous_faction;
		if(raze) {destination->base=captured;}
	});
	preview_active=true;
	captured->m_owner=raze ? previous_owner:data->owner;captured->m_faction=raze ? previous_faction:next_faction;
	captured->m_resource_context.swap(preview_context);captured->m_facilities.swap(preview_facilities);captured->m_production.swap(preview_production);
	for(auto& entry:data->units) {std::swap(std::get<unit::ConventionalUnitState>(entry.first->m_state),entry.second);}
	actor->CommitConventionalMove(actor->Conventional(),destination);source->units.swap(preview_source);destination->units.swap(preview_destination);
	query_simulation.hostility.owners.swap(preview_history.owners);
	query_simulation.victory_history.swap(preview_victory);
	query_simulation.retained_base_totals.swap(preview_totals);
	std::swap(query_simulation.economy.at(new_owner),preview_actor_economy);std::swap(query_simulation.economy.at(old_owner),preview_defender_economy);
	m_game->CommitBaseTerritory(preview_knowledge);
	if(raze) {
		std::optional<map::GlsmacTerritoryTopology> removal_topology;
		auto removal_knowledge=m_game->PrepareBaseTerritory(nullptr,captured,false,&removal_topology);
		m_game->CommitBaseTerritory(removal_knowledge,removal_topology);
		removed_base=query_manager.m_bases.extract(captured->m_id);query_manager.m_base_order.swap(removed_order);destination->base=nullptr;
	}
	const auto calculate=[&] {
		if(!raze) {data->allocation=captured->CalculateSmacWorkers(*captured->m_allocation,data->context,SmacBaseResourcePhase::Ordinary,data->population,nullptr,&*data->production);
		data->cache=SmacBaseUpkeepCache{captured->CalculateSmacResourceCalculation(SmacBaseResourcePhase::Ordinary,&data->allocation,data->population),m_game->CalculateCanonicalUnitSupport(*captured,data->population,nullptr)};}
		data->knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();data->topology=m_game->GetMap()->GetTerritoryTopology();
	};
	const auto prepare=[&] {
		auto sight=m_game->PrepareConventionalSight(actor->m_id,new_owner,destination,source);
        data->disclosed_to_humans=sight.disclosed_to_humans;
        if(sight.knowledge) {auto view=sight.knowledge->CopyData();if(view.occupancy) {
            // Sight reads the projected post-move registry. Its ordinary move
            // cache predicates expect pre-move membership, so stage capture's
            // source departure and destination arrival explicitly here.
            view.occupancy->RemoveUnit({static_cast<int32_t>(source->coord.x),static_cast<int32_t>(source->coord.y)},source->units.empty());
            view.occupancy->PlaceUnit({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},new_owner,false);
            view.occupancy->SetAffiliation({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},new_owner);
            sight.knowledge=map::SmacWorkerKnowledge::Create(view);
        }}
        m_game->CommitBaseTerritory(sight.knowledge);
		std::optional<map::GlsmacTerritoryTopology> topology;
		auto territory=m_game->PrepareBaseTerritory(nullptr,nullptr,false,&topology);m_game->CommitBaseTerritory(territory,topology);
		if(leaving) {
			data->disposition=PrepareSmacFactionDisposition(previous_owner,nullptr,new_owner,data->random);
			auto& plan=*data->disposition;
            if(!query_simulation.IsHuman(old_owner) && plan.site) {THROW("Computer capture restart placement needs its own control");}
			data->faction_units.emplace(um->PrepareFactionUnitsForQuery(previous_owner,plan.site,plan.units,*plan.territory,nullptr,&*leaving));
			data->faction_units->Refresh(actor);
			query_simulation.economy.at(old_owner)=plan.economy;
			if(query_simulation.retained_base_totals) {query_simulation.retained_base_totals->at(old_owner)=0;}
			if(query_simulation.victory_history && plan.initialization_history_turn) {
				query_simulation.victory_history->last_base_expansion_or_advice_turn[old_owner]=*plan.initialization_history_turn;
			}
			auto history=plan.hostility;query_simulation.hostility.owners.swap(history.owners);
			auto search_topology=plan.topology;m_game->CommitBaseTerritoryTopology(search_topology);
			data->faction_units->Query([&] {
				// This is the local, non-network capture caller continuation,
				// after successful placement/factories, not part of Search itself.
				if(plan.site && m_game->GetSlotNum()==data->owner->GetIndex() && !state->GetConnection()) {
					const auto bound=(plan.hostility.owners.at(new_owner).directions.at(old_owner).primary&0x1000u) ? 2 : 4;
					if(plan.random.NextCrtRaw15()%bound==0) {
						const unit::Unit* selected=nullptr;int32_t nearest=9999;
						// Native selector scans IDs ascending and replaces on ties.
						for(const auto& row:um->GetUnits()) {if(row.second->m_owner==previous_owner) {
							const auto* tile=row.second->GetTile();const auto dx=std::min(int32_t(tile->coord.x),int32_t(map->GetWidth()-tile->coord.x));
							const auto dy=int32_t(tile->coord.y);const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
							if(distance<=nearest) {nearest=distance;selected=row.second;}
						}}
						if(selected) {
							auto knowledge=map->GetSmacWorkerKnowledge()->CopyData();const auto bit=uint8_t(1u<<new_owner);
							auto& remembered=knowledge.remembered[new_owner];remembered.resize(knowledge.tiles.size());
							const auto* center=selected->GetTile();
							constexpr std::array<std::array<int32_t,2>,9> offsets{{{{0,0}},{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
							for(const auto offset:offsets) {
								auto x=int32_t(center->coord.x)+offset[0];const auto y=int32_t(center->coord.y)+offset[1];
								if(x<0) {x+=knowledge.width;}else if(x>=knowledge.width) {x-=knowledge.width;}
								if(x<0 || x>=knowledge.width || y<0 || y>=knowledge.height) {continue;}
								const auto* tile=map->GetTile(x,y);const auto index=size_t(y)*(knowledge.width/2)+x/2;
								knowledge.tiles[index].explored_by|=bit;remembered[index]=map::SmacWorkerKnowledge::Improvements{tile->features,tile->terraforming,tile->base!=nullptr,!tile->units.empty()};
								for(auto& observation:knowledge.units) {if(tile->units.count(observation.id)) {observation.visible_to|=bit;}}
								if(tile->base) {
									auto found=std::find_if(knowledge.bases.begin(),knowledge.bases.end(),[&](const auto& value){return value.id==tile->base->m_id;});
									if(found==knowledge.bases.end()) {knowledge.bases.push_back({tile->base->m_id,0,{}});found=std::prev(knowledge.bases.end());}
									found->known_by|=bit;found->population[new_owner]=static_cast<uint8_t>(tile->base==captured ? 1 : tile->base->GetPopulation());
								}
								data->tracked_center=std::array<int32_t,2>{{x,y}};
							}
                            // Read the producer's actual source links; the tracking
                            // continuation must not fabricate a separate stack order.
                            std::map<int16_t,const unit::Unit*> bound;const unit::Unit* head=nullptr;
                            for(const auto& row:center->units) {
                                const auto* member=row.second;const auto binding=member->SourceStack();
                                if(member->m_owner!=previous_owner || !binding || !bound.emplace(binding->id,member).second) {THROW("Restart tracking requires unique owned source rows");}
                                if(binding->previous<0) {if(head) {THROW("Restart tracking found multiple heads");}head=member;}
                            }
                            std::vector<size_t> stack;int16_t previous=-1;
                            while(head) {
                                const auto binding=*head->SourceStack();
                                if(binding.previous!=previous || stack.size()>=bound.size()) {THROW("Restart tracking found invalid reciprocal links");}
                                stack.push_back(head->m_id);previous=binding.id;
                                head=binding.next<0 ? nullptr : bound.at(binding.next);
                            }
                            if(stack.size()!=plan.units.size() || stack.size()!=bound.size()) {THROW("Restart tracking requires its complete linked package");}
							bool pod=false;for(const auto id:stack) {
								auto& observation=*std::find_if(knowledge.units.begin(),knowledge.units.end(),[&](const auto& value){return value.id==id;});observation.visible_to=0;
								const auto* unit=um->GetUnit(id);
								if(!pod && um->ResolveConventional(unit->Conventional(),unit->m_owner).package==unit::ConventionalUnitContext::Package::Colony) {observation.visible_to=bit;pod=true;}
							}
							auto prepared=std::optional<map::SmacWorkerKnowledge>(map::SmacWorkerKnowledge::Create(knowledge));m_game->CommitBaseTerritory(prepared);
						}
					}
				}
				calculate();
			});
		} else {calculate();}
	};
	if(leaving) {leaving->Query(prepare);}else{prepare();}
	// Restore while Data is still owned locally; the guard must not reference a
	// moved-from unique_ptr when return-value construction transfers the plan.
	restore.f_after();
	return PreparedSmacCapture(std::move(data));
}

void BaseManager::ApplySmacCapture(GSE_CALLABLE,PreparedSmacCapture prepared) {
	m_game->CheckRW(GSE_CALL);auto& data=*prepared.m_data;
	if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Capture needs its owning accumulation");}
    if(data.movement_failed) {
        auto* um=m_game->GetUM();um->QueueUnitUpdate(data.actor,unit::UnitManager::UUO_REFRESH);
        data.actor->CommitConventionalMove(data.units.at(0).second,data.source);m_game->m_simulation->value.gameplay_random=data.random;return;
    }
	const auto& names=m_game->GetSmacBasePresentations();Base::SmacAllocationRoles roles{names[0],{names[1],names[2],names[3],names[4],names[5],names[6],names[7]}};
	auto facilities=data.facility_changes;facilities.emplace_back(data.base,data.facilities);
	std::optional<Base::PreparedSmacWorkerState> workers;
	if(!data.raze) {workers.emplace(data.base->PrepareSmacWorkerState(GSE_CALL,data.allocation,roles,data.context,data.production,data.base->m_accumulated_nutrients,data.population,std::move(facilities),data.owner,data.refresh));}
	auto bases=data.raze ? m_bases:decltype(m_bases){};auto order=data.raze ? m_base_order:decltype(m_base_order){};
	auto assignments=data.raze ? m_work_assignments:decltype(m_work_assignments){};auto base_updates=data.raze ? m_base_updates:decltype(m_base_updates){};
	std::unique_lock<std::mutex> removal_lock;std::unordered_set<Base*> triggers;
	if(data.raze) {
		bases.erase(data.base->m_id);order.erase(std::remove(order.begin(),order.end(),data.base->m_id),order.end());
		for(auto it=assignments.begin();it!=assignments.end();) {if(it->second.first==data.base->m_id) {it=assignments.erase(it);}else{++it;}}
		QueueBaseUpdateInto(base_updates,data.base,BUO_DESPAWN);
		for(const auto& entry:data.facility_changes) {QueueBaseUpdateInto(base_updates,entry.first,BUO_REFRESH);}
		removal_lock=std::unique_lock(m_updated_bases_mutex);triggers=m_updated_bases;triggers.erase(data.base);
	}
	auto* um=m_game->GetUM();auto updates=um->m_unit_updates;
	for(const auto& entry:data.units) {
		auto& pending=updates.try_emplace(entry.first->m_id,unit::UnitManager::unit_update_t{unit::UnitManager::UUO_NONE,entry.first}).first->second;
		if(pending.unit!=entry.first || (pending.ops&unit::UnitManager::UUO_DESPAWN)) {THROW("Capture unit has a stale pending update");}
		pending.ops=static_cast<unit::UnitManager::unit_update_op_t>(pending.ops|unit::UnitManager::UUO_REFRESH);
	}
	std::map<size_t,unit::Unit*> destination{{data.actor->m_id,data.actor}},source;
	const auto placement=um->PreparePlacementHistory(*data.actor,data.base->GetTile());
	const auto message=data.base->m_name+(data.raze ? " destroyed (":" captured (")+std::to_string(data.loot)+" energy credits).";
	for(const auto& entry:data.units) {entry.first->CommitConventionalMove(entry.second,entry.first==data.actor ? data.base->GetTile() : entry.first->GetTile());}
	data.source->units.swap(source);data.base->GetTile()->units.swap(destination);
	um->CommitPlacementHistory(placement);
	// Surviving capture detaches the old queue without crediting the recipient.
	// Publish before a possible faction reinitialization, whose clear wins.
	m_game->m_simulation->value.economy.at(data.old_owner).production_reference_counts=data.defender_economy.production_reference_counts;
	if(data.faction_units) {data.faction_units->Commit();}else{um->m_unit_updates.swap(updates);}
	m_game->CommitBaseTerritory(data.knowledge,data.topology);
    um->CommitUnitDisclosures(data.disclosed_to_humans);
	auto& simulation=m_game->m_simulation->value;
	simulation.hostility.owners.swap(data.history.owners);simulation.gameplay_random=data.random;
	data.actor_economy.design_counts=simulation.economy.at(data.new_owner).design_counts;
	data.defender_economy.design_counts=simulation.economy.at(data.old_owner).design_counts;
	data.actor_economy.production_reference_counts=simulation.economy.at(data.new_owner).production_reference_counts;
	data.defender_economy.production_reference_counts=simulation.economy.at(data.old_owner).production_reference_counts;
	simulation.economy.at(data.new_owner)=data.actor_economy;simulation.economy.at(data.old_owner)=data.defender_economy;
	simulation.victory_history=data.victory;
	simulation.retained_base_totals=data.totals;
	for(size_t owner=1;owner<8;++owner) {auto& target=simulation.battle_plan_target[owner];if(target && *target==static_cast<int64_t>(data.base->m_id)) {target=-1;}}
	if(data.disposition) {CommitSmacFactionDisposition(*data.disposition);}
	std::unique_ptr<Base> retired;
	if(data.raze) {
		m_bases.swap(bases);m_base_order.swap(order);m_work_assignments.swap(assignments);m_base_updates.swap(base_updates);m_updated_bases.swap(triggers);
		data.base->GetTile()->base=nullptr;
		for(auto& entry:data.facility_changes) {entry.first->m_facilities.swap(entry.second);}
		retired.reset(data.base);removal_lock.unlock();
	} else {workers->Commit();data.base->m_smac_upkeep_cache.swap(data.cache);data.base->m_commerce_ranking_energy=static_cast<int32_t>(data.base->m_smac_upkeep_cache->calculation.raw_intake[2]);if(data.base->m_smac_upkeep_cache->calculation.native_target_resources) {data.base->m_native_target_resources=data.base->m_smac_upkeep_cache->calculation.native_target_resources;}workers->Finish();}
	if(data.raze) {m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(&data) {{"base",data.base->Wrap(GSE_CALL)},};});}
	if(data.faction_units) {data.faction_units->Notify(GSE_CALL);}
	if(data.tracked_center) {
		auto request=game::FrontendRequest(game::FrontendRequest::FR_MAP_CENTER);request.data.map_center.x=(*data.tracked_center)[0];request.data.map_center.y=(*data.tracked_center)[1];
		m_game->AddFrontendRequest(request);m_game->Message("The restarted faction has been tracked.");
	}
	m_game->Message(message);
    if(simulation.HasEnding()) {m_game->GetState()->TriggerObject(m_game,"smac_ending",ARGS_F(this) { }; });}
}

void BaseManager::ValidateSmacNativeRaid(unit::Unit* actor,map::tile::Tile* destination) const {
    m_game->RequireSmacGameplayActive();
    const auto* map=m_game->GetMap();
    if(!m_game->IsRunning() || !m_game->IsSmacControllerOwner(0) || !actor || m_game->GetUM()->GetUnit(actor->m_id)!=actor
        || !actor->IsNative() || actor->Native().owner || actor->Native().design!=8 || actor->m_animation_id || !destination
        || destination->tiles!=map->GetTilesPtr() || destination->coord.x>=map->GetWidth() || destination->coord.y>=map->GetHeight()
        || map->GetTile(destination->coord.x,destination->coord.y)!=destination || destination->is_water_tile || destination->IsLocked()
        || !actor->GetTile()->IsAdjactentTo(destination) || !destination->base || !destination->units.empty() || m_dispatching_updates) {
        THROW("Native raid requires its scheduled adjacent wild Worm and undefended land Base");
    }
    auto* base=destination->base;
    if(GetBase(base->m_id)!=base || base->GetTile()!=destination || base->GetPopulation()<1 || !base->GetSmacAllocation() || !base->GetProductionState()) {
        THROW("Native raid requires a current canonical Base and retained population/production");
    }
    const auto& simulation=m_game->m_simulation->value;
    const auto context=base->GetSmacResourceContext();
    if(!context.retained_status_word || !simulation.ecology_history || simulation.ecology_history->clean_minerals.at(context.owner_simulation_id)<0
        || !m_game->GetState()->m_settings.global.GetSmacCampaignRules() || m_game->GetUM()->QueryNativeUnit(actor->Native(),0).remaining_allowance<=0) {
        THROW("Native raid requires retained protection, clean-mineral history and remaining movement");
    }
}

void BaseManager::RaidSmacBase(GSE_CALLABLE,unit::Unit* actor,map::tile::Tile* destination) {
    m_game->CheckRW(GSE_CALL);ValidateSmacNativeRaid(actor,destination);
    if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Native raid requires its owning accumulation");}
    auto* base=destination->base;auto* um=m_game->GetUM();auto* source=actor->GetTile();auto* map=m_game->GetMap();
    auto& simulation=m_game->m_simulation->value;const auto context=base->GetSmacResourceContext();const auto owner=context.owner_simulation_id;
    auto* victim_owner=base->m_owner;const auto& rules=*m_game->GetState()->m_settings.global.GetSmacCampaignRules();
    const auto stats=um->QueryNativeUnit(actor->Native(),0);
    const auto cost=(destination->features&map::tile::FEATURE_XENOFUNGUS) ? 1:stats.quantum*(1+int(destination->rockiness==map::tile::ROCKINESS_ROCKY)
        +int(bool(destination->terraforming&map::tile::TERRAFORMING_FOREST)));
    auto random=simulation.gameplay_random;
    const auto movement=unit::smac_move::ResolveSelectedGroundStep({actor->m_id,actor->Native().movement_spent,bool(actor->Native().state&0x8000u)},
        stats.quantum,stats.full_allowance,cost,bool(destination->features&map::tile::FEATURE_XENOFUNGUS),random,unit::smac_move::LotteryStream::SharedLow16);
    random=movement.random;
    if(movement.outcome!=unit::smac_move::Outcome::Moved) {
        auto state=actor->Native();state.state|=4u;state.movement_spent=static_cast<uint8_t>(movement.unit.spent);if(movement.unit.prior_failure_marker) {state.state|=0x8000u;}
        um->QueueUnitUpdate(actor,unit::UnitManager::UUO_REFRESH);std::get<unit::SmacNativeUnitRecord>(actor->m_state)=state;simulation.gameplay_random=random;return;
    }
    // The admitted stock native definition has no FACILITY bonus records.
    // This exclusion belongs to faction0, never the attacked owner's grants.
    auto facilities=base->m_facilities;bool damaged_facility=false;
    for(int attempt=0;attempt<=100;++attempt) {
        const int32_t id=random.NextCrtRaw15()%39;if(id<=1 || id>=34 || (id==26 && destination->is_water_tile)) {continue;}
        const auto found=std::find_if(facilities.begin(),facilities.end(),[&](const auto& row){return row.second && row.second->m_smac_source_id==id;});
        if(found==facilities.end()) {continue;}
        ValidateSmacFacilityDef(found->second);facilities.erase(found);damaged_facility=true;break;
    }
    auto production=*base->GetProductionState();if(!damaged_facility) {production.minerals=0;}
    const auto quota=simulation.ecology_history->clean_minerals.at(owner);
    std::vector<size_t> casualties{actor->m_id};int32_t retained=0;
    static constexpr int offsets[25][2]={{0,0},{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2},
        {2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},{-1,3},{-3,1},{-3,-1},{-1,-3},{4,0},{-4,0},{0,4},{0,-4}};
    for(const auto& offset:offsets) {
        const auto y=int64_t(destination->coord.y)+offset[1];if(y<0 || y>=int64_t(map->GetHeight())) {continue;}
        const auto x=(int64_t(destination->coord.x)+offset[0]+map->GetWidth())%map->GetWidth();auto* tile=map->GetTile(x,y);
        unit::Unit* head=nullptr;
        for(const auto& row:tile->units) {if(row.second->SourceStack() && row.second->SourceStack()->previous<0) {head=row.second;break;}}
        size_t visited=0;
        while(head) {
            if(++visited>um->GetUnits().size()) {THROW("Native raid sweep encountered a cyclic source stack");}
            const auto binding=head->SourceStack();if(!binding) {THROW("Native raid sweep requires retained source stack links");}
            if(head!=actor && head->SimulationOwner()==0) {if(retained<quota) {++retained;}else {casualties.push_back(head->m_id);}}
            head=binding->next>=0 ? um->GetUnit(um->GetUnitRecordOrder().at(binding->next)):nullptr;
        }
    }
    bool objective=(rules.primary&0x20000000u) || (*context.retained_status_word&0x8000u);
    if(rules.primary&0x40000000u) {THROW("Native raid objective-Base list requires its retained authority");}
    if(rules.secondary&0x04000000u) {
        for(const auto& row:facilities) {objective|=row.second->m_smac_source_id==rules.objective_facility && rules.objective_facility<34;}
    }
    auto population=base->GetPopulation();if(m_game->GetTurnId()>=10) {population=population ? population-1:0;if(objective) {population=std::max<size_t>(1,population);}}
    const bool destroyed=population==0;
    Base* replacement=nullptr;int64_t nearest=INT64_MAX;
    if(destroyed) {
        for(const auto id:m_base_order) {
            auto* candidate=GetBase(id);if(candidate==base || candidate->m_owner!=victim_owner) {continue;}
            auto dx=std::abs(int64_t(candidate->GetTile()->coord.x)-int64_t(destination->coord.x));dx=std::min(dx,int64_t(map->GetWidth())-dx);
            const auto dy=std::abs(int64_t(candidate->GetTile()->coord.y)-int64_t(destination->coord.y));const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
            if(distance<=nearest) {nearest=distance;replacement=candidate;}
        }
        if(!replacement) {for(auto it=um->GetUnitRecordOrder().rbegin();it!=um->GetUnitRecordOrder().rend();++it) {
            if(um->GetUnit(*it)->m_owner==victim_owner) {casualties.push_back(*it);}
        }}
    }
    // The source sweep invokes the carrier-loss wrapper. Assigned sea cargo
    // cannot survive merely because it was an earlier H-quota survivor.
    for(size_t i=0;i<casualties.size();++i) {
        auto* lost=um->GetUnit(casualties[i]);if(!lost->IsNative() || lost->Native().design!=9 || !lost->GetTile()->is_water_tile) {continue;}
        for(const auto& row:lost->GetTile()->units) {
            auto* cargo=row.second;
            if(cargo->IsNative() && cargo->Native().order==1 && cargo->Native().waypoint_x[0]==lost->Native().id
                && std::find(casualties.begin(),casualties.end(),cargo->m_id)==casualties.end()) {casualties.push_back(cargo->m_id);}
        }
    }
    auto removal=um->PrepareWorldUnitRemoval(GSE_CALL,casualties);
    if(destroyed && replacement) {removal.PrepareBaseRemovalHomes(*base,*replacement);}
    const auto previous_query=m_game->m_smac_riot_query_owner;m_game->m_smac_riot_query_owner=owner;
    util::FinallyGuard restore_query([&]{m_game->m_smac_riot_query_owner=previous_query;});
    std::optional<Base::PreparedSmacWorkerState> workers;std::optional<SmacBaseUpkeepCache> cache;
    std::optional<SmacDispositionPlan> disposition;std::optional<unit::UnitManager::PreparedFactionUnits> faction_units;
    std::optional<map::SmacWorkerKnowledge> knowledge;std::optional<map::GlsmacTerritoryTopology> topology;
    const auto& names=m_game->GetSmacBasePresentations();Base::SmacAllocationRoles roles{names[0],{names[1],names[2],names[3],names[4],names[5],names[6],names[7]}};
    if(!destroyed) {
        auto future_facilities=facilities;SmacBaseAllocationState allocation;
        base->m_facilities.swap(future_facilities);util::FinallyGuard restore_facilities([&]{base->m_facilities.swap(future_facilities);});
        um->QueryDefectionUnits(nullptr,removal,[&]{
            allocation=base->CalculateSmacWorkers(*base->m_allocation,context,SmacBaseResourcePhase::Ordinary,population,nullptr,&production);
            cache=SmacBaseUpkeepCache{base->CalculateSmacResourceCalculation(SmacBaseResourcePhase::Ordinary,&allocation,population),m_game->CalculateCanonicalUnitSupport(*base,population,nullptr)};
        });
        restore_facilities.f_after();
        workers.emplace(base->PrepareSmacWorkerState(GSE_CALL,allocation,roles,context,production,base->m_accumulated_nutrients,population,{{base,facilities}}));
    } else {
        um->QueryDefectionUnits(nullptr,removal,[&]{
            knowledge=m_game->PrepareBaseTerritory(nullptr,base,false,&topology);
            if(!replacement) {
                disposition.emplace(PrepareSmacFactionDisposition(victim_owner,base,0,random));
                auto& plan=*disposition;
                faction_units.emplace(um->PrepareFactionUnitsForQuery(victim_owner,plan.site,plan.units,*plan.territory,nullptr,&removal));
                knowledge=plan.territory;topology=plan.topology;random=plan.random;
            }
        });
    }
    auto bases=destroyed ? m_bases:decltype(m_bases){};auto order=destroyed ? m_base_order:decltype(m_base_order){};
    auto assignments=destroyed ? m_work_assignments:decltype(m_work_assignments){};auto updates=destroyed ? m_base_updates:decltype(m_base_updates){};
    std::unique_lock<std::mutex> lock;std::unordered_set<Base*> triggers;
    std::optional<faction::SmacBaseTotals> totals;std::optional<faction::SmacEconomyState::RegionalBaseCounts> regional;
    if(destroyed) {
        totals=faction::AdjustSmacBaseTotal(simulation.retained_base_totals,owner,-1);
        regional=m_game->PrepareCanonicalRegionalBaseDelta(*base,-1,map->GetTerritoryTopology());
        bases.erase(base->m_id);order.erase(std::remove(order.begin(),order.end(),base->m_id),order.end());
        for(auto it=assignments.begin();it!=assignments.end();) {if(it->second.first==base->m_id) {it=assignments.erase(it);}else {++it;}}
        QueueBaseUpdateInto(updates,base,BUO_DESPAWN);lock=std::unique_lock(m_updated_bases_mutex);triggers=m_updated_bases;triggers.erase(base);
    }
    const auto message=base->m_name+(destroyed ? " was destroyed by native life.":" was raided by native life.");
    restore_query.f_after();
    if(faction_units) {faction_units->Commit();}else {removal.Commit();}
    simulation.gameplay_random=random;
    std::unique_ptr<Base> retired;
    if(destroyed) {
        m_bases.swap(bases);m_base_order.swap(order);m_work_assignments.swap(assignments);m_base_updates.swap(updates);m_updated_bases.swap(triggers);destination->base=nullptr;
        m_game->CommitBaseTerritory(knowledge,topology);simulation.economy.at(owner).regional_base_counts=regional;simulation.retained_base_totals=totals;
        for(size_t faction=1;faction<8;++faction) {auto& target=simulation.battle_plan_target[faction];if(target && *target==static_cast<int64_t>(base->m_id)) {target=-1;}}
        if(disposition) {CommitSmacFactionDisposition(*disposition);}
        retired.reset(base);lock.unlock();
    } else {
        workers->Commit();base->m_smac_upkeep_cache.swap(cache);
        base->m_commerce_ranking_energy=static_cast<int32_t>(base->m_smac_upkeep_cache->calculation.raw_intake[2]);
        if(base->m_smac_upkeep_cache->calculation.native_target_resources) {
            base->m_native_target_resources=base->m_smac_upkeep_cache->calculation.native_target_resources;
        }
        workers->Finish();
    }
    if(faction_units) {faction_units->Notify(GSE_CALL);}else {removal.Notify(GSE_CALL);}
    if(destroyed) {m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(base) {{"base",base->Wrap(GSE_CALL)},};});}
    if(m_game->RequireSimulationParticipant(m_game->GetPlayer()).simulation_id==owner) {m_game->Message(message);}
    const bool scheduled=simulation.faction_turn && simulation.faction_turn->Current().phase==faction::SmacFactionTurn::Phase::WildlifeCommand;
    if(simulation.HasEnding() && !scheduled) {m_game->GetState()->TriggerObject(m_game,"smac_ending",ARGS_F(this) { }; });}
}

}
