#include "FactionTXTLoader.h"

#include "engine/Engine.h"
#include "resource/ResourceManager.h"

#include <stdexcept>

namespace loader {
namespace txt {
namespace {

std::string Field( std::string value ) {
	const auto first = value.find_first_not_of( " \t\r" );
	if ( first == std::string::npos ) { return {}; }
	return value.substr( first, value.find_last_not_of( " \t\r" ) - first + 1 );
}

std::vector< std::string > Fields( const std::string& row ) {
	std::vector< std::string > result;
	size_t start = 0;
	for ( ;; ) {
		const auto end = row.find( ',', start );
		result.push_back( Field( row.substr( start, end == std::string::npos ? end : end - start ) ) );
		if ( end == std::string::npos ) { return result; }
		start = end + 1;
	}
}

std::array< int32_t, 5 > WorkerPriorities( const std::string& source, const std::string& section ) {
	const auto rows = TXTLoader::ReadUniqueSection( source, section );
	if ( rows.empty() ) { throw std::invalid_argument( "Missing SMAC faction header" ); }
	auto fields = Fields( rows.front().substr( 0, rows.front().find( ';' ) ) );
	if ( fields.size() == 13 && fields.back().empty() ) { fields.pop_back(); }
	if ( fields.size() != 12 ) { throw std::invalid_argument( "SMAC faction header requires twelve fields" ); }
	std::array< int32_t, 5 > result;
	const size_t columns[] = { 7, 11, 9, 10, 8 };
	for ( size_t i = 0; i < result.size(); ++i ) {
		const auto& text = fields[ columns[ i ] ];
		size_t used = 0;
		const auto value = std::stoll( text, &used );
		if ( used != text.size() || value < INT32_MIN || value > INT32_MAX ) { throw std::invalid_argument( "SMAC faction priority requires signed32 decimal" ); }
		result[ i ] = static_cast< int32_t >( value );
	}
	return result;
}

std::array< game::backend::settings::SmacStartupSocialModel, 4 > ValidateLookupTables( const std::string& alpha ) {
	const std::string names = "TECH,MORALE,PSI,FACILITY,RESEARCH,DRONE,TALENT,ENERGY,INTEREST,COMMERCE,POPULATION,HURRY,UNIT,TECHCOST,SHARETECH,TERRAFORM,SOCIAL,ROBUST,IMMUNITY,IMPUNITY,PENALTY,FUNGNUTRIENT,FUNGMINERALS,FUNGENERGY,COMMFREQ,MINDCONTROL,FANATIC,VOTES,FREEPROTO";
	std::vector< std::string > actual;
	for ( const auto& line : TXTLoader::ReadUniqueSection( alpha, "BONUSNAMES" ) ) {
		auto fields = Fields( line.substr( 0, line.find( ';' ) ) );
		// Installed continuation lines end with a comma.
		if ( !fields.empty() && fields.back().empty() ) { fields.pop_back(); }
		actual.insert( actual.end(), fields.begin(), fields.end() );
	}
	if ( actual != Fields( names ) ) { throw std::invalid_argument( "SMAC faction startup requires canonical #BONUSNAMES order" ); }
	const auto socio = TXTLoader::ReadUniqueSection( alpha, "SOCIO" );
	if ( socio.empty() || Fields( socio.front().substr( 0, socio.front().find( ';' ) ) ) !=
		Fields( "ECONOMY,EFFIC,SUPPORT,TALENT,MORALE,POLICE,GROWTH,PLANET,PROBE,INDUSTRY,RESEARCH" ) ) {
		throw std::invalid_argument( "SMAC faction startup requires canonical #SOCIO axes" );
	}
	if ( socio.size() != 19 || Fields( socio[ 2 ].substr( 0, socio[ 2 ].find( ';' ) ) ) !=
		Fields( "Politics,Economics,Values,Future Society" ) ) {
		throw std::invalid_argument( "SMAC startup requires four ordered SOCIO categories with four model rows each" );
	}
	std::array< game::backend::settings::SmacStartupSocialModel, 4 > models;
	for ( size_t i = 0; i < models.size(); ++i ) {
		const auto fields = Fields( socio[ 3 + i * 4 ].substr( 0, socio[ 3 + i * 4 ].find( ';' ) ) );
		if ( fields.size() != 3 ) { throw std::invalid_argument( "SMAC model-zero SOCIO row requires name, prerequisite and empty modifiers" ); }
		models[ i ] = { fields[ 0 ], fields[ 1 ], fields[ 2 ] }; models[ i ].Validate();
	}
	return models;
}

}

std::vector< game::backend::settings::SmacFactionStartupBonus > FactionTXTLoader::ParseStartupBonuses(
	const std::string& source, const std::string& section, const game::backend::settings::SmacTechnologyRules& technologies ) {
	using game::backend::settings::SmacFactionStartupBonus;
	technologies.Validate();
	if ( source.find( '\0' ) != std::string::npos ) { throw std::invalid_argument( "NUL in SMAC faction source" ); }
	bool found = false, active = false, header = false, bonus = false;
	std::string bonus_line;
	size_t start = 0;
	while ( start < source.size() ) {
		const auto end = source.find( '\n', start );
		auto line = Field( source.substr( start, end == std::string::npos ? end : end - start ) );
		start = end == std::string::npos ? source.size() : end + 1;
		if ( line.rfind( "##", 0 ) == 0 || ( !line.empty() && line.front() == ';' ) ) { continue; }
		line = Field( line.substr( 0, line.find( ';' ) ) );
		if ( !line.empty() && line.front() == '#' ) {
			active = line == "#" + section;
			if ( active && found ) { throw std::invalid_argument( "Duplicate SMAC faction section #" + section ); }
			found = found || active;
			continue;
		}
		if ( !active || bonus ) { continue; }
		if ( !header ) { if ( !line.empty() ) { header = true; } }
		else { bonus_line = line; bonus = true; } // an empty bonus row is meaningful
	}
	if ( !found || !header || !bonus ) { throw std::invalid_argument( "Missing SMAC faction header/bonus row in #" + section ); }
	std::vector< SmacFactionStartupBonus > result;
	if ( bonus_line.empty() ) { return result; }
	const auto fields = Fields( bonus_line );
	if ( fields.size() % 2 != 0 ) { throw std::invalid_argument( "SMAC faction bonuses require complete keyword/value pairs" ); }
	for ( size_t i = 0 ; i < fields.size() ; i += 2 ) { result.push_back( SmacFactionStartupBonus::Parse( fields[ i ], fields[ i + 1 ], technologies ) ); }
	return result;
}

game::backend::settings::SmacFactionStartupRules FactionTXTLoader::ParseStartupRules( const std::string& alpha,
	const StartupSource& read_source, const game::backend::settings::SmacTechnologyRules& technologies ) {
	using game::backend::settings::SmacFactionStartupRules;
	technologies.Validate();
	const auto models = ValidateLookupTables( alpha );
	const auto rows = ReadUniqueSection( alpha, "FACTIONS" );
	const auto& bindings = SmacFactionStartupRules::Bindings();
	if ( rows.size() != bindings.size() ) { throw std::invalid_argument( "SMAC faction startup requires seven #FACTIONS rows" ); }
	// Validate the complete host source mapping before opening any faction file.
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto fields = Fields( rows[ i ].substr( 0, rows[ i ].find( ';' ) ) );
		if ( fields != std::vector< std::string >{ bindings[ i ].section, bindings[ i ].section } ) {
			throw std::invalid_argument( "Unsupported SMAC faction filename/section binding" );
		}
	}
	SmacFactionStartupRules result;
	result.initial_social_models = models;
	for ( size_t i = 0 ; i < bindings.size() ; ++i ) {
		const auto& binding = bindings[ i ];
		const auto source = read_source( binding.filename );
		result.factions[ i ] = { binding.port_id, binding.filename, binding.section,
			ParseStartupBonuses( source, binding.section, technologies ), WorkerPriorities( source, binding.section ) };
	}
	result.Validate( technologies );
	return result;
}

const FactionTXTLoader::faction_data_t& FactionTXTLoader::GetFactionData( resource::resource_t res ) {
	return GetFactionDataImpl( g_engine->GetResourceManager()->GetPath( res ) );
}

const FactionTXTLoader::faction_data_t& FactionTXTLoader::GetFactionData( const std::string& filename ) {
	return GetFactionDataImpl( g_engine->GetResourceManager()->GetCustomPath( filename ) );
}

const FactionTXTLoader::faction_data_t& FactionTXTLoader::GetFactionDataImpl( const std::string& path ) {
	auto it = m_faction_data.find( path );
	if ( it == m_faction_data.end() ) {
		const auto data = GetTXTData( path );
		it = m_faction_data.insert(
			{
				path,
				faction_data_t{
					{
						GetSection( data, "BASES" ),
						GetSection( data, "WATERBASES" ),
					}
				}
			}
		).first;
	}
	return it->second;
}

const std::vector< std::string >& FactionTXTLoader::GetSection( const txt_data_t& data, const std::string& name ) const {
	const auto bases_it = data.sections.find( name );
	if ( bases_it == data.sections.end() ) {
		THROW( "file does not contain section #" + name );
	}
	return bases_it->second;
}

}
}
