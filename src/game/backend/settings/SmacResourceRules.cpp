#include "SmacResourceRules.h"

#include <stdexcept>

namespace game::backend::settings {

std::array< SmacTechnologyReference, 4 > SmacResourceRules::TechnologyReferences( const SmacTechnologyRules& technologies ) const {
	std::array< SmacTechnologyReference, 4 > result;
	for ( size_t i = 0; i < technology_tokens.size(); ++i ) {
		const auto& token = technology_tokens[ i ];
		if ( token.empty() || token.size() > MAX_TOKEN_BYTES || token.find( '\0' ) != std::string::npos ) {
			throw std::invalid_argument( "SMAC resource technology token exceeds the admitted text domain" );
		}
		result[ i ] = technologies.ResolveToken( token );
		if ( result[ i ].kind == SmacTechnologyReference::Kind::UNKNOWN ) {
			throw std::invalid_argument( "Unknown SMAC resource technology token" );
		}
	}
	return result;
}

void SmacResourceRules::Validate( const SmacTechnologyRules& technologies ) const {
	(void)TechnologyReferences( technologies );
	if(discovery_rate<0 || discovery_rate>1000) {throw std::invalid_argument("Stored discovery rate must be within0..1000");}
}

bool SmacResourceRules::operator==( const SmacResourceRules& other ) const {
	return rows == other.rows && nutrients_per_citizen == other.nutrients_per_citizen
		&& unroaded_mine_limit == other.unroaded_mine_limit && mine_nutrient_effect == other.mine_nutrient_effect
		&& nutrient_multiplier == other.nutrient_multiplier && hab_complex_limit == other.hab_complex_limit
		&& hab_dome_limit == other.hab_dome_limit && initial_energy == other.initial_energy && discovery_rate == other.discovery_rate
		&& technology_tokens == other.technology_tokens;
}

resource::smac::BaseSquareRules SmacResourceRules::BaseSquare() const { return { rows[ 1 ], rows[ 2 ], rows[ 4 ] }; }
resource::smac::TileNutrientRules SmacResourceRules::TileNutrients() const {
	return { rows[ 0 ][ 0 ], rows[ 2 ][ 0 ], rows[ 3 ][ 0 ], rows[ 5 ][ 0 ], rows[ 6 ][ 0 ], rows[ 7 ][ 0 ], rows[ 8 ][ 0 ], mine_nutrient_effect };
}
resource::smac::TileMineralRules SmacResourceRules::TileMinerals() const {
	return { rows[ 0 ][ 1 ], rows[ 2 ][ 1 ], rows[ 3 ][ 1 ], rows[ 6 ][ 1 ], rows[ 7 ][ 1 ], rows[ 8 ][ 1 ], unroaded_mine_limit };
}
resource::smac::TileEnergyRules SmacResourceRules::TileEnergy() const {
	return { rows[ 0 ][ 2 ], rows[ 2 ][ 2 ], rows[ 3 ][ 2 ], rows[ 6 ][ 2 ], rows[ 7 ][ 2 ], rows[ 8 ][ 2 ] };
}

types::Buffer SmacResourceRules::Serialize( const SmacTechnologyRules& technologies ) const {
	Validate( technologies );
	types::Buffer buffer; buffer.WriteInt( 4 );
	for ( const auto& row : rows ) { for ( const auto value : row ) { buffer.WriteInt( value ); } }
	buffer.WriteInt( nutrients_per_citizen ); buffer.WriteInt( unroaded_mine_limit ); buffer.WriteInt( mine_nutrient_effect );
	buffer.WriteInt( nutrient_multiplier ); buffer.WriteInt( hab_complex_limit ); buffer.WriteInt( hab_dome_limit );
	buffer.WriteInt( initial_energy ); buffer.WriteInt( discovery_rate );
	for ( const auto& token : technology_tokens ) { buffer.WriteString( token ); }
	return buffer;
}

SmacResourceRules SmacResourceRules::Deserialize( types::Buffer buffer, const SmacTechnologyRules& technologies ) {
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC resource profile exceeds wire budget" ); }
	if ( buffer.ReadInt() != 4 ) { throw std::runtime_error( "Unsupported SMAC resource profile version" ); }
	const auto integer = [ & ]() {
		const auto value = buffer.ReadInt();
		if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC resource rule exceeds signed32" ); }
		return static_cast< int32_t >( value );
	};
	SmacResourceRules result;
	for ( auto& row : result.rows ) { for ( auto& value : row ) { value = integer(); } }
	result.nutrients_per_citizen = integer(); result.unroaded_mine_limit = integer(); result.mine_nutrient_effect = integer();
	result.nutrient_multiplier = integer(); result.hab_complex_limit = integer(); result.hab_dome_limit = integer();
	result.initial_energy = integer(); result.discovery_rate = integer();
	for ( auto& token : result.technology_tokens ) { token = buffer.ReadString( MAX_TOKEN_BYTES ); }
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC resource profile fields" ); }
	try { result.Validate( technologies ); }
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

}
