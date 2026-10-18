#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "game/backend/SmacGameplayRandomState.h"

namespace game::backend::unit::smac_scout {

using UnitId = uint64_t;

// Resolved neutral conventional Gun/Scout inputs, not a live Unit or a morale,
// movement-allowance, terrain, diplomacy, or command-admission producer.
struct Participant {
	Participant( UnitId identity, int32_t reactor, int32_t health, int32_t stored_morale, int32_t effective_morale, int32_t spent );
	UnitId identity;
	int32_t reactor, health, stored_morale, effective_morale, spent;
};

struct References {
	References( std::optional< UnitId > selected, std::optional< UnitId > secondary );
	std::optional< UnitId > selected, secondary;
};

enum class Position { Source, Destination };
enum class Victory { Attacker, Defender };
enum class DrawPhase { RoundAttacker, RoundDefender, PromotionAdmission, PromotionParity };

struct Draw {
	DrawPhase phase;
	uint32_t state_before, state_after;
	uint16_t raw;
};
struct Round {
	std::array< int32_t, 2 > health_before;
	std::array< int32_t, 2 > draws;
};
struct Promotion {
	int32_t winner_weight, loser_weight;
	bool admitted;
	int32_t stored_before, stored_after;
};
struct Survivor {
	UnitId identity;
	int32_t reactor, health, stored_morale, spent;
	Position position;
	std::array< std::optional< UnitId >, 2 > stack_links;
};
// Historical return evidence is deliberately separate from the resolved result.
// In this profile wrapper_return is zero after either victory, with no rollback.
struct OriginalReturns { int32_t core_return, wrapper_return; };

struct AttackResult {
	Victory victory;
	Survivor survivor;
	UnitId deleted_identity;
	References references;
	UnitId continuation_identity;
	SmacGameplayRandomState random;
	std::array< int32_t, 2 > strengths, round_widths, health_after;
	std::array< bool, 2 > unit_present;
	std::vector< Round > rounds;
	std::vector< Draw > draws;
	Promotion promotion;
	OriginalReturns original_returns;
};

// Reactor 1..4; HP 1..10R; neutral stored/effective morale equal in 0..6;
// supplied attacker allowance 3 or 6. Spent is 0..255, and attacker spent
// must leave room for +3 without byte wrap (0..252). Distinct identities; refs name either
// participant or are absent. Invalid inputs throw before any result is produced.
// Even exhausted attacks are diagnostic combat inputs; keyboard legality is a
// separate caller contract. No input or external state is mutated.
AttackResult ResolveNeutralFlatAttack( const Participant& attacker, const Participant& defender, int32_t attacker_allowance, const SmacGameplayRandomState& random, const References& references );
// RE-SMAC-FOCUSED-SCOUT-BATTLE-001: actual mode1 effective morale can differ
// from stored morale through a real home/retained rating; FANATIC adds25%
// attack strength. The live caller owns complete component/terrain admission.
AttackResult ResolveFlatScoutAttack( const Participant& attacker, const Participant& defender, int32_t attacker_allowance,
	const SmacGameplayRandomState& random, const References& references, bool fanatic );

enum class EmptyMove { Moved, Exhausted };
struct EmptyMoveResult {
	EmptyMove outcome;
	Survivor survivor;
	References references;
	UnitId continuation_identity;
	SmacGameplayRandomState random;
	std::array< bool, 2 > unit_present;
	int32_t original_wrapper_return;
};

// Narrow continuation for the three checked post-kill sequences: singleton at
// Source, fresh supplied allowance 3/6, either >=3 remaining or exhausted.
// Remaining 1/2 requires a separate movement-lottery model and is rejected here;
// that rejection does not assert keyboard/command illegality. No RNG draws.
EmptyMoveResult ContinueEmptyFlatWithoutLottery( const Survivor& source, int32_t current_allowance, const SmacGameplayRandomState& random, const References& references );

}
