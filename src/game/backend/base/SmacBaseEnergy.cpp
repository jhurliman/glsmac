#include "SmacBaseEnergy.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC energy arithmetic exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
void Nonnegative( SmacEnergyCategories value ) {
	if ( value.economy < 0 || value.psych < 0 || value.labs < 0 ) { throw std::invalid_argument( "Negative SMAC energy category is outside the measured tail" ); }
}
int32_t Multiply( int32_t value, int32_t quarters ) { return Checked( int64_t( Checked( int64_t( value ) * quarters ) ) + 3 ) / 4; }
int32_t Reduce( int32_t value, int32_t percent ) { return Checked( int64_t( Checked( int64_t( value ) * ( 100 - percent ) ) ) + 50 ) / 100; }
}

void SmacEnergyRates::Validate() const {
	if ( psych < 0 || labs < 0 || psych > 10 || labs > 10 - psych ) { throw std::invalid_argument( "SMAC energy rates must be nonnegative tenths totaling at most ten" ); }
}

bool SmacEnergyCategories::operator==( const SmacEnergyCategories& other ) const noexcept {
	return economy == other.economy && psych == other.psych && labs == other.labs;
}

int32_t SmacHeadquartersDistance( int32_t width, bool horizontal_wrap, SmacEnergyCoordinate base,
	const std::vector< SmacEnergyCoordinate >& owned_headquarters ) {
	if ( width < 1 || base.x < 0 || base.x >= width || base.y < 0 ) { throw std::invalid_argument( "Invalid SMAC energy map coordinate" ); }
	int32_t nearest = 999;
	for ( const auto hq : owned_headquarters ) {
		if ( hq.x < 0 || hq.x >= width || hq.y < 0 ) { throw std::invalid_argument( "Invalid SMAC headquarters coordinate" ); }
		int64_t dx = std::abs( int64_t( base.x ) - hq.x ), dy = std::abs( int64_t( base.y ) - hq.y );
		if ( horizontal_wrap && dx > width / 2 ) { dx = width - dx; }
		const auto distance = Checked( 3 * std::max( dx, dy ) + std::min( dx, dy ) + 1 ) / 4;
		if ( distance < nearest ) { nearest = static_cast< int32_t >( distance ); }
	}
	return nearest == 999 ? 16 : nearest;
}

int32_t CalculateSmacEnergyWaste( int32_t energy, int32_t headquarters_distance, int32_t efficiency, bool creche ) {
	if ( headquarters_distance < 0 || headquarters_distance >= 999 ) { throw std::invalid_argument( "Invalid resolved SMAC headquarters distance" ); }
	if ( energy <= 0 || headquarters_distance == 0 ) { return 0; }
	const auto effective = Checked( int64_t( efficiency ) + ( creche ? 2 : 0 ) );
	if ( effective <= -4 ) { return energy; }
	const auto denominator = Checked( int64_t( Checked( int64_t( effective ) + 4 ) ) * 8 );
	const auto numerator = Checked( int64_t( headquarters_distance ) * energy );
	return std::clamp( numerator / denominator, 0, energy );
}

SmacEnergyAllocation AllocateSmacEnergy( int32_t net_energy, SmacEnergyRates rates,
	const std::vector< SmacEnergyCategories >& specialist_outputs ) {
	rates.Validate();
	if ( specialist_outputs.size() > 16 ) { throw std::invalid_argument( "SMAC energy output requires at most sixteen resolved specialists" ); }
	const auto energy = std::clamp( net_energy, 0, 9999 );
	const auto psych = std::clamp( ( energy * rates.psych + 4 ) / 10, 0, energy );
	const auto economy = std::clamp( ( energy * ( 10 - rates.psych - rates.labs ) + 4 ) / 10, 0, energy - psych );
	SmacEnergyAllocation result{ { economy, psych, energy - psych - economy }, {} };
	result.after_specialists = result.allocation;
	for ( const auto output : specialist_outputs ) {
		Nonnegative( output );
		result.after_specialists.economy = Checked( int64_t( result.after_specialists.economy ) + output.economy );
		result.after_specialists.psych = Checked( int64_t( result.after_specialists.psych ) + output.psych );
		result.after_specialists.labs = Checked( int64_t( result.after_specialists.labs ) + output.labs );
	}
	return result;
}

SmacEnergyTail FinishSmacEnergy( const SmacEnergyTailInputs& input ) {
	input.rates.Validate(); Nonnegative( input.categories );
	if ( input.facilities[ 0 ] || input.flat_labs < 0 || input.global_network_nodes < 0 ) { throw std::invalid_argument( "Invalid SMAC energy tail inputs" ); }
	for ( const auto commerce : input.backbone_commerce ) { if ( commerce < 0 ) { throw std::invalid_argument( "Negative SMAC Backbone commerce" ); } }
	const auto& f = input.facilities;
	SmacEnergyTail result{};
	auto& q = result.multiplier_quarters;
	q = { 4, 4, 4 };
	for ( const auto id : { 7, 13, 14, 15, 16 } ) { if ( f[ id ] ) { q.economy += 2; } }
	for ( const auto id : { 8, 17, 18, 15, 16 } ) { if ( f[ id ] ) { q.labs += 2; } }
	if ( f[ 10 ] || ( f[ 8 ] && input.owns_virtual_world ) ) { q.psych += 2; }
	for ( const auto id : { 17, 18 } ) { if ( f[ id ] ) { ++q.psych; } }
	for ( const auto id : { 13, 14 } ) { if ( f[ id ] ) { q.psych += 2; } }
	const auto labs = Checked( int64_t( Checked( int64_t( input.categories.labs ) + input.flat_labs ) ) + ( f[ 9 ] ? 2 : 0 ) );
	auto& before = result.before_penalty;
	before = { Multiply( input.categories.economy, q.economy ), Multiply( input.categories.psych, q.psych ), Multiply( labs, q.labs ) };
	if ( input.local_supercollider ) { before.labs = Checked( int64_t( before.labs ) * 2 ); }
	if ( input.local_theory ) { before.labs = Checked( int64_t( before.labs ) * 2 ); }
	if ( input.local_space_elevator ) { before.economy = Checked( int64_t( before.economy ) * 2 ); }
	if ( input.local_longevity && input.free_market ) { before.economy = Checked( int64_t( before.economy ) + before.economy / 2 ); }
	if ( input.local_backbone ) {
		before.labs = Checked( int64_t( before.labs ) + input.global_network_nodes );
		for ( const auto commerce : input.backbone_commerce ) { before.labs = Checked( int64_t( before.labs ) + commerce ); }
	}
	const auto economy_rate = 10 - input.rates.psych - input.rates.labs;
	const auto k = 2 * ( 4 - std::clamp( input.efficiency, -4, 4 ) ) * ( std::abs( input.rates.labs - economy_rate ) / 2 );
	const bool more_labs = input.rates.labs > economy_rate;
	// Caps follow the category: the smaller allocation loses up to80%, the
	// larger up to40%. Economy/labs exchange these caps when their order flips.
	result.after_penalty = { Reduce( before.economy, more_labs ? std::min( 2 * k, 80 ) : std::min( k, 40 ) ), before.psych,
		Reduce( before.labs, more_labs ? std::min( k, 40 ) : std::min( 2 * k, 80 ) ) };
	result.final = result.after_penalty;
	if ( input.current_stockpile && input.baseline_stockpile && input.mineral_surplus > 0 ) {
		const auto extra = Checked( int64_t( input.mineral_surplus ) + 1 ) / 2;
		result.final.economy = Checked( int64_t( result.final.economy ) + extra );
	}
	return result;
}

}
