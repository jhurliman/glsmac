#include "SmacScoutKeyboardAdmission.h"

#include "game/backend/unit/SmacScoutKeyboardAdmission.h"
#include "game/backend/unit/SmacConventionalStats.h"
#include "game/backend/unit/SmacScoutCombat.h"
#include "fixtures/SmacScoutKeyboardAdmission.h"
#include "task/gsetests/GSETests.h"

#include <array>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace key = game::backend::unit::smac_scout_key;
namespace battle = game::backend::unit::smac_scout;
namespace fixture = fixtures::smac_scout_key;
using namespace game::backend::unit;
constexpr std::array< key::UnitId, 3 > IDS{ UINT64_MAX, 0, 0x8000000012345678ull };

int32_t Allowance( int32_t stored_morale, uint8_t damage ) {
	const auto effective = SmacConventionalMorale( { stored_morale, 0, 0, false, std::nullopt, SmacMoraleMode::Mode1 } );
	return SmacConventionalAllowance( { 3, 3, SmacMovementDomain::Land, 1, damage, effective, false, false } );
}
key::Query Ordinary() {
	return { 99, 1, 128, 64, true, false, false, false, 99,
		key::SelectedUnit{ IDS[ 0 ], 1, 8, 8, 0, 3, key::Status::Ordinary } };
}
template< typename F > bool Rejects( F f ) {
	try { f(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
}

void AddSmacScoutKeyboardAdmissionTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC Scout key admission matches all original selected-unit cases", GT() {
		for ( const auto& c : fixture::CASES ) {
			auto q = Ordinary(); q.key_code = c.key;
			if ( c.selected < 0 ) { q.selected.reset(); }
			else { q.selected = key::SelectedUnit{ IDS.at( c.selected ), c.selected == 1 ? 2 : 1, c.x, c.y, c.spent,
				Allowance( c.morale, static_cast< uint8_t >( c.damage ) ), key::Status::Ordinary }; }
			const auto result = key::AdmitDirectionalKey( q );
			const auto expected_outcome = c.emitted ? key::Outcome::EmitRequest : c.selected < 0 ? key::Outcome::NoSelection : key::Outcome::CannotAct;
			GT_ASSERT( result.outcome == expected_outcome && static_cast< int32_t >( result.decoded_direction ) == c.direction, "original outcome/direction differs: " + std::string( c.name ) );
			GT_ASSERT( result.notification_calls == static_cast< uint32_t >( c.notifications ) && result.pending_action_latch == static_cast< uint32_t >( c.latch ), "original notification/latch differs: " + std::string( c.name ) );
			GT_ASSERT( result.queried_identity == ( c.queried < 0 ? std::optional< key::UnitId >{} : IDS.at( c.queried ) ), "original query identity differs: " + std::string( c.name ) );
			GT_ASSERT( ( c.predicate < 0 ) == !result.queried_identity && ( c.predicate < 0 || c.predicate == static_cast< int32_t >( result.outcome == key::Outcome::EmitRequest ) ), "original actionability return differs" );
			GT_ASSERT( result.request.has_value() == ( c.emitted != 0 ), "original command emission differs" );
			if ( result.request ) {
				GT_ASSERT( result.request->selected_identity == IDS.at( c.selected ) && result.request->direction == result.decoded_direction && result.request->original_third_argument == 0, "original command arguments changed selected identity" );
			}
			GT_ASSERT( q.pending_action_latch == 99 && ( !q.selected || q.selected->spent == c.spent ), "admission mutated caller state" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout key admission continues actual modeled battle survivors without RNG reset", GT() {
		for ( const auto& c : fixture::SEQUENCES ) {
			const game::backend::SmacGameplayRandomState random( c.shared, c.crt );
			const auto resolved = battle::ResolveNeutralFlatAttack(
				{ IDS[ 0 ], 1, 10 - c.attacker_damage, c.attacker_morale, c.attacker_morale, c.spent },
				{ IDS[ 1 ], 1, 10 - c.defender_damage, c.defender_morale, c.defender_morale, 0 },
				Allowance( c.attacker_morale, static_cast< uint8_t >( c.attacker_damage ) ), random, { IDS[ 0 ], IDS[ 1 ] } );
			const auto& survivor = resolved.survivor;
			GT_ASSERT( resolved.victory == battle::Victory::Attacker && survivor.identity == IDS[ 0 ] && resolved.references.selected == IDS[ 0 ] && survivor.position == battle::Position::Source, "actual modeled normal return changed the retained selected survivor" );
			GT_ASSERT( 10 - survivor.health == c.damage_after && survivor.stored_morale == c.morale_after && survivor.spent == c.spent_after, "original survivor input differs before key gate: " + std::string( c.name ) );
			auto q = Ordinary(); q.selected->spent = survivor.spent;
			q.selected->current_full_allowance = Allowance( survivor.stored_morale, static_cast< uint8_t >( 10 - survivor.health ) );
			const auto saved_random = resolved.random; const auto admitted = key::AdmitDirectionalKey( q );
			GT_ASSERT( admitted.request.has_value() == ( c.emitted != 0 ) && admitted.notification_calls == static_cast< uint32_t >( c.notifications ) && admitted.pending_action_latch == static_cast< uint32_t >( c.latch ), "original post-attack key effects differ: " + std::string( c.name ) );
			GT_ASSERT( admitted.queried_identity == survivor.identity && ( !admitted.request || admitted.request->selected_identity == survivor.identity ), "post-attack request used a deleted or continuation identity" );
			GT_ASSERT( resolved.random == saved_random && random.Shared() == c.shared && random.Crt() == c.crt, "key admission reset or drew from paired RNG" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout key admission distinguishes absent and denied selection effects", GT() {
		for ( const auto latch : { uint32_t( 0 ), uint32_t( 99 ), UINT32_MAX } ) {
			auto q = Ordinary(); q.pending_action_latch = latch; q.selected.reset();
			const auto absent = key::AdmitDirectionalKey( q );
			GT_ASSERT( absent.outcome == key::Outcome::NoSelection && !absent.queried_identity && !absent.request && absent.notification_calls == 0 && absent.pending_action_latch == latch, "absent selection queried, notified or cleared latch" );
			q.selected = key::SelectedUnit{ 0, 2, 8, 8, 0, 3, key::Status::Ordinary };
			const auto foreign = key::AdmitDirectionalKey( q );
			GT_ASSERT( foreign.outcome == key::Outcome::CannotAct && foreign.queried_identity == 0 && !foreign.request && foreign.notification_calls == 1 && foreign.pending_action_latch == 0, "foreign zero identity became absent or preserved latch" );
			q.selected->owner = 1;
			const auto owned = key::AdmitDirectionalKey( q );
			q.selected->identity = 3; q.selected->spent = 255;
			GT_ASSERT( owned.request && owned.request->selected_identity == 0 && owned.notification_calls == 0 && owned.pending_action_latch == latch, "owned request borrowed changed caller selection" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout key admission permits partial points and compares full allowance without subtraction", GT() {
		for ( const auto code : { 99, 105 } ) {
			for ( const auto spent : { 1, 2, 3, 4 } ) {
				auto q = Ordinary(); q.key_code = code; q.selected->spent = spent;
				const auto result = key::AdmitDirectionalKey( q );
				GT_ASSERT( result.request.has_value() == ( spent < 3 ), "partial point refused or exhausted point admitted" );
			}
		}
		auto q = Ordinary(); q.selected->spent = 255; q.selected->current_full_allowance = INT32_MAX;
		GT_ASSERT( key::AdmitDirectionalKey( q ).request.has_value(), "large supplied nonnegative allowance overflowed" );
		q.selected->spent = 0; q.selected->current_full_allowance = 0;
		GT_ASSERT( key::AdmitDirectionalKey( q ).outcome == key::Outcome::CannotAct, "zero allowance admitted" );
		GT_OK();
	} );
	task->AddTest( "SMAC Scout key admission rejects unproved context and status domains explicitly", GT() {
		for ( int scenario = 0; scenario < 18; scenario++ ) {
			auto q = Ordinary();
			switch ( scenario ) {
				case 0: q.key_code = 100; break;
				case 1: q.normal_unit_control = false; break;
				case 2: q.queued_direction_modifier = true; break;
				case 3: q.network_busy = true; break;
				case 4: q.scenario_editor = true; break;
				case 5: q.focused_faction = 0; break;
				case 6: q.focused_faction = 8; break;
				case 7: q.width = 127; break;
				case 8: q.height = 0; break;
				case 9: q.selected->owner = 0; break;
				case 10: q.selected->spent = -1; break;
				case 11: q.selected->spent = 256; break;
				case 12: q.selected->current_full_allowance = -1; break;
				case 13: q.selected->status = key::Status::Transported; break;
				case 14: q.selected->status = key::Status::Blocked; break;
				case 15: q.selected->status = key::Status::OtherOrder; break;
				case 16: q.selected->status = static_cast< key::Status >( 99 ); break;
				case 17: q.selected->x = 9; break;
			}
			GT_ASSERT( Rejects( [ & ]() { key::AdmitDirectionalKey( q ); } ) && q.pending_action_latch == 99, "unsupported keyboard domain treated as observed CannotAct: " + std::to_string( scenario ) );
		}
		GT_OK();
	} );
}

}
