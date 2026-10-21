#pragma once

#include "SmacTechnologyState.h"
#include "SmacSocialState.h"
#include "SmacFactionTurn.h"
#include "SmacEconomyState.h"
#include "SmacRankOrder.h"
#include "SmacDominanceHistory.h"
#include "SmacHostileState.h"
#include "SmacRegionalMilitaryState.h"
#include "SmacRegionalStrategy.h"
#include "SmacEcologyWorld.h"
#include "SmacNativeFactoryState.h"
#include "game/backend/settings/SmacCombatRules.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "game/backend/SmacMovementHistory.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "game/backend/RosterRecords.h"
#include <array>
#include <map>

namespace game::backend::faction {

// Saved simulation identity and its accepted controller. These are checks
// against a canonical roster, not instructions to create/rebind Players.
struct SmacSimulationParticipant {
	int32_t simulation_id;
	int32_t slot;
	std::string gsid;
	roster::Role role;
	std::string faction_id;
	// Independent retained authorities. Successful restart preserves both;
	// restart_prohibited is not a restart-used counter or a zero-Base inference.
	bool active = true;
	bool restart_prohibited = false;
	bool operator==( const SmacSimulationParticipant& other ) const;
};

// Mutable faction strategy, shared by research and other strategy consumers.
// Frozen personality headers supply startup values only. These are full
// signed integers until an accepted research-priority question normalizes them.
struct SmacFactionStrategy {
	std::array<int32_t,4> priorities; // Explore/Discover/Build/Conquer
    // Current95d584 AI policy, independent of research choices/social ratings.
    // Pre-V35 frames and an unimplemented random-personality draw stay unknown.
    std::optional<int32_t> fight={};
	static SmacFactionStrategy NewWorld(const settings::SmacFactionStartupRule& profile,
        uint32_t primary_rules=0,uint32_t secondary_rules=0) noexcept;
	// Absence means cancel: retain every prior integer. Mask0 is a valid answer.
	void ApplyResearchPriorities(std::optional<uint8_t> accepted_mask);
	bool operator==(const SmacFactionStrategy& other) const noexcept {return priorities==other.priorities && fight==other.fight;}
};

// Native retained signed32 fields, not live Unit strengths. All seven positive
// faction slots remain available to the foreign-tier scan, including inactive
// or unoccupied slots. Component metadata has its own byte interpretation.
struct SmacCapabilityState {
	static constexpr size_t Weapon=0, Armor=1, LandSpeed=2, PsiWeapon=3, PsiArmor=4;
	using Values=std::array<int32_t,5>;
	struct Faction {
		Values available{}, foreign{};
		bool operator==(const Faction& other) const noexcept {return available==other.available && foreign==other.foreign;}
	};
	std::array<Faction,7> factions{};
	bool operator==(const SmacCapabilityState& other) const noexcept {return factions==other.factions;}
	// One actual producer call. PSI reads the previous foreign caches, then
	// foreign tiers read peers' retained available caches. No random draw.
	void Refresh(int32_t owner,const SmacFactionTechnologyState& technology,
		const settings::SmacTechnologyRules& technologies,const unit::SmacPredefinedUnitCatalog& catalog,
		const settings::SmacCombatRules& combat,const std::array<uint32_t,8>& relations,
		int32_t psi_percent,bool dream_twister,bool neural_amplifier,bool no_colony_pods,
        const unit::SmacStartupDesignState* designs=nullptr);
};

// Authoritative global inputs, distinct from per-base governor options and
// derived intake. The two solar flags belong to different calculation phases.
struct SmacGlobalResourceModifiers {
	int32_t atmospheric_dust_countdown;
	bool solar_ordinary, solar_upkeep;
	std::optional< int32_t > planetary_governor;
	// RE-SMAC-INITIAL-RESOURCE-MODIFIERS-001. Only new worlds use this reset;
	// restoration supplies every saved field without replaying initialization.
	static SmacGlobalResourceModifiers NewWorld() noexcept;
	void Validate() const;
	bool operator==( const SmacGlobalResourceModifiers& other ) const noexcept;
};

// Current faction assets, ordered nutrients/minerals/energy. Defense satellites
// do not contribute intake and are outside this record. Signed diagnostic
// values restore exactly; a calculation rejects an unmeasured overflow path.
struct SmacSatelliteCounts {
	std::array< int32_t, 3 > resources;
	// Explicit GLSMAC new-campaign asset initialization, not a recovered reset.
	static SmacSatelliteCounts NewWorld() noexcept { return { { 0, 0, 0 } }; }
	std::array< int32_t, 3 > Intake( int32_t population, bool aerospace_complex, bool owns_space_elevator ) const;
	bool operator==( const SmacSatelliteCounts& other ) const noexcept { return resources == other.resources; }
};

using SmacBaseTotals=std::array<int32_t,8>;
// Staged immediate action delta; no live recount. Preserve unavailable legacy
// history. Original signed overflow outside ordinary counts is unmeasured.
std::optional<SmacBaseTotals> AdjustSmacBaseTotal(std::optional<SmacBaseTotals> saved,int32_t owner,int32_t delta);

// Original retained signed words, including native slot0. Query-time pressure
// is derived; it is not a saved latch. A history turn is not a faction birth.
struct SmacVictoryHistory {
	std::array<int32_t,8> economic_deadlines{},economic_paid_costs{},last_base_expansion_or_advice_turn{};
	int32_t voice_host=-1;
	void Initialize(int32_t owner,int32_t turn);
	void RecordBaseExpansion(int32_t owner,int32_t turn);
	bool operator==(const SmacVictoryHistory& other) const noexcept {
		return economic_deadlines==other.economic_deadlines && economic_paid_costs==other.economic_paid_costs
			&& last_base_expansion_or_advice_turn==other.last_base_expansion_or_advice_turn && voice_host==other.voice_host;
	}
};

// Raw native faction status words. Legacy saves retain per-slot absence;
// fresh owner initialization establishes only that owner's entire word.
struct SmacFactionStatusWords {
    std::array<std::optional<uint32_t>,8> values{};
    static SmacFactionStatusWords Cold();
    void Initialize(int32_t owner,int32_t turn);
    bool operator==(const SmacFactionStatusWords& other) const noexcept {return values==other.values;}
};

// Native retained signed words, including unoccupied slots and slot0. H is
// event history, not a count of currently installed ecological facilities.
struct SmacEcologyHistory {
    std::array<int32_t,8> clean_minerals{}, major_atrocities{}, research_aggregate{};
    // Fresh owner initialization clears H/atrocities only. A nonzero-turn
    // restart preserves all three arrays; aggregate has a later owner-pass reset.
    void Initialize(int32_t owner,int32_t turn);
    bool operator==(const SmacEcologyHistory& other) const noexcept {
        return clean_minerals==other.clean_minerals && major_atrocities==other.major_atrocities
            && research_aggregate==other.research_aggregate;
    }
};

struct SmacSimulationState {
	// Supported-campaign admission, not inferred project ownership. Future
	// project consumers must retire this capability before activating effects.
	enum class MovementCapability : int64_t { Unavailable = 0, NoProjectsOrdinaryTerrain = 1 };
	explicit SmacSimulationState( SmacGameplayRandomState random, MovementCapability capability, SmacGlobalResourceModifiers modifiers ) noexcept
		: gameplay_random( random ), movement_capability( capability ), resource_modifiers( modifiers ) {}
	static constexpr size_t MAX_WIRE_BYTES = 256 * 1024;
	static constexpr size_t MAX_TEXT_BYTES = 32 * 1024;
	std::vector< SmacSimulationParticipant > roster;
	SmacTechnologyState technology;
	SmacSocialState social;
	unit::SmacStartupDesignState designs;
	SmacGameplayRandomState gameplay_random;
	MovementCapability movement_capability;
	SmacGlobalResourceModifiers resource_modifiers;
	std::optional< SmacMovementHistory > movement_history;
	// Exactly one record for each roster simulation ID; absence is not zero.
	std::map< int32_t, SmacSatelliteCounts > satellites;
	// Required per-owner financial authority. Absence never means neutral
	// allocation rates or a zero treasury; restoration never posts income.
	std::map< int32_t, SmacEconomyState > economy;
	// V11-V15 port saves did not retain this authority. Keep absence explicit
	// on migration; restoring must not replay personality initialization.
	std::optional<std::map<int32_t,SmacFactionStrategy>> strategy;
	// V11-V16 did not record capability history. Migration retains absence;
	// neither loading nor a read-only query reconstructs it from ownership.
	std::optional<SmacCapabilityState> capabilities;
	// Absent before V18. Loading cannot reconstruct stale owner-summary counts.
	std::optional<SmacRegionalMilitaryState> regional_military;
	// Absent in V11-V20. Migration cannot invent economic attempts or parity of
	// history words from the current map; subsequent Base events preserve absence.
	std::optional<SmacVictoryHistory> victory_history;
	// Secondary native status bit0x100, independent of Standard Rules and
	// regional flags. New-world initialization clears it; restore retains it.
	// Absent before V22: current terrain cannot recover its saved lifetime.
	std::optional<bool> retained_land_region_concentration;
	// All eight native signed totals. Separate from population and byte regional
	// counts: action deltas preserve stale offsets, summary refreshes one owner.
	// V11-V22 migration retains absence rather than recounting current Bases.
	std::optional<SmacBaseTotals> retained_base_totals;
	// Retained native bytes, including slot0 and every region0. Cold new port
	// worlds start0; owner summaries replace only that owner's regions1..127.
	// Nonzero faction initialization preserves them. V11-V23 remain absent.
	std::optional<std::array<SmacRegionalStrategyRow,8>> regional_strategy;
	// Global score provider writes positive slots; slot0 retains its prior word.
	// Dominance reads this table at retained rank IDs, not fresh per-owner scores.
	std::optional<std::array<int32_t,8>> ranking_scores;
    // V11-V30 did not retain graph samples. Loading preserves absence.
    std::optional<SmacDominanceHistory> dominance_history;
    // V32: full seven canonical native source slots. Absent V31 payloads keep
    // their original dense port-owner columns without reinterpretation.
    std::optional<std::array<std::string,8>> dominance_native_keys;
	// V11-V24 did not retain these words. Loading preserves absence and never
	// rebuilds H from facilities, atrocity history or aggregate from current Bases.
	std::optional<SmacEcologyHistory> ecology_history;
	// V11-V25 did not retain these inputs. World metadata and the campaign's
	// active perihelion bit have independent authorities and lifetimes.
	std::optional<SmacEcologyWorld> ecology_world;
	std::optional<bool> perihelion;
	// V11-V26 do not own remembered Governor templates. Preserve absence per
	// native slot, independently of each existing Base configuration.
	std::array<std::optional<uint32_t>,8> remembered_governor{};
	// V11-V27 did not own these independent saved authorities. Warming is a
	// global word, not faction H; new-world reset clears it before owner setup.
	SmacFactionStatusWords faction_status;
	std::optional<int32_t> ecology_warming_count;
	// Absent before V29; restore never initializes or recounts native state.
	std::optional<SmacNativeFactoryState> native_factory;
	// V11-V29 did not retain battle-plan targets. IDs are stable manager IDs,
	// not dense original record indices. Preserve unknown and stale history.
	std::array<std::optional<int64_t>,8> battle_plan_target{};
	SmacHostileState hostility;
	// Standard campaign behavior; custom rule selection must retain this value.
	bool do_or_die = false;
	// Original human-loss ending request, distinct from local focus, inactive
	// roster status and a conquest victory. Absence is not inferred from Bases.
	std::optional<int32_t> human_defeat_request;
    // Explicit single-survivor conquest outcome; never inferred during load.
    std::optional<int32_t> conquest_winner;
    std::optional<SmacFactionTurn> faction_turn; // absent on legacy all-human campaigns; never inferred on load
    bool HasEnding() const noexcept {return human_defeat_request.has_value() || conquest_winner.has_value();}
    // Accepted human controllers, independent of active membership, focus and
    // Single/Host/Player transport roles. Elimination/restart preserves this.
    uint8_t human_membership = 0;
    bool IsHuman(int32_t owner) const noexcept {return owner>=1 && owner<=7 && (human_membership & (1u<<owner));}
	void RefreshCapabilities(int32_t owner,const settings::SmacTechnologyRules& technologies,
		const settings::SmacPredefinedUnitRules& units,const settings::SmacCombatRules& combat,bool no_colony_pods);
    // Absent before the first real update and on migrated port campaigns.
    // Retained through capture/elimination; never reconstructed on restore.
    std::optional<SmacRankOrder> rank_order;
	SmacSatelliteCounts GetSatelliteCounts( int32_t simulation_id ) const;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
    void ValidateTurn(size_t turn) const;
	bool operator==( const SmacSimulationState& other ) const;
	const SmacSimulationParticipant* FindSlot( size_t slot ) const;
	types::Buffer Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
	static SmacSimulationState Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units );
};

}
