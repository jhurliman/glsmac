#include "SmacEconomyState.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
constexpr int64_t MAGIC = 0x534d4543; // SMEC
constexpr int64_t VERSION = 4;
int32_t ReadInteger( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC economy integer outside signed32" ); }
	return static_cast< int32_t >( value );
}
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC economy posting exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

SmacEconomyState SmacEconomyState::NewWorld( base::SmacEnergyRates rates, int32_t credits, int32_t fraction ) {
	SmacEconomyState result{ rates, credits, 0, fraction, 0 };
	result.design_counts=SmacDesignCounts{};result.regional_base_counts=RegionalBaseCounts{};result.Validate();return result;
}

void SmacEconomyState::Validate() const {
	rates.Validate();
	if ( research_fraction < 0 || research_fraction >= 100 ) { throw std::invalid_argument( "SMAC research fraction must be in 0..99" ); }
}

bool SmacEconomyState::operator==( const SmacEconomyState& other ) const {
	return rates.psych == other.rates.psych && rates.labs == other.rates.labs && credits == other.credits
		&& pending_debit == other.pending_debit && research_fraction == other.research_fraction && treaty_partners == other.treaty_partners
		&& population_aggregate == other.population_aggregate && regional_base_counts == other.regional_base_counts && design_counts == other.design_counts;
}

types::Buffer SmacEconomyState::Serialize() const {
	Validate(); types::Buffer buffer;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( rates.psych ); buffer.WriteInt( rates.labs );
	buffer.WriteInt( credits ); buffer.WriteInt( pending_debit );
	buffer.WriteInt( research_fraction ); buffer.WriteInt( treaty_partners );
	buffer.WriteBool(population_aggregate.has_value());if(population_aggregate) {buffer.WriteInt(*population_aggregate);}
	buffer.WriteBool(regional_base_counts.has_value());
	if(regional_base_counts) {
		std::string counts;counts.reserve(128);for(const auto count:*regional_base_counts) {counts.push_back(static_cast<char>(count));}
		buffer.WriteString(counts);
	}
    buffer.WriteBool(design_counts.has_value());
    if(design_counts) {
        std::string counts;counts.reserve(512);for(const auto count:*design_counts) {counts.push_back(static_cast<char>(count));}
        buffer.WriteString(counts);
    }
	return buffer;
}

SmacEconomyState SmacEconomyState::Deserialize( const std::string& wire ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "Invalid SMAC economy frame size" ); }
	try {
		types::Buffer buffer( wire );
		if ( buffer.ReadInt() != MAGIC ) { throw std::runtime_error( "Unsupported SMAC economy format" ); }
		const auto version=buffer.ReadInt();if(version!=VERSION && version!=3 && version!=2 && version!=1) {throw std::runtime_error("Unsupported SMAC economy format");}
		const auto psych = ReadInteger( buffer ), labs = ReadInteger( buffer ), credits = ReadInteger( buffer ), debit = ReadInteger( buffer );
		const auto fraction = ReadInteger( buffer ), treaties = ReadInteger( buffer );
		if ( treaties < 0 || treaties > UINT8_MAX ) { throw std::runtime_error( "SMAC treaty bits outside unsigned byte" ); }
		SmacEconomyState result{ { psych, labs }, credits, debit, fraction, static_cast< uint8_t >( treaties ) };
		result.population_aggregate.reset();
		if(version>=2 && buffer.ReadBool()) {result.population_aggregate=ReadInteger(buffer);}
		if(version>=3 && buffer.ReadBool()) {
			const auto counts=buffer.ReadString(128);if(counts.size()!=128) {throw std::runtime_error("SMAC regional Base counts require 128 bytes");}
			result.regional_base_counts=RegionalBaseCounts{};
			for(size_t i=0;i<128;++i) {(*result.regional_base_counts)[i]=static_cast<uint8_t>(counts[i]);}
		}
        if(version>=4 && buffer.ReadBool()) {
            const auto counts=buffer.ReadString(512);if(counts.size()!=512) {throw std::runtime_error("SMAC design counts require 512 bytes");}
            result.design_counts=SmacDesignCounts{};
            for(size_t i=0;i<512;++i) {(*result.design_counts)[i]=static_cast<uint8_t>(counts[i]);}
        }
		if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC economy fields" ); }
		result.Validate(); return result;
	} catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

SmacEconomyState SmacEconomyState::CreditBase( int32_t economy, int32_t maintenance ) const {
	Validate();
	if ( credits < 0 || economy < 0 || maintenance < 0 ) { throw std::runtime_error( "SMAC economy posting requires nonnegative income, fee and balance" ); }
	const auto credited = Checked( int64_t( credits ) + economy );
	const auto balance = Checked( int64_t( credited ) - maintenance );
	if ( balance < 0 ) { throw std::runtime_error( "SMAC maintenance deficit handling is not connected" ); }
	auto result = *this; result.credits = balance; return result;
}

SmacEconomyState SmacEconomyState::SettleDebit() const {
	Validate();
	if ( credits < 0 || pending_debit < 0 ) { throw std::runtime_error( "SMAC settlement requires nonnegative balance and pending debit" ); }
	// RE-SMAC-HURRY-001 resolves this same reserved-spending field: subtract,
	// clear once, and floor the balance at zero after faction upkeep.
	const auto balance = std::max( Checked( int64_t( credits ) - pending_debit ), 0 );
	auto result = *this; result.credits = balance; result.pending_debit = 0; return result;
}

SmacResearchSubmission ScaleSmacResearchSubmission( int32_t labs, int32_t research,
	int32_t turn, int32_t difficulty, bool research_halving_facility, int32_t fraction ) {
	if ( labs < 0 || difficulty < 0 || difficulty > 5 || fraction < 0 || fraction >= 100 ) {
		throw std::invalid_argument( "SMAC research submission input outside ordinary domain" );
	}
	if ( research_halving_facility ) { labs /= 2; }
	if ( int64_t( turn ) < -5 * int64_t( research ) || ( turn < 5 && difficulty < 2 ) ) { labs = 0; }
	const auto product = Checked( int64_t( labs ) * ( 10 + std::clamp( research, -5, 5 ) ) );
	const auto numerator = Checked( int64_t( product ) * 10 );
	const auto carried = fraction + numerator % 100;
	return { labs, Checked( int64_t( numerator / 100 ) + carried / 100 ), carried % 100 };
}

}
