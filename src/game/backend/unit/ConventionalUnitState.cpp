#include "ConventionalUnitState.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t Integer( types::Buffer& buffer, int32_t minimum, int32_t maximum ) {
	const auto value = buffer.ReadInt();
	if ( value < minimum || value > maximum ) { throw std::runtime_error( "Conventional unit integer outside field domain" ); }
	return static_cast< int32_t >( value );
}
}

void ConventionalUnitState::Stack::Validate() const {
    if(id<0 || id>=2048 || next < -1 || next>=2048 || previous < -1 || previous>=2048 || next==id || previous==id) {
        throw std::runtime_error("Invalid conventional source-pool stack binding");
    }
}
ConventionalUnitState::Stack ConventionalUnitState::Stack::AfterUnlinkedIndexErasure(int32_t removed) const {
    Validate();
    if(removed<0 || removed>=2048 || id==removed || next==removed || previous==removed) {
        throw std::runtime_error("Conventional stack erasure requires an already-unlinked survivor");
    }
    auto result=*this;if(result.id>removed) {--result.id;}if(result.next>removed) {--result.next;}if(result.previous>removed) {--result.previous;}
    result.Validate();return result;
}

bool ConventionalUnitState::operator==( const ConventionalUnitState& other ) const {
	return design_id == other.design_id && owner_simulation_id == other.owner_simulation_id
		&& stored_morale == other.stored_morale && damage == other.damage && spent == other.spent
		&& failed_move == other.failed_move && home == other.home && home_base_id == other.home_base_id && render_morale_mode == other.render_morale_mode
		&& work_order == other.work_order && work_progress == other.work_progress
        && order == other.order && order_countdown == other.order_countdown
		&& stack==other.stack && retained_state_word==other.retained_state_word && accounting_markers==other.accounting_markers;
}

uint8_t ConventionalUnitState::AccountingMarkers(uint32_t word) noexcept {
	return uint8_t((word&PAID_SUPPORT ? 1:0) | (word&PACIFISM_PENALIZED ? 2:0) | (word&PACIFISM_EXEMPT ? 4:0));
}
void ConventionalUnitState::SetAccountingMarkers(uint8_t markers) {
	if(markers>7) {throw std::invalid_argument("Invalid conventional accounting markers");}
	accounting_markers=markers;
	if(retained_state_word) {
		*retained_state_word=(*retained_state_word & ~(PAID_SUPPORT|PACIFISM_PENALIZED|PACIFISM_EXEMPT))
			| (markers&1 ? PAID_SUPPORT:0u) | (markers&2 ? PACIFISM_PENALIZED:0u) | (markers&4 ? PACIFISM_EXEMPT:0u);
	}
}
void ConventionalUnitState::AdvanceOrderUpkeep() noexcept {
    if(IsHeld() && order_countdown != 0) {
        // Original word arithmetic, including inherited negative counters.
        order_countdown = order_countdown == INT16_MIN ? INT16_MAX : int16_t(order_countdown - 1);
        if(order_countdown == 0) {order = Order::None;}
    }
}
void ConventionalUnitState::Validate() const {
    if(stack) {stack->Validate();}
    if ((order != Order::None && order != Order::Hold) || (IsHeld() && work_order != WorkOrder::None)) {
        throw std::runtime_error("Invalid conventional general/work order combination");
    }
	if(accounting_markers && (*accounting_markers>7 || (retained_state_word && AccountingMarkers(*retained_state_word)!=*accounting_markers))) {
		throw std::runtime_error("Inconsistent conventional accounting history");
	}
	if ( work_order > WorkOrder::Mine || design_id < 0 || design_id >= 512 || owner_simulation_id < 1 || owner_simulation_id > 7
		|| ( home != Home::None && home != Home::Base )
		|| ( home == Home::Base ) != home_base_id.has_value()
		|| ( home_base_id && ( !*home_base_id || *home_base_id >= uint64_t( INT64_MAX ) || *home_base_id >= SIZE_MAX ) ) || ( render_morale_mode != SmacMoraleMode::Mode0 && render_morale_mode != SmacMoraleMode::Mode1 ) ) {
		throw std::runtime_error( "Unsupported conventional unit identity, home or render morale mode" );
	}
}

void ConventionalUnitState::Serialize( types::Buffer& buffer, bool include_state_history, bool include_orders, bool include_stack ) const {
	Validate();
    if(!include_stack && stack) {throw std::runtime_error("Cannot discard retained stack binding in legacy unit codec");}
    if(!include_orders && HasOrderState()) {throw std::runtime_error("Cannot discard retained orders in legacy unit codec");}
	buffer.WriteInt( design_id ); buffer.WriteInt( owner_simulation_id );
	buffer.WriteInt( stored_morale ); buffer.WriteInt( damage ); buffer.WriteInt( spent );
	buffer.WriteBool( failed_move ); buffer.WriteInt( static_cast< int32_t >( home ) );
	buffer.WriteInt( static_cast< int32_t >( render_morale_mode ) );
	if ( home_base_id ) { buffer.WriteInt( static_cast< int64_t >( *home_base_id ) ); }
	buffer.WriteInt( static_cast< int32_t >( work_order ) ); buffer.WriteInt( work_progress );
	if(include_state_history) {
		buffer.WriteBool(retained_state_word.has_value());if(retained_state_word) {buffer.WriteInt(*retained_state_word);}
		buffer.WriteBool(accounting_markers.has_value());if(accounting_markers) {buffer.WriteInt(*accounting_markers);}
	}
    if(include_orders) {buffer.WriteInt(static_cast<int32_t>(order));buffer.WriteInt(order_countdown);}
    if(include_stack) {buffer.WriteBool(stack.has_value());if(stack) {buffer.WriteInt(stack->id);buffer.WriteInt(stack->next);buffer.WriteInt(stack->previous);}}
}

ConventionalUnitState ConventionalUnitState::Deserialize( types::Buffer& buffer, bool include_state_history, bool include_orders, bool include_stack ) {
	const auto design = Integer( buffer, 0, 511 ), owner = Integer( buffer, 1, 7 );
	const auto morale = Integer( buffer, 0, 255 ), damage = Integer( buffer, 0, 255 ), spent = Integer( buffer, 0, 255 );
	const bool failed = buffer.ReadBool();
	const auto home = Integer( buffer, 0, 1 ), mode = Integer( buffer, 0, 1 );
	std::optional< uint64_t > home_id;
	if ( home ) { const auto id = buffer.ReadInt(); if ( id <= 0 ) { throw std::runtime_error( "Invalid conventional home ID" ); } home_id = static_cast< uint64_t >( id ); }
	ConventionalUnitState result{ design, owner, static_cast< uint8_t >( morale ), static_cast< uint8_t >( damage ), static_cast< uint8_t >( spent ),
		failed, static_cast< Home >( home ), static_cast< SmacMoraleMode >( mode ), home_id };
	result.work_order = static_cast< WorkOrder >( Integer( buffer, 0, static_cast< int32_t >( WorkOrder::Mine ) ) );
	result.work_progress = static_cast< uint8_t >( Integer( buffer, 0, 255 ) );
	if(include_state_history) {
		if(buffer.ReadBool()) {
			const auto word=buffer.ReadInt();if(word<0 || uint64_t(word)>UINT32_MAX) {throw std::runtime_error("Invalid conventional full state word");}
			result.retained_state_word=static_cast<uint32_t>(word);
		}
		if(buffer.ReadBool()) {result.accounting_markers=static_cast<uint8_t>(Integer(buffer,0,7));}
	}
    if(include_orders) {
        result.order = static_cast<Order>(Integer(buffer,0,2));
        result.order_countdown = static_cast<int16_t>(Integer(buffer,INT16_MIN,INT16_MAX));
    }
    if(include_stack && buffer.ReadBool()) {result.stack=Stack{int16_t(Integer(buffer,0,2047)),int16_t(Integer(buffer,-1,2047)),int16_t(Integer(buffer,-1,2047))};}
	result.Validate(); return result;
}

ConventionalUnitStats QueryConventionalUnit( const ConventionalUnitState& state, const ConventionalUnitContext& context, const SmacMoraleMode mode ) {
	state.Validate();
	using Chassis = settings::SmacMovementRules::GroundChassis;
	if ( ( context.chassis != Chassis::Infantry && context.chassis != Chassis::Speeder )
		|| context.quantum < 1 || context.quantum > 100 || context.basic_allowance < 0 || context.basic_allowance > 9900
		|| ( state.home == ConventionalUnitState::Home::Base ) != context.home.has_value()
		|| !context.retained_morale || context.reactor < 1 || context.reactor > 4 || state.damage >= 10 * context.reactor ) {
		throw std::runtime_error( "Unsupported conventional living-unit context" );
	}
	// The producer validates the complete supported faction profile: direct
	// MORALE is absent (0/OFF), including raw records after the eight-slot cap.
	const auto morale = [ & ]( SmacMoraleMode requested ) {
		return SmacConventionalMorale( { state.stored_morale, *context.retained_morale, 0, false, context.home, requested } );
	};
	const auto allowance_morale = morale( SmacMoraleMode::Mode1 );
	const auto full = SmacConventionalAllowance( { context.basic_allowance, context.quantum, SmacMovementDomain::Land,
		context.reactor, state.damage, allowance_morale, false, false } );
	return { morale( mode ), allowance_morale, context.quantum, context.basic_allowance, full,
		std::max( full - static_cast< int32_t >( state.spent ), 0 ), 10 * context.reactor - state.damage, 10 * context.reactor,
		context.offense_rating, context.defense_rating };
}

}
