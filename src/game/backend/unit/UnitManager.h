#pragma once

#include "gse/GCWrappable.h"

#include "gse/value/Object.h"

#include "Types.h"
#include "game/ConventionalUnitVisual.h"
#include "ConventionalUnitState.h"
#include "PreparedConventionalSight.h"
#include "SmacNativeUnitRecord.h"
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
struct TerrainRenderUpdate;
namespace smac_wildlife {struct Input;struct Result;}
namespace tile {
class Tile;
}
}

namespace slot {
class Slot;
}

namespace base { class Base; class BaseManager; class SmacNativeSpawnPlan; }

namespace unit {

struct SmacStartupDesignState;
class MoraleSet;
class Def;
class Unit;

CLASS( UnitManager, gse::GCWrappable )
public:
	UnitManager( Game* game );
	~UnitManager();

    std::string GetDisplayName(const Unit& unit) const;
    std::optional<ConventionalUnitVisual> GetVisual(const Unit& unit) const;
	void Clear();
	void DefineMoraleSet( MoraleSet* moraleset );
	void UndefineMoraleSet( const std::string& id );
	void DefineUnit( Def* def );
	void UndefineUnit( const std::string& id );
	void SpawnUnit( GSE_CALLABLE, Unit* unit );
	SmacNativeUnitStats QueryNativeUnit(const SmacNativeUnitRecord& state,int32_t opposing_faction=0) const;
    // Owning accumulation only. The plan retains attempted factories; this
    // batch stages successful bodies, stacks, sight and native counters together.
    class PreparedNativeBirths {
    public:
        ~PreparedNativeBirths();
        PreparedNativeBirths(PreparedNativeBirths&&) noexcept;
        PreparedNativeBirths& operator=(PreparedNativeBirths&&) noexcept;
        size_t Size() const;
        const Unit& Candidate(size_t index) const;
        void Commit() noexcept;
        void Notify(GSE_CALLABLE);
    private:
        friend class UnitManager;
        friend class game::backend::Game;
        friend class base::Base;
        friend class base::BaseManager;
        void Query(const std::function<void()>& query) const;
        struct Data;
        explicit PreparedNativeBirths(std::unique_ptr<Data> data);
        std::unique_ptr<Data> m_data;
    };
    class PreparedGlobalEcology {
    public:
        ~PreparedGlobalEcology();
        PreparedGlobalEcology(PreparedGlobalEcology&&) noexcept;
        PreparedGlobalEcology& operator=(PreparedGlobalEcology&&) noexcept;
        const map::smac_wildlife::Result& Result() const;
        const map::TerrainRenderUpdate* TerrainPayload() const;
        void Commit();
        void Notify(GSE_CALLABLE);
    private:
        friend class UnitManager;
        struct Data;
        explicit PreparedGlobalEcology(std::unique_ptr<Data> data);
        std::unique_ptr<Data> m_data;
    };
    // Owning synchronous flora/fauna composition. Independent original global
    // wildlife inputs are supplied by the round controller, never synthesized
    // from missing legacy state. No scripts/yields/mutations before commit.
    PreparedGlobalEcology PrepareGlobalEcology(GSE_CALLABLE,const map::smac_wildlife::Input& input);
    PreparedNativeBirths PrepareEcologyNativeBirths(GSE_CALLABLE, const base::SmacNativeSpawnPlan& plan);
    // Synchronous C++ producer only; these closures must not escape, yield or
    // invoke scripts. All operations share one detached factory/sight/stack
    // transaction. Throwing discards the complete transaction.
    struct NativeBirthFactory {
        std::function<int32_t(int32_t,std::array<int32_t,2>,std::optional<uint8_t>)> create;
        std::function<int32_t(int32_t)> capacity;
        std::function<void(int32_t)> sentry,promote_stack;
        std::function<SmacNativeUnitRecord(int32_t)> record;
        std::function<const map::SmacWorkerKnowledge::Data&()> knowledge;
    };
    PreparedNativeBirths PrepareNativeBirths(GSE_CALLABLE,int32_t width,int32_t height,bool horizontal_wrap,
        const std::function<void(const NativeBirthFactory&)>& produce);

	// Same owning accumulation: no callbacks, yields or world mutations between
	// preparation and commit. Destruction before commit publishes nothing.
	class PreparedConventionalUnit {
	public:
		~PreparedConventionalUnit();
		PreparedConventionalUnit( PreparedConventionalUnit&& ) noexcept;
		PreparedConventionalUnit& operator=( PreparedConventionalUnit&& ) noexcept;
		const Unit& Candidate() const; // prepared immutable record, before Commit only
        const SmacStartupDesignState* PrototypeDesigns() const; // detached history for later Base consequences
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
	Unit* SpawnStartingConventionalUnit( GSE_CALLABLE, Def* def, slot::Slot* owner, map::tile::Tile* tile, SmacMoraleMode mode, bool compensation_pod = false );
	bool HasPendingConventionalUnits() const;
	void RequireConventionalUnitsReady() const;
	void CompleteConventionalHomeRestore();
	bool HasConventionalHome( size_t base_id ) const;
	void UpdateConventionalUnit( GSE_CALLABLE, Unit* unit, const ConventionalUnitState& state );
	void ValidateConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const;
	void MoveConventionalUnit( GSE_CALLABLE, Unit* unit, map::tile::Tile* destination, size_t caller );
	void ResetConventionalSpent( GSE_CALLABLE, Unit* unit );
    enum class HumanOrderCommand { Hold, HoldTen, MenuHold, Activate, Skip };
    ConventionalUnitState PrepareHumanOrder(Unit* actor, HumanOrderCommand command, size_t caller) const;
    void ApplyHumanOrder(GSE_CALLABLE,Unit* actor,HumanOrderCommand command,size_t caller);
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
        void RefreshSurvivor(const Unit* actor); // stage alongside a shared design edit, before Commit
		void Commit() noexcept;
		void Notify( GSE_CALLABLE );
	private:
		friend class UnitManager;
		friend class base::BaseManager;
		void Query(const std::function<void()>& query) const;
        void PrepareBaseRemovalHomes(const base::Base& removed,const base::Base& replacement);
        void QueryWithNativeRecords(const std::function<void()>& query) const;
		void DiscoverDefendedBase(base::Base* target, int32_t observer);
		struct Data;
		explicit PreparedConventionalRemoval( std::unique_ptr<Data> data );
		std::unique_ptr<Data> m_data;
	};
	PreparedConventionalRemoval PrepareConventionalRemoval( GSE_CALLABLE, const std::vector<size_t>& ids, PreparedConventionalUnit* created = nullptr, Unit* battle_survivor = nullptr );
    // Environmental transactions share the same atomic registry/knowledge plan,
    // admitting native casualties in addition to conventional Units. The owning
    // terrain/Base transaction selects casualties before changing surface class.
    PreparedConventionalRemoval PrepareWorldUnitRemoval(GSE_CALLABLE,const std::vector<size_t>& ids);
    PreparedConventionalRemoval PrepareBaseCenterRemoval(GSE_CALLABLE,const base::Base& removed);
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

    // Same accumulation as the complete Base consequence. This stages only
    // resolved Unit changes; recipient/radius/order decisions belong to the
    // original defection producer, not to this publication primitive.
    class PreparedDefectionUnits {
    public:
        ~PreparedDefectionUnits();
        PreparedDefectionUnits(PreparedDefectionUnits&&) noexcept;
        PreparedDefectionUnits& operator=(PreparedDefectionUnits&&) noexcept;
        void Commit() noexcept;
    private:
        friend class UnitManager;
        friend class base::BaseManager;
        friend class base::Base;
        void Query(const std::function<void()>& query) const;
        struct Data;
        explicit PreparedDefectionUnits(std::unique_ptr<Data> data);
        std::unique_ptr<Data> m_data;
    };
    PreparedDefectionUnits PrepareDefectionUnits(GSE_CALLABLE,slot::Slot* recipient,
        const std::vector<std::pair<size_t,ConventionalUnitState>>& changes);


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
	void SyncNativeObservationMasks(const map::SmacWorkerKnowledge& knowledge) noexcept;
    void CommitUnitDisclosures(const std::vector<uint64_t>& ids) noexcept;
	void ValidateNativeCreation(size_t id,const Def* def,const map::tile::Tile* tile,const SmacNativeUnitRecord& state) const;
    void ValidateNativeIndexBindings(const std::vector<Unit*>& ordered) const;
    // Detached native preparation for command validation, without scripts,
    // publication, allocator IDs or a required GSE call frame.
    PreparedNativeBirths PrepareNativeBirthsQuery(int32_t width,int32_t height,bool horizontal_wrap,
        const std::function<void(const NativeBirthFactory&)>& produce) const;
	ConventionalUnitContext ResolveConventional( const ConventionalUnitState& state, const slot::Slot* owner ) const;
	struct PreparedConventionalAttack {
		Unit* survivor;
		Unit* loser;
		ConventionalUnitState survivor_state;
		faction::SmacHostileState history;
		smac_scout::AttackResult result;
		std::optional<uint16_t> palette_identifier;
		base::Base* defeated_base;
	};
	bool HasForeignOccupant(const Unit* actor,const map::tile::Tile* destination) const;
	PreparedConventionalAttack PrepareConventionalAttack(Unit* actor,map::tile::Tile* destination,size_t caller) const;
	void ApplyConventionalAttack(GSE_CALLABLE,PreparedConventionalAttack prepared);
	void ValidateConventionalCreation( size_t id, const Def* def, const slot::Slot* owner, const map::tile::Tile* tile,
		const ConventionalUnitState& state, const ConventionalUnitResolver& resolver, bool early_startup = false, bool defer_home = false ) const;
    using StackChanges=std::vector<std::pair<Unit*,ConventionalUnitState::Stack>>;
    StackChanges PrepareConventionalStackMove(Unit* unit,map::tile::Tile* destination,bool authenticate_new=false) const;
    static void CommitStackChanges(const StackChanges& changes) noexcept;
    struct PreparedConventionalEncounter {
        PreparedNativeBirths birth;
        SmacGameplayRandomState random;
    };
	struct PreparedConventionalMove {
		Unit* unit;
		map::tile::Tile* source;
		map::tile::Tile* destination;
		ConventionalUnitState state;
		// Absence is an earlier ZOC refusal, not a movement lottery outcome.
		std::optional< smac_move::Result > result;
		std::optional< int32_t > nearest_distance;
		bool update_nearest = false;
		PreparedConventionalSight sight;
        StackChanges stacks;
        std::optional<PreparedConventionalEncounter> encounter;
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
	void RegisterUnit( Unit* unit, bool count_as_new = true );
    void AdjustDesignCount(const Unit& unit,int32_t delta) const noexcept;
	void TriggerSpawnHook( GSE_CALLABLE, Unit* unit );
	bool HasUnitId( const size_t id ) const;

	enum unit_update_op_t : uint8_t {
		UUO_NONE = 0,
		UUO_SPAWN = 1 << 0,
		UUO_REFRESH = 1 << 1,
		UUO_DESPAWN = 1 << 2,
        UUO_OWNER_CHANGE = 1 << 3,
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
    void QueryDefectionUnits(const PreparedConventionalUnit* created,const PreparedConventionalRemoval& removed,const std::function<void()>& query);
    PreparedDefectionUnits PrepareDefectionUnitsCore(slot::Slot* recipient,
        const std::vector<std::pair<size_t,ConventionalUnitState>>& changes,
        const std::unordered_map<size_t,unit_update_t>& updates);
    PreparedDefectionUnits PrepareDefectionUnitsForQuery(slot::Slot* recipient,
        const std::vector<std::pair<size_t,ConventionalUnitState>>& changes,
        const PreparedConventionalUnit* created=nullptr,const PreparedConventionalRemoval* removed=nullptr);
	PreparedConventionalRemoval PrepareRemovalForQuery(const std::vector<size_t>& ids,PreparedConventionalUnit* created=nullptr,Unit* survivor=nullptr,bool allow_native=false);
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
