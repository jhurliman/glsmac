#pragma once

#include "gse/GCWrappable.h"

#include "gse/value/Object.h"

#include "Types.h"
#include "ConventionalUnitState.h"
#include "SmacEmptyGroundMovement.h"
#include "SmacScoutCombat.h"
#include "game/backend/faction/SmacHostileState.h"
#include "game/backend/map/SmacWorkerKnowledge.h"

#include <deque>
#include <memory>
#include <vector>
#include <functional>

namespace game {
namespace backend {

class Game;
class SmacMovementHistory;

namespace map {
class Map;
namespace tile {
class Tile;
}
}

namespace slot {
class Slot;
}

namespace base { class Base; class BaseManager; }

namespace unit {

class MoraleSet;
class Def;
class Unit;

CLASS( UnitManager, gse::GCWrappable )
public:
	UnitManager( Game* game );
	~UnitManager();

	void Clear();
	void DefineMoraleSet( MoraleSet* moraleset );
	void UndefineMoraleSet( const std::string& id );
	void DefineUnit( Def* def );
	void UndefineUnit( const std::string& id );
	void SpawnUnit( GSE_CALLABLE, Unit* unit );
	// Same owning accumulation: no callbacks, yields or world mutations between
	// preparation and commit. Destruction before commit publishes nothing.
	class PreparedConventionalUnit {
	public:
		~PreparedConventionalUnit();
		PreparedConventionalUnit( PreparedConventionalUnit&& ) noexcept;
		PreparedConventionalUnit& operator=( PreparedConventionalUnit&& ) noexcept;
		const Unit& Candidate() const; // prepared immutable record, before Commit only
		Unit* Commit() noexcept;
		void Notify( GSE_CALLABLE ); // after all caller-owned authority is committed
	private:
		friend class UnitManager;
		friend class base::BaseManager;
		// Same transaction as creation: retain its sight/queues while repairing
		// old homes and removing only the abandoned Base from territory.
		void PrepareBaseAbandonment( const base::Base& abandoned, const base::Base& replacement,
			const map::SmacWorkerKnowledge& territory );
		struct Data;
		explicit PreparedConventionalUnit( std::unique_ptr< Data > data );
		std::unique_ptr< Data > m_data;
	};
	PreparedConventionalUnit PrepareConventionalUnit( GSE_CALLABLE, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, const ConventionalUnitState& state );
	PreparedConventionalUnit PrepareManufacturedConventionalUnit( GSE_CALLABLE, base::Base& home, int32_t design_id, Def* def, SmacMoraleMode mode );
	Unit* SpawnConventionalUnit( GSE_CALLABLE, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, const ConventionalUnitState& state );
	Unit* SpawnStartingConventionalUnit( GSE_CALLABLE, Def* def, slot::Slot* owner, map::tile::Tile* tile, SmacMoraleMode mode );
	bool HasPendingConventionalUnits() const;
	void RequireConventionalUnitsReady() const;
	void CompleteConventionalHomeRestore();
	bool HasConventionalHome( size_t base_id ) const;
	void UpdateConventionalUnit( GSE_CALLABLE, Unit* unit, const ConventionalUnitState& state );
	void ValidateConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const;
	void MoveConventionalUnit( GSE_CALLABLE, Unit* unit, map::tile::Tile* destination, size_t caller );
	void ResetConventionalSpent( GSE_CALLABLE, Unit* unit );
	// Request preserves work/spent; Dispatch uses the actual movement remainder.
	// None requests menu cancellation, retaining both accumulated bytes.
	void ValidateConventionalWork( Unit* unit, ConventionalUnitState::WorkOrder order, size_t caller, bool dispatch ) const;
	void SetConventionalWork( GSE_CALLABLE, Unit* unit, ConventionalUnitState::WorkOrder order, size_t caller, bool dispatch );
	struct ConventionalCommands {
		ConventionalUnitContext::Package package = ConventionalUnitContext::Package::Combat;
		std::vector<int32_t> actions; // 0 cancel, 1..5 WorkOrder, 6 found Base
	};
	// Detached presentation query. Existing command preparation decides admission;
	// unavailable world/turn authority yields no actions and never mutates state.
	ConventionalCommands QueryConventionalCommands( Unit* unit ) const;
	// Caller stages any knowledge/territory changes before Commit. No callback or
	// world mutation may intervene. Notify retires the removed wrapper afterward.
	class PreparedConventionalRemoval {
	public:
		~PreparedConventionalRemoval();
		PreparedConventionalRemoval( PreparedConventionalRemoval&& ) noexcept;
		PreparedConventionalRemoval& operator=( PreparedConventionalRemoval&& ) noexcept;
		bool Removes( size_t id ) const;
		void Commit() noexcept;
		void Notify( GSE_CALLABLE );
	private:
		friend class UnitManager;
		friend class base::BaseManager;
		void Query(const std::function<void()>& query) const;
		struct Data;
		explicit PreparedConventionalRemoval( std::unique_ptr<Data> data );
		std::unique_ptr<Data> m_data;
	};
	PreparedConventionalRemoval PrepareConventionalRemoval( GSE_CALLABLE, const std::vector<size_t>& ids, PreparedConventionalUnit* created = nullptr, Unit* battle_survivor = nullptr );
	// Last-base disposition: retire this owner's complete old roster and stage
	// the admitted restart package at one site. The Base caller commits its own
	// registry and faction state in the same accumulation, before Notify.
	class PreparedFactionUnits {
	public:
		~PreparedFactionUnits();
		PreparedFactionUnits( PreparedFactionUnits&& ) noexcept;
		PreparedFactionUnits& operator=( PreparedFactionUnits&& ) noexcept;
		void Commit() noexcept;
		void Notify( GSE_CALLABLE );
	private:
		friend class UnitManager;
		friend class base::BaseManager;
		void Query(const std::function<void()>& query) const;
		void Refresh(Unit* unit);
		struct Data;
		explicit PreparedFactionUnits( std::unique_ptr<Data> data );
		std::unique_ptr<Data> m_data;
	};
	PreparedFactionUnits PrepareFactionUnits( GSE_CALLABLE, slot::Slot* owner, map::tile::Tile* destination,
		const std::vector<std::pair<Def*,ConventionalUnitState>>& replacements,
		const map::SmacWorkerKnowledge& territory, PreparedConventionalUnit* transient = nullptr );
	void DespawnUnit( GSE_CALLABLE, const size_t unit_id );

	MoraleSet* GetMoraleSet( const std::string& name ) const;
	Unit* GetUnit( const size_t id ) const;
	Def* GetUnitDef( const std::string& name ) const;
	const std::map< size_t, Unit* >& GetUnits() const;
	// Live record order, preserved by removal and save/restore. Numeric IDs are
	// identities and must not choose free support or equal-distance casualties.
	const std::vector< size_t >& GetUnitRecordOrder() const { return m_unit_order; }

	void ProcessUnprocessed( GSE_CALLABLE );
	void PushUpdates();

	WRAPDEFS_PTR( UnitManager )

	void Serialize( types::Buffer& buf ) const;
	void Deserialize( GSE_CALLABLE, types::Buffer& buf );

public:
	// TODO: limit access
	typedef std::function< void() > cb_oncomplete;
	const std::string* MoveUnitToTile( GSE_CALLABLE, Unit* unit, map::tile::Tile* dst_tile, const cb_oncomplete& on_complete );
	const std::string* AttackUnitValidate( GSE_CALLABLE, Unit* attacker, Unit* defender );
	gse::Value* const AttackUnitResolve( GSE_CALLABLE, Unit* attacker, Unit* defender );
	void AttackUnitApply( GSE_CALLABLE, Unit* attacker, Unit* defender, gse::Value* const resolutions );
	void RefreshUnit( GSE_CALLABLE, const Unit* unit );

private:
	friend class game::backend::Game;
	void Deserialize( GSE_CALLABLE, types::Buffer& buf, const ConventionalUnitResolver& resolver, bool defer_home = false );
	ConventionalUnitContext ResolveConventional( const ConventionalUnitState& state, const slot::Slot* owner ) const;
	struct PreparedConventionalAttack {
		Unit* survivor;
		Unit* loser;
		ConventionalUnitState survivor_state;
		faction::SmacHostileState history;
		smac_scout::AttackResult result;
		std::optional<uint16_t> palette_identifier;
	};
	bool HasForeignOccupant(const Unit* actor,const map::tile::Tile* destination) const;
	PreparedConventionalAttack PrepareConventionalAttack(Unit* actor,map::tile::Tile* destination,size_t caller) const;
	void ApplyConventionalAttack(GSE_CALLABLE,PreparedConventionalAttack prepared);
	void ValidateConventionalCreation( size_t id, const Def* def, const slot::Slot* owner, const map::tile::Tile* tile,
		const ConventionalUnitState& state, const ConventionalUnitResolver& resolver, bool early_startup = false, bool defer_home = false ) const;
	struct PreparedConventionalMove {
		Unit* unit;
		map::tile::Tile* source;
		map::tile::Tile* destination;
		ConventionalUnitState state;
		// Absence is an earlier ZOC refusal, not a movement lottery outcome.
		std::optional< smac_move::Result > result;
		std::optional< int32_t > nearest_distance;
		bool update_nearest = false;
		std::optional< map::SmacWorkerKnowledge > sight;
	};
	PreparedConventionalMove PrepareConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const;
	struct PreparedConventionalWork {
		map::tile::Tile* tile;
		std::vector<std::pair<Unit*,ConventionalUnitState>> units;
		std::optional<uint16_t> terraforming;
		std::optional<map::SmacWorkerKnowledge> knowledge;
	};
	PreparedConventionalWork PrepareConventionalWork( Unit* unit, ConventionalUnitState::WorkOrder order, size_t caller, bool dispatch ) const;
	void PreflightConventionalStartup( const ConventionalUnitResolver& resolver ) const;
	void SealConventionalStartup() noexcept;
	Game* m_game = nullptr;

	std::unordered_map< std::string, MoraleSet* > m_unit_moralesets = {};
	std::unordered_map< std::string, Def* > m_unit_defs = {};
	std::map< size_t, Unit* > m_units = {};
	std::vector< size_t > m_unit_order;
	struct pending_unit_t {
		Unit* unit;
		bool is_new;
	};
	std::deque< pending_unit_t > m_unprocessed_units = {};
	// Restored units need the same frontend publication, without new-unit hooks.
	void RegisterUnit( Unit* unit );
	void TriggerSpawnHook( GSE_CALLABLE, Unit* unit );
	bool HasUnitId( const size_t id ) const;

	enum unit_update_op_t : uint8_t {
		UUO_NONE = 0,
		UUO_SPAWN = 1 << 0,
		UUO_REFRESH = 1 << 1,
		UUO_DESPAWN = 1 << 2,
	};
	struct unit_update_t {
		unit_update_op_t ops = UUO_NONE;
		const Unit* unit = nullptr;
	};
	std::unordered_map< size_t, unit_update_t > m_unit_updates = {};
	void QueueUnitUpdate( const Unit* unit, const unit_update_op_t op );

	const morale_t GetMorale( GSE_CALLABLE, const int64_t& morale );
	const health_t GetHealth( GSE_CALLABLE, const float health );

private:
	friend class Unit;
	friend class base::BaseManager; // staged home refresh queues
	friend class base::Base; // prepared home-morale refresh transaction
	PreparedConventionalRemoval PrepareRemovalForQuery(const std::vector<size_t>& ids,PreparedConventionalUnit* created=nullptr,Unit* survivor=nullptr);
	PreparedFactionUnits PrepareFactionUnitsForQuery(slot::Slot* owner,map::tile::Tile* destination,
		const std::vector<std::pair<Def*,ConventionalUnitState>>& replacements,const map::SmacWorkerKnowledge& territory,
		PreparedConventionalUnit* transient=nullptr,const PreparedConventionalRemoval* supplied=nullptr);
	using PreparedOccupants=std::map<map::tile::Tile*,std::map<size_t,Unit*>>;
	// Native queries only, in the owning accumulation: no hooks, yields, ID
	// publication or retirement. All registry/placement/knowledge swaps restore.
	void QueryPreparedUnits(const std::map<size_t,Unit*>& units,const std::vector<size_t>& order,
		const PreparedOccupants& occupants,const std::optional<map::SmacWorkerKnowledge>& knowledge,
		const std::vector<std::pair<Unit*,map::tile::Tile*>>& positions,const std::function<void()>& query);
	std::pair< SmacMovementHistory*, size_t > PreparePlacementHistory( const Unit& unit, const map::tile::Tile* tile ) const;
	void CommitPlacementHistory(const std::pair<SmacMovementHistory*,size_t>& prepared) const noexcept;
	map::Map* GetMap() const;
	slot::Slot* GetSlot( const size_t slot_num ) const;
};

}
}
}
