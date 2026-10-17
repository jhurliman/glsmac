#include "SmacTechnologyState.h"

#include "fixtures/SmacFactionStartupSource.h"
#include "game/backend/faction/SmacTechnologyState.h"
#include "loader/txt/FactionTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>

namespace gse::tests {
namespace {
using namespace game::backend::faction;
using namespace game::backend::settings;
namespace fixture = smac_faction_startup_fixture;

SmacFactionStartupRules Profiles() {
	return loader::txt::FactionTXTLoader::ParseStartupRules( fixture::Alpha(), fixture::Source, fixture::Technologies() );
}

bool Rejects( const std::function< void() >& action ) {
	try { action(); } catch ( const std::exception& ) { return true; }
	return false;
}

SmacTechnologyState Dirty( const SmacTechnologyRules& tech, const SmacFactionStartupRules& profiles ) {
	auto state = SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, tech, profiles );
	for ( auto& faction : state.factions ) {
		faction.owned.fill( true ); faction.score = 77; faction.future_count = 9; faction.labs = 123;
		faction.target = 6; faction.cached_threshold = 456; faction.discount=777; faction.deferred=-37; faction.pending_secrets=true;
	}
	return state;
}

// Independent wire writer permits malformed late records without weakening
// production validation. Numbers: prefix 0..2, then eight per populated record.
std::string Wire( const SmacTechnologyState& state, size_t number_index = SIZE_MAX, int64_t replacement = 0,
	size_t text_index = SIZE_MAX, const std::string& text_replacement = "" ) {
	types::Buffer buffer; size_t numbers = 0, texts = 0;
	const auto number = [ & ]( int64_t value ) { buffer.WriteInt( numbers++ == number_index ? replacement : value ); };
	number( 0x534d5453 ); number( 2 ); number( state.factions.size() );
	for ( const auto& faction : state.factions ) {
		number( faction.admission.simulation_id ); buffer.WriteString( texts++ == text_index ? text_replacement : faction.admission.faction_id );
		for ( const auto owned : faction.owned ) { buffer.WriteBool( owned ); }
		number( faction.score ); number( faction.future_count ); number( faction.labs );
		buffer.WriteBool( faction.target.has_value() ); if ( faction.target ) { number( *faction.target ); }
		buffer.WriteBool( faction.cached_threshold.has_value() ); if ( faction.cached_threshold ) { number( *faction.cached_threshold ); }
		number(faction.discount);number(faction.deferred);buffer.WriteBool(faction.pending_secrets);
	}
	return buffer.ToString();
}
}

void AddSmacTechnologyStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC technology state new world uses explicit simulation admissions and named grants", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles();
		const std::array< int32_t, 7 > expected = { 6, 43, 2, 1, 5, 4, 0 };
		std::vector< SmacSimulationAdmission > admissions;
		for ( size_t i = 0 ; i < 7 ; ++i ) { admissions.push_back( { static_cast< int32_t >( 7 - i ), profiles.factions[ i ].port_id } ); }
		auto state = Dirty( tech, profiles );
		state = SmacTechnologyState::NewWorld( admissions, tech, profiles );
		GT_ASSERT( state.factions.size() == 7, "explicit admitted roster lost" );
		for ( size_t i = 0 ; i < 7 ; ++i ) {
			const auto& faction = state.factions[ i ];
			GT_ASSERT( faction.admission == admissions[ i ] && state.Find( 7 - i ) == &faction, "source/transport position inferred as simulation ID" );
			for ( size_t id = 0 ; id < 89 ; ++id ) { GT_ASSERT( faction.owned[ id ] == ( static_cast< int32_t >( id ) == expected[ i ] ), "dirty ownership survived or numeric extra technology was awarded" ); }
			GT_ASSERT( faction.score == 0 && faction.future_count == 0 && faction.labs == 0 && faction.discount==0 && faction.deferred==0 && !faction.pending_secrets && !faction.target && !faction.cached_threshold, "turn-zero research reset changed" );
		}
		const auto one = SmacTechnologyState::NewWorld( { { 4, "UNIVERSITY" } }, tech, profiles );
		GT_ASSERT( one.factions.size() == 1 && !one.Find( 1 ) && !one.Find( 0 ) && !one.Find( 8 ) && one.Find( 4 )->owned[ 2 ] && !one.Find( 4 )->owned[ 0 ], "unadmitted factions or later numeric award were invented" );
		GT_OK();
	} );
	task->AddTest( "SMAC technology state global clear and recipient reset preserve separate ownership stages", GT() {
		const auto tech = fixture::Technologies(); auto profiles = Profiles(); auto state = Dirty( tech, profiles );
		state.ClearOwnershipForNewWorld();
		for ( const auto& faction : state.factions ) {
			for ( const auto owned : faction.owned ) { GT_ASSERT( !owned, "global clear missed an admitted owner or catalog entry" ); }
			GT_ASSERT( faction.score == 77 && faction.future_count == 9 && faction.labs == 123 && faction.target == 6 && faction.cached_threshold == 456, "ownership clear also initialized research" );
		}
		state.factions[ 1 ].owned[ 88 ] = true; state.factions[ 0 ].owned[ 24 ] = true;
		const auto other = state.factions[ 1 ];
		state.InitializeFactionAtTurnZero( 7, tech, profiles );
		GT_ASSERT( state.factions[ 1 ] == other, "recipient reset cleared or changed another owner" );
		GT_ASSERT( state.factions[ 0 ].owned[ 24 ] && state.factions[ 0 ].owned[ 6 ] && !state.factions[ 0 ].target && !state.factions[ 0 ].cached_threshold && state.factions[ 0 ].score == 0 && state.factions[ 0 ].future_count == 0 && state.factions[ 0 ].labs == 0, "recipient reset cleared possession or retained dirty research" );
		profiles.factions[ 2 ].bonuses = profiles.factions[ 0 ].bonuses;
		state.InitializeFactionAtTurnZero( 2, tech, profiles );
		GT_ASSERT( state.factions[ 0 ].owned[ 6 ] && state.factions[ 1 ].owned[ 6 ] && state.factions[ 1 ].owned[ 88 ], "shared grant transferred ownership or cleared Future Tech" );
		GT_OK();
	} );
	task->AddTest( "SMAC technology state grants all catalog IDs without ordinary availability or record budget bypass", GT() {
		auto tech = fixture::Technologies(); auto profiles = Profiles();
		for ( size_t id = 0 ; id < 89 ; ++id ) {
			// Disabled definitions and absent parents do not gate startup ownership.
			tech.technologies[ id ].prerequisites = { "Disable", "Disable" };
			profiles.factions[ 0 ].bonuses = { SmacFactionStartupBonus::Parse( "TECH", tech.technologies[ id ].key, tech ) };
			const auto state = SmacTechnologyState::NewWorld( { { static_cast< int32_t >( id % 7 + 1 ), "GAIANS" } }, tech, profiles );
			for ( size_t owned_id = 0 ; owned_id < 89 ; ++owned_id ) { GT_ASSERT( state.factions[ 0 ].owned[ owned_id ] == ( owned_id == id ), "catalog startup ownership filtered or extra parents granted" ); }
			GT_ASSERT( state.factions[ 0 ].future_count == 0 && state.factions[ 0 ].score == 0, "startup grant ran ordinary score/Future effects" );
		}
		auto& bonuses = profiles.factions[ 0 ].bonuses;
		bonuses = { SmacFactionStartupBonus::Parse( "TECH", "None", tech ), SmacFactionStartupBonus::Parse( "TECH", "Disable", tech ),
			SmacFactionStartupBonus::Parse( "TECH", "Ecology", tech ), SmacFactionStartupBonus::Parse( "TECH", "Ecology", tech ), SmacFactionStartupBonus::Parse( "TECH", "3", tech ) };
		auto state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, tech, profiles );
		for ( size_t id = 0 ; id < 89 ; ++id ) { GT_ASSERT( state.factions[ 0 ].owned[ id ] == ( id == 6 ), "sentinel/duplicate/numeric bonus changed possession" ); }
		bonuses.clear(); for ( size_t i = 0 ; i < 8 ; ++i ) { bonuses.push_back( SmacFactionStartupBonus::Parse( "SOCIAL", "+SUPPORT", tech ) ); }
		bonuses.push_back( SmacFactionStartupBonus::Parse( "TECH", "Ecology", tech ) );
		state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, tech, profiles );
		for ( const auto owned : state.factions[ 0 ].owned ) { GT_ASSERT( !owned, "ninth shared record became a starting grant" ); }
		GT_OK();
	} );
	task->AddTest( "SMAC technology state rejects invalid admission and stages recipient initialization", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles();
		for ( const auto& admissions : std::vector< std::vector< SmacSimulationAdmission > >{
			{}, { { 0, "GAIANS" } }, { { 8, "GAIANS" } }, { { INT32_MIN, "GAIANS" } }, { { INT32_MAX, "GAIANS" } },
			{ { 1, "GAIANS" }, { 1, "HIVE" } }, { { 1, "gaians" } }, { { 1, "UNIV" } }, { { 1, "unknown" } }, { { 1, "" } }
		} ) { GT_ASSERT( Rejects( [ & ]() { SmacTechnologyState::NewWorld( admissions, tech, profiles ); } ), "invalid admission accepted" ); }
		auto state = Dirty( tech, profiles ); const auto before = state;
		GT_ASSERT( Rejects( [ & ]() { state.InitializeFactionAtTurnZero( 1, tech, profiles ); } ) && state == before, "missing recipient changed state" );
		auto invalid = profiles; invalid.factions.back().bonuses.back().parameter = INT32_MAX;
		GT_ASSERT( Rejects( [ & ]() { state.InitializeFactionAtTurnZero( 7, tech, invalid ); } ) && state == before, "late invalid source published a partial reset" );
		state.factions.back().target = 89; const auto bad_state = state;
		GT_ASSERT( Rejects( [ & ]() { state.InitializeFactionAtTurnZero( 7, tech, profiles ); } ) && state == bad_state, "late invalid state partially initialized" );
		GT_ASSERT( Rejects( [ & ]() { state.Serialize( tech, profiles ); } ), "invalid target serialized" );
		GT_OK();
	} );
	task->AddTest( "SMAC technology state restores exact progress with owned lifetime and no startup replay", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles(); auto expected = Dirty( tech, profiles );
		expected.factions[ 0 ].owned.fill( false ); expected.factions[ 0 ].owned[ 88 ] = true;
		expected.factions[ 0 ].score = INT32_MIN; expected.factions[ 0 ].future_count = INT32_MAX; expected.factions[ 0 ].labs = -123;
		expected.factions[ 0 ].target = 88; expected.factions[ 0 ].cached_threshold = INT32_MIN;
		expected.factions[ 1 ].score = INT32_MAX; expected.factions[ 1 ].future_count = INT32_MIN; expected.factions[ 1 ].labs = INT32_MAX;
		expected.factions[ 1 ].target.reset(); expected.factions[ 1 ].cached_threshold = INT32_MAX;
		SmacTechnologyState restored;
		{
			auto input = expected.Serialize( tech, profiles ).ToString();
			auto local_tech = tech; auto local_profiles = profiles;
			restored = SmacTechnologyState::Deserialize( input, local_tech, local_profiles );
			input.assign( input.size(), 'X' ); local_profiles.factions[ 0 ].port_id.clear(); local_tech.technologies[ 0 ].key.clear();
		}
		GT_ASSERT( restored == expected && restored.Serialize( tech, profiles ).ToString() == expected.Serialize( tech, profiles ).ToString(), "restore replayed startup or borrowed input/profile storage" );
		auto copy = restored; auto moved = std::move( copy ); restored.factions[ 0 ].owned[ 88 ] = false;
		GT_ASSERT( moved == expected, "copied/moved state shares mutable possession" );
		moved.factions[ 0 ].cached_threshold.reset(); moved.factions[ 0 ].target = 0;
		GT_ASSERT( SmacTechnologyState::Deserialize( moved.Serialize( tech, profiles ).ToString(), tech, profiles ) == moved, "absent cache or zero target lost" );
		moved.factions[ 0 ].cached_threshold = -1;
		GT_ASSERT( Rejects( [ & ]() { moved.Serialize( tech, profiles ); } ), "invalid-cache sentinel acquired a second encoding" );
		GT_OK();
	} );
	task->AddTest( "SMAC technology state codec rejects malformed late records before replacement", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles(); auto state = Dirty( tech, profiles ); const auto before = state;
		for ( const auto& mutation : std::vector< std::pair< size_t, int64_t > >{
			{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 3 }, { 2, -1 }, { 2, 0 }, { 2, 8 }, { 2, INT64_MAX },
			{ 3, 0 }, { 11, 7 }, { 11, 8 }, { 11, INT64_MAX }, { 12, static_cast< int64_t >( INT32_MAX ) + 1 },
			{ 13, static_cast< int64_t >( INT32_MIN ) - 1 }, { 14, INT64_MAX }, { 15, -1 }, { 15, 89 }, { 16, -1 }, { 16, INT64_MIN }, { 17, INT64_MAX }, { 18, INT64_MIN }
		} ) { GT_ASSERT( Rejects( [ & ]() { state = SmacTechnologyState::Deserialize( Wire( before, mutation.first, mutation.second ), tech, profiles ); } ) && state == before, "malformed numeric record replaced destination" ); }
		for ( const auto& id : { std::string(), std::string( "unknown" ), std::string( "GAIANS\0x", 8 ), std::string( 1000, 'x' ) } ) {
			GT_ASSERT( Rejects( [ & ]() { state = SmacTechnologyState::Deserialize( Wire( before, SIZE_MAX, 0, 1, id ), tech, profiles ); } ) && state == before, "malformed late identity published" );
		}
		const auto wire = Wire( before );
		GT_ASSERT( wire == before.Serialize( tech, profiles ).ToString(), "independent wire fixture disagrees" );
		for ( size_t length = 0 ; length < wire.size() ; ++length ) {
			GT_ASSERT( Rejects( [ & ]() { state = SmacTechnologyState::Deserialize( wire.substr( 0, length ), tech, profiles ); } ) && state == before, "truncated record partially replaced state" );
		}
		for ( const auto& bad : { wire + "x", std::string( SmacTechnologyState::MAX_WIRE_BYTES + 1, 'x' ) } ) {
			GT_ASSERT( Rejects( [ & ]() { state = SmacTechnologyState::Deserialize( bad, tech, profiles ); } ) && state == before, "trailing/oversized wire accepted" );
		}
		// Actual framed bool with valid checksum but invalid 2 payload.
		types::Buffer cursor( wire ); for ( int i = 0 ; i < 4 ; ++i ) { cursor.ReadInt(); } cursor.ReadString();
		auto invalid_bool = wire; invalid_bool[ cursor.lenr + 5 ] = 2; invalid_bool[ cursor.lenr + 6 ] = 2;
		GT_ASSERT( Rejects( [ & ]() { SmacTechnologyState::Deserialize( invalid_bool, tech, profiles ); } ), "noncanonical ownership bool accepted" );
		GT_OK();
	} );
}

}
