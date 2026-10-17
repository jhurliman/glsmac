#include "SmacProductionSwitch.h"

#include "game/backend/base/SmacProductionSwitch.h"
#include "task/gsetests/GSETests.h"
#include "fixtures/SmacProductionSwitch.h"

#include <array>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Switch = SmacProductionSwitch;
using Kind = ProductionItemKind;
using Choice = SmacPenaltyChoice;

ProductionItem Item( int32_t id ) {
	if ( id >= 0 ) { return { Kind::Unit, std::to_string( id ) }; }
	if ( id == -38 ) { return { Kind::Stockpile, "" }; }
	return { id <= -39 ? Kind::Project : id <= -34 ? Kind::Orbital : Kind::Facility, std::to_string( -id ) };
}

SmacProductionSwitchContext Context() { return { 2, 50, 10, true, 2, false, {} }; }
ProductionState State() { return { Item( 0 ), 30, Item( 0 ), 30 }; }

template< typename F > bool Rejects( F&& callback ) {
	try { callback(); }
	catch ( const std::invalid_argument& ) { return true; }
	return false;
}
}

void AddSmacProductionSwitchTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test SMAC switching named original state decision and inquiry cases", GT() {
		for ( const auto& fixture : fixtures::production_switch::cases ) {
			auto context = Context();
			context.human = fixture.human; context.stored_difficulty = fixture.difficulty;
			context.strictness = fixture.strictness; context.loss_percent = fixture.percent; context.exemption = fixture.exemption;
			for ( const auto facility : fixture.facilities ) {
				context.present_local_facilities.insert( std::to_string( facility ) );
				if ( facility == 10 ) { context.skunkworks = true; }
			}
			ProductionState state{ Item( fixture.current_item ), fixture.current_minerals, Item( fixture.baseline_item ), fixture.baseline_minerals };
			for ( const auto& step : fixture.steps ) {
				const auto before = state;
				if ( fixture.manual ) {
					GT_ASSERT( step.target.has_value(), "manual fixture has no target" );
					const auto result = Switch::Manual( state, Item( *step.target ), fixture.accept ? Choice::Accept : Choice::Cancel, context );
					GT_ASSERT( state == before, "manual calculation mutated its input" );
					GT_ASSERT( result.penalty_prompt_required == step.prompt && result.penalty_cancelled == step.cancelled, "manual decision mismatch in " + std::string( fixture.id ) );
					state = result.state;
				}
				else {
					const auto target = step.target ? std::optional< ProductionItem >( Item( *step.target ) ) : std::nullopt;
					const auto result = Switch::Automatic( state, target, context );
					GT_ASSERT( state == before, "automatic calculation mutated its input" );
					GT_ASSERT( result.refresh_requested == step.refresh, "automatic refresh mismatch in " + std::string( fixture.id ) );
					state = result.state;
				}
				const ProductionState expected{ Item( step.item ), step.minerals, Item( step.baseline_item ), step.baseline_minerals };
				GT_ASSERT( state == expected, "state mismatch in " + std::string( fixture.id ) );
				GT_ASSERT( Switch::BaselineLoss( state, context ) == step.loss, "inquiry mismatch in " + std::string( fixture.id ) );
			}
		}
		GT_OK();
	} );
	task->AddTest( "test SMAC switching semantic categories and existing facility override", GT() {
		const std::array< int32_t, 10 > ids{ 0, 1, -1, -33, -34, -37, -38, -39, -40, -41 };
		const int groups[4][10] = { {0,0,0,0,0,0,0,0,0,0}, {0,0,1,1,2,2,2,3,3,3}, {0,1,2,3,4,5,6,7,7,7}, {0,1,2,3,4,5,6,7,8,9} };
		for ( int strictness = 0; strictness < 4; ++strictness ) {
			for ( const bool skunkworks : { false, true } ) {
				auto context = Context(); context.strictness = strictness; context.skunkworks = skunkworks;
				const auto row = skunkworks && strictness > 0 ? 1 : strictness;
				for ( size_t a = 0; a < ids.size(); ++a ) {
					for ( size_t b = 0; b < ids.size(); ++b ) {
						const ProductionState state{ Item( ids[a] ), 30, Item( ids[a] ), 30 };
						const auto result = Switch::Automatic( state, Item( ids[b] ), context );
						GT_ASSERT( result.state.minerals == ( groups[row][a] == groups[row][b] ? 30 : 20 ), "category boundary or Skunkworks mismatch" );
					}
				}
				context.present_local_facilities.insert( "1" );
				const ProductionState project{ Item( -39 ), 30, Item( -40 ), 30 };
				GT_ASSERT( Switch::Automatic( project, Item( -1 ), context ).state.minerals == 30, "negative-item exception did not precede strictness" );
				GT_ASSERT( Switch::Automatic( State(), Item( -1 ), context ).state.minerals == 20, "unit entered existing-facility exception" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "test SMAC switching manual choice and inquiry are separate from automatic mutation", GT() {
		auto context = Context(); auto state = State();
		state.minerals = 7;
		GT_ASSERT( Switch::Automatic( state, state.current_item, context ).state.minerals == 7, "same automatic item recalculated" );
		GT_ASSERT( Switch::Manual( state, state.current_item, Choice::Cancel, context ).state.minerals == 30, "nonprompting manual path consumed cancel choice" );
		context.stored_difficulty = std::numeric_limits< int32_t >::min();
		const auto easy = Switch::Manual( state, Item( 1 ), Choice::Cancel, context );
		GT_ASSERT( easy.state.minerals == 30 && !easy.penalty_prompt_required && !easy.penalty_cancelled, "signed difficulty exemption changed" );
		GT_ASSERT( Switch::BaselineLoss( easy.state, context ) == 10, "inquiry incorrectly used manual difficulty exemption" );
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == 20, "automatic path used manual difficulty exemption" );
		context = Context(); state.current_item = Item( 1 ); state.minerals = 9;
		const auto repeat = Switch::Manual( state, Item( 2 ), Choice::Accept, context );
		GT_ASSERT( repeat.state.minerals == 9 && !repeat.penalty_prompt_required, "manual change compounded or recalculated a prior penalty" );
		GT_ASSERT( Switch::Automatic( state, Item( 2 ), context ).state.minerals == 20, "automatic change did not use baseline" );
		context.human = false;
		GT_ASSERT( Switch::Automatic( state, Item( 2 ), context ).state.minerals == 30 && Switch::BaselineLoss( state, context ) == 0, "AI automatic exemption changed" );
		state = State();
		GT_ASSERT( Switch::Manual( state, Item( 1 ), Choice::Accept, context ).penalty_prompt_required, "manual path invented a human-only penalty gate" );
		GT_OK();
	} );
	task->AddTest( "test SMAC switching signed storage and checked branch arithmetic", GT() {
		constexpr auto minimum = std::numeric_limits< int32_t >::min();
		constexpr auto maximum = std::numeric_limits< int32_t >::max();
		auto context = Context(); auto state = State();
		state.minerals = minimum; state.baseline_minerals = minimum;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == minimum, "negative balance was clamped" );
		GT_ASSERT( Switch::Manual( state, Item( 1 ), Choice::Accept, context ).state.minerals == minimum, "manual negative balance was rejected" );
		context.exemption = minimum; state.baseline_minerals = minimum + 1;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == minimum, "negative exemption rounding changed" );
		state.baseline_minerals = maximum; context.exemption = 0;
		GT_ASSERT( Rejects( [&]() { Switch::Automatic( state, Item( 1 ), context ); } ), "signed32 product overflow accepted" );
		GT_ASSERT( Rejects( [&]() { Switch::Manual( state, Item( 1 ), Choice::Cancel, context ); } ), "cancel bypassed arithmetic evaluated before the original prompt" );
		GT_ASSERT( Switch::Automatic( state, std::nullopt, context ).state == state, "keep-current evaluated unused penalty arithmetic" );
		GT_ASSERT( Switch::Automatic( state, state.current_item, context ).state == state, "same-item evaluated unused penalty arithmetic" );
		context.human = false;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == maximum, "AI evaluated unused penalty arithmetic" );
		context.human = true; context.loss_percent = 100;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == 0, "full penalty overflowed representable excess/product" );
		context.exemption = minimum;
		GT_ASSERT( Rejects( [&]() { Switch::Automatic( state, Item( 1 ), context ); } ), "overflowing excess hidden by zero multiplier" );
		context.loss_percent = 0;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == maximum, "zero loss evaluated excess" );
		context = Context(); context.exemption = 0; state.baseline_minerals = maximum / 50;
		GT_ASSERT( Switch::Automatic( state, Item( 1 ), context ).state.minerals == ( maximum / 50 ) * int64_t( 50 ) / 100, "last representable product rejected" );
		state.baseline_minerals++;
		GT_ASSERT( Rejects( [&]() { Switch::Automatic( state, Item( 1 ), context ); } ), "first overflowing product accepted" );
		GT_OK();
	} );
	task->AddTest( "test SMAC switching validates all structural inputs without mutation", GT() {
		const auto original = State(); auto context = Context();
		for ( const int64_t balance : { int64_t( std::numeric_limits< int32_t >::min() ) - 1, int64_t( std::numeric_limits< int32_t >::max() ) + 1 } ) {
			for ( const bool baseline : { false, true } ) {
				auto state = original; ( baseline ? state.baseline_minerals : state.minerals ) = balance;
				GT_ASSERT( Rejects( [&]() { Switch::Automatic( state, std::nullopt, context ); } ), "out-of-domain storage accepted even on no-op" );
				GT_ASSERT( Rejects( [&]() { Switch::Manual( state, Item( 1 ), Choice::Accept, context ); } ), "manual out-of-domain storage accepted" );
				GT_ASSERT( Rejects( [&]() { Switch::BaselineLoss( state, context ); } ), "inquiry out-of-domain storage accepted" );
			}
		}
		for ( const auto invalid : { ProductionItem{ static_cast< Kind >( 0 ), "x" }, ProductionItem{ Kind::Unit, "" }, ProductionItem{ Kind::Stockpile, "x" } } ) {
			GT_ASSERT( Rejects( [&]() { Switch::Automatic( original, invalid, context ); } ), "invalid target accepted" );
			auto state = original; state.baseline_item = invalid;
			GT_ASSERT( Rejects( [&]() { Switch::BaselineLoss( state, context ); } ), "invalid baseline accepted" );
		}
		for ( const int invalid : { -1, 4 } ) { auto c = context; c.strictness = invalid; GT_ASSERT( Rejects( [&]() { Switch::Automatic( original, std::nullopt, c ); } ), "invalid strictness accepted" ); }
		for ( const int invalid : { -1, 101 } ) { auto c = context; c.loss_percent = invalid; GT_ASSERT( Rejects( [&]() { Switch::Automatic( original, std::nullopt, c ); } ), "invalid percentage accepted" ); }
		context.present_local_facilities.insert( "" );
		GT_ASSERT( Rejects( [&]() { Switch::BaselineLoss( original, context ); } ), "empty facility metadata accepted" );
		GT_ASSERT( Rejects( [&]() { Switch::Manual( original, Item( 1 ), static_cast< Choice >( 2 ), Context() ); } ), "invalid choice accepted" );
		GT_ASSERT( original == State(), "rejected input was mutated" );
		GT_OK();
	} );
	task->AddTest( "test SMAC switching owns opaque identities and preserves independent baselines", GT() {
		const std::string opaque( "id\0tail", 7 );
		ProductionState state{ { Kind::Unit, opaque }, 9, { Kind::Project, opaque }, 30 };
		auto target = ProductionItem{ Kind::Facility, opaque };
		auto context = Context(); context.strictness = 3; context.present_local_facilities.insert( opaque );
		const auto before = state;
		auto result = Switch::Automatic( state, target, context );
		GT_ASSERT( result.state.minerals == 30, "opaque present-facility membership changed" );
		GT_ASSERT( result.state.baseline_item == before.baseline_item && result.state.baseline_minerals == 30, "baseline was replaced" );
		state.current_item.id = "changed"; state.baseline_item.id = "changed"; target.id = "changed";
		GT_ASSERT( result.state.current_item.id == opaque && result.state.baseline_item.id == opaque, "result borrowed an input identity" );
		result.state.current_item.id = "result changed";
		GT_ASSERT( before.current_item.id == opaque && before.baseline_item.id == opaque, "result changed an independent copy" );
		GT_OK();
	} );
}
}
