#pragma once

#include "game/backend/unit/SmacPredefinedUnitCatalog.h"
#include "types/Buffer.h"

namespace game::backend::settings {

// Frozen host-owned source tables and their checked, initial predefined records.
// This is not a mutable faction design registry or a live Unit/Def catalog.
struct SmacPredefinedUnitRules {
	static constexpr size_t MAX_WIRE_BYTES = 24 * 1024;
	unit::SmacPredefinedUnitCatalog catalog;
	static SmacPredefinedUnitRules Parse( const std::string& source, const SmacTechnologyRules& technologies );
	void Validate( const SmacTechnologyRules& technologies ) const;
	bool operator==( const SmacPredefinedUnitRules& other ) const;
	types::Buffer Serialize( const SmacTechnologyRules& technologies ) const;
	static SmacPredefinedUnitRules Deserialize( const std::string& bytes, const SmacTechnologyRules& technologies );
};

}
