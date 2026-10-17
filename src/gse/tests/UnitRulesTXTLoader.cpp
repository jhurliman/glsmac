#include "UnitRulesTXTLoader.h"

#include "loader/txt/UnitRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {

using loader::txt::UnitRulesTXTLoader;
using loader::txt::WeaponMode;
using loader::txt::DefenseMode;
using loader::txt::ChassisTriad;
using loader::txt::UnitPlan;

// Synthetic metadata: role selection must come from the mode, not names,
// installed row indices, default costs, or prerequisite aliases.
std::vector< std::string > Rows() {
	return {
		"Survey Rifle, Survey, 3, 0, 5, -1, CustomTech",
		"Settlement Kit, Settler, 0, 8, 12, 6, None, ; an annotation",
		"Terrain Kit, Shaper, 0, 9, 7, 2, LandTech"
	};
}

std::string Source( const std::vector< std::string >& rows, const std::string& newline = "\n", const std::string& section = "WEAPONS" ) {
	const std::string other_section = section == "UNITS" ? "WEAPONS" : "UNITS";
	std::string source = "#" + other_section + newline + "ignored" + newline
		+ "## translator comment" + newline + " \t#" + section + " \t" + newline;
	for ( const auto& row : rows ) { source += " ; comment" + newline + "\t" + newline + " " + row + "\t" + newline; }
	return source + "#END" + newline + "#" + other_section + newline + "also ignored" + newline;
}

std::string Rejection( const std::string& source, const std::string& ruleset = "smac" ) {
	try { UnitRulesTXTLoader::ParseWeapons( source, ruleset ); }
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

std::string RowWithField( const size_t column, const std::string& value ) {
	std::vector< std::string > fields = { "Example", "ExampleShort", "1", "0", "1", "-1", "None" };
	fields[ column ] = value;
	std::string row;
	for ( size_t i = 0 ; i < fields.size() ; ++i ) { if ( i != 0 ) { row += ','; } row += fields[ i ]; }
	return row;
}

std::string ComponentRejection( const std::string& source, const bool reactor, const std::string& ruleset = "smac" ) {
	try {
		if ( reactor ) { UnitRulesTXTLoader::ParseReactors( source, ruleset ); }
		else { UnitRulesTXTLoader::ParseDefenses( source, ruleset ); }
	}
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

std::string ComponentRow( const bool reactor, const size_t column, const std::string& value ) {
	auto fields = reactor ? std::vector< std::string >{ "Test Core", "Core", "3", "CoreTech" }
		: std::vector< std::string >{ "Test Armor", "Armor", "3", "0", "5", "ArmorTech" };
	fields[ column ] = value;
	std::string row;
	for ( size_t i = 0 ; i < fields.size() ; ++i ) { if ( i != 0 ) { row += ','; } row += fields[ i ]; }
	return row;
}

std::vector< std::string > ChassisFields() {
	return { "Traveler", "M1", "Travelers", "M2", "Guardian", "F1", "Guardians", "F2",
		"3", "1", "4", "1", "5", "6", "TravelTech", "Grand One", "N1", "Grand Many", "N2" };
}

std::string JoinRow( const std::vector< std::string >& fields ) {
	std::string row;
	for ( size_t i = 0 ; i < fields.size() ; ++i ) { if ( i != 0 ) { row += ','; } row += fields[ i ]; }
	return row;
}

std::string ChassisRejection( const std::string& source, const std::string& ruleset = "smac" ) {
	try { UnitRulesTXTLoader::ParseChassis( source, ruleset ); }
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

std::vector< std::string > UnitFields() {
	return { "Survey Party", "Traveler", "Survey", "Field", "3", "5", "7", "SurveyTech", "-1", "00100" };
}

std::string UnitsSource( const std::vector< std::string >& rows, const std::string& newline = "\n" ) {
	auto counted = rows;
	counted.insert( counted.begin(), std::to_string( rows.size() ) + " ; record count" );
	return Source( counted, newline, "UNITS" );
}

std::string UnitsRejection( const std::string& source, const std::string& ruleset = "smac" ) {
	try { UnitRulesTXTLoader::ParseUnits( source, ruleset ); }
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

}

void AddUnitRulesTXTLoaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "Unit weapon table preserves component metadata and source order", GT() {
		for ( const auto* newline : { "\n", "\r\n" } ) {
			const auto table = UnitRulesTXTLoader::ParseWeapons( Source( Rows(), newline ), "smac" );
			GT_ASSERT( table.ruleset_id == "smac" && table.weapons.size() == 3, "table identity or row count changed" );
			const auto& rifle = table.weapons[ 0 ];
			GT_ASSERT( rifle.name == "Survey Rifle" && rifle.short_name == "Survey" && rifle.offense == 3 && rifle.mode == WeaponMode::Projectile && rifle.cost == 5 && rifle.icon == -1 && rifle.prerequisite == "CustomTech", "weapon metadata changed or was replaced with defaults" );
			const auto& colony = table.weapons[ 1 ];
			GT_ASSERT( colony.name == "Settlement Kit" && colony.short_name == "Settler" && colony.offense == 0 && colony.mode == WeaponMode::Colonist && colony.cost == 12 && colony.icon == 6 && colony.prerequisite == "None", "colonist package did not retain its independent role and metadata" );
			const auto& former = table.weapons[ 2 ];
			GT_ASSERT( former.name == "Terrain Kit" && former.short_name == "Shaper" && former.offense == 0 && former.mode == WeaponMode::Terraformer && former.cost == 7 && former.icon == 2 && former.prerequisite == "LandTech", "terraformer package did not retain its independent role and metadata" );
		}
		// No final newline or section terminator is required.
		GT_ASSERT( UnitRulesTXTLoader::ParseWeapons( "#WEAPONS\n" + Rows()[ 0 ], "smac" ).weapons.size() == 1, "EOF did not terminate the table" );
		GT_OK();
	} );
	task->AddTest( "Unit weapon tables keep explicit ruleset identity and local indices", GT() {
		const auto smac = UnitRulesTXTLoader::ParseWeapons( Source( Rows() ), "smac" );
		auto rows = Rows();
		rows.insert( rows.begin() + 1, "Inserted Beam, Extra, 4, 1, 8, 3, BeamTech" );
		rows.push_back( "Terrain Payload, TerrainPayload, 0, 13, 9, -1, Disable" );
		rows.push_back( "Fungal Payload, FungalPayload, 0, 14, 11, -1, PayloadTech" );
		const auto smax = UnitRulesTXTLoader::ParseWeapons( Source( rows ), "smax" );
		GT_ASSERT( smax.ruleset_id == "smax" && smax.weapons.size() == 6 && smac.weapons.size() == 3, "rulesets shared a catalog or lost their identity" );
		GT_ASSERT( smac.weapons[ 1 ].mode == WeaponMode::Colonist && smax.weapons[ 2 ].mode == WeaponMode::Colonist && smac.weapons[ 2 ].mode == WeaponMode::Terraformer && smax.weapons[ 3 ].mode == WeaponMode::Terraformer, "package indices were assumed to be global" );
		GT_ASSERT( smax.weapons[ 4 ].mode == WeaponMode::Tectonic && smax.weapons[ 5 ].mode == WeaponMode::Fungal, "SMAX payload modes were lost" );
		for ( const auto* ruleset : { "", "SMAC", "smax ", "auto", "planetary" } ) {
			GT_ASSERT( !Rejection( Source( Rows() ), ruleset ).empty(), "implicit or unknown ruleset was accepted" );
		}
		for ( const auto mode : { 13, 14 } ) {
			GT_ASSERT( !Rejection( Source( { RowWithField( 3, std::to_string( mode ) ) } ), "smac" ).empty(), "SMAX-only mode was accepted as SMAC" );
		}
		GT_OK();
	} );
	task->AddTest( "Unit weapon table preserves every known mode and signed metadata", GT() {
		for ( const auto mode : { 0, 1, 2, 7, 8, 9, 10, 11, 12, 13, 14 } ) {
			const auto table = UnitRulesTXTLoader::ParseWeapons( Source( { RowWithField( 3, std::to_string( mode ) ) } ), "smax" );
			GT_ASSERT( static_cast< int32_t >( table.weapons[ 0 ].mode ) == mode, "known mode changed" );
		}
		const auto table = UnitRulesTXTLoader::ParseWeapons( Source( {
			"Low, low, -2147483648, +0002, +2147483647, -0, Disable",
			"High, high, +2147483647, 0, -2147483648, +2147483647, None",
			"Signed, signed, -1, 1, +00017, -2147483648, Unresolved"
		} ), "smac" );
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		GT_ASSERT( table.weapons[ 0 ].offense == minimum && table.weapons[ 0 ].cost == maximum && table.weapons[ 0 ].icon == 0 && table.weapons[ 0 ].mode == WeaponMode::Missile, "low signed endpoints or explicit plus changed" );
		GT_ASSERT( table.weapons[ 1 ].offense == maximum && table.weapons[ 1 ].cost == minimum && table.weapons[ 1 ].icon == maximum, "high signed endpoints changed" );
		GT_ASSERT( table.weapons[ 2 ].offense == -1 && table.weapons[ 2 ].cost == 17 && table.weapons[ 2 ].icon == minimum && table.weapons[ 2 ].prerequisite == "Unresolved", "signed metadata was interpreted, clamped, or rejected" );
		GT_OK();
	} );
	task->AddTest( "Unit weapon table rejects missing duplicate and empty sections", GT() {
		for ( const auto& source : {
			std::string(), std::string( "#WEAPONS_OTHER\n" ), std::string( "##WEAPONS\n" ),
			std::string( "#WEAPONS\n ; comment\n#END\n" ),
			Source( Rows() ) + "#WEAPONS\n", std::string( "#WEAPONS\n#END\n" ) + Source( Rows() )
		} ) {
			GT_ASSERT( !Rejection( source ).empty(), "missing, duplicated, or empty WEAPONS was accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "Unit weapon table rejects malformed columns and duplicate short names", GT() {
		for ( const auto* row : {
			"Example", "Example, short", "Example, short, 1", "Example, short, 1, 0", "Example, short, 1, 0, 1",
			"Example, short, 1, 0, 1, -1", "Example, short, 1, 0, 1, -1,",
			", short, 1, 0, 1, -1, None", "Example, , 1, 0, 1, -1, None",
			"Example, short, 1, 0, 1, -1, None, extra", "Example, short, 1, 0, 1, -1, None,,"
		} ) {
			GT_ASSERT( !Rejection( Source( { row } ) ).empty(), "malformed column layout was accepted: " + std::string( row ) );
		}
		for ( const size_t column : { 0, 1, 6 } ) {
			GT_ASSERT( !Rejection( Source( { RowWithField( column, std::string( "x\0y", 3 ) ) } ) ).empty(), "NUL text was accepted" );
		}
		auto rows = Rows();
		rows.push_back( "Another name, Survey, 9, 1, 10, 2, OtherTech" );
		const auto error = Rejection( Source( rows ) );
		GT_ASSERT( error.find( "row 4 column 2" ) != std::string::npos, "duplicate short-name error did not identify the later record" );
		// Exact identifiers are preserved; no original case-folding or alias
		// resolution behavior is claimed by this structural decoder.
		rows.back() = "Survey Rifle, survey, 9, 1, 10, 2, OtherTech";
		GT_ASSERT( UnitRulesTXTLoader::ParseWeapons( Source( rows ), "smac" ).weapons.size() == 4, "full names or case-distinct short names were treated as duplicate identifiers" );
		GT_OK();
	} );
	task->AddTest( "Unit weapon table rejects malformed numeric fields and unknown modes", GT() {
		const std::vector< std::string > invalid = {
			"", " ", "*", "1junk", "1.0", "0x10", "1 2", "+", "-", "+-1", "++1", "--1",
			"2147483648", "+2147483648", "-2147483649", "999999999999999999999999999999", std::string( "1\0x", 3 )
		};
		for ( size_t column = 2 ; column <= 5 ; ++column ) {
			for ( const auto& field : invalid ) {
				const auto error = Rejection( Source( { Rows()[ 0 ], RowWithField( column, field ) } ) );
				GT_ASSERT( !error.empty(), "malformed numeric field was accepted: " + field );
				GT_ASSERT( error.find( "row 2 column " + std::to_string( column + 1 ) ) != std::string::npos, "numeric error did not identify its row and column" );
			}
		}
		for ( const auto mode : { -2147483647 - 1, -1, 3, 4, 5, 6, 15, 2147483647 } ) {
			for ( const auto* ruleset : { "smac", "smax" } ) {
				GT_ASSERT( !Rejection( Source( { RowWithField( 3, std::to_string( mode ) ) } ), ruleset ).empty(), "undefined mode was accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "Unit defense and reactor tables preserve metadata and ruleset-local order", GT() {
		for ( const auto* newline : { "\n", "\r\n" } ) {
			for ( const auto* ruleset : { "smac", "smax" } ) {
				const auto defenses = UnitRulesTXTLoader::ParseDefenses( Source( {
					"Field Armor, Field, 3, 0, 5, ArmorTech, ; annotation",
					"Energy Screen, Screen, 7, 1, 11, None",
					"Psychic Defense, Psi, -1, 2, 13, PsiTech"
				}, newline, "DEFENSES" ), ruleset );
				GT_ASSERT( defenses.ruleset_id == ruleset && defenses.defenses.size() == 3, "defense table identity or order changed" );
				const auto& field = defenses.defenses[ 0 ];
				GT_ASSERT( field.name == "Field Armor" && field.short_name == "Field" && field.rating == 3 && field.mode == DefenseMode::Projectile && field.cost == 5 && field.prerequisite == "ArmorTech", "defense columns were discarded or misinterpreted" );
				GT_ASSERT( defenses.defenses[ 1 ].mode == DefenseMode::Energy && defenses.defenses[ 2 ].mode == DefenseMode::Binary && defenses.defenses[ 2 ].rating == -1, "defense modes or psi rating changed" );
				const auto reactors = UnitRulesTXTLoader::ParseReactors( Source( {
					"Research Core, Core, 3, CoreTech, ; annotation",
					"Starter Core, Start, 1, None"
				}, newline, "REACTORS" ), ruleset );
				GT_ASSERT( reactors.ruleset_id == ruleset && reactors.reactors.size() == 2, "reactor table identity or count changed" );
				const auto& core = reactors.reactors[ 0 ];
				GT_ASSERT( core.name == "Research Core" && core.short_name == "Core" && core.power == 3 && core.prerequisite == "CoreTech" && reactors.reactors[ 1 ].power == 1, "reactor short name was omitted or catalog sorted by power" );
			}
		}
		const auto smac = UnitRulesTXTLoader::ParseDefenses( "#DEFENSES\nField Armor, Field, 3, 0, 5, None", "smac" );
		const auto smax = UnitRulesTXTLoader::ParseDefenses( "#DEFENSES\nExtra Armor, Extra, 2, 1, 4, ExtraTech\nField Armor, Field, 6, 2, 8, OtherTech", "smax" );
		GT_ASSERT( smac.defenses[ 0 ].short_name == smax.defenses[ 1 ].short_name && smac.defenses[ 0 ].rating == 3 && smax.defenses[ 1 ].rating == 6, "identically named components in distinct catalogs shared state or indices" );
		GT_ASSERT( UnitRulesTXTLoader::ParseReactors( "#REACTORS\nLast Core, Last, 4, None", "smac" ).reactors.size() == 1, "reactor table required a final newline or terminator" );
		GT_OK();
	} );
	task->AddTest( "Unit defense and reactor metadata preserves signed int32 endpoints", GT() {
		const auto defenses = UnitRulesTXTLoader::ParseDefenses( Source( {
			"Low, low, -2147483648, +0002, +2147483647, None",
			"High, high, +2147483647, -0, -2147483648, Disable",
			"Zero, zero, -0, 1, +00017, Unresolved"
		}, "\n", "DEFENSES" ), "smac" );
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		GT_ASSERT( defenses.defenses[ 0 ].rating == minimum && defenses.defenses[ 0 ].mode == DefenseMode::Binary && defenses.defenses[ 0 ].cost == maximum, "defense low endpoint or mode sign changed" );
		GT_ASSERT( defenses.defenses[ 1 ].rating == maximum && defenses.defenses[ 1 ].cost == minimum && defenses.defenses[ 2 ].rating == 0 && defenses.defenses[ 2 ].cost == 17, "defense signed metadata changed" );
		const auto reactors = UnitRulesTXTLoader::ParseReactors( Source( {
			"Low, low, -2147483648, None", "High, high, +2147483647, Disable",
			"Zero, zero, -0, Unresolved", "Positive, positive, +00017, AnyTech"
		}, "\n", "REACTORS" ), "smax" );
		GT_ASSERT( reactors.reactors[ 0 ].power == minimum && reactors.reactors[ 1 ].power == maximum && reactors.reactors[ 2 ].power == 0 && reactors.reactors[ 3 ].power == 17, "raw reactor power was narrowed, clamped or treated as design eligibility" );
		GT_ASSERT( reactors.reactors[ 2 ].prerequisite == "Unresolved", "reactor prerequisite was resolved or defaulted" );
		GT_OK();
	} );
	task->AddTest( "Unit defense and reactor tables require explicit identity and unique complete sections", GT() {
		for ( const bool reactor : { false, true } ) {
			const std::string section = reactor ? "REACTORS" : "DEFENSES";
			const auto row = ComponentRow( reactor, 0, "Example" );
			const auto source = Source( { row }, "\n", section );
			for ( const auto* ruleset : { "", "SMAC", "smax ", "auto" } ) {
				GT_ASSERT( !ComponentRejection( source, reactor, ruleset ).empty(), "component table accepted an implicit ruleset" );
			}
			for ( const auto& invalid : { std::string(), "#" + section + "_OTHER\n", "##" + section + "\n", "#" + section + "\n ; empty\n#END\n", source + "#" + section + "\n" } ) {
				GT_ASSERT( !ComponentRejection( invalid, reactor ).empty(), "missing, duplicate or empty component section was accepted" );
			}
			for ( size_t delimiter = row.find( ',' ) ; delimiter != std::string::npos ; delimiter = row.find( ',', delimiter + 1 ) ) {
				for ( const auto& truncated : { row.substr( 0, delimiter ), row.substr( 0, delimiter + 1 ) } ) {
					GT_ASSERT( !ComponentRejection( Source( { truncated }, "\n", section ), reactor ).empty(), "truncated component row was accepted" );
				}
			}
			for ( const auto* extra : { ",extra", ",," } ) {
				GT_ASSERT( !ComponentRejection( Source( { row + extra }, "\n", section ), reactor ).empty(), "extra component columns were accepted" );
			}
			for ( const size_t column : { size_t( 0 ), size_t( 1 ), reactor ? size_t( 3 ) : size_t( 5 ) } ) {
				for ( const auto& text : { std::string(), std::string( "x\0y", 3 ) } ) {
					GT_ASSERT( !ComponentRejection( Source( { ComponentRow( reactor, column, text ) }, "\n", section ), reactor ).empty(), "empty or NUL component identity was accepted" );
				}
			}
			const auto error = ComponentRejection( Source( { row, ComponentRow( reactor, 0, "Different full name" ) }, "\n", section ), reactor );
			GT_ASSERT( error.find( "#" + section + " row 2 column 2" ) != std::string::npos, "duplicate short ID did not identify the later component" );
		}
		GT_OK();
	} );
	task->AddTest( "Unit defense and reactor tables reject partial numeric fields and unknown defense modes", GT() {
		const std::vector< std::string > invalid = { "", "*", "1junk", "1.0", "0x10", "1 2", "+", "-", "+-1", "++1", "--1", "2147483648", "-2147483649", std::string( "1\0x", 3 ) };
		for ( const bool reactor : { false, true } ) {
			const std::string section = reactor ? "REACTORS" : "DEFENSES";
			for ( size_t column = 2 ; column <= ( reactor ? 2 : 4 ) ; ++column ) {
				for ( const auto& field : invalid ) {
					const auto source = Source( { ComponentRow( reactor, 1, "First" ), ComponentRow( reactor, column, field ) }, "\n", section );
					const auto error = ComponentRejection( source, reactor );
					GT_ASSERT( !error.empty(), "component accepted malformed numeric field: " + field );
					GT_ASSERT( error.find( "#" + section + " row 2 column " + std::to_string( column + 1 ) ) != std::string::npos, "component numeric rejection lost section/row/column context" );
				}
			}
		}
		for ( const auto mode : { -2147483647 - 1, -1, 3, 7, 8, 9, 2147483647 } ) {
			for ( const auto* ruleset : { "smac", "smax" } ) {
				GT_ASSERT( !ComponentRejection( Source( { ComponentRow( false, 3, std::to_string( mode ) ) }, "\n", "DEFENSES" ), false, ruleset ).empty(), "weapon/package or unknown mode was accepted as a defense mode" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "Unit chassis table preserves all nineteen columns and local identity", GT() {
		const auto fields = ChassisFields();
		for ( const auto* newline : { "\n", "\r\n" } ) {
			for ( const auto* ruleset : { "smac", "smax" } ) {
				const auto table = UnitRulesTXTLoader::ParseChassis( Source( { JoinRow( fields ) + ", ; annotation" }, newline, "CHASSIS" ), ruleset );
				GT_ASSERT( table.ruleset_id == ruleset && table.chassis.size() == 1, "chassis identity/count changed" );
				const auto& chassis = table.chassis[ 0 ];
				const std::vector< std::string > names = { "Traveler", "Travelers", "Guardian", "Guardians", "Grand One", "Grand Many" };
				const std::vector< std::string > grammar = { "M1", "M2", "F1", "F2", "N1", "N2" };
				for ( size_t pair = 0 ; pair < names.size() ; ++pair ) {
					GT_ASSERT( chassis.names[ pair ].name == names[ pair ] && chassis.names[ pair ].grammar == grammar[ pair ], "a chassis name/grammar pair was dropped or reordered" );
				}
				GT_ASSERT( chassis.speed == 3 && chassis.triad == ChassisTriad::Sea && chassis.range == 4 && chassis.missile && chassis.cargo == 5 && chassis.cost == 6 && chassis.prerequisite == "TravelTech", "chassis gameplay columns were shifted or inferred from other metadata" );
			}
		}
		auto a = fields, b = fields;
		a[ 0 ] = "'Rotor"; b[ 0 ] = "Rotor";
		const auto smac = UnitRulesTXTLoader::ParseChassis( "#CHASSIS\n" + JoinRow( a ), "smac" );
		const auto smax = UnitRulesTXTLoader::ParseChassis( "#CHASSIS\n" + JoinRow( fields ) + "\n" + JoinRow( b ), "smax" );
		GT_ASSERT( smac.chassis[ 0 ].names[ 0 ].name == "'Rotor" && smax.chassis[ 1 ].names[ 0 ].name == "Rotor", "catalog indices or original apostrophe spellings were normalized" );
		// Alternative names may repeat, including all six names of one chassis.
		for ( const size_t column : { 0, 2, 4, 6, 15, 17 } ) { a[ column ] = "Repeated"; }
		GT_ASSERT( UnitRulesTXTLoader::ParseChassis( Source( { JoinRow( a ), JoinRow( fields ) }, "\n", "CHASSIS" ), "smac" ).chassis.size() == 2, "alternate names were incorrectly used as unique catalog keys" );
		GT_OK();
	} );
	task->AddTest( "Unit chassis table preserves signed metadata and validates triad and missile domains", GT() {
		auto fields = ChassisFields();
		fields[ 8 ] = "-2147483648"; fields[ 10 ] = "+2147483647";
		fields[ 12 ] = "-0"; fields[ 13 ] = "+00017";
		for ( const auto triad : { 0, 1, 2 } ) {
			for ( const auto missile : { 0, 1 } ) {
				fields[ 9 ] = "+" + std::to_string( triad ); fields[ 11 ] = std::to_string( missile );
				const auto table = UnitRulesTXTLoader::ParseChassis( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ), "smax" );
				const auto& chassis = table.chassis[ 0 ];
				GT_ASSERT( chassis.speed == std::numeric_limits< int32_t >::min() && chassis.range == std::numeric_limits< int32_t >::max() && chassis.cargo == 0 && chassis.cost == 17 && static_cast< int32_t >( chassis.triad ) == triad && chassis.missile == ( missile == 1 ), "signed chassis metadata or explicit enum/flag changed" );
			}
		}
		fields = ChassisFields();
		fields[ 8 ] = "+2147483647"; fields[ 10 ] = "-2147483648";
		fields[ 12 ] = "-2147483648"; fields[ 13 ] = "2147483647";
		const auto table = UnitRulesTXTLoader::ParseChassis( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ), "smac" );
		const auto& chassis = table.chassis[ 0 ];
		GT_ASSERT( chassis.speed == std::numeric_limits< int32_t >::max() && chassis.range == std::numeric_limits< int32_t >::min() && chassis.cargo == std::numeric_limits< int32_t >::min() && chassis.cost == std::numeric_limits< int32_t >::max(), "chassis endpoints were narrowed or treated as design eligibility" );
		for ( const size_t column : { 9, 11 } ) {
			for ( const auto value : { -2147483647 - 1, -1, 3, 7, 2147483647 } ) {
				fields = ChassisFields(); fields[ column ] = std::to_string( value );
				GT_ASSERT( !ChassisRejection( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ) ).empty(), "invalid triad/missile was narrowed into a valid enum/flag" );
			}
		}
		fields = ChassisFields(); fields[ 11 ] = "2";
		GT_ASSERT( !ChassisRejection( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ) ).empty(), "missile accepted nonbinary value" );
		GT_OK();
	} );
	task->AddTest( "Unit chassis table rejects missing sections fields and duplicate primary names", GT() {
		const auto fields = ChassisFields();
		const auto source = Source( { JoinRow( fields ) }, "\n", "CHASSIS" );
		for ( const auto* ruleset : { "", "SMAC", "smax ", "auto" } ) {
			GT_ASSERT( !ChassisRejection( source, ruleset ).empty(), "chassis accepted implicit ruleset identity" );
		}
		for ( const auto& invalid : { std::string(), std::string( "#CHASSIS_OTHER\n" ), std::string( "##CHASSIS\n" ), std::string( "#CHASSIS\n;empty\n#END\n" ), source + "#CHASSIS\n" } ) {
			GT_ASSERT( !ChassisRejection( invalid ).empty(), "missing, duplicate or empty chassis section accepted" );
		}
		for ( size_t column = 0 ; column < fields.size() ; ++column ) {
			auto missing = fields; missing.erase( missing.begin() + column );
			GT_ASSERT( !ChassisRejection( Source( { JoinRow( missing ) }, "\n", "CHASSIS" ) ).empty(), "missing positional chassis field accepted" );
		}
		for ( const auto* extra : { ",extra", ",," } ) {
			GT_ASSERT( !ChassisRejection( Source( { JoinRow( fields ) + extra }, "\n", "CHASSIS" ) ).empty(), "additional chassis columns accepted" );
		}
		auto duplicate = fields; duplicate[ 2 ] = "Different alternate";
		const auto error = ChassisRejection( Source( { JoinRow( fields ), JoinRow( duplicate ) }, "\n", "CHASSIS" ) );
		GT_ASSERT( error.find( "#CHASSIS row 2 column 1" ) != std::string::npos, "duplicate primary name did not identify the later row" );
		GT_OK();
	} );
	task->AddTest( "Unit chassis table validates every name grammar and numeric field", GT() {
		for ( const size_t column : { 0, 2, 4, 6, 14, 15, 17 } ) {
			for ( const auto& value : { std::string(), std::string( "x\0y", 3 ) } ) {
				auto fields = ChassisFields(); fields[ column ] = value;
				const auto error = ChassisRejection( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ) );
				GT_ASSERT( error.find( "#CHASSIS row 1 column " + std::to_string( column + 1 ) ) != std::string::npos, "invalid chassis name/prerequisite accepted or mislocated" );
			}
		}
		for ( const size_t column : { 1, 3, 5, 7, 16, 18 } ) {
			for ( const auto& value : { std::string(), std::string( "M" ), std::string( "M0" ), std::string( "M3" ), std::string( "X1" ), std::string( "f1" ), std::string( "N12" ), std::string( "M\0", 2 ) } ) {
				auto fields = ChassisFields(); fields[ column ] = value;
				const auto error = ChassisRejection( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ) );
				GT_ASSERT( error.find( "#CHASSIS row 1 column " + std::to_string( column + 1 ) ) != std::string::npos, "unsupported grammar accepted or mislocated" );
			}
		}
		const std::vector< std::string > invalid = { "", "*", "1junk", "1.0", "0x10", "1 2", "+", "-", "+-1", "++1", "--1", "2147483648", "-2147483649", std::string( "1\0x", 3 ) };
		for ( size_t column = 8 ; column <= 13 ; ++column ) {
			for ( const auto& value : invalid ) {
				auto fields = ChassisFields(); fields[ column ] = value;
				const auto error = ChassisRejection( Source( { JoinRow( fields ) }, "\n", "CHASSIS" ) );
				GT_ASSERT( error.find( "#CHASSIS row 1 column " + std::to_string( column + 1 ) ) != std::string::npos, "invalid chassis number accepted or mislocated" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "Unit design table preserves all ten fields and unresolved references", GT() {
		for ( const auto* newline : { "\n", "\r\n" } ) {
			for ( const auto* ruleset : { "smac", "smax" } ) {
				const auto table = UnitRulesTXTLoader::ParseUnits( UnitsSource( {
					JoinRow( UnitFields() ) + ", ; annotation",
					"Settlement Party, Traveler, Settler, Field, 8, 0, 0, None, -1, 000",
					"*Land Party, 'Traveler, Shaper, field, 9, 0, 0, Disable, 2, 0"
				}, newline ), ruleset );
				GT_ASSERT( table.ruleset_id == ruleset && table.units.size() == 3, "unit table identity/count changed" );
				const auto& survey = table.units[ 0 ];
				GT_ASSERT( survey.name == "Survey Party" && survey.chassis == "Traveler" && survey.weapon == "Survey" && survey.defense == "Field" && survey.plan == UnitPlan::Reconnaissance && survey.cost == 5 && survey.carry == 7 && survey.prerequisite == "SurveyTech" && survey.icon == -1 && survey.ability_bits == "00100", "unit fields shifted, defaulted, or resolved" );
				const auto& colony = table.units[ 1 ];
				GT_ASSERT( colony.weapon == "Settler" && colony.plan == UnitPlan::Colonization && colony.cost == 0 && colony.carry == 0 && colony.prerequisite == "None" && colony.ability_bits == "000", "colony metadata or automatic-value sentinels changed" );
				const auto& former = table.units[ 2 ];
				GT_ASSERT( former.name == "*Land Party" && former.chassis == "'Traveler" && former.weapon == "Shaper" && former.defense == "field" && former.plan == UnitPlan::Terraforming && former.prerequisite == "Disable" && former.icon == 2 && former.ability_bits == "0", "exact names/references or raw ability width were normalized" );
			}
		}
		auto different = UnitFields(); different[ 1 ] = "Another Chassis";
		const auto smac = UnitRulesTXTLoader::ParseUnits( "#UNITS\n1\n" + JoinRow( UnitFields() ), "smac" );
		const auto smax = UnitRulesTXTLoader::ParseUnits( UnitsSource( { JoinRow( different ), JoinRow( UnitFields() ) } ), "smax" );
		GT_ASSERT( smac.units[ 0 ].chassis == "Traveler" && smax.units[ 0 ].chassis == "Another Chassis" && smax.units[ 1 ].name == smax.units[ 0 ].name, "duplicate display names or separate ruleset-local rows were conflated" );
		GT_OK();
	} );
	task->AddTest( "Unit design table preserves signed metadata plans and raw ability strings", GT() {
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		for ( int32_t plan = -1 ; plan <= 12 ; ++plan ) {
			auto fields = UnitFields();
			fields[ 4 ] = std::to_string( plan ); fields[ 5 ] = "-2147483648";
			fields[ 6 ] = "+2147483647"; fields[ 8 ] = "-2147483648";
			const auto table = UnitRulesTXTLoader::ParseUnits( UnitsSource( { JoinRow( fields ) } ), "smac" );
			const auto& unit = table.units[ 0 ];
			GT_ASSERT( static_cast< int32_t >( unit.plan ) == plan && unit.cost == minimum && unit.carry == maximum && unit.icon == minimum, "plan or signed unit metadata changed" );
		}
		for ( const size_t width : { 1, 24, 26, 29, 65 } ) {
			auto fields = UnitFields();
			fields[ 4 ] = "+0003"; fields[ 5 ] = "+2147483647";
			fields[ 6 ] = "-2147483648"; fields[ 8 ] = "+2147483647";
			fields[ 9 ] = std::string( width - 1, '0' ) + '1';
			const auto table = UnitRulesTXTLoader::ParseUnits( UnitsSource( { JoinRow( fields ) } ), "smax" );
			const auto& unit = table.units[ 0 ];
			GT_ASSERT( unit.plan == UnitPlan::Reconnaissance && unit.cost == maximum && unit.carry == minimum && unit.icon == maximum && unit.ability_bits == fields[ 9 ], "unit endpoints or ability spelling/width was narrowed or decoded" );
		}
		auto fields = UnitFields(); fields[ 5 ] = "-0"; fields[ 6 ] = "+00017"; fields[ 8 ] = "-1";
		const auto table = UnitRulesTXTLoader::ParseUnits( UnitsSource( { JoinRow( fields ) } ), "smac" );
		GT_ASSERT( table.units[ 0 ].cost == 0 && table.units[ 0 ].carry == 17 && table.units[ 0 ].icon == -1, "signed unit metadata was treated as eligibility or calculated" );
		GT_OK();
	} );
	task->AddTest( "Unit design table requires an exact nonnegative count and explicit unique section", GT() {
		const auto row = JoinRow( UnitFields() );
		for ( const auto* ruleset : { "smac", "smax" } ) {
			const auto empty = UnitRulesTXTLoader::ParseUnits( "#UNITS\n-0 ; no designs", ruleset );
			GT_ASSERT( empty.ruleset_id == ruleset && empty.units.empty(), "zero-count catalog was rejected or given default units" );
			GT_ASSERT( UnitRulesTXTLoader::ParseUnits( "#UNITS\n+0001\n" + row, ruleset ).units.size() == 1, "valid explicit count sign was rejected" );
		}
		for ( const auto& invalid : { std::string(), std::string( "#UNITS_OTHER\n0" ), std::string( "##UNITS\n0" ), std::string( "#UNITS\n; missing count\n#END\n" ), "#UNITS\n" + row, UnitsSource( { row } ) + "#UNITS\n0" } ) {
			GT_ASSERT( !UnitsRejection( invalid ).empty(), "missing/duplicate section or missing count accepted" );
		}
		for ( const auto* ruleset : { "", "SMAC", "smax ", "auto" } ) {
			GT_ASSERT( !UnitsRejection( UnitsSource( { row } ), ruleset ).empty(), "unit table accepted implicit ruleset identity" );
		}
		for ( const auto* count : { "-1", "1", "2147483647" } ) {
			GT_ASSERT( !UnitsRejection( std::string( "#UNITS\n" ) + count ).empty(), "negative or unmatched count accepted" );
		}
		for ( const auto* count : { "0", "2" } ) {
			const auto error = UnitsRejection( std::string( "#UNITS\n" ) + count + "\n" + row );
			GT_ASSERT( error.find( "#UNITS row 1" ) != std::string::npos, "extra or missing unit records ignored instead of rejecting the count" );
		}
		for ( const auto& count : { std::string( "1," ), std::string( "1junk" ), std::string( "1.0" ), std::string( "0x1" ), std::string( "+" ), std::string( "+-1" ), std::string( "2147483648" ), std::string( "-2147483649" ), std::string( "1\0x", 3 ) } ) {
			const auto error = UnitsRejection( "#UNITS\n" + count + "\n" + row );
			GT_ASSERT( error.find( "#UNITS row 1 column 1" ) != std::string::npos, "malformed count accepted or mislocated" );
		}
		GT_OK();
	} );
	task->AddTest( "Unit design table rejects malformed columns text numbers plans and abilities", GT() {
		const auto original = UnitFields();
		for ( size_t column = 0 ; column < original.size() ; ++column ) {
			auto fields = original; fields.erase( fields.begin() + column );
			GT_ASSERT( !UnitsRejection( UnitsSource( { JoinRow( fields ) } ) ).empty(), "missing positional unit field accepted" );
		}
		for ( const auto* extra : { ",extra", ",," } ) {
			GT_ASSERT( !UnitsRejection( UnitsSource( { JoinRow( original ) + extra } ) ).empty(), "extra unit columns accepted" );
		}
		for ( const size_t column : { 0, 1, 2, 3, 7, 9 } ) {
			for ( const auto& value : { std::string(), std::string( "x\0y", 3 ) } ) {
				auto fields = original; fields[ column ] = value;
				const auto error = UnitsRejection( UnitsSource( { JoinRow( original ), JoinRow( fields ) } ) );
				GT_ASSERT( error.find( "#UNITS row 3 column " + std::to_string( column + 1 ) ) != std::string::npos, "invalid unit text accepted or mislocated" );
			}
		}
		for ( const size_t column : { 4, 5, 6, 8 } ) {
			for ( const auto& value : { std::string(), std::string( "*" ), std::string( "1junk" ), std::string( "1.0" ), std::string( "0x10" ), std::string( "1 2" ), std::string( "+" ), std::string( "-" ), std::string( "+-1" ), std::string( "++1" ), std::string( "--1" ), std::string( "2147483648" ), std::string( "-2147483649" ), std::string( "1\0x", 3 ) } ) {
				auto fields = original; fields[ column ] = value;
				const auto error = UnitsRejection( UnitsSource( { JoinRow( original ), JoinRow( fields ) } ) );
				GT_ASSERT( error.find( "#UNITS row 3 column " + std::to_string( column + 1 ) ) != std::string::npos, "invalid unit number accepted or mislocated" );
			}
		}
		for ( const auto* value : { "-2147483648", "-2", "13", "2147483647" } ) {
			auto fields = original; fields[ 4 ] = value;
			GT_ASSERT( UnitsRejection( UnitsSource( { JoinRow( fields ) } ) ).find( "row 2 column 5" ) != std::string::npos, "unknown unit plan narrowed into a supported value" );
		}
		for ( const auto* value : { "2", "0102", "0b01", "-1", "+01", "0 1", "1.0", "*" } ) {
			auto fields = original; fields[ 9 ] = value;
			GT_ASSERT( UnitsRejection( UnitsSource( { JoinRow( fields ) } ) ).find( "row 2 column 10" ) != std::string::npos, "nonbinary ability spelling accepted" );
		}
		GT_OK();
	} );
}

}
