#pragma once

#include "SmacTechnologyState.h"

namespace game::backend::faction {

using SmacSocialRatings = std::array< int32_t, 11 >;

struct SmacSocialSnapshots {
    std::optional<SmacSocialRatings> retained,third;
    bool operator==(const SmacSocialSnapshots& other) const {return retained==other.retained && third==other.third;}
};
using SmacSocialSnapshotStorage=std::array<SmacSocialSnapshots,8>;

struct SmacFactionSocialState {
	enum class Phase : int32_t { Early, Refreshed };
	SmacSimulationAdmission admission;
	std::array< int32_t, 4 > selected{}, pending{};
	SmacSocialRatings current{};
	// The original early reset does not initialize either vector. Absence is
	// explicit when a new port world has no previously bound value to preserve.
	std::optional< SmacSocialRatings > intrinsic, retained;
	Phase phase = Phase::Early;
    std::optional<SmacSocialRatings> third; // separate mode1 output, never an alias of retained
	bool operator==( const SmacFactionSocialState& other ) const;
};

struct SmacSocialState {
	static constexpr size_t MAX_WIRE_BYTES = 8 * 1024;
	std::vector< SmacFactionSocialState > factions;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const;
	void ValidateTurn( size_t turn ) const;
	bool operator==( const SmacSocialState& other ) const;
	const SmacFactionSocialState* Find( int32_t simulation_id ) const;

	// Reset only the fields written by the original early reset. This is not a
	// world-retirement or restore operation and preserves supplied snapshots.
	static void ResetForStartup( SmacFactionSocialState& faction ) noexcept;
    static SmacSocialSnapshotStorage FreshImageSnapshots();
    void PreserveSnapshots(SmacSocialSnapshotStorage& storage) const;
	static SmacSocialState NewWorld( const std::vector< SmacSimulationAdmission >& admissions,
		const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles,
        const SmacSocialSnapshotStorage* snapshots=nullptr );

	// Supported context: the four frozen neutral model-zero rows, no project or
	// Nexus effects. Duplicate IMMUNITY records retain their shared-slot cost;
	// each admitted record applies the same idempotent final-axis clamp.
	static SmacSocialRatings BuildIntrinsic( const std::string& faction_id,
		const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles );
	static SmacSocialRatings RefreshNeutral( const SmacSocialRatings& intrinsic, const std::string& faction_id,
		const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles );
    // Late turn-zero faction initializer, after the original package factories.
    // Preserve current/retained ratings and Early phase until this owner's upkeep.
    void BuildStartupIntrinsic(int32_t owner,const settings::SmacTechnologyRules& technologies,const settings::SmacFactionStartupRules& profiles);
    // Owner identity is the saved simulation ID, independently of transport/faction.
    // Already refreshed owners remain exact; unrelated owners are never refreshed.
    void RefreshOwnerForFirstCommand(int32_t owner,const settings::SmacTechnologyRules& technologies,const settings::SmacFactionStartupRules& profiles);
	// Stage all owners before replacing this value. Already refreshed owners
	// remain byte-for-byte values; decoding never invokes this transition.
	void RefreshForFirstCommand( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles );

	types::Buffer Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const;
	static SmacSocialState Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles );
};

}
