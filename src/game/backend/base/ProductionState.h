#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "types/Buffer.h"

namespace game::backend::base {

// Stable semantic tags, not original executable indices or build eligibility.
enum class ProductionItemKind : int64_t { Unit = 1, Facility, Orbital, Project, Stockpile };

struct ProductionItem {
	ProductionItemKind kind;
	std::string id; // opaque owned catalog identity; empty only for Stockpile

	void Validate() const;
	bool operator==( const ProductionItem& other ) const;
};

struct ProductionState {
	ProductionItem current_item;
	int64_t minerals;
	ProductionItem baseline_item;
	int64_t baseline_minerals;

	void Validate() const;
	bool operator==( const ProductionState& other ) const;
};

// An absent record means unconfigured, not zero minerals or Stockpile.
// Signed64 is a port storage domain; future arithmetic must validate its inputs.
// This codec neither resolves definitions nor makes a target buildable.
types::Buffer SerializeProductionState( const std::optional< ProductionState >& state );
std::optional< ProductionState > DeserializeProductionState( types::Buffer& buffer );

}
