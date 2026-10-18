#include "UnitManager.h"

#include "MoraleSet.h"
#include "Def.h"
#include "StaticDef.h"
#include "Unit.h"
#include <memory>
#include "gc/Space.h"
#include "util/FinallyGuard.h"

#include "game/backend/Game.h"
#include "game/backend/animation/AnimationManager.h"
#include "game/backend/State.h"
#include "game/backend/Bindings.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/SpriteRender.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "FormerMarkerRender.h"
#include "ColonyMarkerRender.h"
#include "game/backend/Player.h"

#include "gse/context/Context.h"
#include "gse/callable/Native.h"
#include "gse/value/Bool.h"
#include "gse/value/Float.h"
#include "gse/value/Array.h"
#include "game/backend/map/tile/TileManager.h"
#include "game/backend/map/Map.h"
#include <memory>
#include <cmath>
#include <limits>
#include <algorithm>
#include <type_traits>

namespace game {
namespace backend {
namespace unit {

static const std::string s_snapshot_format = "GLSMAC_UNITS_V4";

static size_t ReadSnapshotCount( types::Buffer& buf ) {
	const auto count = buf.ReadInt();
	// Every record contains at least one typed Buffer frame (six bytes).
	if ( count < 0 || buf.lenr > buf.lenw || static_cast< uint64_t >( count ) > ( buf.lenw - buf.lenr ) / 6 ) {
		THROW( "invalid unit snapshot record count" );
	}
	return static_cast< size_t >( count );
}

UnitManager::UnitManager( Game* game )
	: gse::GCWrappable( game->GetGCSpace() )
	, m_game( game ) {
	//
}

UnitManager::~UnitManager() {
	Clear();
}

void UnitManager::Clear() {
	for ( auto& it : m_unprocessed_units ) {
		delete it.unit;
	}
	m_unprocessed_units.clear();

	for ( auto& it : m_units ) {
		delete it.second;
	}
	m_units.clear();
	m_unit_order.clear();

	for ( auto& it : m_unit_defs ) {
		delete it.second;
	}
	m_unit_defs.clear();
	for ( auto& it : m_unit_moralesets ) {
		delete it.second;
	}
	m_unit_moralesets.clear();

	m_unit_updates.clear();
}

void UnitManager::DefineMoraleSet( MoraleSet* moraleset ) {
	Log( "Defining unit moraleset ('" + moraleset->m_id + "')" );

	ASSERT( m_unit_moralesets.find( moraleset->m_id ) == m_unit_moralesets.end(), "Unit moraleset '" + moraleset->m_id + "' already exists" );

	m_unit_moralesets.insert(
		{
			moraleset->m_id,
			moraleset
		}
	);
}

void UnitManager::UndefineMoraleSet( const std::string& id ) {
	Log( "Undefining unit moraleset ('" + id + "')" );

	const auto found = m_unit_moralesets.find( id );
	if ( found == m_unit_moralesets.end() ) { THROW( "unit morale set not found: " + id ); }
	for ( const auto& entry : m_unit_defs ) {
		if ( entry.second->m_moraleset == found->second ) { THROW( "unit morale set is still in use: " + id ); }
	}
	delete found->second;
	m_unit_moralesets.erase( found );
}

void UnitManager::DefineUnit( Def* def ) {
	Log( "Defining unit ('" + def->m_id + "')" );

	ASSERT( m_unit_defs.find( def->m_id ) == m_unit_defs.end(), "Unit definition '" + def->m_id + "' already exists" );

	m_unit_defs.insert(
		{
			def->m_id,
			def
		}
	);

	auto fr = FrontendRequest( FrontendRequest::FR_UNIT_DEFINE );
	NEW( fr.data.unit_define.serialized_unitdef, std::string, Def::Serialize( def ).ToString() );
	m_game->AddFrontendRequest( fr );
}

void UnitManager::UndefineUnit( const std::string& id ) {
	Log( "Undefining unit ('" + id + "')" );

	const auto found = m_unit_defs.find( id );
	if ( found == m_unit_defs.end() ) { THROW( "unit definition not found: " + id ); }
	for ( const auto& entry : m_units ) {
		if ( entry.second->m_def == found->second ) { THROW( "unit definition is still in use: " + id ); }
	}
	for ( const auto& pending : m_unprocessed_units ) {
		if ( pending.unit->m_def == found->second ) { THROW( "unit definition is still in use: " + id ); }
	}
	delete found->second;
	m_unit_defs.erase( found );

	auto fr = FrontendRequest( FrontendRequest::FR_UNIT_UNDEFINE );
	NEW( fr.data.unit_undefine.id, std::string, id );
	m_game->AddFrontendRequest( fr );
}

void UnitManager::SpawnUnit( GSE_CALLABLE, Unit* unit ) {
	m_game->RequireSmacGameplayActive();
	m_game->RequireWorkerKnowledgeMutation();
	if ( !m_game->IsRunning() ) {
		m_unprocessed_units.push_back( { unit, true } );
		return;
	}
	RegisterUnit( unit );
	TriggerSpawnHook( GSE_CALL, unit );
}

ConventionalUnitContext UnitManager::ResolveConventional( const ConventionalUnitState& state, const slot::Slot* owner ) const {
	return m_game->ResolveConventionalUnit( state, owner );
}

void UnitManager::CommitPlacementHistory(const std::pair<SmacMovementHistory*,size_t>& prepared) const noexcept {
	if(prepared.first) {prepared.first->MarkPlaced(prepared.second);}
}

std::pair< SmacMovementHistory*, size_t > UnitManager::PreparePlacementHistory( const Unit& unit, const map::tile::Tile* tile ) const {
	if ( !m_game->m_simulation || !m_game->m_simulation->value.movement_history || unit.m_def->GetMovementType() == MT_AIR ) { return { nullptr, 0 }; }
	m_game->ValidateSimulation( *m_game->m_simulation );
	auto& history = *m_game->m_simulation->value.movement_history;
	const auto* map = GetMap();
	if ( !map || !tile || tile->coord.x >= map->GetWidth() || tile->coord.y >= map->GetHeight()
		|| map->GetTile( tile->coord.x, tile->coord.y ) != tile ) { THROW( "Placement history requires a canonical world tile" ); }
	history.ValidateDimensions( map->GetWidth(), map->GetHeight() );
	return { &history, history.PlacementIndex( static_cast< int32_t >( tile->coord.x ), static_cast< int32_t >( tile->coord.y ) ) };
}

void UnitManager::ValidateConventionalCreation( size_t id, const Def* def, const slot::Slot* owner, const map::tile::Tile* tile,
	const ConventionalUnitState& state, const ConventionalUnitResolver& resolver, bool early_startup, bool defer_home ) const {
	if ( !resolver || id == 0 || id >= static_cast< uint64_t >( INT64_MAX ) || id >= std::numeric_limits< size_t >::max()
		|| HasUnitId( id ) || !tile || tile->units.count( id ) || !GetMap()
		|| tile->coord.x >= GetMap()->GetWidth() || tile->coord.y >= GetMap()->GetHeight()
		|| ( tile->coord.x & 1 ) != ( tile->coord.y & 1 ) || GetMap()->GetTile( tile->coord.x, tile->coord.y ) != tile ) {
		THROW( "Invalid conventional unit ID, world tile or restore context" );
	}
	bool canonical = false;
	for ( const auto& entry : m_unit_defs ) { if ( entry.second == def ) { canonical = true; break; } }
	if ( !canonical || def->m_type != DT_STATIC || !def->m_moraleset || def->m_moraleset != GetMoraleSet( def->m_moraleset->m_id ) ) {
		THROW( "Conventional presentation definition is not canonical" );
	}
	const auto context = resolver( state, owner );
	if ( state.damage >= 10 * context.reactor ) { THROW( "Conventional living unit damage exceeds health" ); }
	if ( !early_startup && !( defer_home && state.home == ConventionalUnitState::Home::Base ) ) { QueryConventionalUnit( state, context, state.render_morale_mode ); }
	const auto* presentation = static_cast< const StaticDef* >( def );
	const auto* render = dynamic_cast< const CvrScoutRender* >( presentation->m_render );
	using Ground = settings::SmacMovementRules::GroundChassis;
	using Profile = ConventionalUnitContext::Presentation;
	const bool scout = context.chassis == Ground::Infantry && context.presentation == Profile::Scout && render
		&& ( ( render->m_type == Render::RT_CVR_SCOUT && render->composition == CvrScoutRender::Composition::Scout )
			|| ( render->m_type == Render::RT_CVR_INITIALIZED_SCOUT && render->composition == CvrScoutRender::Composition::InitializedScout ) );
	const bool rover = context.chassis == Ground::Speeder && context.presentation == Profile::SelectedSpartanRover && render
		&& render->m_type == Render::RT_CVR_SPARTAN_ROVER && render->composition == CvrScoutRender::Composition::SpartanRover;
	const bool former = context.chassis == Ground::Infantry && context.presentation == Profile::FormerMarker
		&& presentation->m_render->m_type == Render::RT_PORT_FORMER_MARKER
		&& dynamic_cast< const FormerMarkerRender* >( presentation->m_render );
	const bool colony = context.chassis == Ground::Infantry && context.presentation == Profile::ColonyMarker
		&& presentation->m_render->m_type == Render::RT_PORT_COLONY_MARKER
		&& dynamic_cast< const ColonyMarkerRender* >( presentation->m_render );
	if ( context.reactor != 1 || presentation->m_movement_type != MT_LAND || ( !scout && !rover && !former && !colony ) ) {
		THROW( "Conventional design has no compatible explicit presentation" );
	}
}


struct UnitManager::PreparedConventionalUnit::Data {
	UnitManager* manager;
	map::tile::Tile* tile;
	std::unique_ptr< Unit > candidate;
	std::map< size_t, Unit* > units, occupants;
	std::vector< size_t > order;
	std::unordered_map< size_t, unit_update_t > updates;
	std::optional< map::SmacWorkerKnowledge > sight;
	std::pair< SmacMovementHistory*, size_t > history;
	std::vector< std::pair<Unit*,ConventionalUnitState> > homes;
	Unit* published = nullptr;
	size_t id = 0;
	bool notified = false;
};
UnitManager::PreparedConventionalUnit::PreparedConventionalUnit( std::unique_ptr< Data > data ) : m_data( std::move( data ) ) {}
UnitManager::PreparedConventionalUnit::~PreparedConventionalUnit() = default;
UnitManager::PreparedConventionalUnit::PreparedConventionalUnit( PreparedConventionalUnit&& ) noexcept = default;
UnitManager::PreparedConventionalUnit& UnitManager::PreparedConventionalUnit::operator=( PreparedConventionalUnit&& ) noexcept = default;
const Unit& UnitManager::PreparedConventionalUnit::Candidate() const {
	if ( !m_data || !m_data->candidate ) { THROW( "Prepared unit candidate is unavailable" ); }
	return *m_data->candidate;
}
void UnitManager::PreparedConventionalUnit::PrepareBaseAbandonment( const base::Base& abandoned,
    const base::Base& replacement, const map::SmacWorkerKnowledge& territory ) {
    if ( !m_data || !m_data->candidate || m_data->published || !m_data->homes.empty() ) {
        THROW( "Abandonment requires its uncommitted newborn" );
    }
    auto& data=*m_data;
    auto* game=data.manager->m_game;
    auto& newborn=*data.candidate;
    auto state=newborn.Conventional();
    if ( newborn.m_owner!=abandoned.m_owner || replacement.m_owner!=abandoned.m_owner
        || data.tile!=abandoned.GetTile() || state.home_base_id!=abandoned.m_id
        || game->ResolveConventionalUnit(state,newborn.m_owner).package!=ConventionalUnitContext::Package::Colony
        || !data.sight ) { THROW( "Abandonment requires the actual Colony factory and territory" ); }
    state.home=ConventionalUnitState::Home::None;
    state.home_base_id.reset();
    (void)game->ResolveConventionalUnit(state,newborn.m_owner);
    data.homes.reserve(data.manager->m_units.size());
    for ( const auto id:data.manager->m_unit_order ) {
        auto* current=data.manager->GetUnit(id);
        if ( !current || !current->IsConventional() ) { THROW( "Abandonment requires current conventional Unit records" ); }
        auto home=current->Conventional();
        if ( home.home_base_id!=abandoned.m_id ) { continue; }
        if ( current->m_owner!=abandoned.m_owner ) { THROW( "Foreign captured home repair is not admitted" ); }
        home.home=ConventionalUnitState::Home::Base;
        home.home_base_id=replacement.m_id;
        (void)game->ResolveConventionalUnit(home,current->m_owner);
        data.homes.emplace_back(current,home);
        auto& update=data.updates.try_emplace(id,unit_update_t{UUO_NONE,current}).first->second;
        if ( update.ops&UUO_DESPAWN ) { THROW( "Rehomed Unit has a pending removal" ); }
        update.ops=static_cast<unit_update_op_t>(update.ops|UUO_REFRESH);
    }
    auto knowledge=data.sight->CopyData();
    const auto claims=territory.CopyData();
    if ( knowledge.width!=claims.width || knowledge.height!=claims.height
        || knowledge.tiles.size()!=claims.tiles.size() ) { THROW( "Abandonment territory geometry differs" ); }
    for ( size_t i=0;i<knowledge.tiles.size();++i ) { knowledge.tiles[i].claimant=claims.tiles[i].claimant; }
    knowledge.bases.erase(std::remove_if(knowledge.bases.begin(),knowledge.bases.end(),[&](const auto& base) {
        return base.id==abandoned.m_id;
    }),knowledge.bases.end());
    // Preserve the factory's visibility, exploration, remembered terrain and
    // new Unit observation. The territory producer changes claims only.
    data.sight=map::SmacWorkerKnowledge::Create(knowledge);
    std::get<ConventionalUnitState>(newborn.m_state)=state;
}
Unit* UnitManager::PreparedConventionalUnit::Commit() noexcept {
	if ( !m_data || !m_data->candidate ) { return nullptr; }
	auto& data = *m_data;
	static_assert( std::is_nothrow_copy_assignable_v<ConventionalUnitState> );
	for ( const auto& home : data.homes ) { std::get<ConventionalUnitState>( home.first->m_state ) = home.second; }
	data.published = data.candidate.release();
	data.manager->m_units.swap( data.units ); data.manager->m_unit_updates.swap( data.updates ); data.tile->units.swap( data.occupants );
	data.manager->m_unit_order.swap( data.order );
	data.published->CommitInitialPlacement( data.tile );
	if ( data.history.first ) { data.history.first->MarkPlaced( data.history.second ); }
	data.manager->m_game->CommitConventionalSight( data.sight );
	return data.published;
}
void UnitManager::PreparedConventionalUnit::Notify( GSE_CALLABLE ) {
	if ( !m_data || !m_data->published || m_data->notified ) { THROW( "Unit creation notification is not pending" ); }
	auto& data = *m_data;
	data.manager->m_game->CheckRW( GSE_CALL );
	if ( data.manager->GetUnit( data.id ) != data.published ) { THROW( "Prepared unit no longer belongs to its manager" ); }
	data.notified = true;
	data.manager->TriggerSpawnHook( GSE_CALL, data.published );
}


struct UnitManager::PreparedConventionalRemoval::Data {
    UnitManager* manager;
    std::vector<Unit*> removed;
    std::vector<size_t> removed_ids;
    std::map<size_t,Unit*> units;
    std::vector<size_t> order;
    std::unordered_map<size_t,unit_update_t> updates;
    std::map<map::tile::Tile*,std::map<size_t,Unit*>> occupants;
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::optional<std::unordered_map<size_t,base::BaseManager::base_update_t>> base_updates;
    std::vector<std::unique_ptr<Unit>> retired;
    bool committed=false;
};
UnitManager::PreparedConventionalRemoval::PreparedConventionalRemoval(std::unique_ptr<Data> data):m_data(std::move(data)) {}
UnitManager::PreparedConventionalRemoval::~PreparedConventionalRemoval() = default;
UnitManager::PreparedConventionalRemoval::PreparedConventionalRemoval(PreparedConventionalRemoval&&) noexcept = default;
UnitManager::PreparedConventionalRemoval& UnitManager::PreparedConventionalRemoval::operator=(PreparedConventionalRemoval&&) noexcept = default;
UnitManager::PreparedConventionalRemoval UnitManager::PrepareConventionalRemoval(GSE_CALLABLE,const std::vector<size_t>& ids,PreparedConventionalUnit* created,Unit* battle_survivor) {
    m_game->RequireSmacGameplayActive();m_game->CheckRW(GSE_CALL);
    if(gc_space!=m_game->GetGCSpace()) {THROW("Removal requires the owning accumulation");}
    return PrepareRemovalForQuery(ids,created,battle_survivor);
}
UnitManager::PreparedConventionalRemoval UnitManager::PrepareRemovalForQuery(const std::vector<size_t>& ids,PreparedConventionalUnit* created,Unit* battle_survivor) {
    m_game->RequireSmacGameplayActive();
    if(!m_game->GetGCSpace()->IsAccumulating() || !m_game->IsRunning()) {THROW("Removal requires the owning running accumulation");}
    m_game->RequireConventionalSightWorld();
    const auto* creation=created ? created->m_data.get() : nullptr;
    if(created && (!creation || creation->manager!=this || !creation->candidate || creation->published)) {THROW("Removal requires an uncommitted same-manager creation");}
    auto data=std::make_unique<PreparedConventionalRemoval::Data>(); data->manager=this;
    data->units=creation ? creation->units : m_units; data->order=creation ? creation->order : m_unit_order;
    data->updates=creation ? creation->updates : m_unit_updates;
    data->knowledge=creation && creation->sight ? creation->sight : m_game->GetMap()->GetSmacWorkerKnowledge();
    auto knowledge=data->knowledge ? std::optional<map::SmacWorkerKnowledge::Data>(data->knowledge->CopyData()) : std::nullopt;
    data->removed.reserve(ids.size()); data->retired.resize(ids.size()); data->removed_ids=ids;
    for(const auto id:ids) {
        const auto found=data->units.find(id);
        if(found==data->units.end() || !found->second || !found->second->IsConventional()
            || std::count(data->order.begin(),data->order.end(),id)!=1) {THROW("Removal has duplicate or unknown Unit identity");}
        auto* unit=found->second;
        const bool newborn=creation && unit==creation->candidate.get();
        unit->RequireConventionalReady(); (void)unit->QueryStats(SmacMoraleMode::Mode1);
        auto* tile=newborn ? creation->tile : unit->GetTile();
        if(!tile || tile->tiles!=m_game->GetMap()->GetTilesPtr() || tile->IsLocked() || unit->m_animation_id) {THROW("Removal Unit has unavailable placement");}
        auto inserted=data->occupants.try_emplace(tile,creation && tile==creation->tile ? creation->occupants : tile->units);
        auto& occupants=inserted.first->second;
        if(!occupants.count(id) || occupants.at(id)!=unit) {THROW("Removal Unit membership differs");}
        occupants.erase(id); data->units.erase(found);
        data->order.erase(std::find(data->order.begin(),data->order.end(),id));
        const auto old=data->updates.find(id);
        if(old!=data->updates.end() && (old->second.ops & UUO_SPAWN)) {data->updates.erase(old);}
        else {data->updates[id]={UUO_DESPAWN,nullptr};}
        if(knowledge) {
            const auto row=std::find_if(knowledge->units.begin(),knowledge->units.end(),[&](const auto& item){return item.id==id;});
            if(row==knowledge->units.end()) {THROW("Removal Unit observation is absent");}
            if(battle_survivor) {
                const auto visible=static_cast<uint8_t>(row->visible_to | (1u<<unit->Conventional().owner_simulation_id));
                const auto index=static_cast<size_t>(tile->coord.y)*(knowledge->width/2)+tile->coord.x/2;
                for(size_t observer=1;observer<8;++observer) {
                    auto& memory=knowledge->remembered[observer];
                    if((visible & (1u<<observer)) && !memory.empty() && memory.at(index)) {memory.at(index)->unit_present=!occupants.empty();}
                }
            }
            knowledge->units.erase(row);
        }
        data->removed.push_back(unit);
    }
    if(battle_survivor) {
        if(created || GetUnit(battle_survivor->m_id)!=battle_survivor || !data->units.count(battle_survivor->m_id)) {THROW("Battle removal requires its current surviving Unit");}
        auto& update=data->updates.try_emplace(battle_survivor->m_id,unit_update_t{UUO_NONE,battle_survivor}).first->second;
        if(update.unit!=battle_survivor || (update.ops & UUO_DESPAWN)) {THROW("Battle survivor has an incompatible pending update");}
        update.ops=static_cast<unit_update_op_t>(update.ops|UUO_REFRESH);
        data->base_updates=m_game->GetBM()->m_base_updates;
        for(const auto* unit:data->removed) {
            if(unit->Conventional().home_base_id) {
                auto* home=m_game->GetBM()->GetBase(*unit->Conventional().home_base_id);
                if(!home) {THROW("Battle victim has no current home Base");}
                base::BaseManager::QueueBaseUpdateInto(*data->base_updates,home,base::BaseManager::BUO_REFRESH);
            }
        }
        if(battle_survivor->Conventional().home_base_id) {
            auto* home=m_game->GetBM()->GetBase(*battle_survivor->Conventional().home_base_id);
            if(!home) {THROW("Battle survivor has no current home Base");}
            base::BaseManager::QueueBaseUpdateInto(*data->base_updates,home,base::BaseManager::BUO_REFRESH);
        }
    }
    if(knowledge) {data->knowledge=map::SmacWorkerKnowledge::Create(*knowledge);}
    return PreparedConventionalRemoval(std::move(data));
}
bool UnitManager::PreparedConventionalRemoval::Removes(size_t id) const {
    return std::find(m_data->removed_ids.begin(),m_data->removed_ids.end(),id)!=m_data->removed_ids.end();
}
void UnitManager::QueryPreparedUnits(const std::map<size_t,Unit*>& units,const std::vector<size_t>& order,
    const PreparedOccupants& occupants,const std::optional<map::SmacWorkerKnowledge>& knowledge,
    const std::vector<std::pair<Unit*,map::tile::Tile*>>& positions,const std::function<void()>& query) {
    if(!m_game->GetGCSpace()->IsAccumulating() || !m_game->IsRunning() || !knowledge || !query) {THROW("Prepared Unit queries require owned world knowledge and accumulation");}
    auto view_units=units;auto view_order=order;auto view_occupants=occupants;auto view_knowledge=knowledge;
    std::vector<std::pair<Unit*,map::tile::Tile*>> old_positions;old_positions.reserve(positions.size());
    for(const auto& row:positions) {old_positions.emplace_back(row.first,row.first->GetTile());}
    bool active=false;util::FinallyGuard restore([&]{
        if(!active) {return;}active=false;
        for(const auto& row:old_positions) {row.first->CommitConventionalMove(row.first->Conventional(),row.second);}
        m_units.swap(view_units);m_unit_order.swap(view_order);
        for(auto& row:view_occupants) {row.first->units.swap(row.second);}
        m_game->CommitConventionalSight(view_knowledge);
    });
    active=true;m_units.swap(view_units);m_unit_order.swap(view_order);
    for(auto& row:view_occupants) {row.first->units.swap(row.second);}
    for(const auto& row:positions) {row.first->CommitConventionalMove(row.first->Conventional(),row.second);}
    m_game->CommitConventionalSight(view_knowledge);query();
}
void UnitManager::PreparedConventionalRemoval::Query(const std::function<void()>& query) const {
    if(!m_data || m_data->committed) {THROW("Removal query requires an uncommitted plan");}
    const auto& d=*m_data;d.manager->QueryPreparedUnits(d.units,d.order,d.occupants,d.knowledge,{},query);
}
void UnitManager::PreparedConventionalRemoval::Commit() noexcept {
    if(!m_data || m_data->committed) {return;}
    auto& d=*m_data;
    static_assert(std::is_nothrow_swappable_v<decltype(d.units)> && std::is_nothrow_swappable_v<decltype(d.order)>
        && std::is_nothrow_swappable_v<decltype(d.updates)>);
    d.manager->m_units.swap(d.units); d.manager->m_unit_order.swap(d.order); d.manager->m_unit_updates.swap(d.updates);
    if(d.base_updates) {d.manager->m_game->GetBM()->m_base_updates.swap(*d.base_updates);}
    for(auto& tile:d.occupants) {tile.first->units.swap(tile.second);}
    d.manager->m_game->CommitConventionalSight(d.knowledge);
    for(size_t i=0;i<d.removed.size();++i) {d.retired[i].reset(d.removed[i]);}
    d.committed=true;
}
void UnitManager::PreparedConventionalRemoval::Notify(GSE_CALLABLE) {
    if(!m_data->committed) {THROW("Removal has not committed");}
    auto retired=std::move(m_data->retired); // deletes every removed wrapper on a hook exception
    for(const auto& owned:retired) {
        auto* unit=owned.get();
        m_data->manager->m_game->GetState()->TriggerObject(m_data->manager,"unit_despawn",ARGS_F(unit) {
            {"unit",unit->Wrap(GSE_CALL)},
        }; });
    }
}

struct UnitManager::PreparedFactionUnits::Data {
    UnitManager* manager;
    map::tile::Tile* destination;
    PreparedConventionalUnit* transient;
    std::optional<PreparedConventionalRemoval> removal;
    std::vector<std::unique_ptr<Unit>> candidates;
    std::vector<Unit*> published;
    std::map<size_t,Unit*> units;
    std::vector<size_t> order;
    std::unordered_map<size_t,unit_update_t> updates;
    std::map<map::tile::Tile*,std::map<size_t,Unit*>> occupants;
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::pair<SmacMovementHistory*,size_t> history{nullptr,0};
    bool committed=false,notified=false;
};
UnitManager::PreparedFactionUnits::PreparedFactionUnits(std::unique_ptr<Data> data):m_data(std::move(data)) {}
UnitManager::PreparedFactionUnits::~PreparedFactionUnits() = default;
UnitManager::PreparedFactionUnits::PreparedFactionUnits(PreparedFactionUnits&&) noexcept = default;
UnitManager::PreparedFactionUnits& UnitManager::PreparedFactionUnits::operator=(PreparedFactionUnits&&) noexcept = default;

UnitManager::PreparedFactionUnits UnitManager::PrepareFactionUnits(GSE_CALLABLE,slot::Slot* owner,map::tile::Tile* destination,
    const std::vector<std::pair<Def*,ConventionalUnitState>>& replacements,const map::SmacWorkerKnowledge& territory,
    PreparedConventionalUnit* transient) {
    m_game->CheckRW(GSE_CALL);if(gc_space!=m_game->GetGCSpace()) {THROW("Faction preparation requires the owning accumulation");}
    return PrepareFactionUnitsForQuery(owner,destination,replacements,territory,transient);
}
UnitManager::PreparedFactionUnits UnitManager::PrepareFactionUnitsForQuery(slot::Slot* owner,map::tile::Tile* destination,
    const std::vector<std::pair<Def*,ConventionalUnitState>>& replacements,const map::SmacWorkerKnowledge& territory,
    PreparedConventionalUnit* transient,const PreparedConventionalRemoval* supplied) {
    m_game->RequireSmacGameplayActive();m_game->RequireSmacLocalHumanOwner(owner);RequireConventionalUnitsReady();
    if(!m_game->GetGCSpace()->IsAccumulating() || !m_game->IsRunning()) {THROW("Faction preparation requires its owning accumulation");}
    if(replacements.size()>8 || (replacements.empty() ? destination!=nullptr : destination==nullptr)) {
        THROW("Faction disposition requires at most eight replacements at one admitted site");
    }
    const auto& participant=m_game->RequireSmacProductionOwner(owner);
    const auto simulation_id=participant.simulation_id;
    std::vector<size_t> removed;
    for(const auto id:m_unit_order) {
        const auto* unit=GetUnit(id);
        if(!unit) {THROW("Faction disposition Unit order is incomplete");}
        if(unit->m_owner==owner) {removed.push_back(id);}
    }
    if(transient) {
        if(!transient->m_data || !transient->m_data->candidate || transient->m_data->manager!=this
            || transient->m_data->published || transient->m_data->candidate->m_owner!=owner
            || transient->m_data->id!=Unit::GetNextId()) {THROW("Faction disposition transient is not its pending factory");}
        removed.push_back(transient->m_data->id);
    }
    auto data=std::make_unique<PreparedFactionUnits::Data>();data->manager=this;data->destination=destination;data->transient=transient;
    if(supplied) {
        if(transient || !supplied->m_data || supplied->m_data->committed || supplied->m_data->manager!=this) {THROW("Capture removal is not an uncommitted same-world plan");}
        // Duplicate prepared tables, never ownership of retired objects. The
        // supplied plan may be installed as a read-only nested query view.
        const auto& old=*supplied->m_data;auto copy=std::make_unique<PreparedConventionalRemoval::Data>();
        copy->manager=this;copy->removed=old.removed;copy->removed_ids=old.removed_ids;copy->units=old.units;copy->order=old.order;
        copy->updates=old.updates;copy->occupants=old.occupants;copy->knowledge=old.knowledge;copy->base_updates=old.base_updates;copy->retired.resize(old.removed.size());
        data->removal.emplace(PreparedConventionalRemoval(std::move(copy)));
    } else {data->removal.emplace(PrepareRemovalForQuery(removed,transient));}
    const auto& removal=*data->removal->m_data;
    if(!removal.knowledge) {THROW("Faction disposition requires saved sight and territory");}
    data->units=removal.units;data->order=removal.order;data->updates=removal.updates;data->occupants=removal.occupants;
    auto knowledge=supplied ? territory.CopyData() : removal.knowledge->CopyData();const auto claims=territory.CopyData();
    if(supplied) {knowledge.units.erase(std::remove_if(knowledge.units.begin(),knowledge.units.end(),[&](const auto& row){return data->removal->Removes(row.id);}),knowledge.units.end());}
    if(knowledge.width!=claims.width || knowledge.height!=claims.height || knowledge.horizontal_wrap!=claims.horizontal_wrap
        || knowledge.tiles.size()!=claims.tiles.size()) {THROW("Faction disposition territory geometry differs");}
    for(size_t i=0;i<knowledge.tiles.size();++i) {knowledge.tiles[i].claimant=claims.tiles[i].claimant;}
    knowledge.bases=claims.bases;
    const uint64_t first=uint64_t(Unit::GetNextId())+(transient ? 1u : 0u);
    if(!replacements.empty() && (!first || first>=uint64_t(INT64_MAX) || first>=SIZE_MAX
        || replacements.size()>uint64_t(INT64_MAX)-first || replacements.size()>SIZE_MAX-first)) {
        THROW("Faction disposition has no available Unit identity range");
    }
    data->candidates.reserve(replacements.size());data->published.resize(replacements.size(),nullptr);
    uint8_t visibility=0;
    if(!replacements.empty()) {
        const auto actual=m_game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
        const auto sight=m_game->PrepareConventionalSight(first,simulation_id,destination,nullptr);
        if(!sight) {THROW("Restart site has no admitted sight producer");}
        const auto explored=sight->CopyData();
        auto& remembered=knowledge.remembered[simulation_id];
        if(remembered.empty()) {remembered.resize(knowledge.tiles.size());}
        for(size_t i=0;i<knowledge.tiles.size();++i) {
            knowledge.tiles[i].explored_by|=explored.tiles[i].explored_by;
            // Preserve any transient-factory reveal, then apply the new site's
            // actual refreshes rather than replacing the complete old memory.
            if(actual.remembered[simulation_id].empty() || !(explored.remembered[simulation_id][i]==actual.remembered[simulation_id][i])) {
                remembered[i]=explored.remembered[simulation_id][i];
            }
        }
        for(auto& observation:knowledge.units) {
            const auto changed=std::find_if(explored.units.begin(),explored.units.end(),[&](const auto& row){return row.id==observation.id;});
            if(changed==explored.units.end()) {THROW("Restart sight lost a surviving Unit observation");}
            observation.visible_to|=changed->visible_to;
        }
        const auto created=std::find_if(explored.units.begin(),explored.units.end(),[&](const auto& row){return row.id==first;});
        if(created==explored.units.end()) {THROW("Restart factory sight lacks its new identity");}
        visibility=created->visible_to;
        data->occupants.try_emplace(destination,destination->units);
    }
    const ConventionalUnitResolver resolver=[this](const auto& state,const auto* slot){return ResolveConventional(state,slot);};
    for(size_t i=0;i<replacements.size();++i) {
        const auto& spec=replacements[i];const auto& state=spec.second;const auto id=static_cast<size_t>(first+i);
        if(state.owner_simulation_id!=simulation_id || state.home!=ConventionalUnitState::Home::None || state.home_base_id
            || state.stored_morale!=1 || state.damage || state.spent || state.failed_move
            || state.work_order!=ConventionalUnitState::WorkOrder::None || state.work_progress) {
            THROW("Restart factory requires fresh owned unhomed Unit state");
        }
        ValidateConventionalCreation(id,spec.first,owner,destination,state,resolver);
        auto candidate=std::unique_ptr<Unit>(new Unit(Unit::Unplaced{},this,id,spec.first,owner,state));
        if(!i) {data->history=PreparePlacementHistory(*candidate,destination);}
        if(!data->units.emplace(id,candidate.get()).second || !data->occupants.at(destination).emplace(id,candidate.get()).second
            || !data->updates.emplace(id,unit_update_t{UUO_SPAWN,candidate.get()}).second) {THROW("Restart Unit publication identity conflicts");}
        data->order.push_back(id);knowledge.units.push_back({id,visibility,false});data->candidates.push_back(std::move(candidate));
    }
    // Own departing positions and Base removal are observed by their owner.
    // Other observers keep their independently retained memories.
    auto& memory=knowledge.remembered[simulation_id];
    if(!memory.empty()) {
        for(const auto& tile:data->occupants) {
            const auto index=tile.first->coord.y*(knowledge.width/2)+tile.first->coord.x/2;
            if(memory[index]) {memory[index]->unit_present=!tile.second.empty();}
        }
        for(const auto& base:m_game->GetBM()->GetBases()) {
            if(base.second->m_owner!=owner || std::any_of(claims.bases.begin(),claims.bases.end(),[&](const auto& row){return row.id==base.first;})) {continue;}
            const auto* tile=base.second->GetTile();const auto index=tile->coord.y*(knowledge.width/2)+tile->coord.x/2;
            if(memory[index]) {memory[index]->base_present=false;}
        }
    }
    data->knowledge=map::SmacWorkerKnowledge::Create(knowledge);
    return PreparedFactionUnits(std::move(data));
}
void UnitManager::PreparedFactionUnits::Query(const std::function<void()>& query) const {
    if(!m_data || m_data->committed) {THROW("Faction Unit query requires an uncommitted plan");}
    const auto& d=*m_data;std::vector<std::pair<Unit*,map::tile::Tile*>> positions;positions.reserve(d.candidates.size());
    for(const auto& row:d.candidates) {positions.emplace_back(row.get(),d.destination);}
    d.manager->QueryPreparedUnits(d.units,d.order,d.occupants,d.knowledge,positions,query);
}
void UnitManager::PreparedFactionUnits::Refresh(Unit* unit) {
    if(!m_data || m_data->committed || !unit || !m_data->units.count(unit->m_id) || m_data->units.at(unit->m_id)!=unit) {THROW("Faction refresh needs a prepared surviving Unit");}
    auto& update=m_data->updates.try_emplace(unit->m_id,unit_update_t{UUO_NONE,unit}).first->second;
    if(update.unit!=unit || (update.ops&UUO_DESPAWN)) {THROW("Faction refresh has a stale pending update");}
    update.ops=static_cast<unit_update_op_t>(update.ops|UUO_REFRESH);
}
void UnitManager::PreparedFactionUnits::Commit() noexcept {
    if(!m_data || m_data->committed) {return;}
    auto& d=*m_data;
    static_assert(std::is_nothrow_swappable_v<decltype(d.units)> && std::is_nothrow_swappable_v<decltype(d.order)>
        && std::is_nothrow_swappable_v<decltype(d.updates)>);
    if(d.transient) {d.transient->Commit();}
    d.removal->Commit();
    d.manager->m_units.swap(d.units);d.manager->m_unit_order.swap(d.order);d.manager->m_unit_updates.swap(d.updates);
    for(auto& tile:d.occupants) {tile.first->units.swap(tile.second);}
    for(size_t i=0;i<d.candidates.size();++i) {d.published[i]=d.candidates[i].release();d.published[i]->CommitInitialPlacement(d.destination);}
    if(d.history.first) {d.history.first->MarkPlaced(d.history.second);}
    d.manager->m_game->CommitConventionalSight(d.knowledge);d.committed=true;
}
void UnitManager::PreparedFactionUnits::Notify(GSE_CALLABLE) {
    if(!m_data || !m_data->committed || m_data->notified) {THROW("Faction Unit notifications are not pending");}
    auto& d=*m_data;d.notified=true;d.removal->Notify(GSE_CALL);
    for(auto* unit:d.published) {d.manager->TriggerSpawnHook(GSE_CALL,unit);}
}

UnitManager::PreparedConventionalUnit UnitManager::PrepareConventionalUnit( GSE_CALLABLE, size_t id, Def* def, slot::Slot* owner,
	map::tile::Tile* tile, const ConventionalUnitState& state ) {
	m_game->RequireSmacGameplayActive();
	m_game->CheckRW( GSE_CALL );
	if ( !m_game->IsRunning() || gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) {
		THROW( "Prepared unit creation requires its running owning accumulation" );
	}
	RequireConventionalUnitsReady();
	const ConventionalUnitResolver resolver = [ this ]( const auto& value, const auto* slot ) { return ResolveConventional( value, slot ); };
	ValidateConventionalCreation( id, def, owner, tile, state, resolver );
	auto data = std::make_unique< PreparedConventionalUnit::Data >();
	data->manager = this; data->tile = tile; data->id = id;
	data->sight = m_game->PrepareConventionalSight( id, state.owner_simulation_id, tile, nullptr );
	data->candidate.reset( new Unit( Unit::Unplaced{}, this, id, def, owner, state ) );
	data->history = PreparePlacementHistory( *data->candidate, tile );
	data->units = m_units; data->updates = m_unit_updates; data->occupants = tile->units;
	data->order = m_unit_order; data->order.push_back( id );
	if ( !data->units.emplace( id, data->candidate.get() ).second || !data->occupants.emplace( id, data->candidate.get() ).second
		|| !data->updates.emplace( id, unit_update_t{ UUO_SPAWN, data->candidate.get() } ).second ) {
		THROW( "Prepared unit creation has conflicting identity publication" );
	}
	return PreparedConventionalUnit( std::move( data ) );
}

UnitManager::PreparedConventionalUnit UnitManager::PrepareManufacturedConventionalUnit( GSE_CALLABLE, base::Base& home,
	int32_t design_id, Def* def, SmacMoraleMode mode ) {
	m_game->CheckRW( GSE_CALL );
	if ( home.GetGame() != m_game || m_game->GetBM()->GetBase( home.m_id ) != &home || m_units.size() >= 2048 ) {
		THROW( "Manufacturing requires a registered home and an available unit slot" );
	}
	const auto context = home.GetSmacResourceContext();
	for ( const auto& entry : home.GetTile()->units ) {
		const auto* occupant = entry.second;
		if ( !occupant || GetUnit(entry.first) != occupant || occupant->m_id != entry.first || occupant->GetTile() != home.GetTile()
			|| !occupant->IsConventional() || occupant->m_owner != home.m_owner
			|| occupant->Conventional().owner_simulation_id != context.owner_simulation_id ) {
			THROW( "Manufacturing requires a canonical same-owner stack" );
		}
	}
	if ( !m_game->m_simulation || m_game->m_simulation->value.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		THROW( "Ordinary manufacturing requires the admitted no-project campaign" );
	}
	bool command_center = false, bioenhancement = false;
	for ( const auto& entry : home.m_facilities ) {
		if ( !entry.second || m_game->GetBM()->GetFacilityDef( entry.first ) != entry.second ) { THROW( "Unregistered manufacturing facility" ); }
		m_game->GetBM()->ValidateSmacFacilityDef( entry.second );
		if ( !entry.second->m_smac_source_id || *entry.second->m_smac_source_id < 1 || *entry.second->m_smac_source_id > 33 ) {
			THROW( "Ordinary manufacturing requires resolved local facility roles" );
		}
		command_center |= entry.second->m_smac_source_id == 27;
		bioenhancement |= entry.second->m_smac_source_id == 30;
	}
	const auto* social = m_game->m_simulation->value.social.Find( context.owner_simulation_id );
	if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Refreshed ) {
		THROW( "Ordinary manufacturing requires current MORALE" );
	}
	// The saved campaign excludes Command Nexus/Cyborg Factory. The admitted
	// conventional designs have no abilities and are already prototyped, so only
	// these two local training categories precede the ordinary baseline one.
	int32_t training = ( command_center ? 2 : 0 ) + ( bioenhancement ? 2 : 0 );
	if ( social->current[ 4 ] < -1 ) { training /= 2; }
	const ConventionalUnitState state{ design_id, context.owner_simulation_id, static_cast< uint8_t >( 1 + training ), 0, 0, false, ConventionalUnitState::Home::Base, mode, home.m_id };
	const auto design = m_game->ResolveConventionalDesign( state, *m_game->m_simulation );
	if ( design.id >= 64 && !design.prototyped ) { THROW( "First custom prototype completion is not yet admitted" ); }
	return PrepareConventionalUnit( GSE_CALL, Unit::GetNextId(), def, home.m_owner, home.GetTile(), state );
}

Unit* UnitManager::SpawnConventionalUnit( GSE_CALLABLE, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, const ConventionalUnitState& state ) {
	m_game->RequireSmacGameplayActive();
	m_game->CheckRW( GSE_CALL );
	if ( m_game->IsRunning() ) {
		auto prepared = PrepareConventionalUnit( GSE_CALL, id, def, owner, tile, state );
		auto* unit = prepared.Commit(); prepared.Notify( GSE_CALL ); return unit;
	}
	const ConventionalUnitResolver resolver = [ this ]( const auto& value, const auto* slot ) { return ResolveConventional( value, slot ); };
	ValidateConventionalCreation( id, def, owner, tile, state, resolver );
	auto* unit = new Unit( GSE_CALL, this, id, def, owner, tile, state );
	SpawnUnit( GSE_CALL, unit ); return unit;
}

Unit* UnitManager::SpawnStartingConventionalUnit( GSE_CALLABLE, Def* def, slot::Slot* owner, map::tile::Tile* tile, SmacMoraleMode mode ) {
	m_game->CheckRW( GSE_CALL );
	const auto state = m_game->PrepareStartingConventionalUnit( owner, mode );
	for ( const auto& entry : m_units ) {
		if ( entry.second->IsConventional() && entry.second->m_owner == owner ) { THROW( "Starting conventional unit already exists" ); }
	}
	const ConventionalUnitResolver resolver = [ this ]( const auto& value, const auto* slot ) {
		return m_game->ResolveConventionalIdentity( value, slot, *m_game->m_simulation );
	};
	const auto id = Unit::GetNextId();
	ValidateConventionalCreation( id, def, owner, tile, state, resolver, true );
	if ( GetMap()->GetSmacWorkerKnowledge() ) {
		// Only this exact claimed startup API can stage a raw pending Unit. The
		// generic prepared path continues to require complete derived readiness.
		auto data = std::make_unique< PreparedConventionalUnit::Data >();
		data->manager=this; data->tile=tile; data->id=id;
		data->sight=m_game->PrepareConventionalSight(id,state.owner_simulation_id,tile,nullptr,true);
		data->candidate.reset(new Unit(Unit::Unplaced{},this,id,def,owner,state));
		data->candidate->m_early_startup=true;
		data->history=PreparePlacementHistory(*data->candidate,tile);
		data->units=m_units; data->updates=m_unit_updates; data->occupants=tile->units;
		data->order=m_unit_order; data->order.push_back(id);
		if(!data->units.emplace(id,data->candidate.get()).second||!data->occupants.emplace(id,data->candidate.get()).second
			||!data->updates.emplace(id,unit_update_t{UUO_SPAWN,data->candidate.get()}).second){THROW("Starting unit identity publication conflicts");}
		PreparedConventionalUnit prepared(std::move(data)); auto* unit=prepared.Commit(); prepared.Notify(GSE_CALL); return unit;
	}
	auto* unit = new Unit( GSE_CALL, this, id, def, owner, tile, state, true );
	SpawnUnit( GSE_CALL, unit );
	return unit;
}

bool UnitManager::HasPendingConventionalUnits() const {
	for ( const auto& entry : m_units ) { if ( entry.second->IsConventionalPending() ) { return true; } }
	for ( const auto& entry : m_unprocessed_units ) { if ( entry.unit->IsConventionalPending() ) { return true; } }
	return false;
}
void UnitManager::RequireConventionalUnitsReady() const {
	if ( HasPendingConventionalUnits() ) { THROW( "Conventional startup units have not reached first-turn seal" ); }
	for ( const auto& item : m_units ) { item.second->RequireConventionalReady(); }
	for ( const auto& item : m_unprocessed_units ) { item.unit->RequireConventionalReady(); }
}
void UnitManager::CompleteConventionalHomeRestore() {
	if ( !m_unprocessed_units.empty() ) { THROW( "Home validation requires registered restored Units" ); }
	// Preflight every link and derived query before releasing any pending Unit.
	for ( const auto& item : m_units ) {
		const auto* unit = item.second;
		if ( unit->m_pending_home ) { const auto& state = unit->Conventional(); QueryConventionalUnit( state, ResolveConventional( state, unit->m_owner ), state.render_morale_mode ); }
	}
	for ( const auto& item : m_units ) { item.second->m_pending_home = false; }
}
bool UnitManager::HasConventionalHome( size_t base_id ) const {
	const auto matches = [ & ]( const Unit* unit ) { return unit->IsConventional() && unit->Conventional().home_base_id == base_id; };
	for ( const auto& item : m_units ) { if ( matches( item.second ) ) { return true; } }
	for ( const auto& item : m_unprocessed_units ) { if ( matches( item.unit ) ) { return true; } }
	return false;
}
void UnitManager::PreflightConventionalStartup( const ConventionalUnitResolver& resolver ) const {
	if ( !m_unprocessed_units.empty() && HasPendingConventionalUnits() ) { THROW( "Startup unit is not registered" ); }
	for ( const auto& entry : m_units ) {
		const auto* unit = entry.second;
		if ( unit->IsConventionalPending() ) {
			QueryConventionalUnit( unit->Conventional(), resolver( unit->Conventional(), unit->m_owner ), unit->Conventional().render_morale_mode );
		}
	}
}
void UnitManager::SealConventionalStartup() noexcept {
	for ( const auto& entry : m_units ) { entry.second->m_early_startup = false; }
}

void UnitManager::UpdateConventionalUnit( GSE_CALLABLE, Unit* unit, const ConventionalUnitState& state ) {
	m_game->RequireSmacGameplayActive();
	m_game->CheckRW( GSE_CALL );
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) { THROW( "Conventional update requires a registered unit" ); }
	unit->RequireConventionalReady();
	const auto& prior = unit->Conventional();
	if ( state.design_id != prior.design_id || state.owner_simulation_id != prior.owner_simulation_id || state.home != prior.home || state.home_base_id != prior.home_base_id
		|| state.render_morale_mode != prior.render_morale_mode ) { THROW( "Conventional update cannot replace identity or presentation policy" ); }
	QueryConventionalUnit( state, ResolveConventional( state, unit->m_owner ), state.render_morale_mode );
	unit->m_state = state;
	RefreshUnit( GSE_CALL, unit );
}

void UnitManager::TriggerSpawnHook( GSE_CALLABLE, Unit* unit ) {
	auto* state = m_game->GetState();
	if ( state->IsMaster() ) {
		state->TriggerObject( this, "unit_spawn", ARGS_F( &unit, this ) {
			{
				"unit",
				unit->Wrap( GSE_CALL )
			},
		}; } );
	}
}

void UnitManager::RegisterUnit( Unit* unit ) {
	m_unit_order.reserve( m_unit_order.size() + 1 );
	if ( !m_units.emplace( unit->m_id, unit ).second ) {
		THROW( "duplicate unit ID" );
	}
	m_unit_order.push_back( unit->m_id );
	QueueUnitUpdate( unit, UUO_SPAWN );
}

bool UnitManager::HasUnitId( const size_t id ) const {
	if ( m_units.find( id ) != m_units.end() ) { return true; }
	for ( const auto& pending : m_unprocessed_units ) {
		if ( pending.unit->m_id == id ) { return true; }
	}
	return false;
}

void UnitManager::DespawnUnit( GSE_CALLABLE, const size_t unit_id ) {
	m_game->RequireSmacGameplayActive();
	m_game->RequireWorkerKnowledgeMutation();

	const auto& it = m_units.find( unit_id );
	if ( it == m_units.end() ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Unit id " + std::to_string( unit_id ) + " not found" );
	}

	auto* unit = it->second;
	const auto ordered = std::find( m_unit_order.begin(), m_unit_order.end(), unit_id );
	if ( ordered == m_unit_order.end() ) { THROW( "Unit is missing its record order" ); }

	Log( "Despawning unit #" + std::to_string( unit->m_id ) + " (" + unit->m_def->m_id + ") at " + unit->GetTile()->ToString() );

	QueueUnitUpdate( unit, UUO_DESPAWN );

	auto* tile = unit->GetTile();
	ASSERT( tile, "unit tile not set" );
	const auto& tile_it = tile->units.find( unit->m_id );
	ASSERT( tile_it != tile->units.end(), "unit id not found in tile" );
	tile->units.erase( tile_it );

	m_units.erase( it );
	m_unit_order.erase( ordered );

	auto* state = m_game->GetState();
	if ( state->IsMaster() ) {
		state->TriggerObject( this, "unit_despawn", ARGS_F( &unit ) {
			{
				"unit",
				unit->Wrap( GSE_CALL )
			}
		}; } );
	}

	delete unit;
}

MoraleSet* UnitManager::GetMoraleSet( const std::string& name ) const {
	const auto& it = m_unit_moralesets.find( name );
	if ( it != m_unit_moralesets.end() ) {
		return it->second;
	}
	return nullptr;
}

Unit* UnitManager::GetUnit( const size_t id ) const {
	const auto& it = m_units.find( id );
	if ( it != m_units.end() ) {
		return it->second;
	}
	return nullptr;
}

Def* UnitManager::GetUnitDef( const std::string& name ) const {
	const auto& it = m_unit_defs.find( name );
	if ( it != m_unit_defs.end() ) {
		return it->second;
	}
	else {
		return nullptr;
	}
}

const std::map< size_t, Unit* >& UnitManager::GetUnits() const {
	return m_units;
}

void UnitManager::ProcessUnprocessed( GSE_CALLABLE ) {
	ASSERT( m_game->IsRunning(), "game not running" );
	while ( !m_unprocessed_units.empty() ) {
		const auto pending = m_unprocessed_units.front();
		RegisterUnit( pending.unit );
		// Ownership has moved to m_units. Remove the pending owner before any
		// gameplay callback can throw; untouched entries remain pending.
		m_unprocessed_units.pop_front();
		if ( pending.is_new ) { TriggerSpawnHook( GSE_CALL, pending.unit ); }
	}
}

void UnitManager::PushUpdates() {
	// Keep queued spawn/update ownership until the whole first-turn seal.
	if ( HasPendingConventionalUnits() ) { return; }
	for ( const auto& item : m_units ) { if ( item.second->m_pending_home ) { return; } }
	if ( m_game->IsRunning() && !m_unit_updates.empty() ) {
		for ( const auto& it : m_unit_updates ) {
			const auto unit_id = it.first;
			const auto& uu = it.second;
			const auto& unit = uu.unit;
			if ( uu.ops & UUO_SPAWN ) {
				const auto presentation = unit->GetPresentation();
				auto fr = FrontendRequest( FrontendRequest::FR_UNIT_SPAWN );
				fr.data.unit_spawn.unit_id = unit->m_id;
				NEW( fr.data.unit_spawn.unitdef_id, std::string, unit->m_def->m_id );
				fr.data.unit_spawn.slot_index = unit->m_owner->GetIndex();
				const auto* tile = unit->GetTile();
				fr.data.unit_spawn.tile_coords = {
					tile->coord.x,
					tile->coord.y
				};
				const auto c = unit->GetRenderCoords();
				fr.data.unit_spawn.render_coords = {
					c.x,
					c.y,
					c.z
				};

				fr.data.unit_spawn.movement = presentation.movement;
				fr.data.unit_spawn.morale = presentation.morale;
				NEW( fr.data.unit_spawn.morale_string, std::string, unit->GetMoraleString() );
				fr.data.unit_spawn.health = presentation.health;
				fr.data.unit_spawn.conventional = unit->IsConventional();
				fr.data.unit_spawn.full_movement = presentation.full_movement;
				fr.data.unit_spawn.has_moves = unit->HasMovesLeft();
				m_game->AddFrontendRequest( fr );
			}
			if ( uu.ops & UUO_REFRESH ) {
				const auto presentation = unit->GetPresentation();
				auto fr = FrontendRequest( FrontendRequest::FR_UNIT_UPDATE );
				fr.data.unit_update.unit_id = unit->m_id;
				fr.data.unit_update.movement = presentation.movement;
				fr.data.unit_update.health = presentation.health;
				fr.data.unit_update.morale = presentation.morale;
				NEW( fr.data.unit_update.morale_string, std::string, unit->GetMoraleString() );
				fr.data.unit_update.conventional = unit->IsConventional();
				fr.data.unit_update.full_movement = presentation.full_movement;
				fr.data.unit_update.has_moves = unit->HasMovesLeft();
				const auto* tile = unit->GetTile();
				fr.data.unit_update.tile_coords = {
					tile->coord.x,
					tile->coord.y
				};
				const auto c = unit->GetRenderCoords();
				fr.data.unit_update.render_coords = {
					c.x,
					c.y,
					c.z
				};
				m_game->AddFrontendRequest( fr );
			}
			if ( uu.ops & UUO_DESPAWN ) {
				auto fr = FrontendRequest( FrontendRequest::FR_UNIT_DESPAWN );
				fr.data.unit_despawn.unit_id = unit_id;
				m_game->AddFrontendRequest( fr );
			}
		}
		m_unit_updates.clear();
		m_game->CheckTurnComplete();
	}
}

WRAPIMPL_BEGIN( UnitManager )
	WRAPIMPL_PROPS
	WRAPIMPL_TRIGGERS
		{
			"define_moraleset",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 2 );
				N_GETVALUE( id, 0, String );
				N_GETVALUE( arr, 1, Array );

				if ( m_unit_moralesets.find( id ) != m_unit_moralesets.end() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Moraleset \"" + id + "\" already exists" );
				}

				const uint8_t expected_count = unit::MORALE_MAX - unit::MORALE_MIN + 1;
				if ( arr.size() != expected_count ) {
					GSE_ERROR( gse::EC.INVALID_CALL, "Moraleset must have exactly " + std::to_string( expected_count ) + " values (found " + std::to_string( arr.size() ) + ")");
				}
				unit::MoraleSet::morale_values_t values = {};
				for ( const auto& v : arr ) {
					if ( v->type != gse::VT_OBJECT ) {
						GSE_ERROR( gse::EC.INVALID_CALL, "Moraleset elements must be objects");
					}
					const auto* obj = (gse::value::Object*)v;
					N_GETPROP( name, obj->value, "name", String );
					values.push_back( unit::Morale{ name } );
				}

				DefineMoraleSet( new unit::MoraleSet( id, values ) );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"undefine_moraleset",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				if ( m_unit_moralesets.find( id ) == m_unit_moralesets.end() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Moraleset \"" + id + "\" not found" );
				}

				UndefineMoraleSet( id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"get_moraleset",
			NATIVE_CALL( this ) {

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				const auto& moraleset = m_unit_moralesets.find( id );
				if ( moraleset == m_unit_moralesets.end() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Moraleset \"" + id + "\" not found" );
				}

				gse::value::array_elements_t result = {};
				for ( const auto& morale : moraleset->second->m_morale_values ) {
					// TODO: refactor with Wrap()
					result.push_back( VALUE( gse::value::String,, morale.m_name ) );
				}

				return VALUE( gse::value::Array,, result );
			} )
		},
		{
			"define_unit",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 2 );
				N_GETVALUE( id, 0, String );
				N_GETVALUE( unit_def, 1, Object );
				N_GETPROP( name, unit_def, "name", String );
				N_GETPROP( morale, unit_def, "morale", String );
				N_GETPROP( unit_type, unit_def, "type", String );

				if ( m_unit_defs.find( id ) != m_unit_defs.end() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Unit def '" + id + "' already exists");
				}

				if ( unit_type == "static" ) {
					N_GETPROP( movement_type_str, unit_def, "movement_type", String );
					unit::movement_type_t movement_type;
					if ( movement_type_str == "land" ) {
						movement_type = unit::MT_LAND;
					}
					else if ( movement_type_str == "water" ) {
						movement_type = unit::MT_WATER;
					}
					else if ( movement_type_str == "air" ) {
						movement_type = unit::MT_AIR;
					}
					else if ( movement_type_str == "immovable" ) {
						movement_type = unit::MT_IMMOVABLE;
					}
					else {
						GSE_ERROR( gse::EC.INVALID_CALL, "Invalid movement type: " + movement_type_str + ". Specify one of: land water air immovable");
					}
					N_GETPROP( movement_per_turn, unit_def, "movement_per_turn", Int );
					N_GETPROP( render_def, unit_def, "render", Object );
					N_GETPROP( render_type, render_def, "type", String );
					if ( render_type == "sprite" ) {
						N_GETPROP( sprite_file, render_def, "file", String );
						N_GETPROP( sprite_x, render_def, "x", Int );
						N_GETPROP( sprite_y, render_def, "y", Int );
						N_GETPROP( sprite_w, render_def, "w", Int );
						N_GETPROP( sprite_h, render_def, "h", Int );
						N_GETPROP( sprite_cx, render_def, "cx", Int );
						N_GETPROP( sprite_cy, render_def, "cy", Int );
						N_GETPROP_OPT_INT( sprite_morale_based_xshift, render_def, "morale_based_xshift" );
						const auto* moraleset = GetMoraleSet( morale );
						if ( !moraleset ) {
							GSE_ERROR( gse::EC.INVALID_CALL, "Morale type '" + morale + "' is not defined");
						}
						auto* def = new unit::StaticDef(
							id,
							moraleset,
							name,
							movement_type,
							movement_per_turn,
							new unit::SpriteRender(
								sprite_file,
								sprite_x,
								sprite_y,
								sprite_w,
								sprite_h,
								sprite_cx,
								sprite_cy,
								sprite_morale_based_xshift
							)
						);

						DefineUnit( def );

						return VALUE( gse::value::Undefined );
					}
					else if ( render_type == "port_former_marker" || render_type == "port_colony_marker" ) {
						const auto* moraleset = GetMoraleSet( morale );
						if ( !moraleset ) { GSE_ERROR( gse::EC.INVALID_CALL, "Morale type '" + morale + "' is not defined" ); }
						std::unique_ptr< unit::Render > render = render_type == "port_former_marker"
							? std::unique_ptr< unit::Render >( new unit::FormerMarkerRender() ) : std::unique_ptr< unit::Render >( new unit::ColonyMarkerRender() );
						auto* def = new unit::StaticDef( id, moraleset, name, movement_type, movement_per_turn, render.get() );
						render.release(); DefineUnit( def ); return VALUE( gse::value::Undefined );
					}
					else if ( render_type == "cvr_scout" || render_type == "cvr_spartan_rover" || render_type == "cvr_initialized_scout" ) {
						N_GETPROP( facing, render_def, "facing", Int );
						N_GETPROP( zoom, render_def, "zoom", Int );
						N_GETPROP( numerator, render_def, "scale_numerator", Int );
						N_GETPROP( denominator, render_def, "scale_denominator", Int );
						N_GETPROP( width, render_def, "world_width", Float );
						N_GETPROP( height, render_def, "world_height", Float );
						N_GETPROP( center_x, render_def, "center_x", Float );
						N_GETPROP( center_y, render_def, "center_y", Float );
						for ( const auto value : { facing, zoom, numerator, denominator } ) {
							if ( value < INT32_MIN || value > INT32_MAX ) { GSE_ERROR( gse::EC.INVALID_CALL, "Scout view integer out of range" ); }
						}
						const auto* moraleset = GetMoraleSet( morale );
						if ( !moraleset ) { GSE_ERROR( gse::EC.INVALID_CALL, "Morale type '" + morale + "' is not defined" ); }
						std::unique_ptr< unit::CvrScoutRender > render;
						try {
							render = std::make_unique< unit::CvrScoutRender >( static_cast< int32_t >( facing ), static_cast< int32_t >( zoom ),
								static_cast< int32_t >( numerator ), static_cast< int32_t >( denominator ), types::Vec2< float >{ width, height }, types::Vec2< float >{ center_x, center_y },
								render_type == "cvr_scout" ? unit::CvrScoutRender::Composition::Scout :
								render_type == "cvr_spartan_rover" ? unit::CvrScoutRender::Composition::SpartanRover : unit::CvrScoutRender::Composition::InitializedScout );
						}
						catch ( const std::invalid_argument& error ) { GSE_ERROR( gse::EC.INVALID_CALL, error.what() ); }
						auto* def = new unit::StaticDef( id, moraleset, name, movement_type, movement_per_turn, render.get() );
						render.release();
						DefineUnit( def );
						return VALUE( gse::value::Undefined );
					}
					else {
						GSE_ERROR( gse::EC.GAME_ERROR, "Unsupported render type: " + render_type );
					}
				}
				else {
					GSE_ERROR( gse::EC.GAME_ERROR, "Unsupported unit type: " + unit_type );
				}
			})
		},
		{
			"undefine_unit",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				if ( m_unit_defs.find( id ) == m_unit_defs.end() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Unit def '" + id + "' not found" );
				}

				UndefineUnit( id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"has_unit",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( unit_id, 0, Int );
				return VALUE( gse::value::Bool,, m_units.find( unit_id ) != m_units.end() );
			} )
		},
		{
			"get_unit",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( unit_id, 0, Int );
				const auto& it = m_units.find( unit_id );
				if ( it != m_units.end() ) {
					return it->second->Wrap( GSE_CALL );
				}
				else {
					GSE_ERROR( gse::EC.GAME_ERROR, "Unit id " + std::to_string( unit_id ) + " not found" );
				}
			} )
		},
		{
			"spawn_unit",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( obj, 0, Object );

				N_GETPROP_OPT( size_t, unit_id, obj, "id", Int, 0 );
				N_GETPROP( def_name, obj, "def", String );
				N_GETPROP_UNWRAP( owner, obj, "owner", Player );
				N_GETPROP_UNWRAP( tile, obj, "tile", map::tile::Tile );
				N_GETPROP( morale, obj, "morale", Int );
				N_GETPROP( health, obj, "health", Float );

				auto* def = GetUnitDef( def_name );
				if ( !def ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Unit type '" + def_name + "' is not defined" );
				}
				ASSERT( def->m_type == unit::DT_STATIC, "only static defs are supported for now" );
				const auto* staticdef = (unit::StaticDef*)def;
				auto* unit = new unit::Unit(
					GSE_CALL,
					this,
					unit_id ? unit_id : unit::Unit::GetNextId(),
					def,
					owner->GetSlot(),
					tile,
					staticdef->m_movement_per_turn,
					morale,
					health,
					false
				);
				SpawnUnit( GSE_CALL, unit );
				return unit->Wrap( GSE_CALL );
			})
		},
		{
			"spawn_starting_conventional_unit", NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 ); N_GETVALUE( obj, 0, Object );
				N_GETPROP( def_id, obj, "def", String );
				N_GETPROP_UNWRAP( owner, obj, "owner", Player ); N_GETPROP_UNWRAP( tile, obj, "tile", map::tile::Tile );
				N_GETPROP( mode, obj, "render_morale_mode", Int );
				if ( mode < 0 || mode > 1 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Explicit conventional render morale mode required" ); }
				return SpawnStartingConventionalUnit( GSE_CALL, GetUnitDef( def_id ), owner->GetSlot(), tile, static_cast< SmacMoraleMode >( mode ) )->Wrap( GSE_CALL );
			} )
		},
		{
			"spawn_conventional_unit", NATIVE_CALL( this ) {
				m_game->CheckGameplayRW( GSE_CALL ); N_EXPECT_ARGS( 1 ); N_GETVALUE( obj, 0, Object );
				N_GETPROP( id, obj, "id", Int ); N_GETPROP( def_id, obj, "def", String );
				N_GETPROP_UNWRAP( owner, obj, "owner", Player ); N_GETPROP_UNWRAP( tile, obj, "tile", map::tile::Tile );
				N_GETPROP( design, obj, "design_id", Int ); N_GETPROP( simulation, obj, "owner_simulation_id", Int );
				N_GETPROP( morale, obj, "stored_morale", Int ); N_GETPROP( damage, obj, "damage", Int ); N_GETPROP( spent, obj, "spent", Int );
				N_GETPROP( failed, obj, "failed_move", Bool ); N_GETPROP( home, obj, "home", String ); N_GETPROP( mode, obj, "render_morale_mode", Int );
				if ( id < 0 || static_cast< uint64_t >( id ) >= std::numeric_limits< size_t >::max() || design < 0 || design >= 512
					|| simulation < 1 || simulation > 7 || morale < 0 || morale > 255 || damage < 0 || damage > 255 || spent < 0 || spent > 255
					|| ( home != "none" && home != "base" ) || mode < 0 || mode > 1 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Invalid explicit conventional unit record" ); }
				ConventionalUnitState state{ static_cast< int32_t >( design ), static_cast< int32_t >( simulation ), static_cast< uint8_t >( morale ),
					static_cast< uint8_t >( damage ), static_cast< uint8_t >( spent ), failed, ConventionalUnitState::Home::None, static_cast< SmacMoraleMode >( mode ) };
				if ( home == "base" ) {
					N_GETPROP( home_id, obj, "home_base_id", Int );
					if ( home_id <= 0 ) { GSE_ERROR( gse::EC.INVALID_CALL, "Invalid conventional home Base ID" ); }
					state.home = ConventionalUnitState::Home::Base; state.home_base_id = static_cast< uint64_t >( home_id );
				}
				auto* unit = SpawnConventionalUnit( GSE_CALL, id ? static_cast< size_t >( id ) : Unit::GetNextId(), GetUnitDef( def_id ), owner->GetSlot(), tile, state );
				return unit->Wrap( GSE_CALL );
			} )
		},
		{
			"restore_unit",
			NATIVE_CALL( this ) {
				m_game->CheckGameplayRW( GSE_CALL );
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( obj, 0, Object );
				N_GETPROP( id, obj, "id", Int );
				N_GETPROP( def_id, obj, "def", String );
				N_GETPROP( owner, obj, "owner", Int );
				N_GETPROP( position, obj, "tile", Object );
				N_GETPROP( x, position, "x", Int );
				N_GETPROP( y, position, "y", Int );
				N_GETPROP( movement, obj, "movement", Float );
				N_GETPROP( morale, obj, "morale", Int );
				N_GETPROP( health, obj, "health", Float );
				N_GETPROP( moved, obj, "moved_this_turn", Bool );

				// A rollback restores an allocated identity; it must not allocate a
				// new ID or resolve links into another manager/world.
				if ( id <= 0 || static_cast< uint64_t >( id ) >= Unit::GetNextId() || HasUnitId( id ) ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Restored unit ID must be vacant and precede the next unit ID" );
				}
				auto* def = GetUnitDef( def_id );
				if ( !def ) { GSE_ERROR( gse::EC.GAME_ERROR, "Restored unit definition not found: " + def_id ); }
				if ( owner < 0 || static_cast< uint64_t >( owner ) >= m_game->GetState()->m_slots->GetCount() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Invalid restored unit owner" );
				}
				auto* map = GetMap();
				if ( x < 0 || y < 0 || static_cast< uint64_t >( x ) >= map->GetWidth() ||
					static_cast< uint64_t >( y ) >= map->GetHeight() || ( x & 1 ) != ( y & 1 ) ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Invalid restored unit tile" );
				}
				if ( morale < MORALE_MIN || morale > MORALE_MAX || !std::isfinite( movement ) || !std::isfinite( health ) ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Invalid restored unit morale, movement or health" );
				}
				// All fields are validated before Unit claims its tile. Finite signed
				// movement/health are retained, including zero and negative health.
				auto* unit = new Unit( GSE_CALL, this, static_cast< size_t >( id ), def, GetSlot( owner ),
					map->GetTile( x, y ), movement, morale, health, moved );
				if ( m_game->IsRunning() ) {
					const auto pending = m_unit_updates.find( unit->m_id );
					const bool cancels_despawn = pending != m_unit_updates.end() && ( pending->second.ops & UUO_DESPAWN );
					RegisterUnit( unit );
					// Cancelling a pending despawn also discarded any earlier refresh.
					// Publish the restored state using its new, live unit pointer.
					if ( cancels_despawn ) { QueueUnitUpdate( unit, UUO_REFRESH ); }
				}
				else { m_unprocessed_units.push_back( { unit, false } ); }
				// Old wrappers stay undefined; the restored numeric ID has a new
				// wrapper. Restoration never invokes new-unit gameplay callbacks.
				return unit->Wrap( GSE_CALL );
			} )
		},
		{
			"despawn_unit",
			NATIVE_CALL( this ) {

				m_game->CheckGameplayRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				if ( arguments.at( 0 )->type == gse::VT_INT ) {
					N_GETVALUE( unit_id, 0, Int );
					DespawnUnit( GSE_CALL, unit_id );
				}
				else {
					N_GETVALUE_UNWRAP( unit, 0, Unit );
					DespawnUnit( GSE_CALL, unit->m_id );
				}
				return VALUE( gse::value::Undefined );
			})
		},
	};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( UnitManager )

void UnitManager::Serialize( types::Buffer& buf ) const {
	RequireConventionalUnitsReady();
	if ( !m_unprocessed_units.empty() ) { THROW( "cannot snapshot units before startup registration" ); }
	buf.WriteString( s_snapshot_format );

	Log( "Serializing " + std::to_string( m_unit_moralesets.size() ) + " unit moralesets" );
	buf.WriteInt( m_unit_moralesets.size() );
	for ( const auto& it : m_unit_moralesets ) {
		buf.WriteString( it.first );
		buf.WriteString( MoraleSet::Serialize( it.second ).ToString() );
	}

	Log( "Serializing " + std::to_string( m_unit_defs.size() ) + " unit defs" );
	buf.WriteInt( m_unit_defs.size() );
	for ( const auto& it : m_unit_defs ) {
		buf.WriteString( it.first );
		buf.WriteString( Def::Serialize( it.second ).ToString() );
	}

	Log( "Serializing " + std::to_string( m_units.size() ) + " units" );
	if ( m_unit_order.size() != m_units.size() ) { THROW( "Unit record order differs from registry" ); }
	buf.WriteInt( m_units.size() );
	for ( const auto id : m_unit_order ) {
		buf.WriteString( Unit::Serialize( m_units.at( id ) ).ToString() );
	}
	buf.WriteInt( Unit::GetNextId() );

	Log( "Saved next unit id: " + std::to_string( Unit::GetNextId() ) );
}

void UnitManager::Deserialize( GSE_CALLABLE, types::Buffer& buf ) {
	Deserialize( GSE_CALL, buf, [ this ]( const auto& state, const auto* owner ) { return ResolveConventional( state, owner ); } );
}

void UnitManager::Deserialize( GSE_CALLABLE, types::Buffer& buf, const ConventionalUnitResolver& resolver, bool defer_home ) {
	if ( !m_unit_moralesets.empty() || !m_unit_defs.empty() || !m_units.empty() || !m_unit_order.empty() || !m_unprocessed_units.empty() ) {
		THROW( "unit snapshot requires an empty manager" );
	}
	// Old unversioned snapshots begin with an integer and fail the typed read.
	if ( buf.ReadString() != s_snapshot_format ) {
		THROW( "unsupported unit snapshot format (expected " + s_snapshot_format + ")" );
	}

	size_t sz = ReadSnapshotCount( buf );
	Log( "Unserializing " + std::to_string( sz ) + " unit moralesets" );
	m_unit_moralesets.reserve( sz );
	for ( size_t i = 0 ; i < sz ; i++ ) {
		const auto name = buf.ReadString();
		auto b = types::Buffer( buf.ReadString() );
		auto morale = std::unique_ptr< MoraleSet >( MoraleSet::Deserialize( b ) );
		if ( name.empty() || name != morale->m_id || GetMoraleSet( name ) || b.lenr != b.lenw ) {
			THROW( "invalid or duplicate morale set in unit snapshot" );
		}
		DefineMoraleSet( morale.release() );
	}

	sz = ReadSnapshotCount( buf );
	Log( "Unserializing " + std::to_string( sz ) + " unit defs" );
	m_unit_defs.reserve( sz );
	for ( size_t i = 0 ; i < sz ; i++ ) {
		const auto name = buf.ReadString();
		auto b = types::Buffer( buf.ReadString() );
		auto def = std::unique_ptr< Def >( Def::Deserialize( b, this ) );
		if ( name != def->m_id || GetUnitDef( name ) || b.lenr != b.lenw ) {
			THROW( "invalid or duplicate unit definition in snapshot" );
		}
		DefineUnit( def.release() );
	}

	sz = ReadSnapshotCount( buf );
	Log( "Unserializing " + std::to_string( sz ) + " units" );
	size_t highest_id = 0;
	for ( size_t i = 0 ; i < sz ; i++ ) {
		auto b = types::Buffer( buf.ReadString() );
		auto* unit = Unit::Deserialize( GSE_CALL, b, this, resolver, defer_home );
		highest_id = std::max( highest_id, unit->m_id );
		if ( m_game->IsRunning() ) { RegisterUnit( unit ); }
		else { m_unprocessed_units.push_back( { unit, false } ); }
	}

	const auto next_id = buf.ReadInt();
	if ( next_id <= 0 || static_cast< uint64_t >( next_id ) <= highest_id ||
		static_cast< uint64_t >( next_id ) >= std::numeric_limits< size_t >::max() || buf.lenr != buf.lenw ) {
		THROW( "invalid next unit ID in snapshot" );
	}
	Unit::SetNextId( static_cast< size_t >( next_id ) );
	Log( "Restored next unit id: " + std::to_string( Unit::GetNextId() ) );
}

void UnitManager::QueueUnitUpdate( const Unit* unit, const unit_update_op_t op ) {
	auto it = m_unit_updates.find( unit->m_id );
	if ( it == m_unit_updates.end() ) {
		it = m_unit_updates.insert(
			{
				unit->m_id,
				{
					{},
					unit,
				}
			}
		).first;
	}
	auto& update = it->second;
	if ( op == UUO_DESPAWN ) {
		if ( update.ops & UUO_SPAWN ) {
			// if unit is despawned immediately after spawning - frontend doesn't need to know
			m_unit_updates.erase( it );
			return;
		}
		update.ops = UUO_NONE; // clear other actions if unit was despawned
	}
	if ( op == UUO_SPAWN || op == UUO_REFRESH ) {
		if ( update.ops & UUO_DESPAWN ) {
			// do not despawn if it needs to spawn or refresh, i.e. if event was rolled back
			update.ops = (unit_update_op_t)( (uint8_t)update.ops & ~UUO_DESPAWN);
			if ( op == UUO_SPAWN ) {
				// if there's pending despawn event it means unit was already spawned, nothing to do
				m_unit_updates.erase( it );
				return;
			}
		}
	}
	// add to operations list
	update.ops = (unit_update_op_t)( (uint8_t)update.ops | (uint8_t)op );
}

const morale_t UnitManager::GetMorale( GSE_CALLABLE, const int64_t& morale ) {
	if ( morale < MORALE_MIN || morale > MORALE_MAX ) {
		GSE_ERROR( gse::EC.INVALID_CALL, "Invalid morale value: " + std::to_string( morale ) + " (should be between " + std::to_string( MORALE_MIN ) + " and " + std::to_string( MORALE_MAX ) + ", inclusive)" );
	}
	return (morale_t)morale;
}

const health_t UnitManager::GetHealth( GSE_CALLABLE, const float health ) {
	if ( health < Unit::MINIMUM_HEALTH_TO_KEEP || health > StaticDef::HEALTH_MAX ) {
		GSE_ERROR( gse::EC.INVALID_CALL, "Invalid health value: " + std::to_string( health ) + " (should be between " + std::to_string( Unit::MINIMUM_HEALTH_TO_KEEP ) + " and " + std::to_string( StaticDef::HEALTH_MAX ) + ", inclusive)" );
	}
	if ( health == 0 ) {
		GSE_ERROR( gse::EC.INVALID_CALL, "Invalid health value: " + std::to_string( health ) + " (you can't spawn a dead unit)" );
	}
	return (health_t)health;
}

const std::string* UnitManager::MoveUnitToTile( GSE_CALLABLE, Unit* unit, map::tile::Tile* dst_tile, const cb_oncomplete& on_complete ) {
	m_game->RequireWorkerKnowledgeMutation();
	if ( unit->IsConventional() ) { return new std::string( "Conventional movement is not activated" ); }

	auto* tm = m_game->GetTM();
	auto* am = m_game->GetAM();

	if ( unit->m_animation_id ) {
		// stop any previous animation
		am->FinishAnimation( unit->m_animation_id );
	}

	auto* src_tile = unit->GetTile();
	if ( src_tile == dst_tile ) {
		return new std::string( "Unit can't move because it's already on target tile" );
	}

	const std::unordered_set< map::tile::Tile* > tiles_to_lock = { src_tile, dst_tile };

	const auto* locked_tile = tm->FindLockedTile( tiles_to_lock );
	if ( locked_tile ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Tile " + locked_tile->coord.ToString() + " is locked" );
	}

	m_game->GetTM()->LockTiles( m_game->GetSlotNum(), tiles_to_lock );

	if ( !src_tile->IsLocked() ) {
		return new std::string( "Source tile must be locked before moving unit" );
	}
	if ( !dst_tile->IsLocked() ) {
		return new std::string( "Destination tile must be locked before moving unit" );
	}
	auto fr = FrontendRequest( FrontendRequest::FR_UNIT_MOVE );
	fr.data.unit_move.unit_id = unit->m_id;
	fr.data.unit_move.dst_tile_coords = {
		dst_tile->coord.x,
		dst_tile->coord.y
	};
	fr.data.unit_move.running_animation_id = unit->m_animation_id = am->AddAnimationCallback(
		[ this, tm, tiles_to_lock, on_complete, unit, dst_tile, ctx, gc_space, ep, si ]( const size_t animation_id ) {
			ASSERT( unit->m_animation_id, "animation id gone" );
			unit->m_animation_id = 0;
			tm->UnlockTiles( m_game->GetSlotNum(), tiles_to_lock );
			gse::ExecutionPointer ep2 = ep;
			unit->SetTile( gc_space, ctx, si, ep2 , dst_tile );
			on_complete();
		}
	);
	m_game->AddFrontendRequest( fr );

	return nullptr; // no error
}

const std::string* UnitManager::AttackUnitValidate( GSE_CALLABLE, Unit* attacker, Unit* defender ) {
	if ( attacker->IsConventional() || defender->IsConventional() ) { return new std::string( "Conventional combat is not activated" ); }
	const auto result = m_game->GetState()->TriggerObject( this, "unit_attack_validate", ARGS_F( &attacker, &defender ) {
		{
			"attacker",
			attacker->Wrap( GSE_CALL )
		},
		{
			"defender",
			defender->Wrap( GSE_CALL )
		},
	}; } );
	switch ( result->type ) {
		case gse::VT_NULL:
		case gse::VT_UNDEFINED:
			return nullptr; // no errors
		case gse::VT_STRING:
			return new std::string( ( (gse::value::String*)result )->value ); // error
		default:
			THROW( "unexpected validation result type: " + result->GetTypeString() );
	}
}

gse::Value* const UnitManager::AttackUnitResolve( GSE_CALLABLE, Unit* attacker, Unit* defender ) {
	if ( attacker->IsConventional() || defender->IsConventional() ) { GSE_ERROR( gse::EC.GAME_ERROR, "Conventional combat is not activated" ); }
	return m_game->GetState()->TriggerObject( this, "unit_attack_resolve", ARGS_F( &attacker, &defender ) {
		{
			"attacker",
			attacker->Wrap( GSE_CALL )
		},
		{
			"defender",
			defender->Wrap( GSE_CALL )
		},
	}; } );
}

void UnitManager::AttackUnitApply( GSE_CALLABLE, Unit* attacker, Unit* defender, gse::Value* const resolutions ) {
	if ( attacker->IsConventional() || defender->IsConventional() ) { GSE_ERROR( gse::EC.GAME_ERROR, "Conventional combat is not activated" ); }
	auto* state = m_game->GetState();
	state->TriggerObject( this, "unit_attack_apply", ARGS_F( &attacker, &defender, &resolutions ) {
		{
			"attacker",
			attacker->Wrap( GSE_CALL )
		},
		{
			"defender",
			defender->Wrap( GSE_CALL )
		},
		{
			"resolutions",
			resolutions
		}
	}; } );
	RefreshUnit( GSE_CALL, attacker );
	RefreshUnit( GSE_CALL, defender );
}

void UnitManager::RefreshUnit( GSE_CALLABLE, const Unit* unit ) {
	QueueUnitUpdate( unit, UUO_REFRESH );
}

map::Map* UnitManager::GetMap() const {
	return m_game->GetMap();
}

slot::Slot* UnitManager::GetSlot( const size_t slot_num ) const {
	return &m_game->GetState()->m_slots->GetSlot( slot_num );
}

}
}
}
