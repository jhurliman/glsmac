#pragma once

#include "SmacTechnologyRules.h"
#include "game/backend/resource/SmacBaseSquareYield.h"
#include "game/backend/resource/SmacTileNutrients.h"
#include "game/backend/resource/SmacTileMinerals.h"
#include "game/backend/resource/SmacTileEnergy.h"

namespace game::backend::settings {

// One owned authority for the nine physical RESOURCEINFO N/M/E rows. Labels,
// fourth numeric columns and annotations are not resource identities.
struct SmacResourceRules {
	static constexpr uint32_t MAX_TOKEN_BYTES = 256; // explicit port ingestion budget
	static constexpr uint32_t MAX_WIRE_BYTES = 31 * 14 + 4 * ( 6 + MAX_TOKEN_BYTES );
	std::array< resource::smac::RuleTriple, 9 > rows;
	int32_t nutrients_per_citizen, unroaded_mine_limit, mine_nutrient_effect;
	// RULES 54/55/56/58: nutrient/mineral/energy limit and advanced platform.
	// Retain source tokens; resolved IDs are derived against the frozen catalog.
	std::array< std::string, 4 > technology_tokens;

	std::array< SmacTechnologyReference, 4 > TechnologyReferences( const SmacTechnologyRules& technologies ) const;
	void Validate( const SmacTechnologyRules& technologies ) const;
	bool operator==( const SmacResourceRules& other ) const;
	resource::smac::BaseSquareRules BaseSquare() const;
	resource::smac::TileNutrientRules TileNutrients() const;
	resource::smac::TileMineralRules TileMinerals() const;
	resource::smac::TileEnergyRules TileEnergy() const;
	types::Buffer Serialize( const SmacTechnologyRules& technologies ) const;
	static SmacResourceRules Deserialize( types::Buffer buffer, const SmacTechnologyRules& technologies );
};

}
