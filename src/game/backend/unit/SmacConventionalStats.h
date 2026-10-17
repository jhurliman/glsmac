#pragma once

#include <cstdint>
#include <optional>

namespace game::backend::unit {

enum class SmacMoraleMode : uint8_t { Mode0, Mode1 };
struct SmacMoraleHome { bool childrens_creche; bool drone_riots; };
struct SmacConventionalMoraleInput {
	int32_t stored_morale; // raw byte domain 0..255; effective result is 0..6
	int32_t retained_morale;
	int32_t faction_morale;
	bool negative_morale_immunity;
	std::optional< SmacMoraleHome > home;
	SmacMoraleMode mode;
};

enum class SmacMovementDomain : uint8_t { Land, Sea, Air };
struct SmacConventionalAllowanceInput {
	int32_t basic_allowance; // already resolved design query, not chassis speed
	int32_t movement_quantum;
	SmacMovementDomain domain;
	uint8_t reactor;
	uint8_t damage;
	int32_t effective_morale; // mode-1 query, supplied separately
	bool maritime_control_center;
	bool suppress_elite_bonus;
};

// Supplied conventional faction-owned units only: excludes native/Psi/Probe
// morale branches and the Artifact-plan allowance branch. No RNG, state
// mutation, ownership lookup or startup defaults.
// Invalid metadata throws invalid_argument; evaluated signed32 arithmetic
// outside the supported port domain throws overflow_error before later clamps.
int32_t SmacConventionalMorale( const SmacConventionalMoraleInput& input );
int32_t SmacConventionalAllowance( const SmacConventionalAllowanceInput& input );

}
