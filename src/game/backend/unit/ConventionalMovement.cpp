#include "UnitManager.h"
#include "SmacUnitFacing.h"

#include "Unit.h"
#include "SmacGroundEdgeCost.h"
#include "SmacSelectedMovement.h"
#include "game/backend/resource/SmacSiteSignal.h"
#include "SmacScoutKeyboardAdmission.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/Base.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace game::backend::unit {
namespace {
bool ForeignBase(const Unit* actor,const map::tile::Tile* tile,const base::BaseManager& manager) {
	if(!actor || !tile || !tile->base) {return false;}
	for(const auto& row:manager.GetBases()) {if(row.second==tile->base) {return row.second->m_owner!=actor->m_owner;}}
	return false;
}
smac_edge::Tile EdgeTile( const map::tile::Tile& tile, const base::Base* admitted_base ) {
	using namespace map::tile;
	constexpr auto features = FEATURE_RIVER | FEATURE_XENOFUNGUS;
	constexpr auto terraforming = TERRAFORMING_ROAD | TERRAFORMING_MAG_TUBE | TERRAFORMING_FOREST
		| TERRAFORMING_FARM | TERRAFORMING_SOLAR | TERRAFORMING_MINE;
	if ( tile.is_water_tile || ( tile.base && tile.base != admitted_base ) || tile.rockiness < ROCKINESS_FLAT || tile.rockiness > ROCKINESS_ROCKY
		|| ( tile.features & ~features ) != 0 || ( tile.terraforming & ~terraforming ) != 0
		|| tile.coord.x > INT32_MAX || tile.coord.y > INT32_MAX ) {
		throw std::runtime_error( "Conventional movement requires admitted ordinary land without base, landmark or interaction features" );
	}
	uint32_t flags = tile.base ? smac_edge::Base : 0;
	if ( tile.features & FEATURE_RIVER ) { flags |= smac_edge::River; }
	if ( tile.features & FEATURE_XENOFUNGUS ) { flags |= smac_edge::Fungus; }
	if ( tile.terraforming & TERRAFORMING_ROAD ) { flags |= smac_edge::Road; }
	if ( tile.terraforming & TERRAFORMING_MAG_TUBE ) { flags |= smac_edge::Tube; }
	if ( tile.terraforming & TERRAFORMING_FOREST ) { flags |= smac_edge::Forest; }
	// Explicit GLSMAC land-equivalence policy. Original bands3..7 have equal
	// cost in the admitted kernel; this does not quantize port meter heights.
	return { static_cast< int32_t >( tile.coord.x ), static_cast< int32_t >( tile.coord.y ), 3,
		static_cast< int32_t >( tile.rockiness - ROCKINESS_FLAT ), flags };
}
}

UnitManager::StackChanges UnitManager::PrepareConventionalStackMove(Unit* actor,map::tile::Tile* destination,bool authenticate_new) const {
    using Stack=ConventionalUnitState::Stack;
    if(actor->GetTile()==destination) {return {};}
    // An unplaced prepared conventional factory appends one source-pool row;
    // a registered actor retains its existing row during directional movement.
    const bool appended=GetUnit(actor->m_id)!=actor;
    if(appended && (actor->GetTile() || actor->SourceStack())) {throw std::runtime_error("New conventional stack member must be unplaced and unbound");}
    bool needed=actor->SourceStack().has_value() || (appended && authenticate_new);
    for(const auto& row:destination->units) {needed=needed || row.second->SourceStack().has_value();}
    if(!needed) {return {};}
    std::vector<Unit*> dense;for(const auto id:m_unit_order) {dense.push_back(m_units.at(id));}ValidateNativeIndexBindings(dense);
    std::map<Unit*,Stack> changes;
    const auto read=[&](Unit* unit)->Stack {
        const auto found=changes.find(unit);if(found!=changes.end()) {return found->second;}
        if(const auto known=unit->SourceStack()) {return *known;}
        if(appended && unit==actor) {if(dense.size()>=2048) {throw std::runtime_error("Conventional source-pool is full");}return {int16_t(dense.size()),-1,-1};}
        const auto position=std::find(dense.begin(),dense.end(),unit);
        if(position==dense.end() || dense.size()>2048) {throw std::runtime_error("Stack movement lost its dense identity");}
        return {int16_t(position-dense.begin()),-1,-1};
    };
    const auto linked=[&](int16_t id)->Unit* {if(id<0 || size_t(id)>=dense.size()) {throw std::runtime_error("Stack movement has an invalid neighbor");}return dense[id];};
    auto moved=read(actor);
    if(actor->SourceStack()) {
        if(moved.previous>=0) {auto* previous=linked(moved.previous);auto state=read(previous);state.next=moved.next;changes[previous]=state;}
        if(moved.next>=0) {auto* next=linked(moved.next);auto state=read(next);state.previous=moved.previous;changes[next]=state;}
    }
    Unit* head=nullptr;
    for(const auto& row:destination->units) {
        auto* resident=row.second;
        if(!resident->SourceStack() && destination->units.size()!=1) {throw std::runtime_error("Destination legacy stack has unknown order");}
        if(read(resident).previous<0) {if(head) {throw std::runtime_error("Destination stack has multiple heads");}head=resident;}
    }
    moved.previous=-1;moved.next=-1;
    if(head) {auto state=read(head);moved.next=state.id;state.previous=moved.id;changes[head]=state;}
    changes[actor]=moved;
    return {changes.begin(),changes.end()};
}
void UnitManager::CommitStackChanges(const StackChanges& changes) noexcept {
    for(const auto& row:changes) {
        if(row.first->IsNative()) {auto& n=std::get<SmacNativeUnitRecord>(row.first->m_state);n.id=row.second.id;n.stack_next=row.second.next;n.stack_previous=row.second.previous;}
        else {std::get<ConventionalUnitState>(row.first->m_state).stack=row.second;}
    }
}

UnitManager::PreparedConventionalMove UnitManager::PrepareConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const {
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) {
		throw std::runtime_error( "Conventional movement requires a registered conventional unit" );
	}
	unit->RequireConventionalReady();
	m_game->RequireConventionalSightWorld();
	const auto* state = m_game->GetState();
	const auto& local = state->m_settings.local;
	if ( !m_game->IsRunning() || state->m_connection || local.game_mode != settings::LocalSettings::GM_SINGLEPLAYER
		|| local.network_type != settings::LocalSettings::NT_NONE || local.network_role != settings::LocalSettings::NR_NONE ) {
		throw std::runtime_error( "Conventional movement supports local single-player only" );
	}
	const auto& stored = unit->Conventional();
    if(stored.IsHeld()) {throw std::runtime_error("Activate held unit before directional movement");}
	const auto context = ResolveConventional( stored, unit->m_owner ); // validates current binding before native owner use
	const auto stats = QueryConventionalUnit( stored, context, SmacMoraleMode::Mode1 );
	const auto& simulation = m_game->m_simulation->value;
	// Active participants and the local controller are distinct. This port
	// route uses the accepted local participant, never an inferred faction slot.
	const auto* participant = simulation.FindSlot( caller );
	if ( !participant || !participant->active || participant->role != roster::Role::Single
		|| caller != m_game->GetSlotNum() || unit->m_owner != &state->m_slots->GetSlot( caller ) ) {
		throw std::runtime_error( "Conventional movement requires the accepted local single-player controller" );
	}
	if ( simulation.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Conventional movement requires explicit no-project ordinary-terrain campaign admission" );
	}
	auto* source = unit->GetTile();
	const auto* map = GetMap();
	const auto canonical = [ & ]( const map::tile::Tile* tile ) {
		return tile && tile->coord.x < map->GetWidth() && tile->coord.y < map->GetHeight()
			&& ( tile->coord.x & 1 ) == ( tile->coord.y & 1 ) && map->GetTile( tile->coord.x, tile->coord.y ) == tile;
	};
	if ( map->GetWidth() > INT32_MAX || map->GetHeight() > INT32_MAX || !canonical( source ) || !canonical( destination ) ) {
		throw std::runtime_error( "Conventional movement requires canonical bounded map tiles" );
	}
	using namespace smac_scout_key;
	if ( !CanAct( { unit->m_id, stored.owner_simulation_id, static_cast< int32_t >( source->coord.x ), static_cast< int32_t >( source->coord.y ),
		stored.spent, stats.full_allowance, Status::Ordinary }, participant->simulation_id,
		static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ) ) ) {
		throw std::runtime_error( "Conventional unit cannot act: foreign, out of map or exhausted" );
	}
	// Actionability precedes edge pricing, including a zero-cost tube edge.
	if ( source == destination || !source->IsAdjactentTo( destination ) || source->IsLocked() || destination->IsLocked()
		|| unit->m_animation_id ) {
		throw std::runtime_error( "Conventional movement requires an unlocked adjacent destination and no animation" );
	}
	for (const auto& entry:destination->units) {
		const auto* other=entry.second;
		if(!other || GetUnit(entry.first)!=other || other->m_id!=entry.first || other->GetTile()!=destination
			|| !other->IsConventional() || other->m_owner!=unit->m_owner || other->Conventional().owner_simulation_id!=stored.owner_simulation_id) {
			throw std::runtime_error("Conventional occupied entry requires a canonical same-owner stack; hostile entry needs combat");
		}
		other->QueryStats(other->Conventional().render_morale_mode);
	}
	if(!destination->units.empty() && (destination->features & map::tile::FEATURE_XENOFUNGUS)) {
		throw std::runtime_error("Occupied fungus entry requires its distinct exemption and encounter-history continuation");
	}
	const auto membership = source->units.find( unit->m_id );
	if ( membership == source->units.end() || membership->second != unit ) { throw std::runtime_error( "Conventional movement requires exact source membership" ); }
	// Original Scouts may depart from a source shared with hostile natives.
	// Authenticate mixed links; destination and terrain admission stay separate.
	bool native_source = false;
	for ( const auto& entry : source->units ) {
		const auto* other = entry.second;
		if ( !other || GetUnit( entry.first ) != other || other->m_id != entry.first || other->GetTile() != source ) {
			throw std::runtime_error( "Conventional departure requires canonical source membership" );
		}
		if ( other->IsNative() ) {
			if ( !unit->SourceStack() ) { throw std::runtime_error( "Mixed native departure requires authenticated source links" ); }
			QueryNativeUnit( other->Native() ); native_source = true;
		} else {
			if ( !other->IsConventional() || other->m_owner != unit->m_owner || other->Conventional().owner_simulation_id != stored.owner_simulation_id ) {
				throw std::runtime_error( "Conventional departure requires an admitted source member" );
			}
			other->QueryStats( other->Conventional().render_morale_mode );
		}
	}
	if ( native_source ) {
		std::vector<Unit*> dense;
		for ( const auto id : m_unit_order ) { dense.push_back( m_units.at( id ) ); }
		ValidateNativeIndexBindings( dense );
	}
	const auto design = m_game->ResolveConventionalDesign( stored, *m_game->m_simulation );
	const auto& catalog = state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	const auto& weapon = catalog.weapons.weapons.at( design.weapon );
	const auto& social = *simulation.social.Find( stored.owner_simulation_id );
	if ( context.package != ConventionalUnitContext::Package::Combat
		&& ( ( source->features | destination->features ) & map::tile::FEATURE_XENOFUNGUS ) ) {
		throw std::runtime_error( "Noncombat runtime fungus encounter admission is not established" );
	}
	// Owned Bases impose no extra ordinary nonfungus terrain or population rule.
	// Fungus still uses the narrower, independently admitted endpoint-HQ path.
	const base::Base* admitted_hq = nullptr;
	const base::Base* admitted_source_base = nullptr;
	const base::Base* admitted_destination_base = nullptr;
	const auto& bases = m_game->GetBM()->GetBases();
	if ( !bases.empty() ) {
		const bool infantry = design.chassis == 0 && stats.basic_allowance == 3;
		// This context is produced only after resolving the canonical Spartan
		// participant and its actual selected owned design, never slot arithmetic.
		const bool selected_rover = context.presentation == ConventionalUnitContext::Presentation::SelectedSpartanRover
			&& design.chassis == 1 && stats.basic_allowance == 6;
		// Damage and effective morale have already produced full_allowance.
		// Base proximity adds no second health or stored-morale admission gate.
		if ( stats.quantum != 3 || ( !infantry && !selected_rover ) ) {
			throw std::runtime_error( "Movement with Bases requires admitted Infantry or selected Spartan Rover with quantum3" );
		}
		// Source fungus does not enter the encounter or movement lottery.
        // A clear destination uses ordinary Base admission at any distance.
        // A fungus-covered Base center still requires its distinct admission.
        const bool encounter_context=simulation.movement_history && simulation.ecology_world
            && state->m_settings.global.GetSmacCampaignRules() && map->GetResourcePolicy()
            && !(source->base && (source->features&map::tile::FEATURE_XENOFUNGUS));
        if ( !((source->features | destination->features) & map::tile::FEATURE_XENOFUNGUS)
            || (!source->base && !(destination->features & map::tile::FEATURE_XENOFUNGUS)) || encounter_context ) {
			for ( const auto& entry : bases ) {
				const auto* base = entry.second;
				const auto* center = base ? base->GetTile() : nullptr;
				if ( !base || base->m_id != entry.first || base->GetGame() != m_game || !canonical( center ) || center->base != base ) {
					throw std::runtime_error( "Ordinary movement requires canonical registered Base centers" );
				}
				if ( center == source || center == destination || source->IsAdjactentTo( center ) || destination->IsAdjactentTo( center ) ) {
					if ( base->m_owner != unit->m_owner || base->m_faction != unit->m_owner->GetPlayer()->GetFaction() ) {
						throw std::runtime_error( "Ordinary movement does not admit foreign Base interactions" );
					}
				}
				if ( center == source ) { admitted_source_base = base; }
				if ( center == destination ) { admitted_destination_base = base; }
			}
		} else if ( !source->base && !destination->base ) {
            // Remote ordinary Infantry can reach the separate encounter stage.
            if(design.chassis!=0 || design.plan!=3 || design.abilities || context.presentation!=ConventionalUnitContext::Presentation::Scout) {
                throw std::runtime_error("Remote fungus requires the admitted conventional Scout profile");
            }
		} else {
			// Select by endpoint identity, not global count or first record. Locate
			// the borrowed pointer in the canonical manager before dereferencing it.
			const auto* endpoint_base = source->base ? source->base : destination->base;
			const auto found = std::find_if( bases.begin(), bases.end(), [ & ]( const auto& entry ) { return entry.second == endpoint_base; } );
			if ( !endpoint_base || found == bases.end() ) { throw std::runtime_error( "Friendly HQ movement requires a registered endpoint HQ" ); }
			const auto& entry = *found;
			const auto* hq = entry.second;
			const auto* definition = m_game->GetBM()->GetFacilityDef( "HEADQUARTERS" );
			if ( !hq || hq->m_id != entry.first || hq->GetGame() != m_game || hq->m_owner != unit->m_owner
				|| hq->m_faction != unit->m_owner->GetPlayer()->GetFaction() || hq->GetPopulation() != 1
				|| !definition || hq->m_facilities.size() != 1 || hq->m_facilities.begin()->first != "HEADQUARTERS"
				|| hq->m_facilities.begin()->second != definition ) {
				throw std::runtime_error( "Friendly HQ movement requires a canonical same-owner population1 HQ with no other facilities" );
			}
			const auto* center = hq->GetTile();
			const auto* outside = center == source ? destination : source;
			using namespace map::tile;
			if ( ( center != source && center != destination ) || center->base != hq || outside->base
				|| center->rockiness != ROCKINESS_FLAT || outside->rockiness != ROCKINESS_FLAT
				|| center->features != FEATURE_NONE || center->terraforming != TERRAFORMING_NONE
				|| ( outside->features != FEATURE_NONE && outside->features != FEATURE_XENOFUNGUS )
				|| ( outside->terraforming != TERRAFORMING_NONE && outside->terraforming != TERRAFORMING_ROAD && outside->terraforming != TERRAFORMING_MAG_TUBE )
				|| ( outside->features != FEATURE_NONE && outside->terraforming != TERRAFORMING_NONE ) ) {
				throw std::runtime_error( "Friendly HQ movement requires the HQ endpoint and adjacent flat ordinary road tube or fungus land" );
			}
			// An endpoint HQ is at distance0/1 from this edge. The original fungus
			// branch consumes nearest-any-base distance <=1, not the chosen base ID;
			// another base winning an equal-distance tie cannot invalidate it.
			admitted_hq = hq;
		}
	}
	if ( auto refusal=m_game->PrepareConventionalZoc(*unit,destination) ) {
		return {unit,source,destination,stored,std::nullopt,std::nullopt,false,std::move(refusal)};
	}
    auto random=simulation.gameplay_random;
    std::optional<int32_t> nearest_distance;
    bool update_nearest=false;
    const bool fungus=(destination->features&map::tile::FEATURE_XENOFUNGUS)!=0;
    if(fungus && !admitted_hq && (source->is_water_tile || destination->is_water_tile
        || (design.chassis!=0 && design.chassis!=1) || static_cast<int32_t>(catalog.chassis.chassis.at(design.chassis).triad)!=0
        || weapon.offense<=0 || static_cast<int32_t>(weapon.mode)!=0 || design.plan<0 || design.plan>3 || design.abilities)) {
        throw std::runtime_error("Fungus encounters require the admitted ordinary land combat profile");
    }
    if(fungus && (!admitted_hq || simulation.movement_history)) {
        if(!simulation.movement_history) {throw std::runtime_error("Fungus nearest-base encounter requires retained placement history");}
        const auto& history=*simulation.movement_history;
        history.ValidateDimensions(map->GetWidth(),map->GetHeight());
        const auto& rules=state->m_settings.global.GetSmacCampaignRules();
        // The existing endpoint-HQ contract predates saved campaign options.
        // Its distance bypass remains valid without inventing those options.
        if(!admitted_hq && !rules) {throw std::runtime_error("Remote encounter admission requires saved primary rules");}
        const bool enabled=!rules || !(rules->primary&0x00200000u);
        bool site=false;
        if(!admitted_hq) {
            const auto& policy=map->GetResourcePolicy();
            if(!policy) {throw std::runtime_error("Fungus encounter requires admitted resource/map semantics");}
            if(const auto* original=policy->OriginalPlanet()) {
                auto flags=original->At(destination->coord.x,destination->coord.y).Items();
                flags&=~0x00102000u;
                if(history.WasPlaced(destination->coord.x,destination->coord.y)) {flags|=0x100000u;}
                if(destination->features&map::tile::FEATURE_MONOLITH) {flags|=0x2000u;}
                site=resource::smac::ResolveSiteSignal({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y),
                    policy->Altitude(*destination),flags,rules->AutomaticLandResources(),original->ResourcePattern(),11,61})!=0;
            } else {site=policy->SiteSignal(*destination)!=0;}
        }
        if(enabled && !site && !(destination->terraforming&map::tile::TERRAFORMING_MAG_TUBE)) {
            std::vector<SmacMovementHistory::BasePosition> positions;
            for(const auto& entry:bases) {
                const auto* base=entry.second;const auto* center=base?base->GetTile():nullptr;
                if(!base || base->m_id!=entry.first || base->GetGame()!=m_game || !canonical(center) || center->base!=base) {
                    throw std::runtime_error("Encounter nearest-Base query requires canonical centers");
                }
                positions.push_back({entry.first,static_cast<int32_t>(center->coord.x),static_cast<int32_t>(center->coord.y)});
            }
            nearest_distance=SmacMovementHistory::NearestAnyBase(destination->coord.x,destination->coord.y,
                static_cast<int32_t>(map->GetWidth()),true,positions,history.NearestDistance()).distance;
            if(!nearest_distance) {throw std::runtime_error("No-Base encounter requires retained nearest distance");}
            update_nearest=true;
            if(*nearest_distance>1) {
                if(!simulation.ecology_world) {throw std::runtime_error("Encounter life category is unavailable");}
                const auto life=simulation.ecology_world->native_life;simulation.ecology_world->Validate();
                int32_t threshold=std::min(*nearest_distance,32)+10*life;
                if(history.WasPlaced(destination->coord.x,destination->coord.y)) {threshold/=2;if(life==0) {threshold/=2;}}
                if(destination->terraforming&map::tile::TERRAFORMING_ROAD) {threshold/=2;}
                static constexpr std::array<std::array<int32_t,2>,25> protection{{
                    {{0,0}},{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}},
                    {{2,-2}},{{2,2}},{{-2,2}},{{-2,-2}},{{1,-3}},{{3,-1}},{{3,1}},{{1,3}},{{-1,3}},{{-3,1}},{{-3,-1}},{{-1,-3}},{{4,0}},{{-4,0}},{{0,4}},{{0,-4}}
                }};
                std::optional<map::SmacWorkerKnowledge::Data> protection_knowledge;
                for(size_t i=0;i<protection.size();++i) {
                    auto x=int64_t(destination->coord.x)+protection[i][0];const auto y=int64_t(destination->coord.y)+protection[i][1];
                    x=(x+map->GetWidth())%map->GetWidth();if(y<0 || y>=int64_t(map->GetHeight())) {continue;}
                    const auto* tile=map->GetTile(x,y);bool protected_base=false;
                    if(tile->base) {
                        const auto& knowledge=map->GetSmacWorkerKnowledge();
                        if(!knowledge) {throw std::runtime_error("Base encounter protection requires retained affiliation");}
                        if(!protection_knowledge) {protection_knowledge=knowledge->CopyData();}
                        if(!protection_knowledge->occupancy) {throw std::runtime_error("Base encounter protection requires retained affiliation");}
                        protected_base=protection_knowledge->occupancy->At({static_cast<int32_t>(x),static_cast<int32_t>(y)}).affiliation<8;
                    }
                    if((tile->terraforming&map::tile::TERRAFORMING_SENSOR) || protected_base) {threshold/=(i<9?4:2);}
                }
                if(random.NextCrtRaw15()%100<threshold) {
                    auto birth=PrepareNativeBirthsQuery(map->GetWidth(),map->GetHeight(),true,[&](const auto& factory) {
                        factory.create(8,{static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},{});
                    });
                    if(birth.Size()) {
                        return {unit,source,destination,stored,std::nullopt,nearest_distance,update_nearest,{}, {},
                            PreparedConventionalEncounter{std::move(birth),random}};
                    }
                    // A full factory retains the CRT draw and falls through to
                    // ordinary allowance/cost/selected movement lottery.
                }
            }
        }
    }
	const auto edge_source = EdgeTile( *source, admitted_hq ? admitted_hq : admitted_source_base );
	const auto edge_destination = EdgeTile( *destination, admitted_hq ? admitted_hq : admitted_destination_base );
	const smac_edge::Query query{
		{ design.id, static_cast< smac_edge::Chassis >( design.chassis ), static_cast< int32_t >( catalog.chassis.chassis.at( design.chassis ).triad ),
			weapon.offense, static_cast< int32_t >( weapon.mode ), design.plan, design.abilities, stats.basic_allowance },
		stats.quantum, stored.owner_simulation_id, social.retained->at( 7 ), false, false,
		static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ), true, edge_source, edge_destination };
	const auto cost = smac_edge::QueryGroundEdgeCost( query );
	auto result = smac_move::ResolveSelectedGroundStep( { unit->m_id, stored.spent, stored.failed_move }, stats.quantum, stats.full_allowance,
		cost, ( edge_destination.features & smac_edge::Fungus ) != 0, random,
        smac_move::LotteryStream::SharedLow16 );
	auto changed = stored;
    if(result.outcome==smac_move::Outcome::Moved) {changed.facing=SmacFacingForStep(*source,*destination);}
	changed.spent = static_cast< uint8_t >( result.unit.spent ); changed.failed_move = result.unit.prior_failure_marker;
	QueryConventionalUnit( changed, context, stored.render_morale_mode );
	auto sight = result.outcome == smac_move::Outcome::Moved
		? m_game->PrepareConventionalSight(unit->m_id,stored.owner_simulation_id,destination,source) : std::nullopt;
	auto stacks=result.outcome==smac_move::Outcome::Moved ? PrepareConventionalStackMove(unit,destination):StackChanges{};
	return { unit, source, destination, changed, std::move( result ), nearest_distance, update_nearest, std::move(sight),std::move(stacks) };
}

void UnitManager::ValidateConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const {
	m_game->RequireSmacGameplayActive();
	if(HasForeignOccupant(unit,destination)) {(void)PrepareConventionalAttack(unit,destination,caller);return;}
	if(ForeignBase(unit,destination,*m_game->GetBM())) {(void)m_game->GetBM()->PrepareSmacCapture(unit,destination,caller);return;}
	(void)PrepareConventionalMove( unit, destination, caller ); // owned copied RNG only
}

void UnitManager::MoveConventionalUnit( GSE_CALLABLE, Unit* unit, map::tile::Tile* destination, size_t caller ) {
	m_game->RequireSmacGameplayActive();
	m_game->CheckRW( GSE_CALL );
	if(HasForeignOccupant(unit,destination)) {ApplyConventionalAttack(GSE_CALL,PrepareConventionalAttack(unit,destination,caller));return;}
	if(ForeignBase(unit,destination,*m_game->GetBM())) {m_game->GetBM()->ApplySmacCapture(GSE_CALL,m_game->GetBM()->PrepareSmacCapture(unit,destination,caller));return;}
	auto prepared = PrepareConventionalMove( unit, destination, caller );
    if(prepared.encounter) {
        const auto placement=PreparePlacementHistory(*unit,destination);
        // All allocation and validation completed in detached preparation.
        auto& encounter=*prepared.encounter;encounter.birth.Commit();
        if(prepared.update_nearest) {m_game->m_simulation->value.movement_history->RetainNearest(prepared.nearest_distance);}
        if(placement.first) {placement.first->MarkPlaced(placement.second);}
        m_game->m_simulation->value.gameplay_random=encounter.random;
        encounter.birth.Notify(GSE_CALL);return;
    }
	if ( !prepared.result ) {
		m_game->CommitConventionalSight(prepared.sight); return;
	}
	const auto placement = PreparePlacementHistory( *unit, destination );
	const auto update = m_unit_updates.find( unit->m_id );
	if ( update != m_unit_updates.end() && ( update->second.unit != unit || ( update->second.ops & UUO_DESPAWN ) ) ) {
		throw std::runtime_error( "Conventional movement has a stale pending presentation record" );
	}
	// Same owner call, no callbacks/yields between preparation and commit.
	// Allocate the destination node before touching any membership or authority.
	std::map< size_t, Unit* > staged_destination;
	const bool moved = prepared.result->outcome == smac_move::Outcome::Moved;
	if ( moved ) { staged_destination.emplace( unit->m_id, unit ); }
	// try_emplace completes all update-node/bucket allocation first. With the
	// existing std::less<size_t> map, node transfer below cannot allocate or run
	// user code. Every remaining authority write is a trivial nonthrowing copy.
	auto& pending = m_unit_updates.try_emplace( unit->m_id, unit_update_t{ UUO_NONE, unit } ).first->second;
	// An accepted failed partial attempt also retains the caller query result.
	// Validation above was side-effect free; no unsupported request gets here.
	if ( prepared.update_nearest ) { m_game->m_simulation->value.movement_history->RetainNearest( prepared.nearest_distance ); }
	if ( moved ) {
		destination->units.insert( staged_destination.extract( unit->m_id ) );
		prepared.source->units.erase( unit->m_id );
		if ( placement.first ) { placement.first->MarkPlaced( placement.second ); }
	}
	static_assert( std::is_nothrow_copy_assignable_v< ConventionalUnitState > );
	static_assert( std::is_nothrow_copy_assignable_v< SmacGameplayRandomState > );
	unit->CommitConventionalMove( prepared.state, moved ? destination : prepared.source );
    CommitStackChanges(prepared.stacks);
	m_game->m_simulation->value.gameplay_random = prepared.result->random;
	m_game->CommitConventionalSight(prepared.sight);
	pending.ops = static_cast< unit_update_op_t >( pending.ops | UUO_REFRESH );
	// PushUpdates later publishes the committed record. No animation callback
	// owns movement, no GSE callbacks execute here, and no network rollback is used.
}

void UnitManager::ResetConventionalSpent( GSE_CALLABLE, Unit* unit ) {
	m_game->CheckRW( GSE_CALL );
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) { throw std::runtime_error( "Conventional upkeep requires a registered unit" ); }
	m_game->RequireSmacProductionOwner(unit->m_owner);
	unit->QueryStats( SmacMoraleMode::Mode1 );
	auto next = unit->Conventional();
	const auto reset = smac_move::ResetSpentForCheckedUnitUpkeep( { unit->m_id, next.spent, next.failed_move } );
	next.spent = static_cast< uint8_t >( reset.spent );
    next.AdvanceOrderUpkeep();
	QueueUnitUpdate( unit, UUO_REFRESH ); // allocation before byte update
	unit->CommitConventionalMove( next, unit->GetTile() );
}

}
