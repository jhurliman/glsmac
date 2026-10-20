#include "SmacEnergyArithmetic.h"
#include "SmacBaseEnergy.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include "types/Buffer.h"

namespace game::backend::base {
namespace {
void ValidateSavedEnergy(const SmacBaseEnergyCalculation& value) {
    if(value.raw_intake<0 || value.headquarters_distance<0 || value.waste<0 || value.net<0 ||
        int64_t(value.raw_intake)-value.waste!=value.net || value.maintenance<0 || value.maintenance_fees.size()>33) {
        throw std::runtime_error("Invalid saved Base energy calculation");
    }
    int32_t previous=0;int64_t total=0;
    for(const auto& [id,fee]:value.maintenance_fees) {
        if(id<=previous || id>33 || fee<0) {throw std::runtime_error("Invalid saved Base maintenance order");}
        previous=id;total+=fee;
    }
    if(total!=value.maintenance) {throw std::runtime_error("Saved Base maintenance total differs");}
    for(const auto categories:{value.allocation.allocation,value.allocation.after_specialists,value.tail.multiplier_quarters,
        value.tail.before_penalty,value.tail.after_penalty,value.tail.final}) {
        if(categories.economy<0 || categories.psych<0 || categories.labs<0) {
            throw std::runtime_error("Negative saved Base energy category");
        }
    }
}
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC energy arithmetic exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

void WriteSmacBaseEnergy(types::Buffer& buffer,const SmacBaseEnergyCalculation& value) {
    ValidateSavedEnergy(value);
    for(const auto field:{value.raw_intake,value.headquarters_distance,value.waste,value.net,value.maintenance}) {buffer.WriteInt(field);}
    for(const auto categories:{value.allocation.allocation,value.allocation.after_specialists,value.tail.multiplier_quarters,
        value.tail.before_penalty,value.tail.after_penalty,value.tail.final}) {
        buffer.WriteInt(categories.economy);buffer.WriteInt(categories.psych);buffer.WriteInt(categories.labs);
    }
    buffer.WriteInt(value.maintenance_fees.size());
    for(const auto& [id,fee]:value.maintenance_fees) {buffer.WriteInt(id);buffer.WriteInt(fee);}
}

SmacBaseEnergyCalculation ReadSmacBaseEnergy(types::Buffer& buffer) {
    const auto integer=[&]() {
        const auto value=buffer.ReadInt();
        if(value<INT32_MIN || value>INT32_MAX) {throw std::runtime_error("Saved Base energy exceeds signed32");}
        return static_cast<int32_t>(value);
    };
    SmacBaseEnergyCalculation value{};
    value.raw_intake=integer();value.headquarters_distance=integer();value.waste=integer();value.net=integer();value.maintenance=integer();
    for(auto* categories:{&value.allocation.allocation,&value.allocation.after_specialists,&value.tail.multiplier_quarters,
        &value.tail.before_penalty,&value.tail.after_penalty,&value.tail.final}) {
        categories->economy=integer();categories->psych=integer();categories->labs=integer();
    }
    const auto count=buffer.ReadInt();
    if(count<0 || count>33 || buffer.lenr>buffer.lenw || uint64_t(count)>(buffer.lenw-buffer.lenr)/28u) {
        throw std::runtime_error("Invalid saved Base maintenance count");
    }
    for(int64_t i=0;i<count;++i) {const auto id=integer(),fee=integer();value.maintenance_fees.emplace_back(id,fee);}
    ValidateSavedEnergy(value);
    return value;
}
namespace {
void Nonnegative( SmacEnergyCategories value ) {
	if ( value.economy < 0 || value.psych < 0 || value.labs < 0 ) { throw std::invalid_argument( "Negative SMAC energy category is outside the measured tail" ); }
}
int32_t Multiply( int32_t value, int32_t quarters ) { return MultiplySmacEnergyQuarters(value,quarters,Checked); }
int32_t Reduce( int32_t value, int32_t percent ) { return ReduceSmacEnergyPercent(value,percent,Checked); }
}

SmacEnergyRates AdjustSmacEnergyRates(SmacEnergyRates rates,int32_t button,uint32_t locks) {
    rates.Validate();
    if(button<29 || button>34 || locks>7) {throw std::invalid_argument("Invalid energy rate button or locks");}
    // Native axis cycle E -> P -> L. The edited axis is a last-resort fallback,
    // including when that axis itself is locked.
    const int axis=button<=30 ? 0 : button<=32 ? 2 : 1;
    std::array<int32_t,3> values{10-rates.psych-rates.labs,rates.psych,rates.labs};
    values[axis]=std::clamp(values[axis]+(button%2==0 ? 1 : -1),0,10);
    int total=values[0]+values[1]+values[2];
    while(total!=10) {
        const int direction=total<10 ? 1 : -1;
        for(int step=1;step<=3;++step) {
            const int other=(axis+step)%3;
            if(other!=axis && (locks&(1u<<other))) {continue;}
            if((direction>0 && values[other]>=10) || (direction<0 && values[other]<=0)) {continue;}
            values[other]+=direction;total+=direction;break;
        }
    }
    return {values[1],values[2]};
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
	if ( f[ 11 ] || ( f[ 8 ] && input.owns_virtual_world ) ) { q.psych += 2; }
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
