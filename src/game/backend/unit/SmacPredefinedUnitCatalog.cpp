#include "SmacPredefinedUnitCatalog.h"

#include "loader/txt/UnitDesignResolver.h"
#include "loader/txt/TXTLoader.h"
#include <charconv>

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace game::backend::unit {
SmacWorkshopNameLabels ReadSmacWorkshopNameLabels(const std::string& source) {
    if(source.size()>1024*1024) {throw std::invalid_argument("Workshop labels exceed source budget");}
    const auto rows=loader::txt::TXTLoader::ReadUniqueSection(source,"LABELS");
    if(rows.empty()) {throw std::invalid_argument("Missing Workshop label count");}
    size_t count=0;const auto parsed=std::from_chars(rows[0].data(),rows[0].data()+rows[0].size(),count);
    if(parsed.ec!=std::errc{} || parsed.ptr!=rows[0].data()+rows[0].size() || count<920 || count>8192 || rows.size()<count+1) {throw std::invalid_argument("Incomplete Workshop label table");}
    SmacWorkshopNameLabels result;const std::array<size_t,7> slots{147,183,544,545,615,616,920};
    for(size_t i=0;i<slots.size();++i) {
        result[i]=rows[slots[i]];if(result[i].empty() || result[i].size()>128 || result[i].find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop name label");}
    }return result;
}

namespace {

using namespace loader::txt;
constexpr uint32_t ABILITY_MASK = ( 1u << 24 ) - 1;
constexpr uint32_t SLOW = 1u << 23, HEAVY = 1u << 19, AIR_SUPERIORITY = 1u << 5;

void Require( const bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}

int32_t Checked( const int64_t value ) {
	Require( value >= 0 && value <= INT32_MAX, "SMAC unit arithmetic is outside supported nonnegative signed32 range" );
	return static_cast< int32_t >( value );
}

void Domain( const ChassisComponent& chassis, const uint32_t reactor, const uint32_t mask ) {
	Require( reactor >= 1 && reactor <= 4, "SMAC unit reactor must be 1..4" );
	Require( ( mask & ~ABILITY_MASK ) == 0, "SMAC unit ability mask exceeds 24 slots" );
	Require( chassis.triad == ChassisTriad::Land || chassis.triad == ChassisTriad::Sea || chassis.triad == ChassisTriad::Air,
		"Unsupported SMAC chassis triad" );
}

uint32_t Binary( const std::string& text, const size_t width ) {
	Require( !text.empty() && text.size() <= width, "SMAC unit binary field has unsupported width" );
	uint32_t result = 0;
	for ( const auto digit : text ) {
		Require( digit == '0' || digit == '1', "SMAC unit binary field contains a nonbinary digit" );
		result = result * 2 + static_cast< uint32_t >( digit - '0' );
	}
	return result;
}

std::array< uint32_t, 24 > AbilityFlags( const AbilityTable& abilities ) {
	Require( abilities.ruleset_id == "smac" && abilities.abilities.size() == 24, "SMAC unit calculations require 24 SMAC ability rows" );
	std::array< uint32_t, 24 > flags{};
	for ( size_t i = 0; i < flags.size(); ++i ) {
		Require( abilities.abilities[ i ].cost >= -7, "Unsupported negative SMAC ability cost code" );
		flags[ i ] = Binary( abilities.abilities[ i ].flags, 11 );
	}
	return flags;
}

uint8_t Byte( const int64_t value ) { return static_cast< uint8_t >( static_cast< uint64_t >( value ) & 0xffu ); }

template< typename Row, typename Key >
uint32_t Index( const std::vector< Row >& rows, const std::string& key, const Key& get_key ) {
	for ( size_t i = 0; i < rows.size(); ++i ) { if ( get_key( rows[ i ] ) == key ) { return static_cast< uint32_t >( i ); } }
	throw std::invalid_argument( "Unresolved SMAC unit component" ); // resolver validates first
}

uint8_t AutomaticPlan( const uint32_t c, const uint32_t w, const uint32_t d,
	const ChassisComponent& chassis, const WeaponComponent& weapon, const DefenseComponent& defense, const uint32_t mask ) {
	// RE-SMAC-PREDEFINED(-ABILITY)-001 checked instances. The integers are
	// positional component identities, not unit/display-name classifications.
	struct Instance { uint32_t chassis, weapon, defense; int32_t speed, mode, offense, rating; ChassisTriad triad; uint8_t plan; uint32_t mask; bool transport_bits; };
	static const std::array< Instance, 14 > admitted = {{
		{ 0, 14, 0, 1, 8, 0, 1, ChassisTriad::Land, 8, 0, false },
		{ 0, 15, 0, 1, 9, 0, 1, ChassisTriad::Land, 9, 0, false },
		{ 0, 0, 0, 1, 0, 1, 1, ChassisTriad::Land, 0, 0, false },
		{ 3, 16, 0, 4, 7, 0, 1, ChassisTriad::Sea, 7, 0, true },
		{ 3, 15, 0, 4, 9, 0, 1, ChassisTriad::Sea, 9, 0, false },
		{ 0, 17, 0, 1, 10, 0, 1, ChassisTriad::Land, 10, 0, false },
		{ 1, 18, 0, 2, 11, 0, 1, ChassisTriad::Land, 11, 0, false },
		{ 0, 19, 0, 1, 12, 0, 1, ChassisTriad::Land, 12, 0, false },
		{ 0, 12, 9, 1, 2, -1, -1, ChassisTriad::Land, 0, 0, false },
		{ 3, 12, 9, 4, 2, -1, -1, ChassisTriad::Sea, 6, 0, false },
		{ 7, 12, 9, 8, 2, -1, -1, ChassisTriad::Air, 4, AIR_SUPERIORITY, false },
		{ 1, 0, 0, 2, 0, 1, 1, ChassisTriad::Land, 3, 0, false },
		{ 6, 0, 0, 8, 0, 1, 1, ChassisTriad::Air, 4, AIR_SUPERIORITY, false },
		{ 3, 0, 0, 4, 0, 1, 1, ChassisTriad::Sea, 6, SLOW | HEAVY, false }
	}};
	for ( const auto& instance : admitted ) {
		const bool bits = instance.transport_bits ? ( mask == 0 || ( mask & ( mask - 1 ) ) == 0 || mask == ( SLOW | HEAVY ) ) : mask == instance.mask;
		if ( c == instance.chassis && w == instance.weapon && d == instance.defense && bits && !chassis.missile
			&& chassis.speed == instance.speed && chassis.triad == instance.triad && static_cast< int32_t >( weapon.mode ) == instance.mode
			&& weapon.offense == instance.offense && defense.rating == instance.rating ) { return instance.plan; }
	}
	throw std::invalid_argument( "Unsupported SMAC predefined automatic-plan instance" );
}

}

int32_t SmacPredefinedUnitCatalog::CalculateCost( const ChassisComponent& chassis, const WeaponComponent& weapon,
	const DefenseComponent& defense, const uint32_t reactor, const uint32_t selected, const AbilityTable& abilities ) {
	Domain( chassis, reactor, selected );
	Require( chassis.cost > 0 && weapon.cost > 0 && defense.cost > 0, "SMAC unit component factors must be positive" );
	const auto mode = static_cast< int32_t >( weapon.mode );
	Require( ( mode >= 0 && mode <= 2 ) || ( mode >= 7 && mode <= 12 ), "Unsupported SMAC weapon mode" );
	const auto flags = AbilityFlags( abilities );
	const int64_t w0 = weapon.cost, a0 = defense.cost, c0 = chassis.cost, r = reactor;
	int64_t w = w0, a = a0, c = c0;
	if ( chassis.triad == ChassisTriad::Sea ) { a /= 2; c = Checked( c + r ); }
	if ( chassis.triad == ChassisTriad::Air ) { c = Checked( c + 2 * r ); if ( a > 1 ) { a = Checked( a * 2 * r ); } }
	if ( chassis.missile && weapon.offense >= 99 ) { return weapon.cost; }
	int64_t p = 0;
	for ( size_t i = 0; i < 24; ++i ) {
		if ( !( selected & ( 1u << i ) ) ) { continue; }
		if ( p ) { p = Checked( p + 1 ); }
		const auto cost = abilities.abilities[ i ].cost;
		int64_t contribution = cost;
		switch ( cost ) {
			case -1: contribution = std::min< int64_t >( w0 / a0, 2 ); break;
			case -2: contribution = w0 - 1; break;
			case -3: contribution = a0 - 1; break;
			case -4: contribution = c0 - 1; break;
			case -5: contribution = Checked( w0 + a0 ) - 2; break;
			case -6: contribution = Checked( w0 + c0 ) - 2; break;
			case -7: contribution = Checked( a0 + c0 ) - 2; break;
			default: break;
		}
		p = Checked( p + contribution );
	}
	if ( chassis.triad == ChassisTriad::Land ) {
		for ( size_t i = 0; i < 24; ++i ) { if ( ( selected & ( 1u << i ) ) && ( flags[ i ] & ( 1u << 10 ) ) ) { p = Checked( p + 1 ); } }
	}
	w = std::max( w, a / 2 + 1 );
	const int64_t denominator = int64_t( 1 ) << ( reactor + 1 );
	const auto sum = Checked( c + a );
	int64_t q = Checked( Checked( int64_t( sum ) * w ) + denominator / 2 ) / denominator;
	if ( c == 1 ) { q = q / 2 + 1; }
	if ( w0 > 1 && a0 > 1 ) { q = Checked( q + 1 + ( chassis.triad == ChassisTriad::Land && c > 1 ? 1 : 0 ) ); }
	if ( chassis.triad == ChassisTriad::Sea && weapon.mode != WeaponMode::InfoWar ) { q = Checked( q + 1 ) / 2; }
	if ( chassis.triad == ChassisTriad::Air ) { q /= mode <= 2 ? 4 : 2; }
	q = std::max( q, 2 * r - r / 2 );
	if ( w == 1 && a == 1 && c == 1 && r == 1 ) { q = 1; }
	const auto multiplier = Checked( p + 4 );
	return Checked( Checked( int64_t( multiplier ) * q ) + 2 ) / 4;
}

int32_t SmacPredefinedUnitCatalog::CalculateCapacity( const ChassisComponent& chassis, const uint32_t reactor, const uint32_t selected ) {
	Domain( chassis, reactor, selected );
	Require( chassis.cargo >= 0 && chassis.cargo <= 255, "SMAC chassis capacity must be 0..255" );
	int64_t value = chassis.cargo;
	if ( chassis.triad == ChassisTriad::Sea ) { value *= reactor; }
	if ( selected & SLOW ) { value /= 2; }
	if ( selected & HEAVY ) { value = ( 3 * value + 1 ) / 2; }
	return Checked( value );
}

SmacPredefinedUnitCatalog SmacPredefinedUnitCatalog::Build( const UnitTable& source, const ChassisTable& chassis,
	const WeaponTable& weapons, const DefenseTable& defenses, const ReactorTable& reactors, const AbilityTable& abilities,
	const settings::SmacTechnologyRules& technologies ) {
	Require( source.ruleset_id == "smac" && reactors.ruleset_id == "smac", "Predefined catalog requires explicit SMAC tables" );
	Require( source.units.size() <= 64 && chassis.chassis.size() == 9 && weapons.weapons.size() == 21
		&& defenses.defenses.size() == 10 && reactors.reactors.size() == 4, "Unsupported SMAC predefined catalog shape" );
	AbilityFlags( abilities ); technologies.Validate();
	const auto resolved = UnitDesignResolver::Resolve( source, chassis, weapons, defenses );
	SmacPredefinedUnitCatalog result{ source, chassis, weapons, defenses, reactors, abilities, {} };
	for ( const auto& entry : resolved.units ) {
		const auto& row = entry.record;
		Require( !row.name.empty() && row.name.find( '\0' ) == std::string::npos && !row.prerequisite.empty()
			&& row.prerequisite.find( '\0' ) == std::string::npos, "Invalid SMAC predefined unit text" );
		const auto c = Index( chassis.chassis, row.chassis, []( const ChassisComponent& value ) -> const std::string& { return value.names[ 0 ].name; } );
		const auto w = Index( weapons.weapons, row.weapon, []( const WeaponComponent& value ) -> const std::string& { return value.short_name; } );
		const auto d = Index( defenses.defenses, row.defense, []( const DefenseComponent& value ) -> const std::string& { return value.short_name; } );
		const auto mask = Binary( row.ability_bits, 24 );
		const auto calculated_cost = Byte( CalculateCost( entry.chassis, entry.weapon, entry.defense, 1, mask, abilities ) );
		const auto calculated_carry = entry.weapon.mode == WeaponMode::Transport ? Byte( CalculateCapacity( entry.chassis, 1, mask ) ) : uint8_t( 0 );
		const auto automatic = AutomaticPlan( c, w, d, entry.chassis, entry.weapon, entry.defense, mask );
		const auto plan = static_cast< int32_t >( row.plan );
		result.units.push_back( { static_cast< uint32_t >( entry.source_index ), c, w, d, 1, mask, automatic, calculated_cost, calculated_carry,
			plan >= 0 ? Byte( plan ) : automatic, row.cost ? Byte( row.cost ) : calculated_cost,
			row.carry ? Byte( row.carry ) : calculated_carry, Byte( row.icon ), technologies.ResolveToken( row.prerequisite ), true, false, 0 } );
	}
	return result;
}

}
