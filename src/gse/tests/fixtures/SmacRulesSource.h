#pragma once
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

inline std::string ProductionSource() {
	const auto rules = Production();
	std::string source = "#RULES\n";
	for ( size_t row = 1 ; row <= 70 ; ++row ) {
		const auto value = row == 7 ? rules.mineral_multiplier : row == 43 ? rules.retool_loss_percent
			: row == 44 ? rules.retool_strictness : row == 45 ? rules.retool_exemption : 0;
		source += std::to_string( value ) + ", ; synthetic annotation\n";
	}
	source += "#FACILITIES\n";
	for ( const auto& facility : rules.facilities ) {
		source += facility.name + ", " + std::to_string( facility.cost ) + ", " + std::to_string( facility.maintenance )
			+ ", " + facility.prerequisite + ", ObsoleteToken, unused effect, with commas, 1, 2, 3, 4, 5,\n";
	}
	return source + "#END\n";
}

// Synthetic numeric values intentionally differ from installed defaults.
inline std::string FactionAlpha() {
	return "#BONUSNAMES\nTECH,MORALE,PSI,FACILITY,RESEARCH,DRONE,TALENT,ENERGY,\n"
		"INTEREST,COMMERCE,POPULATION,HURRY,UNIT,TECHCOST,SHARETECH,TERRAFORM,\n"
		"SOCIAL,ROBUST,IMMUNITY,IMPUNITY,PENALTY,FUNGNUTRIENT,FUNGMINERALS,FUNGENERGY,\n"
		"COMMFREQ,MINDCONTROL,FANATIC,VOTES,FREEPROTO\n#SOCIO\n"
		"ECONOMY,EFFIC,SUPPORT,TALENT,MORALE,POLICE,GROWTH,PLANET,PROBE,INDUSTRY,RESEARCH\n"
		"#FACTIONS\nGAIANS,GAIANS\nHIVE,HIVE\nUNIV,UNIV\nMORGAN,MORGAN\nSPARTANS,SPARTANS\nBELIEVE,BELIEVE\nPEACE,PEACE\n#END\n";
}

inline std::string FactionSource( const std::string& filename ) {
	const auto& bindings = game::backend::settings::SmacFactionStartupRules::Bindings();
	for ( size_t i = 0; i < bindings.size(); ++i ) {
		if ( filename == bindings[ i ].filename ) {
			return "#" + std::string( bindings[ i ].section ) + "\nSynthetic header\n"
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
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( profile.factions.size() );
	for ( const auto& faction : profile.factions ) {
		result.WriteString( faction.port_id ); result.WriteString( faction.filename ); result.WriteString( faction.section );
		result.WriteInt( faction.bonuses.size() );
		for ( const auto& bonus : faction.bonuses ) {
			result.WriteString( bonus.keyword ); result.WriteString( bonus.value ); result.WriteInt( static_cast< int32_t >( bonus.kind ) );
			result.WriteInt( bonus.parameter ); result.WriteInt( bonus.amount );
		}
	}
	return result;
}

inline game::backend::settings::SmacPredefinedUnitRules PredefinedUnits( const game::backend::settings::SmacTechnologyRules& technologies = Technologies() ) {
	return game::backend::settings::SmacPredefinedUnitRules::Parse( PredefinedSource(), technologies );
}

inline std::string Source( const std::string& filename ) {
	if ( filename != "alpha.txt" ) { return FactionSource( filename ); }
	return ProductionSource() + TechnologySource() + WorldSizeSource() + FactionAlpha() + PredefinedSource() + "#RESOURCEINFO\nOcean Square,*,*,*\nBase Square,17,-2147483648,2147483647\n"
		"Bonus Square,-2,300,0\nForest Square,*,*,*\nRecycling Tanks,-7,8,9\n"
		"Improved Land,*,*,*\nImproved Sea,*,*,*\nMonolith,*,*,*\nBorehole Square,*,*,*\n#END\n";
}

inline bool Matches( const game::backend::settings::GlobalSettings& settings ) {
	const auto& rules = settings.GetSmacBaseSquareRules();
	using game::backend::resource::smac::RuleTriple;
	return rules && rules->base_square == RuleTriple{ 17, INT32_MIN, INT32_MAX }
		&& rules->bonus_square == RuleTriple{ -2, 300, 0 } && rules->recycling == RuleTriple{ -7, 8, 9 }
		&& settings.GetSmacProductionRules() && *settings.GetSmacProductionRules() == Production()
		&& settings.GetSmacTechnologyRules() && *settings.GetSmacTechnologyRules() == Technologies()
		&& settings.GetSmacWorldSizeRules() && *settings.GetSmacWorldSizeRules() == WorldSizes()
		&& settings.GetSmacFactionStartupRules() && *settings.GetSmacFactionStartupRules() == Factions()
		&& settings.GetSmacPredefinedUnitRules() && *settings.GetSmacPredefinedUnitRules() == PredefinedUnits();
}

inline types::Buffer Payload( const std::string& id, const bool present,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
	const std::optional< std::string >& production = std::nullopt,
	const std::optional< std::string >& technologies = std::nullopt,
	const std::optional< std::string >& sizes = std::nullopt,
	const std::optional< std::string >& factions = std::nullopt,
	const std::optional< std::string >& units = std::nullopt ) {
	types::Buffer result;
	result.WriteInt( 7 );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) {
		for ( const auto value : values ) { result.WriteInt( value ); }
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
	game::backend::settings::MapSettings map;
	map.size_x = 86;
	result.WriteString( map.Serialize().ToString() );
	result.WriteString( "" ); // Default::Serialize currently has no fields.
	result.WriteInt( 1 );
	result.WriteString( "Host game" );
	return result;
}

}
