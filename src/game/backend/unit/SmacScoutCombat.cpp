#include "SmacScoutCombat.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace game::backend::unit::smac_scout {
namespace {

constexpr int32_t Q = 3;
constexpr size_t MAX_ROUNDS = 79; // At most 40+40-1 hits before one HP reaches zero.
constexpr size_t MAX_DRAWS = 2 * MAX_ROUNDS + 2;

void Require( const bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}
int32_t Checked( const int64_t value ) {
	if ( value < 0 || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC Scout combat intermediate exceeds its nonnegative signed32 domain" );
	}
	return static_cast< int32_t >( value );
}
void Validate( const Participant& p ) {
	Require( p.reactor >= 1 && p.reactor <= 4, "SMAC Scout combat requires reactor 1..4" );
	Require( p.health >= 1 && p.health <= 10 * p.reactor, "SMAC Scout combat requires positive HP within 10*reactor" );
	Require( p.stored_morale >= 0 && p.stored_morale <= 6 && p.effective_morale == p.stored_morale, "SMAC Scout combat requires neutral equal stored/effective morale 0..6" );
	Require( p.spent >= 0 && p.spent <= 255, "SMAC Scout combat requires byte-domain spent movement 0..255" );
}
void ValidateAllowance( const int32_t allowance ) {
	Require( allowance == 3 || allowance == 6, "SMAC Scout profile requires supplied allowance 3 or 6" );
}
bool Names( const std::optional< UnitId >& ref, const UnitId a, const UnitId b ) {
	return !ref || *ref == a || *ref == b;
}
References Retire( const References& input, const UnitId dead ) {
	return { input.selected == dead ? std::nullopt : input.selected, input.secondary == dead ? std::nullopt : input.secondary };
}
uint16_t Raw( SmacGameplayRandomState& random, std::vector< Draw >& trace, const DrawPhase phase ) {
	const auto before = random.Crt();
	const auto raw = random.NextCrtRaw15();
	trace.push_back( { phase, before, random.Crt(), raw } );
	return raw;
}
int32_t Range( SmacGameplayRandomState& random, std::vector< Draw >& trace, const DrawPhase phase, const int32_t width ) {
	return width <= 1 ? 0 : Raw( random, trace, phase ) % width;
}
}

Participant::Participant( const UnitId identity, const int32_t reactor, const int32_t health, const int32_t stored_morale, const int32_t effective_morale, const int32_t spent )
	: identity( identity ), reactor( reactor ), health( health ), stored_morale( stored_morale ), effective_morale( effective_morale ), spent( spent ) {}
References::References( const std::optional< UnitId > selected, const std::optional< UnitId > secondary ) : selected( selected ), secondary( secondary ) {}

AttackResult ResolveNeutralFlatAttack( const Participant& attacker, const Participant& defender, const int32_t attacker_allowance, const SmacGameplayRandomState& initial_random, const References& references ) {
	Validate( attacker ); Validate( defender ); ValidateAllowance( attacker_allowance );
	Require( attacker.identity != defender.identity, "SMAC Scout participants require distinct stable identities" );
	Require( Names( references.selected, attacker.identity, defender.identity ) && Names( references.secondary, attacker.identity, defender.identity ), "SMAC Scout references must name a participant or be absent" );
	Require( attacker.spent <= 255 - Q, "SMAC Scout attack spending would exceed the verified nonwrapping byte domain" );
	const std::array< int32_t, 2 > strengths{ Checked( INT64_C( 32 ) * ( 6 + attacker.effective_morale ) ), Checked( INT64_C( 32 ) * ( 6 + std::max( 1, defender.effective_morale ) ) ) };
	const auto remaining = std::clamp( int64_t( attacker_allowance ) - attacker.spent, INT64_C( 0 ), INT64_C( 999 ) );
	const std::array< int32_t, 2 > widths{ remaining < Q ? Checked( strengths[ 0 ] * remaining / Q ) : strengths[ 0 ], strengths[ 1 ] };
	std::array< int32_t, 2 > health{ attacker.health, defender.health };
	auto random = initial_random;
	std::vector< Round > rounds; rounds.reserve( MAX_ROUNDS );
	std::vector< Draw > draws; draws.reserve( MAX_DRAWS );
	while ( health[ 0 ] > 0 && health[ 1 ] > 0 ) {
		const auto a = Range( random, draws, DrawPhase::RoundAttacker, widths[ 0 ] );
		const auto d = Range( random, draws, DrawPhase::RoundDefender, widths[ 1 ] );
		rounds.push_back( { health, { a, d } } );
		--health[ a > d ? 1 : 0 ]; // Ties hit the attacker.
	}
	const bool attacker_won = health[ 0 ] > 0;
	const size_t w = attacker_won ? 0 : 1, l = 1 - w;
	const auto& winner = attacker_won ? attacker : defender;
	const auto& loser = attacker_won ? defender : attacker;
	const auto winner_weight = Checked( int64_t( winner.reactor ) * health[ w ] * widths[ w ] / winner.health );
	const auto loser_weight = Checked( int64_t( loser.health ) * widths[ l ] / 10 );
	const bool admitted = winner.effective_morale <= 1 || Range( random, draws, DrawPhase::PromotionAdmission, Checked( int64_t( winner_weight ) + loser_weight ) ) <= loser_weight;
	auto morale = winner.stored_morale;
	if ( admitted && winner.effective_morale < 6 && morale < 6 ) {
		if ( winner.effective_morale <= 2 || ( Raw( random, draws, DrawPhase::PromotionParity ) & 1 ) == 0 ) { ++morale; }
	}
	Survivor survivor{ winner.identity, winner.reactor, health[ w ], morale, attacker_won ? Checked( int64_t( attacker.spent ) + Q ) : defender.spent, attacker_won ? Position::Source : Position::Destination, { std::nullopt, std::nullopt } };
	return { attacker_won ? Victory::Attacker : Victory::Defender, survivor, loser.identity, Retire( references, loser.identity ), winner.identity, random,
		strengths, widths, health, { attacker_won, !attacker_won }, std::move( rounds ), std::move( draws ), { winner_weight, loser_weight, admitted, winner.stored_morale, morale }, { attacker_won ? 1 : 0, 0 } };
}

EmptyMoveResult ContinueEmptyFlatWithoutLottery( const Survivor& source, const int32_t current_allowance, const SmacGameplayRandomState& random, const References& references ) {
	Validate( { source.identity, source.reactor, source.health, source.stored_morale, source.stored_morale, source.spent } );
	ValidateAllowance( current_allowance );
	Require( source.position == Position::Source && !source.stack_links[ 0 ] && !source.stack_links[ 1 ], "SMAC empty-flat continuation requires a singleton at Source" );
	Require( Names( references.selected, source.identity, source.identity ) && Names( references.secondary, source.identity, source.identity ), "SMAC empty-flat references must name the survivor or be absent" );
	const auto remaining = int64_t( current_allowance ) - source.spent;
	Require( remaining <= 0 || remaining >= Q, "Partial-cost empty movement requires a separate lottery model; no command-legality claim" );
	auto survivor = source;
	const bool moved = remaining >= Q;
	if ( moved ) { survivor.spent = Checked( int64_t( source.spent ) + Q ); survivor.position = Position::Destination; }
	return { moved ? EmptyMove::Moved : EmptyMove::Exhausted, survivor, references, survivor.identity, random, { !moved, moved }, moved ? 1 : 0 };
}

}
