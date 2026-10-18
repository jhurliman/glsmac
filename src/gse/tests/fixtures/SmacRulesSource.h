#pragma once

#include "SmacTerraformingRules.h"
#include "SmacPredefinedUnitSource.h"

#include "game/backend/settings/Settings.h"
#include "loader/txt/FactionTXTLoader.h"

#include <array>
#include <bitset>
#include <stdexcept>
#include <string>

namespace gse::tests::smac_rules_fixture {

inline game::backend::settings::SmacWorldSizeRules WorldSizes() {
	return { { { { "Fixture tiny", 4, 8 }, { "Fixture small", 24, 48 }, { "Fixture standard", 32, 64 }, { "Fixture large", 40, 80 }, { "Fixture huge", 64, 128 } } } };
}

inline std::string WorldSizeSource() {
	std::string source = "#WORLDSIZE\n5\n";
	for ( const auto& row : WorldSizes().presets ) { source += row.name + ", " + std::to_string( row.source_x ) + ", " + std::to_string( row.source_y ) + "\n"; }
	return source + "#END\n";
}

inline void AcceptWorldSize( game::backend::settings::GlobalSettings& settings ) {
	if ( settings.GetRulesetId() == "smac" ) { settings.SelectSmacWorldSize( 0 ); }
}

inline game::backend::settings::SmacTechnologyRules Technologies() {
	game::backend::settings::SmacTechnologyRules result;
	for ( size_t i = 0 ; i < result.technologies.size() ; ++i ) {
		result.technologies[ i ] = { static_cast< int32_t >( i ), "Repeated technology name", "Tech" + std::to_string( i ),
			{ INT32_MIN, static_cast< int32_t >( i ), -static_cast< int32_t >( i ), INT32_MAX }, { "None", "None" }, static_cast< uint16_t >( i * 5 ) };
	}
	return result;
}

inline std::string TechnologySource() {
	std::string source = "#TECHNOLOGY\n";
	for ( const auto& row : Technologies().technologies ) {
		source += row.name + ", " + row.key;
		for ( const auto value : row.priorities ) { source += ", " + std::to_string( value ); }
		for ( const auto& prerequisite : row.prerequisites ) { source += ", " + prerequisite; }
		source += ", " + std::bitset< 9 >( row.flags ).to_string() + "\n";
	}
	return source + "#END\n";
}

inline game::backend::settings::SmacProductionRules Production() {
	game::backend::settings::SmacProductionRules result{ 123, -50, INT32_MAX, INT32_MIN, {} };
	for ( size_t i = 0 ; i < result.facilities.size() ; ++i ) {
		result.facilities[ i ] = { static_cast< int32_t >( i + 1 ), i < 2 ? "Repeated display name" : "Facility " + std::to_string( i + 1 ),
			i == 0 ? INT32_MIN : i == 70 ? INT32_MAX : static_cast< int32_t >( 100 + i ), -static_cast< int32_t >( i ),
			i == 0 || i == 37 ? "None" : i == 70 ? "Disable" : i == 2 ? "Tech2" : "UnknownTech" + std::to_string( i ) };
	}
	return result;
}

inline game::backend::settings::SmacMovementRules Movement() { return game::backend::settings::SmacMovementRules( -1 ); }

inline game::backend::settings::SmacTerritoryRules Territory() { return game::backend::settings::SmacTerritoryRules(8); }

inline game::backend::settings::SmacCombatRules Combat() {
	return {25,25,25,game::backend::settings::SmacCombatRules::PsiRatios{{{18,19},{19,20},{20,21}}}};
}

inline game::backend::settings::SmacSpecialistRules Specialists() {
	game::backend::settings::SmacSpecialistRules result{ -3, {} };
	for ( size_t i = 0; i < result.specialists.size(); ++i ) {
		result.specialists[ i ] = { static_cast< int32_t >( i ), "Repeated singular", "Repeated plural",
			i == 0 ? "None" : i == 1 ? "Disable" : i == 2 ? "Unknown" : "tEcH" + std::to_string( i ),
			i == 6 ? "Disable" : "Tech" + std::to_string( i ),
			i == 0 ? INT32_MIN : static_cast< int32_t >( i ), -static_cast< int32_t >( i ), i == 6 ? INT32_MAX : 0, "opaque" + std::to_string( i ) };
	}
	return result;
}

inline std::string SpecialistSource() {
	std::string source = "#CITIZENS\n";
	for ( const auto& row : Specialists().specialists ) {
		source += row.singular + "," + row.plural + "," + row.prerequisite + "," + row.obsolete + ","
			+ std::to_string( row.economy ) + "," + std::to_string( row.psych ) + "," + std::to_string( row.labs ) + "," + row.trailing_token + "\n";
	}
	return source + "Ordinary A,Ordinary plural A,\nOrdinary B,Ordinary plural B,\nOrdinary C,Ordinary plural C,\n#END\n";
}

// Independent, deliberately unchecked framing for malformed fixtures.
inline types::Buffer UncheckedSpecialists( const game::backend::settings::SmacSpecialistRules& profile ) {
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( profile.minimum_base_size ); result.WriteInt( profile.specialists.size() );
	for ( const auto& row : profile.specialists ) {
		result.WriteInt( row.source_id ); result.WriteString( row.singular ); result.WriteString( row.plural );
		result.WriteString( row.prerequisite ); result.WriteString( row.obsolete );
		result.WriteInt( row.economy ); result.WriteInt( row.psych ); result.WriteInt( row.labs ); result.WriteString( row.trailing_token );
	}
	return result;
}

inline std::string ProductionSource() {
	const auto rules = Production();
	std::string source = "#RULES\n";
	for ( size_t row = 1 ; row <= 70 ; ++row ) {
		const auto value = row == 1 ? Movement().RawRoadRate() : row == 7 ? rules.mineral_multiplier : row == 43 ? rules.retool_loss_percent
			: row == 44 ? rules.retool_strictness : row == 45 ? rules.retool_exemption : row == 11 ? Specialists().minimum_base_size : row == 48 ? Territory().RawBorderDistance() : (row == 22 || row == 28 || row == 30) ? 25 : 0;
		const auto token = row == 54 ? "Tech0" : row == 55 ? "Tech1" : row == 56 ? "Tech2" : row == 58 ? "Tech3" : "";
		source += (row>=18 && row<=20 ? std::to_string(row)+","+std::to_string(row+1) : *token ? std::string( token ) : std::to_string( value )) + ", ; synthetic annotation\n";
	}
	source += "#FACILITIES\n";
	for ( const auto& facility : rules.facilities ) {
		source += facility.name + ", " + std::to_string( facility.cost ) + ", " + std::to_string( facility.maintenance )
			+ ", " + facility.prerequisite + ", ObsoleteToken, unused effect, with commas, 1, 2, 3, 4, 5,\n";
	}
	return source + "#END\n";
}

inline std::string StartupSocialSource() {
	std::string source = "#SOCIO\nECONOMY,EFFIC,SUPPORT,TALENT,MORALE,POLICE,GROWTH,PLANET,PROBE,INDUSTRY,RESEARCH\n"
		"Synthetic axis descriptions\nPolitics,Economics,Values,Future Society\n";
	for ( size_t group = 0; group < 4; ++group ) {
		source += "Fixture model " + std::to_string( group ) + ",None,\n";
		for ( size_t unused = 1; unused < 4; ++unused ) { source += "Unused model,UnusedTech,+UNUSED\n"; }
	}
	return source;
}

// Synthetic numeric values intentionally differ from installed defaults.
inline std::string FactionAlpha() {
	return "#BONUSNAMES\nTECH,MORALE,PSI,FACILITY,RESEARCH,DRONE,TALENT,ENERGY,\n"
		"INTEREST,COMMERCE,POPULATION,HURRY,UNIT,TECHCOST,SHARETECH,TERRAFORM,\n"
		"SOCIAL,ROBUST,IMMUNITY,IMPUNITY,PENALTY,FUNGNUTRIENT,FUNGMINERALS,FUNGENERGY,\n"
		"COMMFREQ,MINDCONTROL,FANATIC,VOTES,FREEPROTO\n" + StartupSocialSource() +
		"#FACTIONS\nGAIANS,GAIANS\nHIVE,HIVE\nUNIV,UNIV\nMORGAN,MORGAN\nSPARTANS,SPARTANS\nBELIEVE,BELIEVE\nPEACE,PEACE\n#END\n";
}

inline std::string FactionSource( const std::string& filename ) {
	const auto& bindings = game::backend::settings::SmacFactionStartupRules::Bindings();
	for ( size_t i = 0; i < bindings.size(); ++i ) {
		if ( filename == bindings[ i ].filename ) {
			return "#" + std::string( bindings[ i ].section ) + "\nSynthetic,Faction,Plural,F,1,Leader,F,-1,0,0,1,1,\n"
				"TECH,Tech2,TECH,Tech2,TECH,None,TECH,Disable,SOCIAL,+SUPPORT,FACILITY,4,IMMUNITY,EFFIC,"
				"FUNGNUTRIENT,1,VOTES,2,TECH,1,TECH,-3,ENERGY," + std::to_string( 10 + i ) + "\n#END\n";
		}
	}
	throw std::runtime_error( "Unexpected faction filename: " + filename );
}

inline game::backend::settings::SmacFactionStartupRules Factions(
	const game::backend::settings::SmacTechnologyRules& technologies = Technologies() ) {
	auto result = loader::txt::FactionTXTLoader::ParseStartupRules( FactionAlpha(), FactionSource, Technologies() );
	// Valid catalog-difference fixtures must remain valid settings even when a
	// referenced key is renamed. Keep the same intended positional source grant.
	for ( auto& faction : result.factions ) {
		for ( auto& bonus : faction.bonuses ) {
			if ( bonus.kind == game::backend::settings::SmacFactionStartupBonus::Kind::NAMED_TECH && bonus.parameter >= 0 ) {
				bonus = game::backend::settings::SmacFactionStartupBonus::Parse( "TECH", technologies.technologies[ bonus.parameter ].key, technologies );
			}
		}
	}
	return result;
}

// Deliberately bypass profile validation to exercise late malformed wire input.
inline types::Buffer UncheckedFactions( const game::backend::settings::SmacFactionStartupRules& profile ) {
	types::Buffer result; result.WriteInt( 3 ); result.WriteInt( profile.initial_social_models.size() );
	for ( const auto& model : profile.initial_social_models ) {
		result.WriteString( model.name ); result.WriteString( model.prerequisite ); result.WriteString( model.modifiers );
	}
	result.WriteInt( profile.factions.size() );
	for ( const auto& faction : profile.factions ) {
		result.WriteString( faction.port_id ); result.WriteString( faction.filename ); result.WriteString( faction.section );
		result.WriteInt( faction.bonuses.size() );
		for ( const auto& bonus : faction.bonuses ) {
			result.WriteString( bonus.keyword ); result.WriteString( bonus.value ); result.WriteInt( static_cast< int32_t >( bonus.kind ) );
			result.WriteInt( bonus.parameter ); result.WriteInt( bonus.amount );
		}
	}
	for ( const auto& faction : profile.factions ) { for ( const auto priority : faction.worker_priorities ) { result.WriteInt( priority ); } }
	return result;
}

inline game::backend::settings::SmacPredefinedUnitRules PredefinedUnits( const game::backend::settings::SmacTechnologyRules& technologies = Technologies() ) {
	return game::backend::settings::SmacPredefinedUnitRules::Parse( PredefinedSource(), technologies );
}

inline std::string Source( const std::string& filename ) {
	if ( filename != "alpha.txt" ) { return FactionSource( filename ); }
	return terraforming_rules_fixture::Source() + ProductionSource() + SpecialistSource() + TechnologySource() + WorldSizeSource() + FactionAlpha() + PredefinedSource() + "#RESOURCEINFO\nOcean Square,*,*,*\nBase Square,17,-2147483648,2147483647\n"
		"Bonus Square,-2,300,0\nForest Square,*,*,*\nRecycling Tanks,-7,8,9\n"
		"Improved Land,*,*,*\nImproved Sea,*,*,*\nMonolith,*,*,*\nBorehole Square,*,*,*\n#END\n";
}

inline game::backend::settings::SmacResourceRules Resources( const game::backend::settings::SmacTechnologyRules& technologies = Technologies() ) {
	return { { { { 0, 0, 0 }, { 17, INT32_MIN, INT32_MAX }, { -2, 300, 0 }, { 0, 0, 0 }, { -7, 8, 9 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } } }, 0, 0, 0, 0, 0, 0, 0, 0,
		{ technologies.technologies[ 0 ].key, technologies.technologies[ 1 ].key, technologies.technologies[ 2 ].key, technologies.technologies[ 3 ].key } };
}

// Independent wire builder intentionally permits malformed late values.
inline types::Buffer UncheckedResources( const game::backend::settings::SmacResourceRules& profile,
	const std::optional< std::array< int64_t, 9 > >& center = {} ) {
	types::Buffer result; result.WriteInt( 4 );
	for ( size_t row = 0; row < 9; ++row ) {
		for ( size_t column = 0; column < 3; ++column ) {
			const int index = row == 1 ? 0 : row == 2 ? 1 : row == 4 ? 2 : -1;
			result.WriteInt( center && index >= 0 ? ( *center )[ index * 3 + column ] : profile.rows[ row ][ column ] );
		}
	}
	result.WriteInt( profile.nutrients_per_citizen ); result.WriteInt( profile.unroaded_mine_limit ); result.WriteInt( profile.mine_nutrient_effect );
	result.WriteInt( profile.nutrient_multiplier ); result.WriteInt( profile.hab_complex_limit ); result.WriteInt( profile.hab_dome_limit ); result.WriteInt( profile.initial_energy ); result.WriteInt( profile.discovery_rate );
	for ( const auto& token : profile.technology_tokens ) { result.WriteString( token ); }
	return result;
}

inline std::string ResourceFrame( const std::array< int64_t, 9 >& center, const std::optional< std::string >& technology_bytes = {} ) {
	auto catalog = Technologies();
	if ( technology_bytes ) { try { catalog = game::backend::settings::SmacTechnologyRules::Deserialize( types::Buffer( *technology_bytes ) ); } catch ( const std::runtime_error& ) {} }
	return UncheckedResources( Resources( catalog ), center ).ToString();
}

inline bool Matches( const game::backend::settings::GlobalSettings& settings ) {
	const auto& rules = settings.GetSmacResourceRules();
	return rules && *rules == Resources()
		&& settings.GetSmacProductionRules() && *settings.GetSmacProductionRules() == Production()
		&& settings.GetSmacTechnologyRules() && *settings.GetSmacTechnologyRules() == Technologies()
		&& settings.GetSmacWorldSizeRules() && *settings.GetSmacWorldSizeRules() == WorldSizes()
		&& settings.GetSmacFactionStartupRules() && *settings.GetSmacFactionStartupRules() == Factions()
		&& settings.GetSmacPredefinedUnitRules() && *settings.GetSmacPredefinedUnitRules() == PredefinedUnits()
		&& settings.GetSmacMovementRules() && *settings.GetSmacMovementRules() == Movement()
		&& settings.GetSmacCombatRules() && *settings.GetSmacCombatRules() == Combat()
		&& settings.GetSmacSpecialistRules() && *settings.GetSmacSpecialistRules() == Specialists()
		&& settings.GetSmacTerraformingRules() && *settings.GetSmacTerraformingRules() == terraforming_rules_fixture::Profile();
}

inline types::Buffer Payload( const std::string& id, const bool present,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
	const std::optional< std::string >& production = std::nullopt,
	const std::optional< std::string >& technologies = std::nullopt,
	const std::optional< std::string >& sizes = std::nullopt,
	const std::optional< std::string >& factions = std::nullopt,
	const std::optional< std::string >& units = std::nullopt,
	const std::optional< std::string >& movement = std::nullopt,
	const std::optional< bool > movement_present = std::nullopt,
	const std::optional< std::string >& resources = std::nullopt,
	const std::optional< std::string >& specialists = std::nullopt,
	const std::optional< bool > specialists_present = std::nullopt ) {
	types::Buffer result;
	result.WriteInt( 17 );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) {
		result.WriteString( resources ? *resources : ResourceFrame( values, technologies ) );
		result.WriteString( production ? *production : Production().Serialize().ToString() );
		result.WriteString( technologies ? *technologies : Technologies().Serialize().ToString() );
		result.WriteString( sizes ? *sizes : WorldSizes().Serialize().ToString() );
		std::string faction_bytes = factions ? *factions : Factions().Serialize( Technologies() ).ToString();
		if ( technologies && !factions ) {
			try {
				const auto catalog = game::backend::settings::SmacTechnologyRules::Deserialize( types::Buffer( *technologies ) );
				faction_bytes = Factions( catalog ).Serialize( catalog ).ToString();
			} catch ( const std::runtime_error& ) { /* Preserve deliberately malformed catalog bytes above. */ }
		}
		result.WriteString( faction_bytes );
	}
	result.WriteBool( present );
	if ( present ) {
		auto catalog = Technologies();
		if ( technologies ) { try { catalog = game::backend::settings::SmacTechnologyRules::Deserialize( types::Buffer( *technologies ) ); } catch ( const std::runtime_error& ) {} }
		result.WriteString( units ? *units : PredefinedUnits( catalog ).Serialize( catalog ).ToString() );
	}
	const auto has_movement = movement_present.value_or( present );
	result.WriteBool( has_movement );
	if ( has_movement ) { result.WriteString( movement ? *movement : Movement().Serialize().ToString() ); }
	const auto has_specialists = specialists_present.value_or( present ); result.WriteBool( has_specialists );
	if ( has_specialists ) { result.WriteString( specialists ? *specialists : Specialists().Serialize().ToString() ); }
	result.WriteBool(present); if(present) { result.WriteString(Territory().Serialize().ToString()); }
	result.WriteBool(present); if(present) { result.WriteString(terraforming_rules_fixture::Wire(terraforming_rules_fixture::Profile()).ToString()); }
	result.WriteBool(present); if(present) {result.WriteString(Combat().Serialize().ToString());}
	game::backend::settings::MapSettings map;
	map.size_x = 86;
	result.WriteString( map.Serialize().ToString() );
	result.WriteString( "" ); // Default::Serialize currently has no fields.
	result.WriteInt( 1 );
	result.WriteString( "Host game" );
	return result;
}

}
