#pragma once

#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/WorldSnapshotTurn.h"
#include "types/Buffer.h"
#include "network/Network.h"
#include "SmacRulesSource.h"

#include <array>
#include <functional>
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
	const std::optional< std::string >& production = std::nullopt,
	const std::optional< bool > technologies_present = std::nullopt,
	const std::optional< std::string >& technologies = std::nullopt,
	const std::optional< bool > sizes_present = std::nullopt,
	const std::optional< std::string >& sizes = std::nullopt,
	const std::optional< std::string >& selection = std::nullopt,
	const std::optional< bool > factions_present = std::nullopt,
	const std::optional< std::string >& factions = std::nullopt,
	const std::optional< bool > units_present = std::nullopt,
	const std::optional< std::string >& units = std::nullopt,
	const std::optional< bool > movement_present = std::nullopt,
	const std::optional< std::string >& movement = std::nullopt,
	const std::optional< std::string >& resources = std::nullopt,
	const std::optional< bool > specialists_present = std::nullopt,
	const std::optional< std::string >& specialists = std::nullopt ) {
	types::Buffer result;
	result.WriteInt( marker );
	result.WriteInt( version );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) {
		if ( version >= 16 ) { result.WriteString( resources ? *resources : smac_rules_fixture::ResourceFrame( values, technologies ) ); }
		else { for ( const auto value : values ) { result.WriteInt( value ); } }
	}
	if ( version >= 4 ) {
		const auto has_production = production_present.value_or( id == "smac" );
		result.WriteBool( has_production );
		if ( has_production ) { result.WriteString( production ? *production : smac_rules_fixture::Production().Serialize().ToString() ); }
	}
	if ( version >= 5 ) {
		const auto has_technologies = technologies_present.value_or( id == "smac" );
		result.WriteBool( has_technologies );
		if ( has_technologies ) { result.WriteString( technologies ? *technologies : smac_rules_fixture::Technologies().Serialize().ToString() ); }
	}
	if ( version >= 6 ) {
		const auto present_sizes = sizes_present.value_or( id == "smac" ); result.WriteBool( present_sizes );
		if ( present_sizes ) {
			result.WriteString( sizes ? *sizes : smac_rules_fixture::WorldSizes().Serialize().ToString() );
			result.WriteString( selection ? *selection : smac_rules_fixture::WorldSizes().SelectPreset( 0 ).Serialize().ToString() );
		}
	}
	if ( version >= 7 ) {
		const auto present_factions = factions_present.value_or( id == "smac" ); result.WriteBool( present_factions );
		if ( present_factions ) { result.WriteString( factions ? *factions : smac_rules_fixture::Factions().Serialize( smac_rules_fixture::Technologies() ).ToString() ); }
	}
	if ( version >= 8 ) {
		const auto present_units = units_present.value_or( id == "smac" ); result.WriteBool( present_units );
		if ( present_units ) { result.WriteString( units ? *units : smac_rules_fixture::PredefinedUnits().Serialize( smac_rules_fixture::Technologies() ).ToString() ); }
	}
	if ( version >= 12 ) {
		const auto has_movement = movement_present.value_or( id == "smac" ); result.WriteBool( has_movement );
		if ( has_movement ) { result.WriteString( movement ? *movement : smac_rules_fixture::Movement().Serialize().ToString() ); }
	}
	if ( version >= 17 ) {
		const auto has_specialists = specialists_present.value_or( id == "smac" ); result.WriteBool( has_specialists );
		if ( has_specialists ) { result.WriteString( specialists ? *specialists : smac_rules_fixture::Specialists().Serialize().ToString() ); }
	}
	return result;
}

inline types::Buffer ResourceHeader( const std::string& id, const std::string& resources ) {
	auto original = Header( id, id == "smac" );
	original.ReadInt(); original.ReadInt(); original.ReadString(); original.ReadBool();
	const auto prefix = original.ToString().substr( 0, original.lenr );
	original.ReadString(); types::Buffer frame; frame.WriteString( resources );
	return types::Buffer( prefix + frame.ToString() + original.ToString().substr( original.lenr ) );
}

inline std::vector< std::pair< std::string, std::string > > InvalidResourceHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	result.emplace_back( "old V15", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
		game::backend::world_snapshot::rules_marker, 15 ).ToString() );
	if ( id != "smac" ) { return result; }
	for ( size_t field = 0; field < 5; ++field ) {
		auto changed = smac_rules_fixture::Resources();
		if ( field == 0 ) { changed.rows[ 8 ][ 2 ] = 19; }
		else if ( field == 1 ) { changed.nutrients_per_citizen = -2; }
		else if ( field == 2 ) { changed.mine_nutrient_effect = -3; }
		else { changed.technology_tokens[ 3 ] = field == 3 ? "Tech4" : "Missing"; }
		result.emplace_back( "late resource field " + std::to_string( field ), ResourceHeader( id, smac_rules_fixture::UncheckedResources( changed ).ToString() ).ToString() );
	}
	const auto valid = smac_rules_fixture::UncheckedResources( smac_rules_fixture::Resources() ).ToString();
	result.emplace_back( "truncated last token", ResourceHeader( id, valid.substr( 0, valid.size() - 1 ) ).ToString() );
	return result;
}

inline std::string SpecialistPrefix( const std::string& id ) {
	auto wire = Header( id, id == "smac" );
	wire.ReadInt(); wire.ReadInt(); wire.ReadString();
	for ( size_t i = 0; i < 7; ++i ) {
		if ( wire.ReadBool() ) { wire.ReadString(); if ( i == 3 ) { wire.ReadString(); } }
	}
	return wire.ToString().substr( 0, wire.lenr );
}

inline types::Buffer SpecialistHeader( const std::string& id, const bool present, const std::string& bytes ) {
	types::Buffer tail; tail.WriteBool( present ); if ( present ) { tail.WriteString( bytes ); }
	return types::Buffer( SpecialistPrefix( id ) + tail.ToString() );
}

inline std::vector< std::pair< std::string, std::string > > InvalidSpecialistHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	result.emplace_back( "old V16", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, game::backend::world_snapshot::rules_marker, 16 ).ToString() );
	result.emplace_back( "old V17", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, game::backend::world_snapshot::rules_marker, 17 ).ToString() );
	result.emplace_back( "old V18 without movement caller history", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, game::backend::world_snapshot::rules_marker, 18 ).ToString() );
	const auto valid = smac_rules_fixture::Specialists().Serialize().ToString();
	result.emplace_back( "wrong presence", SpecialistHeader( id, id != "smac", valid ).ToString() );
	if ( id != "smac" ) { return result; }
	result.emplace_back( "missing presence", SpecialistPrefix( id ) );
	const auto add = [ & ]( const std::string& label, const std::string& bytes ) { result.emplace_back( label, SpecialistHeader( id, true, bytes ).ToString() ); };
	for ( size_t field = 0; field < 10; ++field ) {
		auto changed = smac_rules_fixture::Specialists(); auto& row = changed.specialists.back();
		switch ( field ) {
			case 0: ++changed.minimum_base_size; break;
			case 1: row.singular += " changed"; break;
			case 2: row.plural += " changed"; break;
			case 3: row.prerequisite = "Missing"; break;
			case 4: row.obsolete = "None"; break;
			case 5: ++row.economy; break;
			case 6: --row.psych; break;
			case 7: --row.labs; break;
			case 8: row.trailing_token += " changed"; break;
			case 9: row.source_id = 5; break;
		}
		add( "last row field " + std::to_string( field ), smac_rules_fixture::UncheckedSpecialists( changed ).ToString() );
	}
	add( "truncated last token", valid.substr( 0, valid.size() - 1 ) ); add( "trailing nested byte", valid + 'x' );
	for ( const auto size : { game::backend::settings::SmacSpecialistRules::MAX_WIRE_BYTES + 1, 32768u } ) { add( "oversized " + std::to_string( size ), std::string( size, 'x' ) ); }
	types::Buffer version; version.WriteInt( 2 ); add( "unknown nested version", version.ToString() + valid.substr( version.lenw ) );
	return result;
}

struct FrozenFrame {
	std::string name, prefix, payload;
	uint32_t limit;
};

inline std::vector< FrozenFrame > FrozenFrames( const std::string& id ) {
	auto wire = Header( id, id == "smac" );
	std::vector< FrozenFrame > result;
	const auto frame = [ & ]( const std::string& name, const uint32_t limit ) {
		const auto prefix = wire.ToString().substr( 0, wire.lenr );
		result.push_back( { name, prefix, wire.ReadString(), limit } );
	};
	wire.ReadInt(); wire.ReadInt(); frame( "ruleset", 4 );
	if ( id != "smac" ) { return result; }
	wire.ReadBool(); frame( "resources", game::backend::settings::SmacResourceRules::MAX_WIRE_BYTES );
	for ( const auto* name : { "production", "technology" } ) { wire.ReadBool(); frame( name, network::Network::MAX_PACKET_BYTES ); }
	wire.ReadBool(); frame( "world sizes", network::Network::MAX_PACKET_BYTES );
	// Independently derive the accepted-selection extent from its four fields.
	types::Buffer integer; integer.WriteInt( 0 ); frame( "selection", 4 * integer.lenw );
	wire.ReadBool(); frame( "factions", network::Network::MAX_PACKET_BYTES );
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidFrozenFrameHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	for ( const auto& field : FrozenFrames( id ) ) {
		for ( const auto length : { field.limit + 1, field.limit * 2 } ) {
			types::Buffer tail; tail.WriteString( std::string( length, 'x' ) ); tail.WriteString( "unread world body" );
			result.emplace_back( field.name + " oversized " + std::to_string( length ), field.prefix + tail.ToString() );
		}
	}
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidPredefinedHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	const auto header = [ & ]( const bool present, const std::optional< std::string >& bytes = {} ) {
		return Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
			game::backend::world_snapshot::rules_marker, game::backend::world_snapshot::rules_version,
			{}, {}, {}, {}, {}, {}, {}, {}, {}, present, bytes ).ToString();
	};
	result.emplace_back( "old V7", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
		game::backend::world_snapshot::rules_marker, 7 ).ToString() );
	if ( id == "smax" ) { result.emplace_back( "unexpected predefined profile", header( true ) ); return result; }
	result.emplace_back( "absent predefined profile", header( false ) );
	types::Buffer absent, present, wrong; absent.WriteBool( false ); present.WriteBool( true ); wrong.WriteInt( 1 );
	// Walk to predefined presence rather than trimming the new movement tail.
	types::Buffer before_units( header( false ) );
	before_units.ReadInt(); before_units.ReadInt(); before_units.ReadString(); before_units.ReadBool();
	before_units.ReadString();
	for ( size_t i = 0; i < 2; ++i ) { before_units.ReadBool(); before_units.ReadString(); }
	before_units.ReadBool(); before_units.ReadString(); before_units.ReadString();
	before_units.ReadBool(); before_units.ReadString();
	const auto prefix = before_units.ToString().substr( 0, before_units.lenr );
	result.emplace_back( "missing predefined presence", prefix );
	result.emplace_back( "mistyped predefined presence", prefix + wrong.ToString() );
	result.emplace_back( "missing predefined payload", prefix + present.ToString() );
	result.emplace_back( "mistyped predefined payload", prefix + present.ToString() + wrong.ToString() );
	const auto add = [ & ]( const std::string& name, const std::string& bytes ) { result.emplace_back( name, header( true, bytes ) ); };
	const auto technologies = smac_rules_fixture::Technologies();
	const auto profile = smac_rules_fixture::PredefinedUnits();
	const auto bytes = profile.Serialize( technologies ).ToString();
	for ( const auto length : { size_t( 0 ), size_t( 1 ), bytes.size() - 1 } ) { add( "truncated predefined payload", bytes.substr( 0, length ) ); }
	add( "extra predefined fields", bytes + wrong.ToString() );
	add( "oversized predefined payload", std::string( game::backend::settings::SmacPredefinedUnitRules::MAX_WIRE_BYTES + 1, 'x' ) );
	add( "oversized downloaded predefined payload", std::string( game::backend::settings::SmacPredefinedUnitRules::MAX_WIRE_BYTES * 4, 'x' ) );
	types::Buffer bad_version; bad_version.WriteInt( 2 );
	add( "unknown predefined version", bad_version.ToString() + bytes.substr( bad_version.lenw ) );
	for ( const auto value : { int64_t( 1 ), INT64_MAX } ) {
		types::Buffer bad; bad.WriteInt( value ); add( "forged last derived exclusion", bytes.substr( 0, bytes.size() - bad.lenw ) + bad.ToString() );
	}
	add( "mistyped last derived exclusion", bytes.substr( 0, bytes.size() - wrong.lenw ) + present.ToString() );
	using Profile = game::backend::settings::SmacPredefinedUnitRules;
	const std::vector< std::function< void( Profile& ) > > changes = {
		[]( auto& p ) { p.catalog.chassis.chassis.back().names[ 5 ].name += " changed"; },
		[]( auto& p ) { p.catalog.chassis.chassis.back().range = INT32_MIN; },
		[]( auto& p ) { p.catalog.weapons.weapons.back().name += " changed"; },
		[]( auto& p ) { p.catalog.defenses.defenses.back().name += " changed"; },
		[]( auto& p ) { p.catalog.reactors.reactors.back().power = INT32_MIN; },
		[]( auto& p ) { p.catalog.abilities.abilities.back().description += "; changed"; },
		[]( auto& p ) { p.catalog.source.units.back().name += " changed"; },
		[]( auto& p ) { p.catalog.source.units.back().cost = 17; },
		[]( auto& p ) { p.catalog.source.units.back().carry = 17; },
		[]( auto& p ) { p.catalog.source.units.back().icon = 17; },
		[]( auto& p ) { p.catalog.source.units.back().prerequisite = "Unknown"; },
		[]( auto& p ) { p.catalog.source.units.pop_back(); }
	};
	for ( size_t i = 0; i < changes.size(); ++i ) {
		auto changed = profile; changes[ i ]( changed ); auto& c = changed.catalog;
		c = game::backend::unit::SmacPredefinedUnitCatalog::Build( c.source, c.chassis, c.weapons, c.defenses, c.reactors, c.abilities, technologies );
		add( "different valid predefined source/derived field " + std::to_string( i ), changed.Serialize( technologies ).ToString() );
	}
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidMovementHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	const auto header = [ & ]( const bool present, const std::optional< std::string >& bytes = {} ) {
		return Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
			game::backend::world_snapshot::rules_marker, game::backend::world_snapshot::rules_version,
			{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, present, bytes ).ToString();
	};
	result.emplace_back( "old V11", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
		game::backend::world_snapshot::rules_marker, 11 ).ToString() );
	result.emplace_back( "old V12 without social continuation", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
		game::backend::world_snapshot::rules_marker, 12 ).ToString() );
	if ( id == "smax" ) { result.emplace_back( "unexpected movement profile", header( true ) ); return result; }
	result.emplace_back( "absent movement profile", header( false ) );
	types::Buffer absent, present, wrong; absent.WriteBool( false ); present.WriteBool( true ); wrong.WriteInt( 1 );
	auto before_movement = Header( id, true );
	before_movement.ReadInt(); before_movement.ReadInt(); before_movement.ReadString();
	for ( size_t i = 0; i < 3; ++i ) { before_movement.ReadBool(); before_movement.ReadString(); }
	before_movement.ReadBool(); before_movement.ReadString(); before_movement.ReadString();
	for ( size_t i = 0; i < 2; ++i ) { before_movement.ReadBool(); before_movement.ReadString(); }
	const auto prefix = before_movement.ToString().substr( 0, before_movement.lenr );
	result.emplace_back( "missing movement presence", prefix );
	result.emplace_back( "mistyped movement presence", prefix + wrong.ToString() );
	result.emplace_back( "missing movement payload", prefix + present.ToString() );
	result.emplace_back( "mistyped movement payload", prefix + present.ToString() + wrong.ToString() );
	const auto add = [ & ]( const std::string& name, const std::string& bytes ) { types::Buffer frame; frame.WriteString( bytes ); result.emplace_back( name, prefix + present.ToString() + frame.ToString() ); };
	const auto bytes = smac_rules_fixture::Movement().Serialize().ToString();
	add( "oversized movement cap+1", bytes + 'x' );
	add( "oversized movement complete frame", bytes + std::string( 4096 - bytes.size(), 'x' ) );
	for ( const auto length : { size_t( 0 ), size_t( 1 ), bytes.size() - 1 } ) { add( "truncated movement payload", bytes.substr( 0, length ) ); }
	add( "extra movement fields", bytes + wrong.ToString() );
	for ( const int64_t value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) {
		types::Buffer bad; bad.WriteInt( 1 ); bad.WriteInt( value ); add( "out of range movement rule", bad.ToString() );
	}
	types::Buffer bad; bad.WriteInt( 1 ); bad.WriteString( "3" ); add( "mistyped movement rule", bad.ToString() );
	types::Buffer version; version.WriteInt( 2 ); version.WriteInt( 3 ); add( "unknown movement version", version.ToString() );
	for ( const int32_t value : { 0, 3, INT32_MIN, INT32_MAX } ) {
		add( "different movement raw rule " + std::to_string( value ), game::backend::settings::SmacMovementRules( value ).Serialize().ToString() );
	}
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidFactionHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > result;
	const auto header = [ & ]( bool present, const std::optional< std::string >& bytes = {} ) {
		return Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
			game::backend::world_snapshot::rules_marker, game::backend::world_snapshot::rules_version,
			{}, {}, {}, {}, {}, {}, {}, present, bytes ).ToString();
	};
	result.emplace_back( "old V10", Header( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, game::backend::world_snapshot::rules_marker, 10 ).ToString() );
	if ( id == "smax" ) { result.emplace_back( "unexpected faction profile", header( true ) ); return result; }
	const auto add = [ & ]( const std::string& name, const std::string& bytes ) { result.emplace_back( name, header( true, bytes ) ); };
	result.emplace_back( "absent faction profile", header( false ) );
	types::Buffer present, wrong; present.WriteBool( true ); wrong.WriteInt( 1 );
	// Stop before faction presence; later versions append additional profiles.
	types::Buffer before_faction( header( false ) );
	before_faction.ReadInt(); before_faction.ReadInt(); before_faction.ReadString(); before_faction.ReadBool();
	before_faction.ReadString();
	for ( size_t i = 0; i < 2; ++i ) { before_faction.ReadBool(); before_faction.ReadString(); }
	before_faction.ReadBool(); before_faction.ReadString(); before_faction.ReadString();
	const auto prefix = before_faction.ToString().substr( 0, before_faction.lenr );
	result.emplace_back( "missing faction presence", prefix );
	result.emplace_back( "mistyped faction presence", prefix + wrong.ToString() );
	result.emplace_back( "missing faction payload", prefix + present.ToString() );
	result.emplace_back( "mistyped faction payload", prefix + present.ToString() + wrong.ToString() );
	const auto technologies = smac_rules_fixture::Technologies();
	const auto profile = smac_rules_fixture::Factions();
	const auto bytes = profile.Serialize( technologies ).ToString();
	for ( const auto* keyword : { "FUNGMINERALS", "FUNGENERGY" } ) {
		auto changed = profile;
		changed.factions.back().bonuses.push_back( game::backend::settings::SmacFactionStartupBonus::Parse( keyword, "3", technologies ) );
		add( std::string( "changed appended fungal metadata " ) + keyword, changed.Serialize( technologies ).ToString() );
	}
	for ( const auto length : { size_t( 0 ), size_t( 1 ), bytes.size() - 1 } ) { add( "truncated faction payload", bytes.substr( 0, length ) ); }
	auto trailing = profile.Serialize( technologies ); trailing.WriteInt( 1 ); add( "extra faction fields", trailing.ToString() );
	for ( size_t i = 0; i < 4; ++i ) {
		auto changed = profile; changed.initial_social_models[ i ].name += " changed";
		add( "different model-zero source name", changed.Serialize( technologies ).ToString() );
		for ( const int field : { 0, 1, 2 } ) {
			changed = profile; auto& row = changed.initial_social_models[ i ];
			if ( field == 0 ) { row.name = std::string( 257, 'x' ); }
			if ( field == 1 ) { row.prerequisite = "Disable"; }
			if ( field == 2 ) { row.modifiers = "+MORALE"; }
			add( "invalid model-zero source field", smac_rules_fixture::UncheckedFactions( changed ).ToString() );
		}
	}
	types::Buffer old_version; old_version.WriteInt( 1 );
	add( "old faction V1", old_version.ToString() + bytes.substr( old_version.lenw ) );
	using Bonus = game::backend::settings::SmacFactionStartupBonus;
	const std::array< const char*, 12 > values = { "Tech3", "tech2 ", "Disable", "None", "--SUPPORT", "17", "POLICE", "2", "3", "+1suffix", "7", "23" };
	for ( size_t i = 0; i < values.size(); ++i ) {
		auto changed = profile; auto& bonus = changed.factions.back().bonuses[ i ];
		bonus = Bonus::Parse( bonus.keyword, values[ i ], technologies );
		add( "changed late bonus " + std::to_string( i ), changed.Serialize( technologies ).ToString() );
	}
	for ( const int field : { 0, 1, 2, 3, 4, 5 } ) {
		auto changed = profile; auto& faction = changed.factions.back(); auto& bonus = faction.bonuses.back();
		if ( field == 0 ) { faction.port_id = "CUSTOM"; }
		if ( field == 1 ) { faction.filename = "custom.txt"; }
		if ( field == 2 ) { faction.section = "CUSTOM"; }
		if ( field == 3 ) { bonus.kind = Bonus::Kind::FANATIC; }
		if ( field == 4 ) { ++bonus.parameter; }
		if ( field == 5 ) { ++bonus.amount; }
		add( "invalid late faction field " + std::to_string( field ), smac_rules_fixture::UncheckedFactions( changed ).ToString() );
	}
	// Independently corrupt framed integers after all preceding records decode.
	auto wire = profile.Serialize( technologies ); wire.ReadInt(); const auto models = wire.ReadInt();
	for ( int64_t i = 0; i < models; ++i ) { wire.ReadString(); wire.ReadString(); wire.ReadString(); }
	const auto count = wire.ReadInt();
	std::vector< std::pair< size_t, size_t > > numbers;
	for ( int64_t faction = 0; faction < count; ++faction ) {
		wire.ReadString(); wire.ReadString(); wire.ReadString(); const auto bonuses = wire.ReadInt();
		for ( int64_t bonus = 0; bonus < bonuses; ++bonus ) {
			wire.ReadString(); wire.ReadString();
			for ( int field = 0; field < 3; ++field ) { const auto start = wire.lenr; wire.ReadInt(); numbers.emplace_back( start, wire.lenr ); }
		}
	}
	for ( size_t offset = 0; offset < 3; ++offset ) {
		const auto span = numbers[ numbers.size() - 1 - offset ];
		for ( const int64_t value : { int64_t( INT32_MAX ) + 1, int64_t( INT32_MIN ) - 1 } ) {
			types::Buffer number; number.WriteInt( value );
			add( "out-of-range late faction integer", bytes.substr( 0, span.first ) + number.ToString() + bytes.substr( span.second ) );
		}
		add( "mistyped late faction integer", bytes.substr( 0, span.first ) + present.ToString() + bytes.substr( span.second ) );
	}
	return result;
}

inline std::vector< std::pair< std::string, std::string > > InvalidWorldSizeHeaders( const std::string& id ) {
	std::vector< std::pair< std::string, std::string > > invalid;
	const std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
	const auto add = [ & ]( const std::string& label, bool present, const std::optional< std::string >& sizes = {}, const std::optional< std::string >& selection = {} ) {
		invalid.emplace_back( label, Header( id, id == "smac", values, game::backend::world_snapshot::rules_marker, game::backend::world_snapshot::rules_version,
			{}, {}, {}, {}, present, sizes, selection ).ToString() );
	};
	if ( id == "smax" ) { add( "unexpected size profile", true ); return invalid; }
	add( "absent size profile", false );
	add( "different valid accepted preset", true, {}, smac_rules_fixture::WorldSizes().SelectPreset( 1 ).Serialize().ToString() );
	add( "different valid accepted custom", true, {}, game::backend::settings::SmacWorldSizeRules::SelectCustom( 48, 48 ).Serialize().ToString() );
	for ( size_t i = 0; i < 5; ++i ) {
		for ( int field = 0; field < 3; ++field ) {
			auto changed = smac_rules_fixture::WorldSizes();
			if ( field == 0 ) { changed.presets[ i ].name += " changed"; }
			if ( field == 1 ) { ++changed.presets[ i ].source_x; }
			if ( field == 2 ) { changed.presets[ i ].source_y += 2; }
			add( "changed source row " + std::to_string( i ) + ":" + std::to_string( field ), true, changed.Serialize().ToString() );
		}
	}
	auto rows = smac_rules_fixture::WorldSizes().Serialize(); rows.WriteInt( 1 ); add( "extra catalog fields", true, rows.ToString() );
	for ( const auto category : { int64_t( -1 ), int64_t( 1 ), int64_t( 5 ), int64_t( 99 ), INT64_MAX } ) {
		types::Buffer selection; selection.WriteInt( 1 ); selection.WriteInt( category ); selection.WriteInt( 8 ); selection.WriteInt( 8 );
		add( "invalid or different category", true, {}, selection.ToString() );
	}
	for ( const auto dimensions : { std::pair< int64_t, int64_t >{ -1, 8 }, { 8, 9 }, { INT64_MAX, 8 }, { 10, 8 } } ) {
		types::Buffer selection; selection.WriteInt( 1 ); selection.WriteInt( 0 ); selection.WriteInt( dimensions.first ); selection.WriteInt( dimensions.second );
		add( "invalid or changed selected dimensions", true, {}, selection.ToString() );
	}
	const auto encoded = smac_rules_fixture::WorldSizes().Serialize().ToString();
	for ( const auto length : { size_t( 0 ), size_t( 1 ), encoded.size() - 1 } ) { add( "truncated catalog", true, encoded.substr( 0, length ) ); }
	types::Buffer malformed; malformed.WriteInt( 1 ); malformed.WriteInt( 5 );
	for ( size_t i = 0; i < 5; ++i ) { malformed.WriteString( "valid name" ); malformed.WriteInt( 24 ); if ( i == 4 ) { malformed.WriteString( "48" ); } else { malformed.WriteInt( 48 ); } }
	add( "mistyped last catalog dimension", true, malformed.ToString() );
	return invalid;
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
	auto old_v4 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 4 );
	game::backend::world_snapshot::WriteRandomState( old_v4, { 1, 2, 3, 4 } );
	game::backend::world_snapshot::WriteTurnState( old_v4, { 37, { { 0, true }, { 2, false } } } );
	old_v4.WriteString( "old map body" ); add( "old V4", old_v4 );
	auto old_v5 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 5 );
	game::backend::world_snapshot::WriteRandomState( old_v5, { 1, 2, 3, 4 } );
	game::backend::world_snapshot::WriteTurnState( old_v5, { 37, { { 0, true }, { 2, false } } } );
	old_v5.WriteString( "old map body" ); add( "old V5", old_v5 );
	auto old_v6 = Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 6 );
	game::backend::world_snapshot::WriteRandomState( old_v6, { 1, 2, 3, 4 } );
	game::backend::world_snapshot::WriteTurnState( old_v6, { 37, { { 0, true }, { 2, false } } } );
	old_v6.WriteString( "old map body" ); add( "old V6", old_v6 );
	add( "old V14 without campaign capability", Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 14 ) );
	add( "old V13 without gameplay pair", Header( "smac", true, valid_rules, game::backend::world_snapshot::rules_marker, 13 ) );
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
	// Stop before production presence; later header versions append more fields.
	types::Buffer before_production( header( false ) );
	before_production.ReadInt(); before_production.ReadInt(); before_production.ReadString(); before_production.ReadBool();
	before_production.ReadString();
	const auto prefix = before_production.ToString().substr( 0, before_production.lenr );
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

inline std::string ReplaceTechnologyFrame( const size_t index, const types::Buffer& replacement ) {
	auto original = smac_rules_fixture::Technologies().Serialize();
	for ( size_t i = 0; i <= index; ++i ) {
		const auto start = original.lenr;
		const auto field = i >= 2 ? ( i - 2 ) % 10 : SIZE_MAX;
		if ( field == 1 || field == 2 || field == 7 || field == 8 ) { original.ReadString(); }
		else { original.ReadInt(); }
		if ( i == index ) {
			const auto bytes = original.ToString();
			return bytes.substr( 0, start ) + replacement.ToString() + bytes.substr( original.lenr );
		}
	}
	throw std::runtime_error( "technology test frame was not found" );
}

inline std::vector< std::pair< std::string, std::string > > InvalidTechnologyHeaders( const std::string& id = "smac" ) {
	std::vector< std::pair< std::string, std::string > > result;
	const std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
	const auto header = [ & ]( const bool present, const std::optional< std::string >& bytes = std::nullopt ) {
		return Header( id, id == "smac", values, game::backend::world_snapshot::rules_marker,
			game::backend::world_snapshot::rules_version, {}, {}, present, bytes ).ToString();
	};
	if ( id == "smax" ) { result.emplace_back( "unexpected SMAX technology catalog", header( true ) ); return result; }
	result.emplace_back( "missing SMAC technology catalog", header( false ) );
	types::Buffer absent, present, wrong_type;
	absent.WriteBool( false ); present.WriteBool( true ); wrong_type.WriteInt( 1 );
	types::Buffer before_technology( header( false ) );
	before_technology.ReadInt(); before_technology.ReadInt(); before_technology.ReadString(); before_technology.ReadBool();
	before_technology.ReadString();
	before_technology.ReadBool(); before_technology.ReadString();
	const auto prefix = before_technology.ToString().substr( 0, before_technology.lenr );
	result.emplace_back( "missing technology presence", prefix );
	result.emplace_back( "mistyped technology presence", prefix + wrong_type.ToString() );
	result.emplace_back( "missing technology payload", prefix + present.ToString() );
	result.emplace_back( "mistyped technology payload", prefix + present.ToString() + wrong_type.ToString() );
	const auto add = [ & ]( const std::string& name, const std::string& bytes ) { result.emplace_back( name, header( true, bytes ) ); };
	using game::backend::settings::SmacTechnologyRule;
	for ( const auto member : { &SmacTechnologyRule::name, &SmacTechnologyRule::key } ) {
		auto changed = smac_rules_fixture::Technologies(); changed.technologies.back().*member = "Changed";
		add( "changed last technology text", changed.Serialize().ToString() );
	}
	for ( size_t field = 0; field < 4; ++field ) {
		auto changed = smac_rules_fixture::Technologies(); changed.technologies.back().priorities[ field ] = 42;
		add( "changed last technology priority", changed.Serialize().ToString() );
	}
	for ( size_t field = 0; field < 2; ++field ) {
		auto changed = smac_rules_fixture::Technologies(); changed.technologies.back().prerequisites[ field ] = "Disable";
		add( "changed last technology prerequisite", changed.Serialize().ToString() );
	}
	auto changed = smac_rules_fixture::Technologies(); changed.technologies.back().flags ^= 1;
	add( "changed last technology flags", changed.Serialize().ToString() );
	for ( const auto field : { size_t( 0 ), size_t( 1 ), size_t( 882 ), size_t( 885 ), size_t( 891 ) } ) {
		for ( const auto value : { int64_t( -1 ), int64_t( INT32_MAX ) + 1 } ) {
			// A negative priority is legitimate; use only out-of-range there.
			if ( field == 885 && value == -1 ) { continue; }
			types::Buffer wrong; wrong.WriteInt( value );
			add( "invalid technology integer " + std::to_string( field ), ReplaceTechnologyFrame( field, wrong ) );
		}
		types::Buffer wrong; wrong.WriteBool( true );
		add( "mistyped technology integer " + std::to_string( field ), ReplaceTechnologyFrame( field, wrong ) );
	}
	for ( const auto field : { size_t( 883 ), size_t( 884 ), size_t( 889 ), size_t( 890 ) } ) {
		for ( const auto& text : { std::string{}, std::string( "bad\0text", 8 ) } ) {
			types::Buffer wrong; wrong.WriteString( text );
			add( "invalid last technology text " + std::to_string( field ), ReplaceTechnologyFrame( field, wrong ) );
		}
	}
	auto trailing = smac_rules_fixture::Technologies().Serialize(); trailing.WriteInt( 99 );
	add( "extra technology fields", trailing.ToString() );
	const auto valid = smac_rules_fixture::Technologies().Serialize().ToString();
	for ( const auto length : { size_t( 0 ), size_t( 1 ), valid.size() - 1 } ) { add( "truncated technology catalog", valid.substr( 0, length ) ); }
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
