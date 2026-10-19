#include "BaseManager.h"
#include "Base.h"
#include "FacilityDef.h"
#include "SmacFactionDisposition.h"
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
		|| !participant || !participant->active || participant->role!=roster::Role::Single || caller!=m_game->GetSlotNum()
		|| actor->m_owner!=&state->m_slots->GetSlot(caller)
		|| simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {THROW("Capture currently requires the local human controller");}
	const auto* map=m_game->GetMap();auto* source=actor->GetTile();
	const auto ordinary=[&](const map::tile::Tile* tile) {
		return tile && tile->tiles==map->GetTilesPtr() && tile->coord.x<map->GetWidth() && tile->coord.y<map->GetHeight()
			&& ((tile->coord.x^tile->coord.y)&1)==0 && map->GetTile(tile->coord.x,tile->coord.y)==tile
			&& !tile->is_water_tile && !tile->IsLocked() && tile->rockiness==map::tile::ROCKINESS_FLAT
			&& tile->features==map::tile::FEATURE_NONE && tile->terraforming==map::tile::TERRAFORMING_NONE;
	};
	if(!ordinary(source) || !ordinary(destination) || !source->IsAdjactentTo(destination) || source->base || source->units.size()!=1
		|| source->units.begin()->second!=actor || source->units.begin()->first!=actor->m_id || actor->m_animation_id) {THROW("Capture currently requires adjacent clear flat land");}
	if(!destination->base || !destination->units.empty()) {THROW("Capturing a defended Base is not supported yet");}
	const auto registered=std::find_if(m_bases.begin(),m_bases.end(),[&](const auto& row){return row.second==destination->base;});
	if(registered==m_bases.end() || registered->second->m_id!=registered->first) {THROW("Capture target is not a current Base");}
	auto* captured=registered->second;const auto before=captured->GetSmacResourceContext();
	if(captured->m_owner==actor->m_owner || captured->GetPopulation()<2 || before.conquest || before.golden_age || before.drone_riots || before.resource_events) {THROW("This Base requires a different capture rule");}
	m_game->RequireSmacLocalHumanOwner(actor->m_owner);m_game->RequireSmacLocalHumanOwner(captured->m_owner);
	const auto resolved=um->ResolveConventional(actor->Conventional(),actor->m_owner);
	const auto design=m_game->ResolveConventionalDesign(actor->Conventional(),*m_game->m_simulation);
	const auto& catalog=state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	const auto stats=actor->QueryStats(unit::SmacMoraleMode::Mode1);
	if(resolved.package!=unit::ConventionalUnitContext::Package::Combat || resolved.chassis!=settings::SmacMovementRules::GroundChassis::Infantry
		|| design.abilities || resolved.reactor!=1 || catalog.weapons.weapons.at(design.weapon).offense!=1 || catalog.defenses.defenses.at(design.defense).rating!=1
		|| stats.full_allowance!=3 || stats.remaining_allowance!=3 || actor->Conventional().work_order!=unit::ConventionalUnitState::WorkOrder::None
		|| simulation.designs.global_difficulty!=3 || unit::SmacStartupDesignState::OriginalDifficulty(actor->m_owner->GetPlayer()->GetDifficultyLevel())!=3
		|| unit::SmacStartupDesignState::OriginalDifficulty(captured->m_owner->GetPlayer()->GetDifficultyLevel())!=3) {THROW("This capture requires different unit or difficulty rules");}
	const auto old_owner=before.owner_simulation_id,new_owner=actor->Conventional().owner_simulation_id;
	const auto ordinary_war=[](uint32_t word){return word==16 || word==0x80008118u || word==0x8000a118u;};
	for(const auto& pair:{std::pair<int32_t,int32_t>{old_owner,new_owner},{new_owner,old_owner}}) {
		const auto& relation=simulation.hostility.owners.at(pair.first).directions.at(pair.second);
		if(relation.secondary || !ordinary_war(relation.primary)) {THROW("Declare Vendetta before capturing this Base");}
	}
	const auto technology=[&](int32_t owner)->const faction::SmacFactionTechnologyState& {
		for(const auto& row:simulation.technology.factions) {if(row.admission.simulation_id==owner) {return row;}}
		THROW("Capture owner has no technology state");
	};
	for(size_t id=0;id<89;++id) {if(technology(old_owner).owned[id] && !technology(new_owner).owned[id]) {THROW("This capture requires a technology choice that is not available yet");}}
	const auto& defender_economy=simulation.economy.at(old_owner);
	if(!defender_economy.population_aggregate || *defender_economy.population_aggregate<0 || *defender_economy.population_aggregate==INT32_MAX
		|| defender_economy.credits<0 || simulation.economy.at(new_owner).credits<0) {THROW("Capture needs the defender's saved population summary and ordinary treasuries");}
	auto data=std::make_unique<PreparedSmacCapture::Data>(simulation.gameplay_random,simulation.hostility,simulation.economy.at(new_owner),defender_economy);
	data->totals=faction::AdjustSmacBaseTotal(faction::AdjustSmacBaseTotal(simulation.retained_base_totals,old_owner,-1),new_owner,1);
	data->victory=simulation.victory_history;
	if(data->victory) {
		if(m_game->GetTurnId()>INT32_MAX) {THROW("Capture history turn exceeds signed32");}
		data->victory->RecordBaseExpansion(new_owner,static_cast<int32_t>(m_game->GetTurnId()));
	}
	data->actor=actor;data->base=captured;data->source=source;data->owner=actor->m_owner;data->old_owner=old_owner;data->new_owner=new_owner;
	data->population=captured->GetPopulation()-1;
	const int64_t numerator=int64_t(defender_economy.credits)*static_cast<int64_t>(captured->GetPopulation());
	if(numerator>INT32_MAX) {THROW("Capture treasury multiplication exceeds the admitted ordinary domain");}
	const int64_t booty=numerator/(*defender_economy.population_aggregate+1);
	if(booty>INT32_MAX || int64_t(data->actor_economy.credits)+booty>INT32_MAX) {THROW("Capture treasury arithmetic exceeds the admitted ordinary domain");}
	data->loot=static_cast<int32_t>(booty);data->actor_economy.credits+=data->loot;
	data->context=before;data->context.owner_simulation_id=new_owner;data->context.conquest=SmacConquestState{old_owner,50};data->context.uncaptured_former_owner.reset();data->context.former_owner_migration=false;
	data->context.recent_defender_loss=false;data->context.SyncKnownStatusBits();
	data->facilities=captured->m_facilities;
	bool headquarters=false;
	for(const auto& entry:data->facilities) {
		if(!entry.second || GetFacilityDef(entry.first)!=entry.second) {THROW("Capture facility is not registered");}
		ValidateSmacFacilityDef(entry.second);
		if(!entry.second->m_smac_source_id || *entry.second->m_smac_source_id>33) {THROW("Advanced facility capture needs its separate rules");}
		headquarters=headquarters || *entry.second->m_smac_source_id==1;
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
	const auto remove=[&](int32_t source_id) {for(auto it=data->facilities.begin();it!=data->facilities.end();) {if(it->second->m_smac_source_id==source_id) {it=data->facilities.erase(it);}else{++it;}}};
	for(int32_t id=1;id<=38;++id) {if(id!=26 && data->random.NextCrtRaw15()%3==0) {remove(id);}}
	remove(1);remove(6);remove(3);
	const auto* profile=state->m_settings.global.GetSmacFactionStartupRules()->Find(actor->m_owner->GetPlayer()->GetFaction()->m_id);
	if(!profile) {THROW("Capture owner has no faction profile");}
	for(const auto& bonus:profile->Records()) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::FACILITY) {
		const auto source_id=std::clamp(bonus.parameter,1,38);const auto* facility=GetSmacFacilityDef(source_id);
		if(!facility || source_id==1) {THROW("Capture requires a registered ordinary faction facility grant");}data->facilities[facility->m_id]=facility;
	}}
	Base* replacement=nullptr;int64_t nearest=INT64_MAX,best_governor=0;
	for(const auto id:m_base_order) {
		auto* candidate=GetBase(id);if(!candidate) {THROW("Capture Base order is incomplete");}candidate->GetSmacResourceContext();
		if(candidate!=captured && candidate->m_owner==captured->m_owner) {
			const auto* tile=candidate->GetTile();auto dx=std::abs(int64_t(tile->coord.x)-int64_t(destination->coord.x));const auto dy=std::abs(int64_t(tile->coord.y)-int64_t(destination->coord.y));
			dx=std::min(dx,int64_t(map->GetWidth())-dx);const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
			if(distance<=nearest) {nearest=distance;replacement=candidate;}
		}
		if(candidate!=captured && candidate->m_owner==actor->m_owner) {
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
	auto actor_state=actor->Conventional();actor_state.spent=3;actor_state.failed_move=false;data->units.emplace_back(actor,actor_state);
	for(const auto& entry:um->GetUnits()) {
		auto* unit=entry.second;if(unit->IsNative()) {continue;}auto next=unit->Conventional();
		if(replacement && unit->m_owner==captured->m_owner && next.home_base_id==captured->m_id) {
			next.home_base_id=replacement->m_id;next.home=unit::ConventionalUnitState::Home::Base;
			(void)um->ResolveConventional(next,unit->m_owner);data->units.emplace_back(unit,next);
		}
	}
	if(replacement) {data->refresh.push_back(replacement);}
	if(actor_state.home_base_id) {data->refresh.push_back(GetBase(*actor_state.home_base_id));}
	const auto target=m_game->SelectSmacFoundingProduction(actor->m_owner,destination);
	data->production=ProductionState{target,0,m_game->MakeSmacProductionItem(actor->m_owner,ProductionItemKind::Stockpile,38),0,std::vector<ProductionItem>{}};
	// Successful ordinary survivor capture preserves the independent paid marker.
	data->production->hurry_partial_paid=captured->m_production->hurry_partial_paid;
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
	bool preview_active=false;
	util::FinallyGuard restore([&]{
		if(!preview_active) {return;}preview_active=false;
		m_game->CommitBaseTerritory(preview_knowledge,preview_topology);
		query_simulation.hostility.owners.swap(preview_history.owners);
		query_simulation.victory_history.swap(preview_victory);
		query_simulation.retained_base_totals.swap(preview_totals);
		std::swap(query_simulation.economy.at(new_owner),preview_actor_economy);std::swap(query_simulation.economy.at(old_owner),preview_defender_economy);
		actor->CommitConventionalMove(actor->Conventional(),source);source->units.swap(preview_source);destination->units.swap(preview_destination);
		for(auto& entry:data->units) {std::swap(std::get<unit::ConventionalUnitState>(entry.first->m_state),entry.second);}
		captured->m_resource_context.swap(preview_context);captured->m_facilities.swap(preview_facilities);captured->m_production.swap(preview_production);
		captured->m_owner=previous_owner;captured->m_faction=previous_faction;
	});
	preview_active=true;
	captured->m_owner=data->owner;captured->m_faction=next_faction;
	captured->m_resource_context.swap(preview_context);captured->m_facilities.swap(preview_facilities);captured->m_production.swap(preview_production);
	for(auto& entry:data->units) {std::swap(std::get<unit::ConventionalUnitState>(entry.first->m_state),entry.second);}
	actor->CommitConventionalMove(actor->Conventional(),destination);source->units.swap(preview_source);destination->units.swap(preview_destination);
	query_simulation.hostility.owners.swap(preview_history.owners);
	query_simulation.victory_history.swap(preview_victory);
	query_simulation.retained_base_totals.swap(preview_totals);
	std::swap(query_simulation.economy.at(new_owner),preview_actor_economy);std::swap(query_simulation.economy.at(old_owner),preview_defender_economy);
	m_game->CommitBaseTerritory(preview_knowledge);
	const auto calculate=[&] {
		data->allocation=captured->CalculateSmacWorkers(*captured->m_allocation,data->context,SmacBaseResourcePhase::Ordinary,data->population,nullptr,&*data->production);
		data->cache=SmacBaseUpkeepCache{captured->CalculateSmacResourceCalculation(SmacBaseResourcePhase::Ordinary,&data->allocation,data->population),m_game->CalculateCanonicalUnitSupport(*captured,data->population,nullptr)};
		data->knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();data->topology=m_game->GetMap()->GetTerritoryTopology();
	};
	const auto prepare=[&] {
		auto sight=m_game->PrepareConventionalSight(actor->m_id,new_owner,destination,source);
        if(sight) {auto view=sight->CopyData();if(view.occupancy) {
            // Sight reads the projected post-move registry. Its ordinary move
            // cache predicates expect pre-move membership, so stage capture's
            // source departure and destination arrival explicitly here.
            view.occupancy->RemoveUnit({static_cast<int32_t>(source->coord.x),static_cast<int32_t>(source->coord.y)},source->units.empty());
            view.occupancy->PlaceUnit({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},new_owner,false);
            view.occupancy->SetAffiliation({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},new_owner);
            sight=map::SmacWorkerKnowledge::Create(view);
        }}
        m_game->CommitBaseTerritory(sight);
		std::optional<map::GlsmacTerritoryTopology> topology;
		auto territory=m_game->PrepareBaseTerritory(nullptr,nullptr,false,&topology);m_game->CommitBaseTerritory(territory,topology);
		if(leaving) {
			data->disposition=PrepareSmacFactionDisposition(previous_owner,nullptr,new_owner,data->random);
			auto& plan=*data->disposition;
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
							// On the initially empty restart square, factories prepend;
							// the initializer demotes C2 then finally promotes C3:
							// C3,Cn,...,C4,C1,C2. Keep returned creation identities.
							std::vector<size_t> creation,stack;
							for(const auto id:um->GetUnitRecordOrder()) {const auto* unit=um->GetUnit(id);if(unit->m_owner==previous_owner && unit->GetTile()==center) {creation.push_back(id);}}
							if(creation.size()!=plan.units.size() || creation.size()<4) {THROW("Restart tracking requires its fresh completed package");}
							stack.push_back(creation[2]);for(size_t i=creation.size();i>3;--i) {stack.push_back(creation[i-1]);}stack.push_back(creation[0]);stack.push_back(creation[1]);
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
	const auto& names=m_game->GetSmacBasePresentations();Base::SmacAllocationRoles roles{names[0],{names[1],names[2],names[3],names[4],names[5],names[6],names[7]}};
	auto facilities=data.facility_changes;facilities.emplace_back(data.base,data.facilities);
	auto workers=data.base->PrepareSmacWorkerState(GSE_CALL,data.allocation,roles,data.context,data.production,data.base->m_accumulated_nutrients,data.population,std::move(facilities),data.owner,data.refresh);
	auto* um=m_game->GetUM();auto updates=um->m_unit_updates;
	for(const auto& entry:data.units) {
		auto& pending=updates.try_emplace(entry.first->m_id,unit::UnitManager::unit_update_t{unit::UnitManager::UUO_NONE,entry.first}).first->second;
		if(pending.unit!=entry.first || (pending.ops&unit::UnitManager::UUO_DESPAWN)) {THROW("Capture unit has a stale pending update");}
		pending.ops=static_cast<unit::UnitManager::unit_update_op_t>(pending.ops|unit::UnitManager::UUO_REFRESH);
	}
	std::map<size_t,unit::Unit*> destination{{data.actor->m_id,data.actor}},source;
	const auto placement=um->PreparePlacementHistory(*data.actor,data.base->GetTile());
	const auto message=data.base->m_name+" captured ("+std::to_string(data.loot)+" energy credits).";
	for(const auto& entry:data.units) {entry.first->CommitConventionalMove(entry.second,entry.first==data.actor ? data.base->GetTile() : entry.first->GetTile());}
	data.source->units.swap(source);data.base->GetTile()->units.swap(destination);
	um->CommitPlacementHistory(placement);
	if(data.faction_units) {data.faction_units->Commit();}else{um->m_unit_updates.swap(updates);}
	m_game->CommitBaseTerritory(data.knowledge,data.topology);
	auto& simulation=m_game->m_simulation->value;
	simulation.hostility.owners.swap(data.history.owners);simulation.gameplay_random=data.random;
	data.actor_economy.design_counts=simulation.economy.at(data.new_owner).design_counts;
	data.defender_economy.design_counts=simulation.economy.at(data.old_owner).design_counts;
	simulation.economy.at(data.new_owner)=data.actor_economy;simulation.economy.at(data.old_owner)=data.defender_economy;
	simulation.victory_history=data.victory;
	simulation.retained_base_totals=data.totals;
	for(size_t owner=1;owner<8;++owner) {auto& target=simulation.battle_plan_target[owner];if(target && *target==static_cast<int64_t>(data.base->m_id)) {target=-1;}}
	if(data.disposition) {CommitSmacFactionDisposition(*data.disposition);}
	workers.Commit();data.base->m_smac_upkeep_cache.swap(data.cache);workers.Finish();
	if(data.faction_units) {data.faction_units->Notify(GSE_CALL);}
	if(data.tracked_center) {
		auto request=game::FrontendRequest(game::FrontendRequest::FR_MAP_CENTER);request.data.map_center.x=(*data.tracked_center)[0];request.data.map_center.y=(*data.tracked_center)[1];
		m_game->AddFrontendRequest(request);m_game->Message("The restarted faction has been tracked.");
	}
	m_game->Message(message);
}

}
