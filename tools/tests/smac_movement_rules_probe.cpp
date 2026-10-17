// Opt-in real parser/profile/basic producer probe. No source search or defaults.
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/UnitRulesTXTLoader.h"

#include <array>
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

using game::backend::settings::SmacMovementRules;
using loader::txt::SmacRulesTXTLoader;
using loader::txt::UnitRulesTXTLoader;
using Chassis = SmacMovementRules::GroundChassis;

void Print( const std::string& source, size_t index, Chassis chassis ) {
	const auto rules = SmacRulesTXTLoader::ParseMovementRules( source );
	const auto restored = SmacMovementRules::Deserialize( rules.Serialize() );
	if ( !( restored == rules ) ) { throw std::runtime_error( "profile roundtrip differs" ); }
	const auto table = UnitRulesTXTLoader::ParseChassis( source, "smac" );
	const auto& component = table.chassis.at( index );
	if ( component.triad != loader::txt::ChassisTriad::Land ) { throw std::invalid_argument( "requires ground chassis" ); }
	std::cout << "ok " << restored.RawRoadRate() << ' ' << restored.Quantum() << ' ' << component.speed << ' ' << component.cost
		<< ' ' << restored.OrdinaryGroundAllowance( chassis, component.speed ) << '\n';
}

int main( int argc, char** argv ) {
	try {
		if ( argc == 2 ) {
			std::ifstream input( argv[ 1 ], std::ios::binary );
			if ( !input ) { return 2; }
			// Explicit installed canonical SMAC source, bounded before materializing.
			input.seekg( 0, std::ios::end );
			if ( input.tellg() < 0 || input.tellg() > 1024 * 1024 ) { return 2; }
			input.seekg( 0 );
			const std::string source( std::istreambuf_iterator< char >( input ), {} );
			Print( source, 0, Chassis::Infantry ); Print( source, 1, Chassis::Speeder );
			return 0;
		}
		if ( argc != 1 ) { return 2; }
		char line[ 256 ];
		while ( std::cin.getline( line, sizeof( line ) ) ) {
			const auto length = static_cast< size_t >( std::cin.gcount() ) - ( std::cin.eof() ? 0 : 1 );
			const std::string record( line, length );
			if ( record.find( '\0' ) != std::string::npos ) { return 2; }
			std::istringstream stream( record );
			std::array< int32_t, 4 > values{}; // raw road rate, chassis0/1, parsed Moves, parsed Cost
			std::string token; size_t count = 0;
			while ( stream >> token ) {
				if ( count == values.size() ) { return 2; }
				const auto parsed = std::from_chars( token.data(), token.data() + token.size(), values[ count ] );
				if ( parsed.ec != std::errc() || parsed.ptr != token.data() + token.size() ) { return 2; }
				++count;
			}
			if ( count != values.size() || values[ 1 ] < 0 || values[ 1 ] > 1 ) { return 2; }
			std::string source = "#RULES\n" + std::to_string( values[ 0 ] ) + ", ; deliberately uninformative\n";
			for ( size_t i = 1; i < 70; ++i ) { source += "unused\n"; }
			source += "#CHASSIS\nSynthetic,M1,Plural,M2,Synthetic,F1,Plural,F2," + std::to_string( values[ 2 ] )
				+ ",0,0,0,0," + std::to_string( values[ 3 ] ) + ",None,Large,N1,LargePlural,N2\n#END\n";
			Print( source, 0, static_cast< Chassis >( values[ 1 ] ) );
		}
		return std::cin.bad() || ( std::cin.fail() && !std::cin.eof() ) ? 2 : 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 2; }
}
