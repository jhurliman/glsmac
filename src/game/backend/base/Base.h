#pragma once
#include "game/backend/faction/SmacDesignCounts.h"

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
#include "SmacHurry.h"
#include "SmacBaseEnergy.h"
#include "BaseResourceCalculation.h"
#include "SmacColonyContinuation.h"
#include "SmacBaseUpkeepCache.h"
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
namespace unit { struct SmacWorkshopDraft; class Unit; class UnitManager; class Def; enum class SmacMoraleMode : uint8_t; }
namespace faction {
class Faction;
}
namespace map::tile {
class Tile;
}

namespace base {

class FacilityDef;
struct SmacManualSwitchResult;

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
	// Flood damage can reduce the retained count before its later worker pass.
	// Pop nodes continue projecting the stored allocation until that pass commits.
	size_t GetPopulation() const noexcept { return m_retained_population.value_or(m_pops.size()); }

	Pop* const AddPop( const Pop& pop );
	void RemovePop( const size_t pop_id );
	void ChangePopType( GSE_CALLABLE, const size_t pop_id, const std::string& def_id );
	void SetPopAssignment( GSE_CALLABLE, Pop* pop, const std::string& def_id, map::tile::Tile* tile );
	SmacBaseResourceCalculation PrepareSmacResourceCalculation( SmacBaseResourcePhase phase = SmacBaseResourcePhase::Ordinary ) const;
	int64_t GetAccumulatedNutrients() const;
	void SetAccumulatedNutrients( const int64_t nutrients );
	const std::optional< ProductionState >& GetProductionState() const;
	// Detached chooser projection; execution rechecks the complete saved state.
	gse::Value* CopySmacProductionChoices( GSE_CALLABLE ) const;
	gse::Value* CopySmacWorkshop( GSE_CALLABLE, const unit::SmacWorkshopDraft* draft=nullptr ) const;
	std::string SmacWorkshopState() const;
	void SetProductionState( GSE_CALLABLE, const std::optional< ProductionState >& production );

	bool HasSmacResourceContext() const;
	SmacBaseResourceContext GetSmacResourceContext() const;
	// Explicit state replacement; cannot adopt a legacy Base or change ownership.
	void SetSmacResourceContext( GSE_CALLABLE, const SmacBaseResourceContext& context );
	// Read-only menu/initial-selector queries from this Base's actual population
	// and bound owner. They do not run or mutate the manual worker handler.
	SmacSpecialistMenu GetSmacSpecialistMenu( std::optional< size_t > pop_id = std::nullopt ) const;
	uint8_t SelectAutomaticSmacSpecialist() const;
	std::string SmacSpecialistSelectionState() const;
	void OpenSmacSpecialistSelection( GSE_CALLABLE, size_t pop_id, bool apply );
	void SelectSmacSpecialist( GSE_CALLABLE, size_t pop_id, int32_t type, bool apply );
	gse::Value* CopySmacSpecialistChoices( GSE_CALLABLE, size_t pop_id ) const;
	void RequireSmacSpecialistSelection() const;
	bool CanSmacRename() const;
	void OpenSmacRename(GSE_CALLABLE,bool apply);
	gse::Value* CopySmacRenameChoices(GSE_CALLABLE) const;
	gse::Value* CopySmacQueueEditor(GSE_CALLABLE) const;
	std::string SmacGovernorSelectionState() const;
	gse::Value* CopySmacGovernorSettings(GSE_CALLABLE) const;
	struct HurryOffer { std::optional<SmacHurryContext> context; int32_t credits,reserved; std::string expected,reason; };
	HurryOffer GetSmacHurryOffer() const;
	gse::Value* CopySmacHurryPreview(GSE_CALLABLE,int32_t amount) const;
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
		std::vector<size_t> removed_units; // shortage order; may include completed_unit
		bool removed_base = false; // caller must not dereference the retired Base
		std::optional<SmacColonyUpkeepPause> colony_pause;
        std::optional<SmacBaseEnergyCalculation> retained_energy;
        bool needs_former_owner=false; // no part of this Base attempt was published
        bool ownership_changed=false; // skip old-owner research/income/tail
	};
	SmacProductionResult ProcessSmacProduction( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode );
	SmacProductionResult ProcessSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode );
	// The complete native choice controller uses this entry once its UI and
	// abandonment provider are installed. Existing upkeep remains strict.
	SmacProductionResult BeginSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode );
	const std::optional<SmacBaseUpkeepCache>& GetSmacUpkeepCache() const;
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
	static Base* Deserialize( types::Buffer& buf, Game* game, bool allow_retained_population = true );

	WRAPDEFS_DYNAMIC( Base );

	WRAPDEF_SERIALIZABLE;

	void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override;

private:
    std::optional<faction::SmacDesignCounts>* SmacProductionReferencesTarget() const noexcept;
    std::optional<faction::SmacDesignCounts> PrepareSmacProductionReferences(const std::optional<ProductionState>& replacement) const;
	friend class backend::Game;
	friend class unit::UnitManager;
	struct ProductionChoice { int32_t source_id; std::string name; int32_t cost; int64_t minerals; bool confirmation; };
	void RequireSmacProductionSelection() const;
	std::vector< ProductionChoice > GetSmacProductionChoices() const;
	SmacManualSwitchResult PrepareSmacProductionSelection( int32_t source_id, bool confirm ) const;
	void SelectSmacProduction( GSE_CALLABLE, int32_t source_id, const std::string& expected, bool confirm );
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
	std::optional<size_t> m_retained_population;
	std::optional<SmacBaseUpkeepCache> m_smac_upkeep_cache;
	// Optional view interaction, not a suspended turn or saved gameplay authority.
	struct SpecialistSelectionRequest { int64_t id; size_t pop_id; std::string expected; };
	int64_t m_next_specialist_request = 1;
	std::optional<SpecialistSelectionRequest> m_specialist_request;
	struct RenameRequest { int64_t id; std::string expected; };
	int64_t m_next_rename_request = 1;
	std::optional<RenameRequest> m_rename_request;
	std::vector< map::tile::coords_t > AllocationCoordinates( const SmacBaseAllocationState& allocation, std::optional< size_t > population = {} ) const;
	void ValidateAllocationProjection( bool registered, bool attached = false ) const;
	void RequireGenericPopulationMutation() const;
	SmacBaseAllocationState CalculateSmacWorkers( const SmacBaseAllocationState& allocation, const SmacBaseResourceContext& context, SmacBaseResourcePhase phase = SmacBaseResourcePhase::Ordinary, std::optional< size_t > population = {},
		const unit::Unit* prepared_unit = nullptr, const ProductionState* production = nullptr, std::optional<int32_t> mineral_support = {}, const FacilityDef* granted = nullptr, std::optional<uint32_t> unavailable = {} ) const;
	void PublishSmacCalculatedWorkers( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles,
		const SmacBaseResourceContext& context, SmacBaseResourcePhase phase );
	SmacAllocationRoles SpecialistSelectionRoles() const;
	void PublishSmacSpecialistState( GSE_CALLABLE, const SmacBaseAllocationState&, const SmacBaseResourceContext& );
	void PublishSmacWorkerState( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles, const SmacBaseResourceContext& context );
	SmacProductionResult ProcessSmacProductionAndPopulation( GSE_CALLABLE, const SmacAllocationRoles& roles,
		const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode, bool population_step,
		bool allow_colony_pause = false, const SmacColonyUpkeepPause* pause = nullptr, const SmacBaseUpkeepCache* retained = nullptr, std::optional<int32_t> former_repair = {}, bool* publication = nullptr );
	void ValidateSmacColonyPause( const SmacColonyUpkeepPause& pause ) const;
	SmacProductionResult ResumeSmacAbandonmentTail( GSE_CALLABLE, const SmacBaseUpkeepCache& retained,
		const SmacAllocationRoles& roles, unit::SmacMoraleMode mode );
	SmacProductionResult ResumeSmacColonyUpkeep( GSE_CALLABLE, const SmacColonyUpkeepPause& pause,
		SmacColonyChoice choice, const SmacAllocationRoles& roles, const std::function< unit::Def*( int32_t ) >& definition );
    struct SmacDefectionAttempt;
    SmacProductionResult CompleteSmacRiotDefection(GSE_CALLABLE,SmacProductionResult result,SmacDefectionAttempt& attempt);
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
		const std::optional< ProductionState >& production, int64_t nutrients, std::optional< size_t > population = {}, std::vector<std::pair<Base*,facilities_t>> facility_changes = {}, slot::Slot* captured_owner = nullptr, std::vector<Base*> refresh_only = {}, bool finish_upkeep = false, PreparedSmacWorkerState* previous = nullptr, bool riot_defection = false );
	SmacBaseResourceCalculation CalculateSmacResourceCalculation( SmacBaseResourcePhase phase,
		const SmacBaseAllocationState* allocation, std::optional< size_t > population = {},
		std::optional< int64_t > stored = {}, const unit::Unit* prepared_unit = nullptr, std::optional<int32_t> mineral_support = {}, const FacilityDef* granted = nullptr, const std::vector<size_t>& removed = {} ) const;
	std::optional<int32_t> CalculateSmacEcologyDamage( const SmacBaseResourceCalculation& calculation,
		const SmacBaseAllocationState* allocation, size_t population, const FacilityDef* granted ) const;
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
