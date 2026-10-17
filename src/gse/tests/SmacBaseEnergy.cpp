#include "SmacBaseEnergy.h"
#include "fixtures/SmacBaseEnergy.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
using namespace game::backend::base;
namespace {
template< typename F > bool Rejects( F&& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}
}

std::string CheckSmacEnergyWaste() {
	for ( const auto& row : smac_energy_fixture::waste ) {
		const auto distance = SmacHeadquartersDistance( row.width, row.wrap, row.base, row.hqs );
		const auto before = CalculateSmacEnergyWaste( row.energy, distance, row.efficiency, row.creche );
		const auto after = CalculateSmacEnergyWaste( row.energy + row.commerce, distance, row.efficiency, row.creche );
		if ( ( row.distance >= 0 && distance != row.distance ) || before != row.waste_before || row.energy - before != row.net_before
			|| after != row.waste_after || row.energy + row.commerce - after != row.net_after ) { return row.id; }
	}
	return {};
}

std::string CheckSmacEnergyAllocation() {
	for ( const auto& row : smac_energy_fixture::allocation ) {
		const auto allocation = AllocateSmacEnergy( row.energy, row.rates, row.specialists );
		SmacEnergyTailInputs input{ allocation.after_specialists, row.rates, 4, 0, row.facilities };
		input.owns_virtual_world = row.virtual_world;
		const auto tail = FinishSmacEnergy( input );
		if ( !( allocation.allocation == row.expected.allocation ) || !( allocation.after_specialists == row.expected.after_specialists )
			|| tail.multiplier_quarters.psych != row.quarters || tail.final.psych != row.psych ) { return row.id; }
	}
	return {};
}

std::string CheckSmacEnergyTail() {
	for ( const auto& row : smac_energy_fixture::tail ) {
		const auto result = FinishSmacEnergy( row.input );
		if ( !( result.multiplier_quarters == row.expected.multiplier_quarters ) || !( result.before_penalty == row.expected.before_penalty )
			|| !( result.after_penalty == row.expected.after_penalty ) || !( result.final == row.expected.final ) ) { return row.id; }
	}
	return {};
}

std::string CheckSmacEnergyBoundaries() {
	if ( !Rejects( [] { AllocateSmacEnergy( 1, { 6, 5 }, {} ); } ) || !Rejects( [] { AllocateSmacEnergy( 1, { 0, -1 }, {} ); } )
		|| !Rejects( [] { AllocateSmacEnergy( 0, { 0, 5 }, std::vector< SmacEnergyCategories >( 17, { 0, 0, 0 } ) ); } )
		|| !Rejects( [] { AllocateSmacEnergy( 1, { 0, 5 }, { { 0, -1, 0 } } ); } ) ) { return "invalid allocation admitted"; }
	if ( !( AllocateSmacEnergy( INT32_MAX, { 0, 0 }, { { 3, 2, 4 } } ).after_specialists == SmacEnergyCategories{ 10002, 2, 4 } ) ) {
		return "specialists incorrectly included in allocation cap";
	}
	if ( CalculateSmacEnergyWaste( INT32_MAX, 0, INT32_MAX, true ) != 0 || CalculateSmacEnergyWaste( -1, 16, INT32_MAX, true ) != 0 ) {
		return "unevaluated waste arithmetic rejected";
	}
	if ( !Rejects( [] { CalculateSmacEnergyWaste( INT32_MAX, 16, 0, false ); } )
		|| !Rejects( [] { SmacHeadquartersDistance( 64, true, { 64, 0 }, {} ); } )
		|| !Rejects( [] { SmacHeadquartersDistance( 64, false, { 0, INT32_MAX }, { { 0, 0 } } ); } ) ) { return "invalid geometry or evaluated overflow admitted"; }
	SmacEnergyTailInputs input{ { INT32_MAX, 0, 0 }, { 0, 5 }, 0, 0 };
	if ( !Rejects( [ & ] { FinishSmacEnergy( input ); } ) ) { return "unmeasured tail overflow admitted"; }
	input.categories = { 0, 0, 0 }; input.current_stockpile = input.baseline_stockpile = true; input.mineral_surplus = INT32_MAX;
	if ( !Rejects( [ & ] { FinishSmacEnergy( input ); } ) ) { return "stockpile increment overflow admitted"; }
	return {};
}

void AddSmacBaseEnergyTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC Base energy waste matches original headquarters and commerce cases", GT() {
		const auto error = CheckSmacEnergyWaste(); GT_ASSERT( error.empty(), "energy check: " + error ); GT_OK();
	} );
	task->AddTest( "SMAC Base energy allocation matches original rounding and specialist cases", GT() {
		const auto error = CheckSmacEnergyAllocation(); GT_ASSERT( error.empty(), "energy check: " + error ); GT_OK();
	} );
	task->AddTest( "SMAC Base energy tail matches original multipliers penalties and Stockpile", GT() {
		const auto error = CheckSmacEnergyTail(); GT_ASSERT( error.empty(), "energy check: " + error ); GT_OK();
	} );
	task->AddTest( "SMAC Base energy rejects unsupported arithmetic and preserves cap ordering", GT() {
		const auto error = CheckSmacEnergyBoundaries(); GT_ASSERT( error.empty(), "energy check: " + error ); GT_OK();
	} );
}
}
