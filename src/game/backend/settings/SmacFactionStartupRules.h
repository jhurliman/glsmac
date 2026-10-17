#pragma once

#include <array>
#include <vector>

#include "SmacTechnologyRules.h"

namespace game::backend::settings {

struct SmacFactionStartupBinding {
	const char* port_id;
	const char* filename;
	const char* section;
};

struct SmacFactionStartupBonus {
	enum class Kind : int32_t {
		NAMED_TECH, EXTRA_TECH, SOCIAL, FACILITY, IMMUNITY, FUNGNUTRIENT, VOTES,
		ENERGY, DRONE, TALENT, POPULATION, COMMERCE, FREEPROTO, FANATIC,
		FUNGMINERALS = 14, FUNGENERGY = 15 // appended V2 variants; existing wire values stay fixed
	};
	std::string keyword, value; // original fields after the file reader's lexical trimming
	Kind kind;
	int32_t parameter; // reference (-2/-1/0..88), social axis (0..10), or numeric metadata
	int32_t amount; // SOCIAL signed amount / IMMUNITY 1; zero for all other tags
	bool operator==( const SmacFactionStartupBonus& other ) const;
	bool ConsumesRecord() const;
	static SmacFactionStartupBonus Parse( const std::string& keyword, const std::string& value, const SmacTechnologyRules& technologies );
};

struct SmacFactionStartupRule {
	std::string port_id, filename, section;
	// Keep even supported records beyond the original eight-record budget for
	// exact frozen equality. Records() is the admitted, ordered projection.
	std::vector< SmacFactionStartupBonus > bonuses;
	std::vector< SmacFactionStartupBonus > Records() const;
	std::vector< int32_t > NamedTechnologyReferences() const;
	int32_t ExtraTechnologyCount() const;
	bool operator==( const SmacFactionStartupRule& other ) const;
};

// The selected model-zero row for each canonical SOCIO category. This
// bounded startup profile admits neutral effects only; names remain source data.
struct SmacStartupSocialModel {
	static constexpr size_t MAX_NAME_BYTES = 256;
	std::string name, prerequisite, modifiers;
	void Validate() const;
	bool operator==( const SmacStartupSocialModel& other ) const;
};

struct SmacFactionStartupRules {
	std::array< SmacStartupSocialModel, 4 > initial_social_models;
	std::array< SmacFactionStartupRule, 7 > factions;
	static const std::array< SmacFactionStartupBinding, 7 >& Bindings();
	void Validate( const SmacTechnologyRules& technologies ) const;
	bool operator==( const SmacFactionStartupRules& other ) const;
	const SmacFactionStartupRule* Find( const std::string& port_id ) const;
	types::Buffer Serialize( const SmacTechnologyRules& technologies ) const;
	static SmacFactionStartupRules Deserialize( types::Buffer buffer, const SmacTechnologyRules& technologies );
};

}
