#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <utility>

namespace types { class Buffer; }

namespace game::backend::base {

struct SmacEnergyRates {
	int32_t psych, labs; // tenths; economy is the remainder
	void Validate() const;
};

// Original six buttons29..34; locks use cyclic Economy/Psych/Labs bits0..2.
SmacEnergyRates AdjustSmacEnergyRates(SmacEnergyRates rates,int32_t button,uint32_t locks);

struct SmacEnergyCategories {
	int32_t economy, psych, labs;
	bool operator==( const SmacEnergyCategories& other ) const noexcept;
};

struct SmacEnergyCoordinate { int32_t x, y; };

// Supplied, authenticated owned HQ coordinates. This is the original energy
// distance, not movement distance or nearest-base territory selection.
int32_t SmacHeadquartersDistance( int32_t width, bool horizontal_wrap, SmacEnergyCoordinate base,
	const std::vector< SmacEnergyCoordinate >& owned_headquarters );
int32_t CalculateSmacEnergyWaste( int32_t energy, int32_t headquarters_distance, int32_t efficiency, bool creche );

struct SmacEnergyAllocation {
	SmacEnergyCategories allocation, after_specialists;
};
SmacEnergyAllocation AllocateSmacEnergy( int32_t net_energy, SmacEnergyRates rates,
	const std::vector< SmacEnergyCategories >& specialist_outputs );

struct SmacEnergyTailInputs {
	SmacEnergyCategories categories;
	SmacEnergyRates rates;
	int32_t efficiency, flat_labs;
	// Canonical FACILITIES source slots 1..33; index zero must remain false.
	std::array< bool, 34 > facilities{};
	bool owns_virtual_world = false;
	bool local_supercollider = false, local_theory = false, local_space_elevator = false;
	bool local_longevity = false, free_market = false, local_backbone = false;
	int32_t global_network_nodes = 0;
	// Already gated Backbone contributions, independently supplied from gross
	// commerce. A caller must not infer either amount from missing diplomacy.
	std::array< int32_t, 7 > backbone_commerce{};
	bool current_stockpile = false, baseline_stockpile = false;
	int32_t mineral_surplus = 0;
};

struct SmacEnergyTail {
	SmacEnergyCategories multiplier_quarters, before_penalty, after_penalty, final;
};

struct SmacBaseEnergyCalculation {
	int32_t raw_intake, headquarters_distance, waste, net, maintenance;
	SmacEnergyAllocation allocation;
	SmacEnergyTail tail;
	// Frozen at the final resource pass, before research can change authority.
	// Source ID order is the original individual maintenance charge order.
	std::vector< std::pair< int32_t, int32_t > > maintenance_fees;
};

// Saved final-pass result, retained before research changes technology or
// maintenance authority. Restore never recalculates already earned outputs.
void WriteSmacBaseEnergy(types::Buffer&,const SmacBaseEnergyCalculation&);
SmacBaseEnergyCalculation ReadSmacBaseEnergy(types::Buffer&);

// RE-SMAC-ENERGY-TAIL-001 nonnegative category domain. Evaluated signed32
// overflow rejects explicitly; unmeasured original overflow is not emulated.
SmacEnergyTail FinishSmacEnergy( const SmacEnergyTailInputs& input );

}
