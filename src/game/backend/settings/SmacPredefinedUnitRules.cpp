#include "SmacPredefinedUnitRules.h"

#include <limits>
#include <set>
#include <stdexcept>

namespace game::backend::settings {
namespace {
using namespace loader::txt;
using Catalog = unit::SmacPredefinedUnitCatalog;

void Require( const bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}

void Text( const std::string& text, const bool empty = false, const size_t limit = 128 ) {
	Require( ( empty || !text.empty() ) && text.size() <= limit && text.find( '\0' ) == std::string::npos,
		"Invalid or overlong predefined unit source text" );
}

void Binary( const std::string& text, const size_t limit ) {
	Text( text, false, limit );
	Require( text.find_first_not_of( "01" ) == std::string::npos, "Invalid predefined unit binary metadata" );
}

void Source( const Catalog& c ) {
	for ( const auto* id : { &c.source.ruleset_id, &c.chassis.ruleset_id, &c.weapons.ruleset_id,
		&c.defenses.ruleset_id, &c.reactors.ruleset_id, &c.abilities.ruleset_id } ) {
		Require( *id == "smac", "Predefined unit profile requires SMAC source tables" );
	}
	Require( c.chassis.chassis.size() == 9 && c.weapons.weapons.size() == 21 && c.defenses.defenses.size() == 10
		&& c.reactors.reactors.size() == 4 && c.abilities.abilities.size() == 24 && c.source.units.size() <= 64,
		"Unsupported predefined unit source table counts" );
	for ( const auto& r : c.chassis.chassis ) {
		for ( const auto& n : r.names ) {
			Text( n.name ); Require( n.grammar == "M1" || n.grammar == "M2" || n.grammar == "F1"
				|| n.grammar == "F2" || n.grammar == "N1" || n.grammar == "N2", "Invalid chassis grammar" );
		}
		Require( r.triad >= ChassisTriad::Land && r.triad <= ChassisTriad::Air, "Invalid chassis triad" ); Text( r.prerequisite );
	}
	for ( const auto& r : c.weapons.weapons ) {
		Text( r.name ); Text( r.short_name ); Text( r.prerequisite ); const auto mode = static_cast< int32_t >( r.mode );
		Require( ( mode >= 0 && mode <= 2 ) || ( mode >= 7 && mode <= 12 ), "Invalid SMAC weapon mode" );
	}
	for ( const auto& r : c.defenses.defenses ) {
		Text( r.name ); Text( r.short_name ); Text( r.prerequisite );
		Require( r.mode >= DefenseMode::Projectile && r.mode <= DefenseMode::Binary, "Invalid defense mode" );
	}
	std::set< std::string > reactor_keys;
	for ( const auto& r : c.reactors.reactors ) {
		Text( r.name ); Text( r.short_name ); Text( r.prerequisite );
		Require( reactor_keys.insert( r.short_name ).second, "Duplicate reactor key" );
	}
	for ( const auto& r : c.abilities.abilities ) {
		Text( r.name ); Text( r.prerequisite ); Text( r.abbreviation, true ); Binary( r.flags, 11 ); Text( r.description, false, 512 );
	}
	for ( const auto& r : c.source.units ) {
		Text( r.name ); Text( r.chassis ); Text( r.weapon ); Text( r.defense ); Text( r.prerequisite ); Binary( r.ability_bits, 24 );
		Require( r.plan >= UnitPlan::AutoCalculate && r.plan <= UnitPlan::AlienArtifact, "Unsupported source unit plan" );
	}
}

types::Buffer Encode( const Catalog& c ) {
	const int version=!c.workshop_name_labels ? 1:c.workshop_name_labels->at(7).empty() ? 2:3;
	types::Buffer b; b.WriteInt(version);
	b.WriteString( c.chassis.ruleset_id ); b.WriteInt( c.chassis.chassis.size() );
	for ( const auto& r : c.chassis.chassis ) {
		for ( const auto& n : r.names ) { b.WriteString( n.name ); b.WriteString( n.grammar ); }
		b.WriteInt( r.speed ); b.WriteInt( static_cast< int32_t >( r.triad ) ); b.WriteInt( r.range ); b.WriteBool( r.missile );
		b.WriteInt( r.cargo ); b.WriteInt( r.cost ); b.WriteString( r.prerequisite );
	}
	b.WriteString( c.weapons.ruleset_id ); b.WriteInt( c.weapons.weapons.size() );
	for ( const auto& r : c.weapons.weapons ) {
		b.WriteString( r.name ); b.WriteString( r.short_name ); b.WriteInt( r.offense ); b.WriteInt( static_cast< int32_t >( r.mode ) );
		b.WriteInt( r.cost ); b.WriteInt( r.icon ); b.WriteString( r.prerequisite );
	}
	b.WriteString( c.defenses.ruleset_id ); b.WriteInt( c.defenses.defenses.size() );
	for ( const auto& r : c.defenses.defenses ) {
		b.WriteString( r.name ); b.WriteString( r.short_name ); b.WriteInt( r.rating ); b.WriteInt( static_cast< int32_t >( r.mode ) );
		b.WriteInt( r.cost ); b.WriteString( r.prerequisite );
	}
	b.WriteString( c.reactors.ruleset_id ); b.WriteInt( c.reactors.reactors.size() );
	for ( const auto& r : c.reactors.reactors ) { b.WriteString( r.name ); b.WriteString( r.short_name ); b.WriteInt( r.power ); b.WriteString( r.prerequisite ); }
	b.WriteString( c.abilities.ruleset_id ); b.WriteInt( c.abilities.abilities.size() );
	for ( const auto& r : c.abilities.abilities ) {
		b.WriteString( r.name ); b.WriteInt( r.cost ); b.WriteString( r.prerequisite ); b.WriteString( r.abbreviation ); b.WriteString( r.flags ); b.WriteString( r.description );
	}
	b.WriteString( c.source.ruleset_id ); b.WriteInt( c.source.units.size() );
	for ( const auto& r : c.source.units ) {
		b.WriteString( r.name ); b.WriteString( r.chassis ); b.WriteString( r.weapon ); b.WriteString( r.defense );
		b.WriteInt( static_cast< int32_t >( r.plan ) ); b.WriteInt( r.cost ); b.WriteInt( r.carry ); b.WriteString( r.prerequisite ); b.WriteInt( r.icon ); b.WriteString( r.ability_bits );
	}
	b.WriteInt( c.units.size() );
	for ( const auto& r : c.units ) {
		for ( const uint32_t v : { r.source_id, r.chassis_id, r.weapon_id, r.defense_id, uint32_t( r.reactor ), r.abilities,
			uint32_t( r.automatic_plan ), uint32_t( r.calculated_cost ), uint32_t( r.calculated_carry ), uint32_t( r.plan ), uint32_t( r.cost ), uint32_t( r.carry ), uint32_t( r.icon ) } ) { b.WriteInt( v ); }
		b.WriteInt( static_cast< int32_t >( r.prerequisite.kind ) ); b.WriteBool( r.prerequisite.technology_id.has_value() );
		if ( r.prerequisite.technology_id ) { b.WriteInt( *r.prerequisite.technology_id ); }
		b.WriteBool( r.active ); b.WriteBool( r.prototyped ); b.WriteInt( r.faction_exclusion );
	}
	if(c.workshop_name_labels) {for(size_t i=0;i<(version==3 ? 8u:7u);++i) {const auto& label=c.workshop_name_labels->at(i);Text(label);b.WriteString(label);}}
	return b;
}

int32_t Integer( types::Buffer& b, const int64_t lo = INT32_MIN, const int64_t hi = INT32_MAX ) {
	const auto n = b.ReadInt(); Require( n >= lo && n <= hi, "Invalid predefined unit integer" ); return static_cast< int32_t >( n );
}

std::string String( types::Buffer& b, const bool empty = false, const size_t limit = 128 ) {
	auto value = b.ReadString(); Text( value, empty, limit ); return value;
}

void Table( types::Buffer& b, std::string& id, const int32_t count ) {
	id = String( b ); Require( id == "smac" && Integer( b ) == count, "Invalid predefined table identity or count" );
}

Catalog Decode( types::Buffer& b ) {
	const auto version=Integer(b,1,3);Catalog c;
	Table( b, c.chassis.ruleset_id, 9 );
	for ( size_t i = 0; i < 9; ++i ) {
		ChassisComponent r{}; for ( auto& n : r.names ) { n.name = String( b ); n.grammar = String( b, false, 2 ); }
		r.speed = Integer( b ); r.triad = static_cast< ChassisTriad >( Integer( b, 0, 2 ) ); r.range = Integer( b ); r.missile = b.ReadBool();
		r.cargo = Integer( b ); r.cost = Integer( b ); r.prerequisite = String( b ); c.chassis.chassis.push_back( std::move( r ) );
	}
	Table( b, c.weapons.ruleset_id, 21 );
	for ( size_t i = 0; i < 21; ++i ) {
		WeaponComponent r{}; r.name = String( b ); r.short_name = String( b ); r.offense = Integer( b );
		const auto mode = Integer( b, 0, 12 ); Require( mode <= 2 || mode >= 7, "Invalid SMAC weapon mode" ); r.mode = static_cast< WeaponMode >( mode );
		r.cost = Integer( b ); r.icon = Integer( b ); r.prerequisite = String( b ); c.weapons.weapons.push_back( std::move( r ) );
	}
	Table( b, c.defenses.ruleset_id, 10 );
	for ( size_t i = 0; i < 10; ++i ) {
		DefenseComponent r{}; r.name = String( b ); r.short_name = String( b ); r.rating = Integer( b ); r.mode = static_cast< DefenseMode >( Integer( b, 0, 2 ) );
		r.cost = Integer( b ); r.prerequisite = String( b ); c.defenses.defenses.push_back( std::move( r ) );
	}
	Table( b, c.reactors.ruleset_id, 4 );
	for ( size_t i = 0; i < 4; ++i ) {
		ReactorComponent r{}; r.name = String( b ); r.short_name = String( b ); r.power = Integer( b ); r.prerequisite = String( b ); c.reactors.reactors.push_back( std::move( r ) );
	}
	Table( b, c.abilities.ruleset_id, 24 );
	for ( size_t i = 0; i < 24; ++i ) {
		AbilityRecord r{}; r.name = String( b ); r.cost = Integer( b ); r.prerequisite = String( b ); r.abbreviation = String( b, true );
		r.flags = String( b, false, 11 ); r.description = String( b, false, 512 ); c.abilities.abilities.push_back( std::move( r ) );
	}
	c.source.ruleset_id = String( b ); Require( c.source.ruleset_id == "smac", "Invalid unit table identity" );
	const auto count = Integer( b, 0, 64 );
	for ( int32_t i = 0; i < count; ++i ) {
		UnitDesignRecord r{}; r.name = String( b ); r.chassis = String( b ); r.weapon = String( b ); r.defense = String( b );
		r.plan = static_cast< UnitPlan >( Integer( b, -1, 12 ) ); r.cost = Integer( b ); r.carry = Integer( b ); r.prerequisite = String( b );
		r.icon = Integer( b ); r.ability_bits = String( b, false, 24 ); c.source.units.push_back( std::move( r ) );
	}
	Require( Integer( b ) == count, "Mismatched predefined derived record count" );
	for ( int32_t i = 0; i < count; ++i ) {
		unit::SmacPredefinedUnit r{};
		r.source_id = Integer( b, 0, 63 ); r.chassis_id = Integer( b, 0, 8 ); r.weapon_id = Integer( b, 0, 20 ); r.defense_id = Integer( b, 0, 9 );
		r.reactor = Integer( b, 1, 4 ); r.abilities = Integer( b, 0, 0xffffff );
		r.automatic_plan = Integer( b, 0, 255 ); r.calculated_cost = Integer( b, 0, 255 ); r.calculated_carry = Integer( b, 0, 255 );
		r.plan = Integer( b, 0, 255 ); r.cost = Integer( b, 0, 255 ); r.carry = Integer( b, 0, 255 ); r.icon = Integer( b, 0, 255 );
		r.prerequisite.kind = static_cast< SmacTechnologyReference::Kind >( Integer( b, 0, 3 ) );
		if ( b.ReadBool() ) { r.prerequisite.technology_id = Integer( b, 0, 88 ); }
		r.active = b.ReadBool(); r.prototyped = b.ReadBool(); r.faction_exclusion = Integer( b, 0, 0 ); c.units.push_back( r );
	}
	if(version>=2) {unit::SmacWorkshopNameLabels labels;for(size_t i=0;i<(version==3 ? 8u:7u);++i) {labels[i]=String(b);}c.workshop_name_labels=std::move(labels);}
	Require( b.lenr == b.lenw, "Unexpected predefined unit profile fields" ); return c;
}
}

SmacPredefinedUnitRules SmacPredefinedUnitRules::Parse( const std::string& source, const SmacTechnologyRules& technologies ) {
	using L = loader::txt::UnitRulesTXTLoader;
	SmacPredefinedUnitRules result{ Catalog::Build( L::ParseUnits( source, "smac" ), L::ParseChassis( source, "smac" ),
		L::ParseWeapons( source, "smac" ), L::ParseDefenses( source, "smac" ), L::ParseReactors( source, "smac" ), L::ParseAbilities( source, "smac" ), technologies ) };
	result.Validate( technologies ); return result;
}

void SmacPredefinedUnitRules::Validate( const SmacTechnologyRules& technologies ) const {
	Source( catalog ); Require( catalog.units.size() == catalog.source.units.size(), "Mismatched predefined record count" );
	const auto encoded = Encode( catalog ); Require( encoded.lenw <= MAX_WIRE_BYTES, "Predefined unit profile exceeds transport budget" );
	auto rebuilt = Catalog::Build( catalog.source, catalog.chassis, catalog.weapons, catalog.defenses, catalog.reactors, catalog.abilities, technologies );
    rebuilt.workshop_name_labels=catalog.workshop_name_labels;
	Require( encoded.ToString() == Encode( rebuilt ).ToString(), "Predefined unit derived records differ from the checked source producer" );
}

bool SmacPredefinedUnitRules::operator==( const SmacPredefinedUnitRules& other ) const { return Encode( catalog ).ToString() == Encode( other.catalog ).ToString(); }

types::Buffer SmacPredefinedUnitRules::Serialize( const SmacTechnologyRules& technologies ) const { Validate( technologies ); return Encode( catalog ); }

SmacPredefinedUnitRules SmacPredefinedUnitRules::Deserialize( const std::string& bytes, const SmacTechnologyRules& technologies ) {
	if ( bytes.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "Predefined unit profile exceeds transport budget" ); }
	try {
		types::Buffer buffer( bytes ); SmacPredefinedUnitRules result{ Decode( buffer ) }; result.Validate( technologies ); return result;
	} catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

}
