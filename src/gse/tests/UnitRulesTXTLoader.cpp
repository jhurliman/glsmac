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

// Synthetic metadata: role selection must come from the mode, not names,
// installed row indices, default costs, or prerequisite aliases.
std::vector< std::string > Rows() {
	return {
		"Survey Rifle, Survey, 3, 0, 5, -1, CustomTech",
		"Settlement Kit, Settler, 0, 8, 12, 6, None, ; an annotation",
		"Terrain Kit, Shaper, 0, 9, 7, 2, LandTech"
	};
}

std::string Source( const std::vector< std::string >& rows, const std::string& newline = "\n" ) {
	std::string source = "#UNITS" + newline + "ignored" + newline
		+ "## translator comment" + newline + " \t#WEAPONS \t" + newline;
	for ( const auto& row : rows ) { source += " ; comment" + newline + "\t" + newline + " " + row + "\t" + newline; }
	return source + "#END" + newline + "#UNITS" + newline + "also ignored" + newline;
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
}

}
