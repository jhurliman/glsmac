#include "SmacProductionSwitch.h"

#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {

int32_t Checked( int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::invalid_argument( "SMAC production switching requires signed32 arithmetic" );
	}
	return static_cast< int32_t >( value );
}

void ValidateItem( const ProductionItem& item ) {
	try { item.Validate(); }
	catch ( const std::runtime_error& error ) { throw std::invalid_argument( error.what() ); }
}

void Validate( const ProductionState& state, const SmacProductionSwitchContext& context ) {
	ValidateItem( state.current_item );
	ValidateItem( state.baseline_item );
	Checked( state.minerals );
	Checked( state.baseline_minerals );
	if ( context.strictness < 0 || context.strictness > 3 || context.loss_percent < 0 || context.loss_percent > 100 ) {
		throw std::invalid_argument( "Unsupported SMAC production switching rules" );
	}
	for ( const auto& id : context.present_local_facilities ) {
		if ( id.empty() ) { throw std::invalid_argument( "Present local facility requires an identity" ); }
	}
}

// Compare categories without encoding original numeric indices or borrowing IDs.
bool SameCategory( const ProductionItem& a, const ProductionItem& b, const ProductionItem& current, const SmacProductionSwitchContext& context ) {
	using Kind = ProductionItemKind;
	const auto a_unit = a.kind == Kind::Unit;
	const auto b_unit = b.kind == Kind::Unit;
	if ( current.kind == Kind::Facility && context.present_local_facilities.count( current.id ) ) {
		// The existing-facility exception precedes even strictness zero.
		if ( !a_unit || !b_unit ) { return !a_unit && !b_unit; }
	}
	const auto strictness = context.skunkworks && context.strictness > 0 ? 1 : context.strictness;
	switch ( strictness ) {
		case 0: return true;
		case 1:
			if ( a_unit || b_unit ) { return a_unit && b_unit; }
			if ( a.kind == Kind::Project || b.kind == Kind::Project ) { return a.kind == b.kind; }
			if ( a.kind == Kind::Facility || b.kind == Kind::Facility ) { return a.kind == b.kind; }
			return true; // orbital and Stockpile share one category
		case 2:
			if ( a.kind == Kind::Project && b.kind == Kind::Project ) { return true; }
			[[fallthrough]];
		default: return a == b;
	}
}

int32_t RetainedFraction( int32_t baseline, const SmacProductionSwitchContext& context ) {
	const auto remaining_percent = Checked( int64_t( 100 ) - context.loss_percent );
	const auto excess = Checked( int64_t( baseline ) - context.exemption );
	const auto product = Checked( int64_t( excess ) * remaining_percent );
	return product / 100;
}

int32_t Loss( const ProductionState& state, const ProductionItem& current, const SmacProductionSwitchContext& context ) {
	if ( !context.human || context.loss_percent == 0 ||
		SameCategory( state.baseline_item, current, current, context ) || state.baseline_minerals <= context.exemption ) {
		return 0;
	}
	const auto baseline = Checked( state.baseline_minerals );
	const auto fraction = RetainedFraction( baseline, context );
	// Preserve the inquiry/setter's actual subtraction order before final storage.
	return Checked( int64_t( Checked( int64_t( baseline ) - fraction ) ) - context.exemption );
}

}

SmacAutomaticSwitchResult SmacProductionSwitch::Automatic( const ProductionState& state, const std::optional< ProductionItem >& target, const SmacProductionSwitchContext& context ) {
	Validate( state, context );
	if ( target ) { ValidateItem( *target ); }
	SmacAutomaticSwitchResult result{ state, false };
	if ( !target || *target == state.current_item ) { return result; }
	result.state.current_item = *target;
	result.state.minerals = Checked( state.baseline_minerals - Loss( state, *target, context ) );
	result.refresh_requested = true;
	return result;
}

SmacManualSwitchResult SmacProductionSwitch::Manual( const ProductionState& state, const ProductionItem& target, SmacPenaltyChoice choice, const SmacProductionSwitchContext& context ) {
	Validate( state, context );
	ValidateItem( target );
	if ( choice != SmacPenaltyChoice::Accept && choice != SmacPenaltyChoice::Cancel ) {
		throw std::invalid_argument( "Unknown SMAC penalty choice" );
	}
	SmacManualSwitchResult result{ state, false, false };
	if ( state.baseline_minerals != 0 ) {
		if ( SameCategory( target, state.baseline_item, state.current_item, context ) || context.stored_difficulty <= 1 ||
			context.loss_percent == 0 || state.baseline_minerals <= context.exemption || state.baseline_item.kind == ProductionItemKind::Stockpile ) {
			result.state.minerals = state.baseline_minerals;
		}
		else if ( SameCategory( state.current_item, state.baseline_item, state.current_item, context ) ) {
			const auto fraction = RetainedFraction( Checked( state.baseline_minerals ), context );
			const auto retained = Checked( int64_t( context.exemption ) + fraction );
			result.penalty_prompt_required = true;
			if ( choice == SmacPenaltyChoice::Cancel ) {
				result.penalty_cancelled = true;
				return result;
			}
			result.state.minerals = retained;
		}
	}
	result.state.current_item = target;
	return result;
}

int32_t SmacProductionSwitch::BaselineLoss( const ProductionState& state, const SmacProductionSwitchContext& context ) {
	Validate( state, context );
	return Loss( state, state.current_item, context );
}

}
