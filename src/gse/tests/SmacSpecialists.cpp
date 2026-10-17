#include "SmacSpecialists.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacSpecialists.h"
#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slots.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Undefined.h"
#include "util/FinallyGuard.h"
#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
namespace fixture = smac_specialist_fixture;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
uint8_t Mask( const SmacSpecialistMenu& menu ) { uint8_t result = 0; for ( size_t id = 0; id < 7; ++id ) { if ( menu.eligible[ id ] ) { result |= 1 << id; } } return result; }

settings::SmacSpecialistRules Rules() {
	// Installed output triples, positional identities only. Local technology
	// keys are deliberately synthetic; ownership gates use real resolution.
	settings::SmacSpecialistRules result{ 5, {} };
	const int32_t triples[ 7 ][ 3 ] = { {3,0,0}, {0,2,0}, {0,0,3}, {3,0,2}, {2,2,0}, {0,1,3}, {2,2,4} };
	const char* prerequisite[] = { "None", "None", "Tech0", "Tech1", "Tech2", "Tech3", "Tech4" };
	const char* obsolete[] = { "Tech1", "Tech2", "Tech3", "Disable", "Tech4", "Tech4", "Disable" };
	for ( size_t id = 0; id < 7; ++id ) {
		result.specialists[ id ] = { static_cast<int32_t>( id ), "Identical name", "Identical plural", prerequisite[ id ], obsolete[ id ],
			triples[ id ][ 0 ], triples[ id ][ 1 ], triples[ id ][ 2 ], "opaque" };
	}
	return result;
}

Base* MakeBase( GSE_CALLABLE, Game* game, size_t population = 5, size_t slot = 0, size_t id = 901, bool worker = false ) {
	auto* bm = game->GetBM();
	if ( !bm->GetPopDef( "ROLE0" ) ) {
		for ( uint8_t role = 0; role < 7; ++role ) { bm->DefinePop( new PopDef( "ROLE" + std::to_string( role ), "Doctor", {}, {}, PopDef::PF_NONE, role ) ); }
		bm->DefinePop( new PopDef( "WORKER", "Doctor", {}, {}, PopDef::PF_TILE_WORKER ) );
	}
	Base::pops_t pops;
	for ( size_t i = 1; i <= population; ++i ) {
		pops.emplace( i, Pop( nullptr, i, bm->GetPopDef( worker && i == 1 ? "WORKER" : "ROLE1" ), 0,
			worker && i == 1 ? game->GetMap()->GetTile( 4, 2 ) : nullptr ) );
	}
	auto* owner = &game->GetState()->m_slots->GetSlot( slot );
	auto* base = new Base( game, id, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( slot ? 6 : 2, 2 ),
		"Specialists", {}, pops, population + 1, 7, {}, SmacBaseResourceContext{ slot ? 2 : 1, true, 37, -7, 0x8f000041u, false } );
	bm->SpawnBase( GSE_CALL, base ); return base;
}

Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
}

void AddSmacSpecialistTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC specialist query matches all fourteen original menu projections", GT() {
		const auto technologies = smac_rules_fixture::Technologies();
		for ( const auto& row : fixture::menus ) {
			auto rules = Rules(); rules.minimum_base_size = row.minimum;
			for ( uint8_t id = 0; id < 7; ++id ) {
				rules.specialists[ id ].prerequisite = row.available & ( 1 << id ) ? "None" : "Disable";
				rules.specialists[ id ].obsolete = row.obsolete & ( 1 << id ) ? "None" : "Disable";
				rules.specialists[ id ].psych = row.psych[ id ];
			}
			const auto current = row.current < 0 ? std::optional<uint8_t>{} : std::optional<uint8_t>( row.current );
			const auto result = QuerySmacSpecialistMenu( rules, technologies, {}, row.population, current );
			GT_ASSERT( result.current == current && Mask( result ) == row.expected && result.size_explanation_requested == row.explanation, "literal original menu differs: " + std::string( row.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC specialist query automatic choice matches six captured overflow contexts", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); size_t count = 0;
		for ( const auto& row : fixture::automatic ) {
			auto rules = Rules(); rules.minimum_base_size = row.minimum; std::array<bool,89> owned{};
			for ( size_t id = 0; id < 5; ++id ) { owned[ id ] = row.known & ( 1 << id ); }
			for ( size_t i = 0; i < row.repetitions; ++i ) {
				GT_ASSERT( SelectAutomaticSmacSpecialist( rules, technologies, owned, row.population ) == row.expected, "literal initial choice differs: " + std::string( row.id ) ); ++count;
			}
		}
		GT_ASSERT( count == 119, "captured repeated selector count changed" ); GT_OK();
	} );
	task->AddTest( "SMAC specialist query direct possession ties and arithmetic have explicit bounds", GT() {
		const auto tech = smac_rules_fixture::Technologies(); auto rules = Rules(); std::array<bool,89> owned{};
		owned[ 1 ] = true; // no prerequisite-tree closure is required by the measured helper
		auto menu = QuerySmacSpecialistMenu( rules, tech, owned, 5, 0 );
		GT_ASSERT( menu.eligible[ 0 ] && menu.eligible[ 3 ] && !menu.eligible[ 2 ], "current obsolete or direct technology possession changed" );
		GT_ASSERT( !QuerySmacSpecialistMenu( rules, tech, owned, 4, 0 ).eligible[ 0 ], "obsolete current type bypassed size" );
		for ( auto& row : rules.specialists ) { row.prerequisite = "None"; row.obsolete = "None"; row.economy = 0; row.psych = 2; row.labs = 0; }
		GT_ASSERT( SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ) == 0, "automatic ties did not retain earliest obsolete record" );
		rules.specialists[ 0 ].psych = -1;
		GT_ASSERT( QuerySmacSpecialistMenu( rules, tech, {}, 5, 0 ).eligible[ 0 ] && !QuerySmacSpecialistMenu( rules, tech, {}, 4, 0 ).eligible[ 0 ], "signed psych metadata was normalized" );
		rules.specialists[ 0 ].psych = INT32_MAX;
		GT_ASSERT( Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ); } ), "3*psych overflow admitted" );
		rules.specialists[ 0 ].psych = 1; rules.specialists[ 0 ].economy = INT32_MAX; rules.specialists[ 0 ].labs = -INT32_MAX;
		GT_ASSERT( Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ); } ), "hidden intermediate score overflow admitted" );
		GT_ASSERT( SelectAutomaticSmacSpecialist( rules, tech, {}, 4 ) == 1, "below-threshold selector evaluated unused economy/labs" );
		for ( auto& row : rules.specialists ) { row.psych = row.economy = row.labs = 0; }
		GT_ASSERT( Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ); } ), "unproved zero-score fallback invented" );
		for ( auto& row : rules.specialists ) { row.prerequisite = "Disable"; }
		GT_ASSERT( !Mask( QuerySmacSpecialistMenu( rules, tech, {}, 5, {} ) ) && Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ); } ), "empty availability invented a type" );
		rules.specialists.back().obsolete = "Unknown";
		GT_ASSERT( Rejects( [ & ] { QuerySmacSpecialistMenu( rules, tech, {}, 5, {} ); } ) && Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, 5 ); } ), "unknown frozen token silently ignored" );
		rules = Rules();
		GT_ASSERT( Rejects( [ & ] { QuerySmacSpecialistMenu( rules, tech, {}, 128, {} ); } ) && Rejects( [ & ] { QuerySmacSpecialistMenu( rules, tech, {}, 0, 7 ); } )
			&& Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, {}, UINT32_MAX ); } ), "port population/type limits bypassed" ); GT_OK();
	} );
	task->AddTest( "SMAC specialist query uses the ordinary availability helper for definitions and Future", GT() {
		auto tech = smac_rules_fixture::Technologies(); auto rules = Rules(); std::array<bool,89> owned{}; owned[ 87 ] = owned[ 88 ] = true;
		for ( auto& row : rules.specialists ) { row.prerequisite = "Disable"; row.obsolete = "Disable"; }
		rules.specialists[ 0 ].prerequisite = "Tech87";
		// Exact definition-sign boundaries from TECH-PRE-001. The specialist
		// menu/overflow harnesses call that same original 0x5d17b0 helper.
		for ( const auto& row : std::array< std::pair< std::array<std::string,2>, bool >, 5 >{{
			{ { "None", "None" }, true }, { { "Disable", "None" }, false }, { { "Tech0", "Disable" }, false },
			{ { "None", "Disable" }, true }, { { "Tech0", "Tech1" }, true }
		}} ) {
			tech.technologies[ 87 ].prerequisites = row.first;
			GT_ASSERT( QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ).eligible[ 0 ] == row.second, "specialist prerequisite skipped ordinary definition sign guard" );
			if ( row.second ) { GT_ASSERT( SelectAutomaticSmacSpecialist( rules, tech, owned, 5 ) == 0, "ordinary defined owned technology became unavailable" ); }
			else { GT_ASSERT( Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, owned, 5 ); } ), "automatic selector admitted disabled definition" ); }
		}
		rules.specialists[ 0 ].prerequisite = "Tech88";
		GT_ASSERT( !Mask( QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ) ) && Rejects( [ & ] { SelectAutomaticSmacSpecialist( rules, tech, owned, 5 ); } ), "owned Future admitted as ordinary prerequisite" );
		rules.specialists[ 0 ].prerequisite = "None"; rules.specialists[ 0 ].obsolete = "Tech88";
		GT_ASSERT( QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ).eligible[ 0 ], "owned Future incorrectly made specialist obsolete" );
		rules.specialists[ 0 ].obsolete = "Tech87"; tech.technologies[ 87 ].prerequisites = { "Disable", "None" };
		GT_ASSERT( QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ).eligible[ 0 ], "disabled definition incorrectly made specialist obsolete" );
		tech.technologies[ 87 ].prerequisites = { "None", "Disable" };
		GT_ASSERT( !QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ).eligible[ 0 ] && QuerySmacSpecialistMenu( rules, tech, owned, 5, 0 ).eligible[ 0 ], "asymmetric availability/current exception differs" );
		tech.technologies[ 87 ].prerequisites = { "Unknown", "None" };
		GT_ASSERT( Rejects( [ & ] { QuerySmacSpecialistMenu( rules, tech, owned, 5, {} ); } ), "unknown definition became a silent Disable" ); GT_OK();
	} );
	task->AddTest( "SMAC specialist query uses actual Base owner roles and detached GSE results", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureSpecialists( Rules() );
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = MakeBase( GSE_CALL, game, 5, 0, 901, true ); auto* other = MakeBase( GSE_CALL, game, 5, 6, 902 );
			std::array<bool,89> owned{}; fixture.SpecialistKnowledge( 1, owned ); owned[ 4 ] = true; fixture.SpecialistKnowledge( 2, owned );
			const auto before = Base::Serialize( base ).ToString(), foreign = Base::Serialize( other ).ToString();
			const auto rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom(); const auto updates = fixture.QueuedFrontendRequests().size();
			GT_ASSERT( base->SelectAutomaticSmacSpecialist() == 1 && other->SelectAutomaticSmacSpecialist() == 6, "query used controller or slot instead of Base owner technology" );
			GT_ASSERT( !base->GetSmacSpecialistMenu( 1 ).current && base->GetSmacSpecialistMenu( 2 ).current == 1, "current role came from display name or worker was treated as specialist" );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			auto* result = static_cast< value::Object* >( Call( GSE_CALL, wrapper, "get_smac_specialist_menu", { base->m_pops.at( 2 ).Wrap( GSE_CALL ) } ) );
			static_cast< value::Array* >( result->Get( "eligible_types" ) )->value.clear();
			static_cast< value::Int* >( result->Get( "current_type" ) )->value = 6;
			GT_ASSERT( Mask( base->GetSmacSpecialistMenu( 2 ) ) == 3 && base->GetSmacSpecialistMenu( 2 ).current == 1, "detached query output aliases allocation" );
			GT_ASSERT( static_cast< value::Int* >( Call( GSE_CALL, wrapper, "get_smac_automatic_specialist" ) )->value == 1, "actual GSE selector differs" );
			GT_ASSERT( Call( GSE_CALL, wrapper, "get_smac_specialist_menu", { VALUE( value::Undefined ) } )->type == VT_OBJECT
				&& Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_smac_specialist_menu", { other->m_pops.at( 2 ).Wrap( GSE_CALL ) } ); } ), "new context or foreign same-ID Pop check failed" );
			fixture.Early(); GT_ASSERT( base->SelectAutomaticSmacSpecialist() == 1, "technology-only query invented a retained-social requirement" );
			GT_ASSERT( Base::Serialize( base ).ToString() == before && Base::Serialize( other ).ToString() == foreign && game->GetRandom()->GetStateString() == rng
				&& fixture.GameplayRandom() == pair && fixture.QueuedFrontendRequests().size() == updates, "read-only query changed allocation, governor, RNG, or frontend queue" );
			game->GetState()->RetireGameCallbacks();
			GT_ASSERT( Rejects( [ & ] { base->GetSmacSpecialistMenu(); } ) && Rejects( [ & ] { base->SelectAutomaticSmacSpecialist(); } ), "retired bound owner remained queryable" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC specialist query restores decisions and recomputes saved ownership", []( GSE*, context::Context* ) -> std::string {
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		ConventionalUnitFixture source; source.ConfigureSpecialists( Rules() ); std::string saved, base_bytes;
		const auto input = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* base = MakeBase( GSE_CALL, source.Game() ); std::array<bool,89> owned{}; owned[ 4 ] = true; source.SpecialistKnowledge( 1, owned );
			GT_ASSERT( base->SelectAutomaticSmacSpecialist() == 6, "source ownership missing" ); base_bytes = Base::Serialize( base ).ToString(); saved = source.Snapshot(); GT_OK();
		} ); GT_ASSERT( input.empty(), "source: " + input );
		for ( bool absent : { false, true } ) {
			ConventionalUnitFixture restored; restored.ConfigureSpecialists( Rules() ); restored.SpecialistKnowledge( 1, {} );
			const auto output = restored.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				restored.Restore( GSE_CALL, saved, absent ); auto* base = restored.Game()->GetBM()->GetBase( 901 );
				GT_ASSERT( base && base->SelectAutomaticSmacSpecialist() == 6 && Base::Serialize( base ).ToString() == base_bytes, "restore derived a current-world choice or rewrote saved Pop roles" );
				restored.SpecialistKnowledge( 1, {} ); GT_ASSERT( base->SelectAutomaticSmacSpecialist() == 1 && Base::Serialize( base ).ToString() == base_bytes, "query cached a technology result or auto-reassigned population" ); GT_OK();
			} ); GT_ASSERT( output.empty(), "restore: " + output );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC specialist query refuses absent context invalid roles and unbound overflow", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureSpecialists( Rules() );
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = MakeBase( GSE_CALL, game, 17 ); const auto before = Base::Serialize( base ).ToString();
			GT_ASSERT( Rejects( [ & ] { base->GetSmacSpecialistMenu( 1 ); } ) && Rejects( [ & ] { base->GetSmacSpecialistMenu( 999 ); } ), "overflow current-type or foreign ID was inferred" );
			GT_ASSERT( base->GetSmacSpecialistMenu().current == std::nullopt && base->SelectAutomaticSmacSpecialist() == 1, "new-choice query unnecessarily treated stored overflow as authoritative" );
			auto* def = base->m_pops.at( 1 ).m_def; base->m_pops.at( 1 ).m_def = nullptr;
			const bool rejected = Rejects( [ & ] { base->SelectAutomaticSmacSpecialist(); } ); base->m_pops.at( 1 ).m_def = def;
			GT_ASSERT( rejected && Base::Serialize( base ).ToString() == before, "invalid role was read or query mutated allocation" );
			auto* owner = base->m_owner; auto* legacy = new Base( game, 902, owner, base->m_faction, game->GetMap()->GetTile( 6, 2 ), "Legacy", {}, {} );
			game->GetBM()->SpawnBase( GSE_CALL, legacy );
			GT_ASSERT( Rejects( [ & ] { legacy->GetSmacSpecialistMenu(); } ) && Rejects( [ & ] { legacy->SelectAutomaticSmacSpecialist(); } ), "legacy absence became neutral canonical context" ); GT_OK();
		} );
	} );
}
}
