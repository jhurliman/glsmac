#pragma once

#include "game/backend/faction/SmacTechnologyState.h"
#include "game/backend/settings/SmacPredefinedUnitRules.h"
#include <functional>

namespace game::backend::unit {

struct SmacRiotPrototypeMetadata {
    int32_t source_design;
    uint8_t flags,prerequisite;
    std::array<uint8_t,5> auxiliary;
    bool operator==(const SmacRiotPrototypeMetadata& other) const {
        return source_design==other.source_design && flags==other.flags && prerequisite==other.prerequisite && auxiliary==other.auxiliary;
    }
};
struct SmacCustomDesign {
	int32_t id = 0;
	uint32_t chassis = 0, weapon = 0, defense = 0;
	uint8_t reactor = 1, cost = 0, plan = 0, carry = 0;
	uint32_t abilities = 0, faction_exclusion = 0;
	bool active = true, prototyped = false;
	std::optional<std::string> name; // absent until an actual naming producer owns it
	std::optional<SmacRiotPrototypeMetadata> riot;
	bool operator==( const SmacCustomDesign& other ) const;
};

struct SmacDesignOwner {
	int32_t simulation_id = 0;
	std::vector< SmacCustomDesign > designs; // ascending explicit IDs in this owner's 64-slot block
	int32_t starting_scout = 2; // saved selection, not recomputed during restore
	bool operator==( const SmacDesignOwner& other ) const;
};

// Mutable owned designs; component indices refer to the frozen unit catalog.
// Generation is a bounded human startup producer, not a workshop/AI classifier.
struct SmacStartupDesignState {
	static constexpr size_t MAX_WIRE_BYTES = 16 * 1024;
	int32_t global_difficulty = 0; // original semantic index, independent of port encoding
	std::vector< SmacDesignOwner > owners;
	static int32_t OriginalDifficulty( const std::string& canonical_name );
	static SmacDesignOwner GenerateEmptyOwner( const faction::SmacFactionTechnologyState& technology,
		const settings::SmacTechnologyRules& technologies, const SmacPredefinedUnitCatalog& catalog, int32_t global_difficulty );
	// Separate original startup pass: mark active records, keep last active Recon.
	static void MarkAndSelect( SmacDesignOwner& owner, const SmacPredefinedUnitCatalog& catalog );
	static SmacStartupDesignState NewWorld( const faction::SmacTechnologyState& technology,
		const settings::SmacTechnologyRules& technologies, const settings::SmacPredefinedUnitRules& units, int32_t global_difficulty,
		const std::function<void(const faction::SmacFactionTechnologyState&)>& before_generate = {});
    // Raw helper result: a negative human duplicate result retains the source.
    // Bounded measured starting packages; no generic workshop policy.
    int32_t RequestHumanRiotPrototype(int32_t source,int32_t recipient,bool morgan,const SmacPredefinedUnitCatalog& catalog);
	void Validate( const SmacPredefinedUnitCatalog& catalog ) const;
	const SmacDesignOwner* Find( int32_t simulation_id ) const;
	bool operator==( const SmacStartupDesignState& other ) const;
	types::Buffer Serialize( const SmacPredefinedUnitCatalog& catalog ) const;
	static SmacStartupDesignState Deserialize( const std::string& wire, const SmacPredefinedUnitCatalog& catalog );
};

}
