#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <functional>

#include "gse/Wrappable.h"
#include "game/backend/MapObject.h"
#include "game/backend/ResourceRelated.h"

#include "types/Buffer.h"

#include "Pop.h"
#include "ProductionState.h"
#include "BaseResourceCalculation.h"
#include "SmacBaseResourceContext.h"
#include "SmacSpecialists.h"
#include "SmacBaseAllocationState.h"

namespace gse::value {
class Object;
class Array;
}

namespace game {
namespace backend {

class Game;
namespace slot {
class Slot;
}
namespace unit { class Unit; class Def; enum class SmacMoraleMode : uint8_t; }
namespace faction {
class Faction;
}
namespace map::tile {
class Tile;
}

namespace base {

class FacilityDef;

class Base : public gse::Wrappable, public MapObject, public ResourceRelated {
public:

	static const size_t GetNextId();
	static const void SetNextId( const size_t id );

	typedef std::map< size_t, Pop > pops_t;
	typedef std::map< std::string, const FacilityDef* > facilities_t;

	Base(
		Game* game,
		const size_t id,
		slot::Slot* owner,
		faction::Faction* faction, // faction may differ from owner's faction in some cases, i.e. after being conquered
		map::tile::Tile* tile,
		const std::string& name,
		const facilities_t& facilities,
		const pops_t& pops,
		const size_t next_pop_id = 1,
		const int64_t accumulated_nutrients = 0,
		const std::optional< ProductionState >& production = std::nullopt,
		const std::optional< SmacBaseResourceContext >& resource_context = std::nullopt,
		const std::optional< SmacBaseAllocationState >& allocation = std::nullopt
	);
	virtual ~Base() = default;

	const Game* const GetGame() const;

	Pop* const AddPop( const Pop& pop );
	void RemovePop( const size_t pop_id );
	void ChangePopType( GSE_CALLABLE, const size_t pop_id, const std::string& def_id );
	void SetPopAssignment( GSE_CALLABLE, Pop* pop, const std::string& def_id, map::tile::Tile* tile );
	SmacBaseResourceCalculation PrepareSmacResourceCalculation( SmacBaseResourcePhase phase = SmacBaseResourcePhase::Ordinary ) const;
	int64_t GetAccumulatedNutrients() const;
	void SetAccumulatedNutrients( const int64_t nutrients );
	const std::optional< ProductionState >& GetProductionState() const;
	void SetProductionState( GSE_CALLABLE, const std::optional< ProductionState >& production );
	bool HasSmacResourceContext() const;
	SmacBaseResourceContext GetSmacResourceContext() const;
	// Explicit state replacement; cannot adopt a legacy Base or change ownership.
	void SetSmacResourceContext( GSE_CALLABLE, const SmacBaseResourceContext& context );
	// Read-only menu/initial-selector queries from this Base's actual population
	// and bound owner. They do not run or mutate the manual worker handler.
	SmacSpecialistMenu GetSmacSpecialistMenu( std::optional< size_t > pop_id = std::nullopt ) const;
	uint8_t SelectAutomaticSmacSpecialist() const;
	// Explicit native publication only, in the owning writable GSE scope. No
	// allocation calculation or founding/adoption policy is implied. Existing
	// Pop nodes/IDs/variants survive; ascending IDs receive ascending worker
	// radius positions, followed by the ordered active specialist slots.
	struct SmacAllocationRoles {
		std::string worker;
		std::array< std::string, 7 > specialists;
	};
	void PublishSmacAllocation( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles );
	// Local-human, governor-off recalculation from owned world inputs. Explicit
	// phase selects resource/allocator upkeep inputs; this alone does not post
	// resources or advance a turn. Retains complete valid selections.
	void RecalculateSmacWorkers( GSE_CALLABLE, const SmacAllocationRoles& roles, SmacBaseResourcePhase phase = SmacBaseResourcePhase::Ordinary );
	// Ordinary local tile click. The Game event dispatcher authenticates caller;
	// all edits and the final allocation are staged before native publication.
	void CommandSmacWorkerTile( GSE_CALLABLE, map::tile::Tile* tile );
	// Native upkeep production stage. No script entry point: the turn caller
	// owns once-per-turn invocation. The supplied resolver chooses registered
	// presentation, while the factory authenticates the actual design package.
	struct SmacProductionResult {
		SmacBaseResourceCalculation first_calculation;
		std::optional< size_t > completed_unit;
		bool queue_advanced = false;
		std::optional< SmacBaseResourceCalculation > final_calculation;
	};
	SmacProductionResult ProcessSmacProduction( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode );
	SmacProductionResult ProcessSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode );
	std::optional< SmacBaseAllocationState > GetSmacAllocation() const;
	// Also used by actual Pop, frontend and save reads; absence preserves legacy.
	void ValidateSmacAllocation() const;

	const size_t m_id;
	slot::Slot* m_owner;
	faction::Faction* m_faction;
	std::string m_name;
	facilities_t m_facilities;
	pops_t m_pops;

	static const types::Buffer Serialize( const Base* base );
	static Base* Deserialize( types::Buffer& buf, Game* game );

	WRAPDEFS_DYNAMIC( Base );

	WRAPDEF_SERIALIZABLE;

	void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override;

private:
	friend class backend::Game;
	friend class BaseManager;
	// Only complete native record decoding can request snapshot construction.
	// A running/initializing phase or script callback is not restore authority.
	enum class ConstructionKind { Live, Snapshot, PreparedStartup };
	void CommitStartupPlacement() noexcept;
	Base(
		ConstructionKind kind,
		Game* game, size_t id, slot::Slot* owner, faction::Faction* faction,
		map::tile::Tile* tile, const std::string& name,
		const facilities_t& facilities, const pops_t& pops, size_t next_pop_id,
		int64_t accumulated_nutrients, const std::optional< ProductionState >& production,
		const std::optional< SmacBaseResourceContext >& resource_context,
		const std::optional< SmacBaseAllocationState >& allocation
	);
	Game* const m_game;

	size_t m_next_pop_id = 1;
	int64_t m_accumulated_nutrients = 0;
	std::optional< ProductionState > m_production;
	std::optional< SmacBaseResourceContext > m_resource_context;
	std::optional< SmacBaseAllocationState > m_allocation;
	std::vector< map::tile::coords_t > AllocationCoordinates( const SmacBaseAllocationState& allocation, std::optional< size_t > population = {} ) const;
	void ValidateAllocationProjection( bool registered, bool attached = false ) const;
	void RequireGenericPopulationMutation() const;
	SmacBaseAllocationState CalculateSmacWorkers( const SmacBaseAllocationState& allocation, const SmacBaseResourceContext& context, SmacBaseResourcePhase phase = SmacBaseResourcePhase::Ordinary, std::optional< size_t > population = {},
		const unit::Unit* prepared_unit = nullptr, const ProductionState* production = nullptr ) const;
	void PublishSmacWorkerState( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles, const SmacBaseResourceContext& context );
	SmacProductionResult ProcessSmacProductionAndPopulation( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode, bool population_step );
	class PreparedSmacWorkerState {
	public:
		~PreparedSmacWorkerState();
		PreparedSmacWorkerState( PreparedSmacWorkerState&& ) noexcept;
		void Commit() noexcept;
		void Finish(); // release lock and retire removed wrappers after authority commit
	private:
		friend class Base;
		struct Data;
		explicit PreparedSmacWorkerState( std::unique_ptr< Data > data );
		std::unique_ptr< Data > m_data;
	};
	PreparedSmacWorkerState PrepareSmacWorkerState( GSE_CALLABLE, const SmacBaseAllocationState& allocation,
		const SmacAllocationRoles& roles, const SmacBaseResourceContext& context,
		const std::optional< ProductionState >& production, int64_t nutrients, std::optional< size_t > population = {} );
	SmacBaseResourceCalculation CalculateSmacResourceCalculation( SmacBaseResourcePhase phase,
		const SmacBaseAllocationState* allocation, std::optional< size_t > population = {},
		std::optional< int64_t > stored = {}, const unit::Unit* prepared_unit = nullptr ) const;
	void ValidatePopulationRole( const PopDef* def, const Pop::worked_tile_t& tile ) const;
	uint32_t ValidateSmacSpecialistPopulation() const;
	gse::Value* CopySmacSpecialistMenu( GSE_CALLABLE, std::optional< size_t > pop_id ) const;

	const PopDef* const GetPopDef( GSE_CALLABLE, const std::string& id ) const;

	gse::value::Array* const GetWorkableTiles( GSE_CALLABLE );
	gse::value::Array* const GetWorkedTiles( GSE_CALLABLE );
	gse::value::Array* const GetUnworkedTiles( GSE_CALLABLE );
	gse::Value* CopySmacResourceCalculation( GSE_CALLABLE ) const;
	gse::value::Object* const GetIntake( GSE_CALLABLE );
	gse::value::Object* const GetConsumption( GSE_CALLABLE );

	void TriggerUpdate();
};

}
}
}
