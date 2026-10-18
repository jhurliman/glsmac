#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "types/Buffer.h"

namespace game::backend::base {

// Stable semantic tags, not original executable indices or build eligibility.
enum class ProductionItemKind : int64_t { Unit = 1, Facility, Orbital, Project, Stockpile };

struct ProductionItem {
	ProductionItemKind kind;
	std::string id; // opaque port identity; empty only for Stockpile
	// Canonical positional identity. Absence preserves legacy/unresolved state;
	// consumers never infer this value from the opaque ID or display name.
	std::optional< int32_t > smac_source_id;

	void Validate() const;
	bool operator==( const ProductionItem& other ) const;
};

struct ProductionState {
	ProductionItem current_item;
	int64_t minerals;
	ProductionItem baseline_item;
	int64_t baseline_minerals;
	// A missing queue is unconfigured, not an empty human production queue.
	// Only active logical entries are owned; no unused original storage implied.
	std::optional< std::vector< ProductionItem > > queue;
	// Old port frames predate Hurry commands and therefore have a clear marker.
	bool hurry_partial_paid = false;
	static constexpr size_t MAX_QUEUE_ITEMS = 9; // port budget: maximum length in retained queue cases
	static constexpr size_t MAX_ID_BYTES = 256; // canonical port identity budget

	void Validate() const;
	bool operator==( const ProductionState& other ) const;
};

// An absent record means unconfigured, not zero minerals or Stockpile.
// Signed64 is a port storage domain; future arithmetic must validate its inputs.
// This codec neither resolves definitions nor makes a target buildable.
types::Buffer SerializeProductionState( const std::optional< ProductionState >& state );
std::optional< ProductionState > DeserializeProductionState( types::Buffer& buffer );

}
