#include "Game.h"
#include "LocalMoveCommand.h"

#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "map/SmacOrdinaryLandExploration.h"
#include "gse/value/Object.h"
#include "map/tile/Tiles.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"

namespace game::backend {

bool QueueLocalConventionalMove(GSE_CALLABLE, Game& game, size_t unit_id, size_t x, size_t y) {
    std::string rejection;
    try {
        auto* actor=game.GetUM()->GetUnit(unit_id);
        const auto* map=game.GetMap();
        if(!actor || !actor->IsConventional() || !map || x>=map->GetWidth() || y>=map->GetHeight() || (x&1)!=(y&1)) {
            throw std::runtime_error("The selected Unit or destination is no longer available");
        }
        auto* destination=game.GetMap()->GetTile(x,y);
        // Event owns canonical command validation and copied-sight preparation.
        game.Event(GSE_CALL,"move_unit",{{"unit",actor->Wrap(GSE_CALL)},{"tile",destination->Wrap(GSE_CALL)}});
        return true;
    } catch(const std::runtime_error& error) {
        rejection=error.what();
    }
    game.Message("Movement unavailable: "+rejection);
    return false;
}

map::SmacWorkerMapFrame Game::ProjectSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge, int32_t observer, const base::Base* pending_base, bool startup ) const {
	using Frame = map::SmacWorkerMapFrame;
	if ( startup ) { RequireConventionalStartupClaim(); }
	ValidateCurrentSimulation();
	if ( !m_simulation || !m_map || !m_bm || !m_um || m_state->m_settings.global.GetRulesetId() != "smac" ) {
		throw std::runtime_error( "Worker knowledge requires a current canonical SMAC world" );
	}
	const auto data = knowledge.CopyData();
	bool participant = false;
	for ( const auto& record : m_simulation->value.roster ) { if ( record.simulation_id == observer ) { participant = true; break; } }
	if ( !participant ) { throw std::runtime_error( "Worker observer is not a current simulation participant" ); }
	if ( data.width != static_cast< int64_t >( m_map->GetWidth() ) || data.height != static_cast< int64_t >( m_map->GetHeight() ) || !data.horizontal_wrap ) {
		throw std::runtime_error( "Worker knowledge geometry differs from the current periodic-X map" );
	}
	for ( const auto& record : data.units ) {
		if ( record.active_convoy ) { throw std::runtime_error( "Live worker knowledge does not yet admit convoy orders" ); }
	}
	const auto coordinate = [ this ]( const map::tile::Tile* tile ) -> Frame::Coordinate {
		if ( !tile || tile->tiles != m_map->GetTilesPtr() || tile->coord.x >= m_map->GetWidth() || tile->coord.y >= m_map->GetHeight()
			|| ( ( tile->coord.x ^ tile->coord.y ) & 1 ) || m_map->GetTile( tile->coord.x, tile->coord.y ) != tile ) {
			throw std::runtime_error( "Worker entity has no current map coordinate" );
		}
		return { static_cast< int32_t >( tile->coord.x ), static_cast< int32_t >( tile->coord.y ) };
	};
	std::vector< Frame::Base > bases;
	for ( const auto& entry : m_bm->GetBases() ) {
		const auto* base = entry.second;
		if ( !base || base->m_id != entry.first || base->GetGame() != this ) { throw std::runtime_error( "Worker Base registry identity differs" ); }
		const auto position = coordinate( base->GetTile() );
		const auto context = base->GetSmacResourceContext();
		const auto allocation = base->GetSmacAllocation();
		if ( !allocation || base->GetTile()->base != base ) { throw std::runtime_error( "Worker Base lacks allocation authority or center membership" ); }
		bases.push_back( { context.owner_simulation_id, position, allocation->selected & Frame::RADIUS_MASK } );
	}
	for ( const auto& observation : data.bases ) {
		if ( !m_bm->GetBase(observation.id) ) { throw std::runtime_error("Remembered Base ID is not registered"); }
	}
	std::vector< map::SmacWorkerKnowledge::LiveUnit > units;
	for ( const auto& entry : m_um->GetUnits() ) {
		const auto* unit = entry.second;
        if(!unit || unit->m_id!=entry.first || (!unit->IsConventional() && !unit->IsNative())) {
            throw std::runtime_error("Worker knowledge requires canonical live units");
        }
        const auto position=coordinate(unit->GetTile());
        if(unit->IsNative()) {
            const auto& record=unit->Native();
            (void)m_um->QueryNativeUnit(record);
            const auto observation=std::find_if(data.units.begin(),data.units.end(),[&](const auto& row){return row.id==entry.first;});
            if(unit->m_owner || record.x!=static_cast<int64_t>(unit->GetTile()->coord.x)
                || record.y!=static_cast<int64_t>(unit->GetTile()->coord.y) || observation==data.units.end()
                || observation->visible_to!=record.observation_mask) {
                throw std::runtime_error("Native owner, placement or saved observation differs from its live record");
            }
        } else {
            const auto& state=unit->Conventional();
            if(startup) {
                if(!unit->IsConventionalPending() || state.home_base_id || state.home!=unit::ConventionalUnitState::Home::None) {
                    throw std::runtime_error("Startup knowledge requires actual Early homeless units");
                }
                ResolveConventionalIdentity(state,unit->m_owner,*m_simulation);
            } else {unit->RequireConventionalReady();ResolveConventionalUnit(state,unit->m_owner);}
        }

		const auto member = unit->GetTile()->units.find( entry.first );
		if ( member == unit->GetTile()->units.end() || member->second != unit ) { throw std::runtime_error( "Worker unit is missing current tile membership" ); }
		units.push_back( { entry.first, unit->SimulationOwner(), position } );
	}
	std::vector< bool > sea;
	sea.reserve( data.tiles.size() );
	// Reverse membership is checked as well: no unregistered center or stack
	// entry may quietly vanish from the derived frame.
	for ( int32_t y = 0; y < data.height; ++y ) { for ( int32_t x = y & 1; x < data.width; x += 2 ) {
		const auto* tile = m_map->GetTile( x, y );
		coordinate( tile );
		sea.push_back( tile->is_water_tile );
		if ( tile->base ) {
			bool found = false;
			for ( const auto& entry : m_bm->GetBases() ) { if ( entry.second == tile->base ) { found = entry.second->GetTile() == tile; break; } }
			if ( !found && !( tile->base == pending_base && pending_base->GetTile() == tile ) ) { throw std::runtime_error( "Worker map has an unregistered or displaced Base center" ); }
		}
		for ( const auto& entry : tile->units ) {
			const auto found = m_um->GetUnits().find( entry.first );
			if ( found == m_um->GetUnits().end() || found->second != entry.second || !entry.second || entry.second->GetTile() != tile ) {
				throw std::runtime_error( "Worker map has an unregistered or displaced unit" );
			}
		}
	} }
	return knowledge.Project( observer, sea, bases, units );
}

void Game::ValidateSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge ) const {
	const auto data = knowledge.CopyData();
	for ( int32_t observer = 1; observer <= 7; ++observer ) {
		if ( data.observers[ observer ] ) { (void)ProjectSmacWorkerKnowledge( knowledge, observer ); }
	}
}

void Game::PublishSmacWorkerKnowledge( GSE_CALLABLE, const map::SmacWorkerKnowledge& knowledge ) {
	CheckRW( GSE_CALL );
	if ( gc_space != GetGCSpace() || !gc_space->IsAccumulating() || !IsRunning() ) {
		throw std::runtime_error( "Worker knowledge publication requires its running owning accumulation" );
	}
	ValidateSmacWorkerKnowledge( knowledge );
	auto topology = m_map->m_territory_topology;
	if ( topology ) { topology->ValidateTerrain( m_map->TerritorySea() ); }
	else { topology = m_map->PrepareFreshTerritoryTopology(); }
	// Initial fresh-world admission can follow diagnostic/legacy-script Base
	// construction. Establish their radius marks once; later publication and
	// restore must retain the owned history without replaying these writers.
	if(!m_map->m_worker_knowledge && topology->SiteRatings()) {
		auto sites=*topology->SiteRatings();
		for(const auto id:m_bm->GetBaseRecordOrder()) {
			const auto* base=m_bm->GetBase(id);if(!base) {throw std::runtime_error("Initial site-radius Base order is incomplete");}
			const auto* center=base->GetTile();sites=sites.FoundBase({{static_cast<int32_t>(center->coord.x),static_cast<int32_t>(center->coord.y)}});
		}
		topology=topology->WithSiteRatings(std::move(sites));
	}
	std::optional< map::SmacWorkerKnowledge > replacement = knowledge;
	static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
	static_assert( std::is_nothrow_swappable_v< decltype( topology ) > );
	m_map->m_worker_knowledge.swap( replacement );
	m_map->m_territory_topology.swap( topology );
	m_map->m_worker_knowledge_ready = true;
}

void Game::CompleteSmacWorkerKnowledgeRestore() {
	if ( m_map && m_map->m_worker_knowledge ) {
		if ( !m_map->m_territory_topology ) { throw std::runtime_error( "Worker knowledge lacks saved territory topology" ); }
		m_map->m_territory_topology->ValidateTerrain( m_map->TerritorySea() );
        auto data=m_map->m_worker_knowledge->CopyData();
        if(data.occupancy) {
            std::vector<map::SmacOccupancyCache::Entity> units,bases;
            for(const auto id:m_um->GetUnitRecordOrder()) {const auto* unit=m_um->GetUnit(id);const auto* tile=unit->GetTile();
                units.push_back({{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},unit->SimulationOwner()});}
            for(const auto id:m_bm->GetBaseRecordOrder()) {const auto* base=m_bm->GetBase(id);const auto* tile=base->GetTile();
                bases.push_back({{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},base->GetSmacResourceContext().owner_simulation_id});}
            data.occupancy->Repair(units,bases);
            auto repaired=std::optional<map::SmacWorkerKnowledge>(map::SmacWorkerKnowledge::Create(data));
            ValidateSmacWorkerKnowledge(*repaired);m_map->m_worker_knowledge.swap(repaired);
        }
		ValidateSmacWorkerKnowledge( *m_map->m_worker_knowledge );
		m_map->m_worker_knowledge_ready = true;
	}
}

map::SmacWorkerMapFrame::Evaluation Game::GetSmacWorkerAvailability( const base::Base& base ) const {
	if ( !m_map || !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready ) { throw std::runtime_error( "Live worker knowledge is unavailable" ); }
	size_t index = 0;
	for ( const auto& entry : m_bm->GetBases() ) {
		if ( entry.second == &base ) { return ProjectSmacWorkerKnowledge( *m_map->m_worker_knowledge, base.GetSmacResourceContext().owner_simulation_id ).Evaluate( index ); }
		++index;
	}
	throw std::runtime_error( "Worker query Base is not registered in this world" );
}

void Game::RequireConventionalSightWorld() const {
	if ( m_map && m_map->m_worker_knowledge ) {
		if ( !m_map->m_worker_knowledge_ready ) { throw std::runtime_error("Sight requires completely bound worker knowledge"); }
		ValidateSmacWorkerKnowledge(*m_map->m_worker_knowledge);
	}
}

std::optional< map::SmacWorkerKnowledge > Game::PrepareConventionalZoc( const unit::Unit& actor, const map::tile::Tile* destination ) const {
	const auto* source = actor.GetTile();
	const auto owner = actor.Conventional().owner_simulation_id;
	bool foreign = false;
	for ( const auto* endpoint : {source,destination} ) {
		for ( const auto* neighbour : endpoint->neighbours ) {
			for ( const auto& entry : neighbour->units ) {
				if (!entry.second || (!entry.second->IsConventional() && !entry.second->IsNative())) {throw std::runtime_error("ZOC requires canonical foreign units");}
				foreign |= entry.second->SimulationOwner() != owner;
			}
		}
	}
	if ( !foreign ) { return {}; }
	if ( ResolveConventionalUnit( actor.Conventional(), actor.m_owner ).package != unit::ConventionalUnitContext::Package::Combat ) {
		throw std::runtime_error("Foreign ZOC for noncombat movement is not established");
	}
	if ( !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready ) { throw std::runtime_error("Foreign ZOC requires authenticated saved knowledge and directed Pact state"); }
	auto data = m_map->m_worker_knowledge->CopyData();
	if ( !data.observers[owner] || !data.observers[owner]->pact_with ) { throw std::runtime_error("Directed Pact state is unavailable for ZOC"); }
	const auto pacts = *data.observers[owner]->pact_with;
	const auto ordinary = [this](const map::tile::Tile* tile) {
		if ( tile->is_water_tile || tile->rockiness != map::tile::ROCKINESS_FLAT || tile->features
			|| (tile->terraforming & ~map::tile::TERRAFORMING_ROAD) || tile->coord.x<2 || tile->coord.x+2>=m_map->GetWidth()
			|| tile->coord.y<2 || tile->coord.y+2>=m_map->GetHeight() ) {
			throw std::runtime_error("Foreign ZOC requires measured ordinary interior land");
		}
	};
	ordinary(source); ordinary(destination);
	// Current conventional definitions admit ordinary, unconcealed Scout/Rover
	// only. Validate every relevant stack before deciding a normal refusal.
	for ( const auto* endpoint : {source,destination} ) { for ( const auto* neighbour : endpoint->neighbours ) {
		for ( const auto& entry : neighbour->units ) {
			const auto* other = entry.second;
			if ( other->SimulationOwner() == owner ) { continue; }
			if (other->IsNative()) { (void)m_um->QueryNativeUnit(other->Native()); }
            else if ( ResolveConventionalUnit( other->Conventional(), other->m_owner ).package != unit::ConventionalUnitContext::Package::Combat ) {
				throw std::runtime_error("Foreign noncombat ZOC influence is not established");
			}
			ordinary(neighbour);
			if ( neighbour->units.size()!=1 ) { throw std::runtime_error("Foreign ZOC mixed or multiple stacks are not admitted"); }
		}
	} }
	const auto controlled = [&](const map::tile::Tile* endpoint) {
		if ( endpoint->base ) { return false; } // independently authenticated owned endpoint
		for ( const auto* neighbour : endpoint->neighbours ) {
			if ( neighbour->is_water_tile != endpoint->is_water_tile ) { continue; }
			for ( const auto& entry : neighbour->units ) {
				const auto faction = entry.second->SimulationOwner();
				if ( faction != owner && !(pacts & (1u<<faction)) ) { return true; }
			}
		}
		return false;
	};
	if ( !controlled(source) || !controlled(destination) ) { return {}; }
	// This is an accepted command whose earlier ZOC stage refuses movement.
	// Only its unseen non-Pact destination neighbors are revealed. Validation
	// returns this owned candidate without publishing it or running ordinary sight.
	for ( const auto* neighbour : destination->neighbours ) { for ( const auto& entry : neighbour->units ) {
		const auto faction = entry.second->SimulationOwner();
		if ( faction==owner || (pacts & (1u<<faction)) ) { continue; }
		auto& observation = *std::find_if(data.units.begin(),data.units.end(),[&](const auto& value){return value.id==entry.first;});
		const auto bit = uint8_t(1u<<owner);
		if ( observation.visible_to & bit ) { continue; }
		observation.visible_to |= bit;
		const auto index = static_cast<size_t>(neighbour->coord.y)*(data.width/2)+neighbour->coord.x/2;
		data.tiles[index].explored_by |= bit;
		auto& remembered = data.remembered[owner]; if ( remembered.empty() ) { remembered.resize(data.tiles.size()); }
		remembered[index] = map::SmacWorkerKnowledge::Improvements{neighbour->features,neighbour->terraforming,neighbour->base!=nullptr,true};
	} }
	return map::SmacWorkerKnowledge::Create(data);
}

std::optional< map::SmacWorkerKnowledge > Game::PrepareConventionalSight( uint64_t id, int32_t owner,
	const map::tile::Tile* destination, const map::tile::Tile* source, bool startup ) const {
	if ( !m_map || !m_map->m_worker_knowledge ) { return {}; }
	if ( startup ) {
		RequireConventionalStartupClaim();
		if ( source || !m_map->m_worker_knowledge_ready ) { throw std::runtime_error("Startup sight requires initial placement and bound knowledge"); }
		const auto prior = m_map->m_worker_knowledge->CopyData();
		for ( int32_t observer = 1; observer <= 7; ++observer ) {
			if ( prior.observers[observer] ) { (void)ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,observer,nullptr,true); }
		}
	} else { RequireConventionalSightWorld(); }
	auto data = m_map->m_worker_knowledge->CopyData();
	if ( owner < 1 || owner > 7 || !data.observers[owner] || !destination || destination->tiles != m_map->GetTilesPtr()
		|| destination->coord.x >= m_map->GetWidth() || destination->coord.y >= m_map->GetHeight()
		|| m_map->GetTile(destination->coord.x,destination->coord.y) != destination || destination->is_water_tile ) {
		throw std::runtime_error("Ordinary sight requires a canonical land position and supplied observer");
	}
	if ( !source ) { for ( const auto& entry : destination->units ) {
		const auto* other = entry.second;
		if ( !other || m_um->GetUnit(entry.first) != other || other->m_id != entry.first || other->GetTile() != destination
			|| !other->IsConventional() || other->Conventional().owner_simulation_id != owner ) {
			throw std::runtime_error("Ordinary sight placement requires a canonical same-owner stack");
		}
	} }
	map::smac_exploration::Input input{data.width,data.height,data.horizontal_wrap,
		{static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},owner,{},{}};
	input.sea.reserve(data.tiles.size()); input.explored.reserve(data.tiles.size());
	for ( int32_t y=0; y<data.height; ++y ) { for ( int32_t x=y&1; x<data.width; x+=2 ) {
		const auto index=static_cast<size_t>(y)*(data.width/2)+x/2;
		input.sea.push_back(m_map->GetTile(x,y)->is_water_tile); input.explored.push_back(data.tiles[index].explored_by);
	} }
	const auto sight=map::smac_exploration::Explore(input);
	for ( size_t i=0; i<data.tiles.size(); ++i ) { data.tiles[i].explored_by=sight.explored[i]; }
	auto& remembered=data.remembered[owner]; if ( remembered.empty() ) { remembered.resize(data.tiles.size()); }
	for ( const auto& coordinate : sight.refresh_improvements ) {
		const auto* tile=m_map->GetTile(coordinate.x,coordinate.y);
		bool occupied=!tile->units.empty();
		if ( tile==source && tile->units.size()==1 && tile->units.count(id) ) { occupied=false; }
		if ( tile==destination ) { occupied=true; }
		remembered[static_cast<size_t>(coordinate.y)*(data.width/2)+coordinate.x/2]=map::SmacWorkerKnowledge::Improvements{
			tile->features,tile->terraforming,tile->base!=nullptr,occupied};
	}
	if ( source ) {
		auto found=std::find_if(data.units.begin(),data.units.end(),[&](const auto& value){return value.id==id;});
		if ( found==data.units.end() ) { throw std::runtime_error("Moving sight unit has no saved observation"); }
		found->visible_to=0; // moving playable stack clears before detection
	} else {
		for ( const auto& value:data.units ) { if(value.id==id) { throw std::runtime_error("New sight identity already exists"); } }
		data.units.push_back({id,0,false});
	}
	// Stored visibility is updated by this event, never rebuilt globally. The
	// receipt admits unobscured flat ordinary land; sensor/edge/concealment
	// branches remain explicit failures when foreign detection is relevant.
	auto& moving=*std::find_if(data.units.begin(),data.units.end(),[&](const auto& value){return value.id==id;});
	const auto adjacent=[](const map::tile::Tile* a,const map::tile::Tile* b) {
		const auto dx=std::abs(int64_t(a->coord.x)-int64_t(b->coord.x));
		const auto dy=std::abs(int64_t(a->coord.y)-int64_t(b->coord.y));
		return dx+dy==2;
	};
	const auto ordinary=[&](const map::tile::Tile* tile) {
		if ( tile->is_water_tile || tile->rockiness!=map::tile::ROCKINESS_FLAT || tile->features!=map::tile::FEATURE_NONE
			|| tile->terraforming!=map::tile::TERRAFORMING_NONE || tile->coord.x<2 || tile->coord.x+2>=m_map->GetWidth()
			|| tile->coord.y<2 || tile->coord.y+2>=m_map->GetHeight() ) { throw std::runtime_error("Foreign sight requires evidenced unobscured interior flat land"); }
	};
	bool foreign=false;
	for ( const auto& entry:m_um->GetUnits() ) {
        foreign |= entry.second->IsConventional() && entry.second->Conventional().owner_simulation_id!=owner;
    }
	for ( const auto& entry:m_bm->GetBases() ) { foreign |= entry.second->GetSmacResourceContext().owner_simulation_id!=owner; }
	if ( foreign ) {
		ordinary(destination);
		for ( int32_t y=0;y<data.height;++y ) { for(int32_t x=y&1;x<data.width;x+=2) {
			if(m_map->GetTile(x,y)->terraforming & map::tile::TERRAFORMING_SENSOR) { throw std::runtime_error("Foreign sight sensor-range producer is not admitted"); }
		} }
	}
	for ( const auto& entry:m_um->GetUnits() ) {
		const auto* other=entry.second; const auto faction=other->SimulationOwner();
		if(entry.first==id || faction==owner) {continue;}
		const auto* tile=other->GetTile();
		if(other->IsNative()) {
            // Native detection is adjacent-eight on all five captured land
            // terrain families. It does not inherit positive-owner flat-land
            // restrictions or globally clear stationary masks.
            const auto interior=[&](const map::tile::Tile* position) {
                if(position->is_water_tile || position->coord.x<2 || position->coord.x+2>=m_map->GetWidth()
                    || position->coord.y<2 || position->coord.y+2>=m_map->GetHeight()) {
                    throw std::runtime_error("Native detection requires interior land geometry");
                }
            };
            if(destination!=tile && !destination->IsAdjactentTo(tile)) {continue;}
            interior(destination);interior(tile);
            if(destination==tile || tile->units.size()!=1) {throw std::runtime_error("Native detection requires distinct singleton placement");}
            if(adjacent(destination,tile)) {
                moving.visible_to |= 1u; // Faction0 is independent of controller slot0.
                auto& observation=*std::find_if(data.units.begin(),data.units.end(),[&](const auto& value){return value.id==entry.first;});
                observation.visible_to |= static_cast<uint8_t>(1u<<owner);
            }
            continue;
        }
        // No wrapping foreign detection has been established, even when the
		// exploration kernel independently supports periodic-X geometry.
		if(tile->coord.x<2 || tile->coord.x+2>=m_map->GetWidth() || tile->coord.y<2 || tile->coord.y+2>=m_map->GetHeight()) {ordinary(tile);}
		if(adjacent(destination,tile) || destination==tile) {
			ordinary(tile); if(tile->units.size()!=1 || destination==tile) {throw std::runtime_error("Foreign sight stacks are not admitted");}
			moving.visible_to |= uint8_t(1u<<faction);
			auto& observation=*std::find_if(data.units.begin(),data.units.end(),[&](const auto& value){return value.id==entry.first;});
			observation.visible_to |= uint8_t(1u<<owner);
		}
	}
	for ( const auto& entry:m_bm->GetBases() ) {
		const auto* base=entry.second; const auto faction=base->GetSmacResourceContext().owner_simulation_id;
		if(faction==owner) {continue;}
		if(base->GetTile()==destination) {throw std::runtime_error("Foreign Base entry is not admitted");}
		const auto* center=base->GetTile();
		if(center->coord.x<2 || center->coord.x+2>=m_map->GetWidth() || center->coord.y<2 || center->coord.y+2>=m_map->GetHeight()) {ordinary(center);}
		if(adjacent(destination,center)) {ordinary(center); moving.visible_to |= uint8_t(1u<<faction);}
	}
    if(data.occupancy) {
        if(source) {data.occupancy->RemoveUnit({static_cast<int32_t>(source->coord.x),static_cast<int32_t>(source->coord.y)},source->units.size()==1 && source->units.count(id));}
        data.occupancy->PlaceUnit({static_cast<int32_t>(destination->coord.x),static_cast<int32_t>(destination->coord.y)},owner,destination->units.empty());
    }
	return map::SmacWorkerKnowledge::Create(data);
}

void Game::CommitConventionalSight( std::optional< map::SmacWorkerKnowledge >& prepared ) noexcept {
	if ( prepared ) {
        m_map->m_worker_knowledge.swap(prepared);
        m_um->SyncNativeObservationMasks(*m_map->m_worker_knowledge);
    }
}

bool Game::HandleConventionalMoveEvent( GSE_CALLABLE, const std::string& name,
	const gse::value::object_properties_t& args, size_t caller, bool apply ) {
	if ( !m_map || !m_map->m_worker_knowledge || name!="move_unit" ) { return false; }
	const auto found=args.find("unit"), target=args.find("tile");
	if ( found==args.end() || !found->second ) { return false; }
	const auto* object=found->second->Deref();
	if(object->type==gse::VT_UNDEFINED) {
		// A casualty unlinks every existing Unit wrapper. A second direction
		// already in the extracted batch must not bind to a compacted survivor.
		const auto* retired=dynamic_cast<const gse::value::Object*>(object);
		if(retired && retired->object_class==unit::Unit::WRAP_CLASS && !retired->wrapobj) {
			if(caller!=m_slot_num || args.size()!=2 || target==args.end() || !target->second) {throw std::runtime_error("Retired move has invalid authority or arguments");}
			return true;
		}
	}
	if ( object->type!=gse::VT_OBJECT || static_cast<const gse::value::Object*>(object)->object_class!=unit::Unit::WRAP_CLASS ) { return false; }
	auto* unit=unit::Unit::Unwrap(found->second);
	if ( !unit->IsConventional() ) { return false; }
	if ( args.size()!=2 || target==args.end() || !target->second ) { throw std::runtime_error("Conventional move requires only Unit and tile"); }
	auto* tile=map::tile::Tile::Unwrap(target->second);
	if ( apply ) { m_um->MoveConventionalUnit(GSE_CALL,unit,tile,caller); }
	else { m_um->ValidateConventionalMove(unit,tile,caller); }
	return true;
}

void Game::RequireWorkerKnowledgeMutation() const {
	if ( m_map && m_map->m_worker_knowledge ) { throw std::runtime_error( "This mutation requires an integrated worker sight/territory producer" ); }
}

}
