#include "UnitDesignResolver.h"

#include "loader/txt/UnitDesignResolver.h"
#include "task/gsetests/GSETests.h"

#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using namespace loader::txt;

struct Tables {
	UnitTable units;
	ChassisTable chassis;
	WeaponTable weapons;
	DefenseTable defenses;
};

Tables Decode( const std::string& ruleset = "smac" ) {
	const std::string air = ruleset == "smac" ? "'Copter" : "Copter";
	return {
		UnitRulesTXTLoader::ParseUnits( "#UNITS\n3\n"
			"Shared Name, Foot, Rifle, Field, 3, 5, 7, SurveyTech, -1, 00100\n"
			"Shared Name, Foot, Settler, Field, 8, 0, 0, None, -1, 000\n"
			"*Land Party, " + air + ", Shaper, Screen, 9, 0, 0, Disable, 2, 01", ruleset ),
		UnitRulesTXTLoader::ParseChassis( "#CHASSIS\n"
			"Unused, M1, Unuseds, M2, Guard, F1, Guards, F2, 2, 1, 0, 0, 4, 6, SeaTech, Large, N1, Larges, N2\n"
			"Foot, M1, Feet, M2, Guard, F1, Guards, F2, 3, 0, 4, 0, 5, 6, MoveTech, Large, N1, Larges, N2\n" +
			air + ", M1, Rotors, M2, Flyer, M1, Flyers, M2, 8, 2, 1, 1, 9, 10, AirTech, Big, M1, Bigs, M2", ruleset ),
		UnitRulesTXTLoader::ParseWeapons( "#WEAPONS\n"
			"Settlement Kit, Settler, 0, 8, 12, 6, None\n"
			"Unused Weapon, Unused, 4, 1, 8, 2, WeaponTech\n"
			"Survey Rifle, Rifle, -1, 0, 5, -1, SurveyTech\n"
			"Terrain Kit, Shaper, 0, 9, 7, 2, LandTech", ruleset ),
		UnitRulesTXTLoader::ParseDefenses( "#DEFENSES\n"
			"Unused Defense, Unused, 1, 0, 2, None\n"
			"Energy Screen, Screen, 7, 1, 11, ScreenTech\n"
			"Field Armor, Field, 3, 2, 5, ArmorTech", ruleset )
	};
}

ResolvedUnitTable Resolve( const Tables& tables ) {
	return UnitDesignResolver::Resolve( tables.units, tables.chassis, tables.weapons, tables.defenses );
}

std::string Rejection( const Tables& tables ) {
	try { Resolve( tables ); }
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

}

void AddUnitDesignResolverTests( task::gsetests::GSETests* task ) {
	task->AddTest( "Unit resolution owns complete metadata after source tables are destroyed", GT() {
		const auto resolved = []() {
			auto tables = Decode();
			auto result = Resolve( tables );
			// Changing caller-owned tables cannot change the returned metadata.
			tables.units.units[ 0 ].name = "Changed";
			tables.chassis.chassis[ 1 ].names[ 0 ].name = "Changed";
			tables.weapons.weapons[ 2 ].cost = 99;
			tables.defenses.defenses[ 2 ].short_name = "Changed";
			return result;
		}();
		GT_ASSERT( resolved.ruleset_id == "smac" && resolved.units.size() == 3, "resolved identity/count changed" );
		const auto& first = resolved.units[ 0 ];
		const auto& record = first.record;
		GT_ASSERT( record.name == "Shared Name" && record.chassis == "Foot" && record.weapon == "Rifle" && record.defense == "Field" && record.plan == UnitPlan::Reconnaissance && record.cost == 5 && record.carry == 7 && record.prerequisite == "SurveyTech" && record.icon == -1 && record.ability_bits == "00100", "owned unit record lost source metadata" );
		const auto& chassis = first.chassis;
		const std::vector< std::string > names = { "Foot", "Feet", "Guard", "Guards", "Large", "Larges" };
		const std::vector< std::string > grammar = { "M1", "M2", "F1", "F2", "N1", "N2" };
		for ( size_t i = 0 ; i < names.size() ; ++i ) {
			GT_ASSERT( chassis.names[ i ].name == names[ i ] && chassis.names[ i ].grammar == grammar[ i ], "owned chassis lost a name or grammar pair" );
		}
		GT_ASSERT( chassis.speed == 3 && chassis.triad == ChassisTriad::Land && chassis.range == 4 && !chassis.missile && chassis.cargo == 5 && chassis.cost == 6 && chassis.prerequisite == "MoveTech", "wrong chassis key/index or incomplete owned metadata" );
		const auto& weapon = first.weapon;
		GT_ASSERT( weapon.name == "Survey Rifle" && weapon.short_name == "Rifle" && weapon.offense == -1 && weapon.mode == WeaponMode::Projectile && weapon.cost == 5 && weapon.icon == -1 && weapon.prerequisite == "SurveyTech", "wrong weapon key/index or incomplete owned metadata" );
		const auto& defense = first.defense;
		GT_ASSERT( defense.name == "Field Armor" && defense.short_name == "Field" && defense.rating == 3 && defense.mode == DefenseMode::Binary && defense.cost == 5 && defense.prerequisite == "ArmorTech", "wrong defense key/index or incomplete owned metadata" );
		for ( size_t i = 0 ; i < resolved.units.size() ; ++i ) {
			GT_ASSERT( resolved.units[ i ].source_index == i, "source identity was replaced with display name or component index" );
		}
		GT_ASSERT( resolved.units[ 1 ].record.name == first.record.name && resolved.units[ 1 ].weapon.mode == WeaponMode::Colonist && resolved.units[ 1 ].record.cost == 0 && resolved.units[ 1 ].record.carry == 0, "duplicate unit names were merged or automatic values inferred" );
		GT_ASSERT( resolved.units[ 2 ].record.name == "*Land Party" && resolved.units[ 2 ].weapon.mode == WeaponMode::Terraformer && resolved.units[ 2 ].record.ability_bits == "01" && resolved.units[ 2 ].chassis.missile && resolved.units[ 2 ].defense.mode == DefenseMode::Energy, "resolver inferred eligibility or lost metadata" );
		auto copy = resolved;
		copy.units[ 0 ].chassis.names[ 0 ].name = "Independent";
		auto moved = std::move( copy );
		GT_ASSERT( moved.units[ 0 ].chassis.names[ 0 ].name == "Independent" && first.chassis.names[ 0 ].name == "Foot", "copy/move retained borrowed component state" );
		GT_OK();
	} );
	task->AddTest( "Unit resolution requires matching explicit ruleset identities and exact references", GT() {
		const auto smac = Resolve( Decode( "smac" ) ), smax = Resolve( Decode( "smax" ) );
		GT_ASSERT( smac.ruleset_id == "smac" && smac.units[ 2 ].chassis.names[ 0 ].name == "'Copter" && smax.ruleset_id == "smax" && smax.units[ 2 ].chassis.names[ 0 ].name == "Copter", "SMAC and SMAX chassis spellings were conflated" );
		for ( const auto* invalid : { "", "SMAC", "auto", "smac " } ) {
			auto tables = Decode();
			tables.units.ruleset_id = tables.chassis.ruleset_id = tables.weapons.ruleset_id = tables.defenses.ruleset_id = invalid;
			GT_ASSERT( !Rejection( tables ).empty(), "matching but unsupported ruleset identities were accepted" );
		}
		for ( size_t table = 0 ; table < 4 ; ++table ) {
			auto tables = Decode();
			std::string* identities[] = { &tables.units.ruleset_id, &tables.chassis.ruleset_id, &tables.weapons.ruleset_id, &tables.defenses.ruleset_id };
			*identities[ table ] = "smax";
			GT_ASSERT( !Rejection( tables ).empty(), "one mismatched catalog was accepted" );
			*identities[ table ] = "";
			GT_ASSERT( !Rejection( tables ).empty(), "one untagged catalog was accepted" );
		}
		for ( const auto* ruleset : { "smac", "smax" } ) {
			auto tables = Decode( ruleset );
			tables.units.units[ 2 ].chassis = std::string( ruleset ) == "smac" ? "Copter" : "'Copter";
			GT_ASSERT( Rejection( tables ).find( "#UNITS row 4: unresolved chassis" ) != std::string::npos, "resolver silently normalized ruleset-specific apostrophe" );
		}
		GT_OK();
	} );
	task->AddTest( "Unit resolution rejects missing exact keys without returning a partial table", GT() {
		for ( const size_t field : { 0, 1, 2 } ) {
			for ( const auto& key : { std::string(), std::string( "Unknown" ), std::string( "foot" ), std::string( "Feet" ), std::string( "Survey Rifle" ), std::string( "Field Armor" ), std::string( "Foot " ), std::string( "x\0y", 3 ) } ) {
				auto tables = Decode();
				auto& unit = tables.units.units[ 1 ];
				std::string* refs[] = { &unit.chassis, &unit.weapon, &unit.defense };
				*refs[ field ] = key;
				const auto error = Rejection( tables );
				const char* labels[] = { "chassis", "weapon", "defense" };
				GT_ASSERT( error.find( std::string( "#UNITS row 3: unresolved " ) + labels[ field ] ) != std::string::npos, "unknown, alternate, full, case-folded or normalized key accepted or mislocated" );
				GT_ASSERT( tables.units.units.size() == 3 && unit.name == "Shared Name" && *refs[ field ] == key, "rejected resolution changed source records" );
			}
			auto tables = Decode();
			if ( field == 0 ) { tables.chassis.chassis.clear(); }
			if ( field == 1 ) { tables.weapons.weapons.clear(); }
			if ( field == 2 ) { tables.defenses.defenses.clear(); }
			GT_ASSERT( !Rejection( tables ).empty(), "empty required catalog produced a usable resolution" );
		}
		auto tables = Decode();
		tables.units.units.clear();
		GT_ASSERT( Resolve( tables ).units.empty(), "empty unit table acquired inferred records" );
		GT_OK();
	} );
	task->AddTest( "Unit resolution rejects ambiguous and malformed keys throughout every catalog", GT() {
		for ( const size_t catalog : { 0, 1, 2 } ) {
			for ( const bool unused : { false, true } ) {
				auto tables = Decode();
				if ( catalog == 0 ) { tables.chassis.chassis.push_back( tables.chassis.chassis[ unused ? 0 : 1 ] ); }
				if ( catalog == 1 ) { tables.weapons.weapons.push_back( tables.weapons.weapons[ unused ? 1 : 2 ] ); }
				if ( catalog == 2 ) { tables.defenses.defenses.push_back( tables.defenses.defenses[ unused ? 0 : 2 ] ); }
				GT_ASSERT( Rejection( tables ).find( "ambiguous duplicate component key" ) != std::string::npos, "duplicate catalog key accepted, including an unused key" );
				tables.units.units.clear();
				GT_ASSERT( !Rejection( tables ).empty(), "zero units bypassed whole-catalog ambiguity validation" );
			}
			for ( const auto& key : { std::string(), std::string( "x\0y", 3 ) } ) {
				auto tables = Decode();
				if ( catalog == 0 ) { tables.chassis.chassis[ 0 ].names[ 0 ].name = key; }
				if ( catalog == 1 ) { tables.weapons.weapons[ 1 ].short_name = key; }
				if ( catalog == 2 ) { tables.defenses.defenses[ 0 ].short_name = key; }
				GT_ASSERT( Rejection( tables ).find( "missing or invalid component key" ) != std::string::npos, "empty/NUL unused key accepted" );
			}
		}
		// Repeated display/alternate names are not ambiguous lookup keys.
		auto tables = Decode();
		tables.weapons.weapons[ 0 ].name = tables.weapons.weapons[ 2 ].name;
		tables.defenses.defenses[ 0 ].name = tables.defenses.defenses[ 2 ].name;
		GT_ASSERT( Resolve( tables ).units.size() == 3, "display names were mistaken for unique component keys" );
		GT_OK();
	} );
}

}
