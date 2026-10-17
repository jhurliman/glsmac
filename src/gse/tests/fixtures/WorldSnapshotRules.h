#pragma once

#include "game/backend/WorldSnapshotRules.h"
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
	add( "unknown version", Header( "smac", false, {}, game::backend::world_snapshot::rules_marker, 2 ) );
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

}
