#pragma once

#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/WorldSnapshotTurn.h"
#include "types/Buffer.h"
#include "SmacRulesSource.h"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace gse::tests::world_rules_fixture {

inline types::Buffer Header( const std::string& id = "smac", const bool present = true,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
	const int64_t marker = game::backend::world_snapshot::rules_marker,
	const int64_t version = game::backend::world_snapshot::rules_version,
	const std::optional< bool > production_present = std::nullopt,
	const std::optional< std::string >& production = std::nullopt ) {
	types::Buffer result;
	result.WriteInt( marker );
	result.WriteInt( version );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) { for ( const auto value : values ) { result.WriteInt( value ); } }
	if ( version >= 4 ) {
		const auto has_production = production_present.value_or( id == "smac" );
		result.WriteBool( has_production );
		if ( has_production ) { result.WriteString( production ? *production : smac_rules_fixture::Production().Serialize().ToString() ); }
	}
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
	auto old_v3 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 3 );
	game::backend::world_snapshot::WriteRandomState( old_v3, { 1, 2, 3, 4 } );
	game::backend::world_snapshot::WriteTurnState( old_v3, { 37, { { 0, true }, { 2, false } } } );
	old_v3.WriteString( "old map body" ); add( "old V3", old_v3 );
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

// Replace one framed value in the production codec's output. This test-only
// schema walk can corrupt late fields without reimplementing its serializer.
inline std::string ReplaceProductionFrame( const size_t index, const types::Buffer& replacement ) {
	auto original = smac_rules_fixture::Production().Serialize();
	for ( size_t i = 0; i <= index; ++i ) {
		const auto start = original.lenr;
		if ( i >= 6 && ( ( i - 6 ) % 5 == 1 || ( i - 6 ) % 5 == 4 ) ) { original.ReadString(); }
		else { original.ReadInt(); }
		if ( i == index ) {
			const auto bytes = original.ToString();
			return bytes.substr( 0, start ) + replacement.ToString() + bytes.substr( original.lenr );
		}
	}
	throw std::runtime_error( "production test frame was not found" );
}

inline std::vector< std::pair< std::string, std::string > > InvalidProductionHeaders( const std::string& id = "smac" ) {
	std::vector< std::pair< std::string, std::string > > result;
	const std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
	const auto header = [ & ]( const bool present, const std::optional< std::string >& bytes = std::nullopt ) {
		return Header( id, id == "smac", values, game::backend::world_snapshot::rules_marker,
			game::backend::world_snapshot::rules_version, present, bytes ).ToString();
	};
	if ( id == "smax" ) { result.emplace_back( "unexpected SMAX production profile", header( true ) ); return result; }
	result.emplace_back( "missing SMAC production profile", header( false ) );
	types::Buffer absent, present, wrong_type;
	absent.WriteBool( false ); present.WriteBool( true ); wrong_type.WriteInt( 1 );
	const auto prefix = header( false ).substr( 0, header( false ).size() - absent.lenw );
	result.emplace_back( "missing production presence", prefix );
	result.emplace_back( "mistyped production presence", prefix + wrong_type.ToString() );
	result.emplace_back( "missing production payload", prefix + present.ToString() );
	result.emplace_back( "mistyped production payload", prefix + present.ToString() + wrong_type.ToString() );
	const auto add = [ & ]( const std::string& name, const std::string& bytes ) { result.emplace_back( name, header( true, bytes ) ); };
	using game::backend::settings::SmacProductionRules;
	using game::backend::settings::SmacFacilityRule;
	for ( const auto member : { &SmacProductionRules::mineral_multiplier, &SmacProductionRules::retool_loss_percent,
		&SmacProductionRules::retool_strictness, &SmacProductionRules::retool_exemption } ) {
		auto changed = smac_rules_fixture::Production(); changed.*member = 42;
		add( "changed production constant", changed.Serialize().ToString() );
	}
	for ( const auto member : { &SmacFacilityRule::cost, &SmacFacilityRule::maintenance } ) {
		auto changed = smac_rules_fixture::Production(); changed.facilities.back().*member = 42;
		add( "changed last facility number", changed.Serialize().ToString() );
	}
	for ( const auto member : { &SmacFacilityRule::name, &SmacFacilityRule::prerequisite } ) {
		auto changed = smac_rules_fixture::Production(); changed.facilities.back().*member = "changed";
		add( "changed last facility text", changed.Serialize().ToString() );
	}
	for ( const auto field : { size_t( 0 ), size_t( 5 ), size_t( 356 ), size_t( 358 ), size_t( 359 ) } ) {
		for ( const auto value : { int64_t( -1 ), int64_t( INT32_MAX ) + 1 } ) {
			types::Buffer wrong; wrong.WriteInt( value );
			add( "invalid production integer " + std::to_string( field ), ReplaceProductionFrame( field, wrong ) );
		}
		types::Buffer wrong; wrong.WriteBool( true );
		add( "mistyped production integer " + std::to_string( field ), ReplaceProductionFrame( field, wrong ) );
	}
	for ( const auto field : { size_t( 357 ), size_t( 360 ) } ) {
		for ( const auto& text : { std::string{}, std::string( "bad\0text", 8 ) } ) {
			types::Buffer wrong; wrong.WriteString( text );
			add( "invalid last facility text " + std::to_string( field ), ReplaceProductionFrame( field, wrong ) );
		}
	}
	auto trailing = smac_rules_fixture::Production().Serialize(); trailing.WriteInt( 99 );
	add( "extra production fields", trailing.ToString() );
	const auto valid = smac_rules_fixture::Production().Serialize().ToString();
	for ( const auto length : { size_t( 0 ), size_t( 1 ), valid.size() - 1 } ) { add( "truncated production profile", valid.substr( 0, length ) ); }
	return result;
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
