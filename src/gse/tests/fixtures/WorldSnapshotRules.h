#pragma once

#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/WorldSnapshotTurn.h"
#include "types/Buffer.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace gse::tests::world_rules_fixture {

inline types::Buffer Header( const std::string& id = "smac", const bool present = true,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
	const int64_t marker = game::backend::world_snapshot::rules_marker,
	const int64_t version = game::backend::world_snapshot::rules_version ) {
	types::Buffer result;
	result.WriteInt( marker );
	result.WriteInt( version );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) { for ( const auto value : values ) { result.WriteInt( value ); } }
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidSmacHeaders() {
	std::vector< std::pair< std::string, std::string > > invalid;
	const auto add = [ & ]( const std::string& name, const types::Buffer& header ) { invalid.emplace_back( name, header.ToString() ); };
	types::Buffer legacy;
	legacy.WriteString( "legacy map body" );
	add( "legacy string-first", legacy );
	add( "wrong marker", Header( "smac", false, {}, 0 ) );
	const std::array< int64_t, 9 > valid_rules = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
	auto old_v1 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 1 );
	old_v1.WriteString( "old map body" );
	add( "old V1", old_v1 );
	auto old_v2 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 2 );
	game::backend::world_snapshot::WriteRandomState( old_v2, { 1, 2, 3, 4 } ); old_v2.WriteString( "old map body" );
	add( "old V2", old_v2 );
	add( "unknown version", Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, game::backend::world_snapshot::rules_version + 1 ) );
	add( "unknown identity", Header( "gog", false ) );
	add( "different identity", Header( "smax", false ) );
	add( "missing rules", Header( "smac", false ) );
	for ( size_t i = 0 ; i < 9 ; ++i ) {
		std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
		values[ i ] += values[ i ] == INT32_MAX ? -1 : 1;
		add( "numeric mismatch " + std::to_string( i ), Header( "smac", true, values ) );
		for ( const auto value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) {
			values[ i ] = value;
			add( "numeric range " + std::to_string( i ) + ":" + std::to_string( value ), Header( "smac", true, values ) );
		}
	}
	const auto valid = Header().ToString();
	for ( const size_t length : { size_t( 0 ), size_t( 1 ), valid.size() - 1 } ) {
		invalid.emplace_back( "truncated " + std::to_string( length ), valid.substr( 0, length ) );
	}
	return invalid;
}

inline types::Buffer FullHeader( const std::array< int64_t, 4 >& words = { 1, 2, 3, 4 }, const std::string& algorithm = "jsf32-v1" ) {
	auto result = Header();
	result.WriteString( algorithm );
	for ( const auto word : words ) { result.WriteInt( word ); }
	return result;
}

inline types::Buffer WorldHeader( const game::backend::world_snapshot::TurnState& turn = { 0, {} } ) {
	auto result = FullHeader();
	game::backend::world_snapshot::WriteTurnState( result, turn );
	return result;
}

// The established session has occupied slots 0 and 2. Build malformed frames
// directly, independently of the production writer's canonical-record checks.
inline std::vector< std::pair< std::string, std::string > > InvalidTurnHeaders() {
	std::vector< std::pair< std::string, std::string > > result;
	const auto add = [ & ]( const std::string& name, const types::Buffer& turn ) { result.emplace_back( name, FullHeader().ToString() + turn.ToString() ); };
	types::Buffer empty; add( "missing turn block", empty );
	types::Buffer tag; tag.WriteInt( 1 ); add( "wrong turn tag type", tag );
	types::Buffer unknown; unknown.WriteString( "turn-state-v2" ); add( "unknown turn tag", unknown );
	for ( const int64_t id : { int64_t( -1 ), INT64_MIN, INT64_MAX } ) {
		types::Buffer turn; turn.WriteString( "turn-state-v1" ); turn.WriteInt( id ); add( "invalid turn ID", turn );
	}
	const auto prefix = []() { types::Buffer turn; turn.WriteString( "turn-state-v1" ); turn.WriteInt( 37 ); return turn; };
	add( "missing roster count", prefix() );
	auto wrong_count = prefix(); wrong_count.WriteString( "2" ); add( "wrong roster count type", wrong_count );
	for ( const int64_t count : { int64_t( -1 ), int64_t( 0 ), int64_t( 1 ), int64_t( 3 ), INT64_MAX } ) {
		auto turn = prefix(); turn.WriteInt( count );
		turn.WriteInt( 0 ); turn.WriteBool( true ); turn.WriteInt( 2 ); turn.WriteBool( false );
		add( "mismatched roster count " + std::to_string( count ), turn );
	}
	for ( size_t position = 0 ; position < 2 ; ++position ) {
		auto base = prefix(); base.WriteInt( 2 );
		if ( position ) { base.WriteInt( 0 ); base.WriteBool( true ); }
		add( "missing completion slot " + std::to_string( position ), base );
		auto mistyped = base; mistyped.WriteBool( true ); add( "mistyped completion slot", mistyped );
		for ( const int64_t id : { int64_t( -1 ), int64_t( 0 ), int64_t( 1 ), int64_t( 2 ), int64_t( 3 ), INT64_MAX } ) {
			if ( id == static_cast< int64_t >( position * 2 ) ) { continue; }
			auto turn = base; turn.WriteInt( id ); turn.WriteBool( true ); add( "noncanonical completion slot " + std::to_string( position ) + ":" + std::to_string( id ), turn );
		}
		base.WriteInt( position * 2 ); add( "missing completion flag", base );
		auto wrong_flag = base; wrong_flag.WriteInt( 1 ); add( "wrong completion flag type", wrong_flag );
		base.WriteBool( true ); auto noncanonical = base.ToString();
		noncanonical[ noncanonical.size() - 2 ] = 2; noncanonical.back() = 2; // payload and XOR checksum
		result.emplace_back( "noncanonical Bool byte", FullHeader().ToString() + noncanonical );
	}
	const auto complete = WorldHeader( { 37, { { 0, true }, { 2, false } } } ).ToString();
	result.emplace_back( "truncated completion Bool", complete.substr( 0, complete.size() - 1 ) );
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidRandomHeaders() {
	std::vector< std::pair< std::string, std::string > > invalid;
	invalid.emplace_back( "missing RNG", Header().ToString() );
	invalid.emplace_back( "unknown RNG", FullHeader( {}, "jsf32-v2" ).ToString() );
	auto wrong_algorithm_type = Header(); wrong_algorithm_type.WriteInt( 1 );
	invalid.emplace_back( "wrong RNG algorithm type", wrong_algorithm_type.ToString() );
	for ( size_t i = 0 ; i < 4 ; ++i ) {
		for ( const auto value : { int64_t( -1 ), int64_t( UINT32_MAX ) + 1, INT64_MIN, INT64_MAX } ) {
			std::array< int64_t, 4 > words = { 1, 2, 3, 4 }; words[ i ] = value;
			invalid.emplace_back( "out-of-range RNG word " + std::to_string( i ), FullHeader( words ).ToString() );
		}
		auto short_state = Header(); short_state.WriteString( "jsf32-v1" );
		for ( size_t j = 0 ; j < i ; ++j ) { short_state.WriteInt( j ); }
		invalid.emplace_back( "missing RNG word " + std::to_string( i ), short_state.ToString() );
		short_state.WriteString( "wrong word type" );
		invalid.emplace_back( "wrong RNG word type " + std::to_string( i ), short_state.ToString() );
	}
	const auto full = FullHeader().ToString();
	invalid.emplace_back( "truncated RNG frame", full.substr( 0, full.size() - 1 ) );
	return invalid;
}

}
