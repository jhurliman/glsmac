// Opt-in read-only probe. Never installs or applies faction startup state.
#include "loader/txt/FactionTXTLoader.h"
#include "loader/txt/SmacRulesTXTLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
std::string Lower( std::string text ) {
	for ( auto& c : text ) { if ( c >= 'A' && c <= 'Z' ) { c += 'a' - 'A'; } }
	return text;
}

std::filesystem::path InstalledFile( const std::filesystem::path& directory, const std::string& filename ) {
	std::filesystem::path result;
	size_t matches = 0;
	for ( const auto& entry : std::filesystem::directory_iterator( directory ) ) {
		if ( entry.is_regular_file() && Lower( entry.path().filename().string() ) == Lower( filename ) ) { result = entry.path(); ++matches; }
	}
	if ( matches != 1 ) { throw std::runtime_error( "Expected one case-insensitive installed file: " + filename ); }
	return result;
}

std::string Read( const std::filesystem::path& path ) {
	std::ifstream input( path, std::ios::binary );
	if ( !input ) { throw std::runtime_error( "Cannot read supplied source: " + path.string() ); }
	return { std::istreambuf_iterator< char >( input ), {} };
}

void Projection( std::ostream& output, const game::backend::settings::SmacFactionStartupRule& row ) {
	output << row.ExtraTechnologyCount();
	for ( const auto& record : row.Records() ) {
		output << ' ' << static_cast< int32_t >( record.kind ) << ':' << record.parameter << ':' << record.amount;
	}
	output << '\n';
}
}

int main( int argc, char** argv ) {
	if ( argc != 3 || ( std::string( argv[ 2 ] ) != "--installed" && std::string( argv[ 2 ] ) != "--lines" ) ) {
		std::cerr << "usage: smac_faction_startup_probe installed-directory --installed|--lines\n"; return 2;
	}
	try {
		using loader::txt::FactionTXTLoader;
		const std::filesystem::path directory = argv[ 1 ];
		const auto alpha = Read( InstalledFile( directory, "alpha.txt" ) );
		const auto technologies = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( alpha );
		std::ostringstream output;
		if ( std::string( argv[ 2 ] ) == "--installed" ) {
			const auto profile = FactionTXTLoader::ParseStartupRules( alpha, [ & ]( const std::string& filename ) { return Read( InstalledFile( directory, filename ) ); }, technologies );
			for ( const auto& row : profile.factions ) { output << row.port_id << ' '; Projection( output, row ); }
		}
		else {
			std::string line;
			while ( std::getline( std::cin, line ) ) {
				game::backend::settings::SmacFactionStartupRule row{ "", "", "", FactionTXTLoader::ParseStartupBonuses( "#PROBE\nSynthetic header\n" + line + "\n#END\n", "PROBE", technologies ) };
				Projection( output, row );
			}
		}
		std::cout << output.str();
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
