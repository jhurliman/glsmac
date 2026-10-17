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

bool ConventionalUnitState::operator==( const ConventionalUnitState& other ) const {
	return design_id == other.design_id && owner_simulation_id == other.owner_simulation_id
		&& stored_morale == other.stored_morale && damage == other.damage && spent == other.spent
		&& failed_move == other.failed_move && home == other.home && home_base_id == other.home_base_id && render_morale_mode == other.render_morale_mode;
}

void ConventionalUnitState::Validate() const {
	if ( design_id < 0 || design_id >= 512 || owner_simulation_id < 1 || owner_simulation_id > 7
		|| ( home != Home::None && home != Home::Base )
		|| ( home == Home::Base ) != home_base_id.has_value()
		|| ( home_base_id && ( !*home_base_id || *home_base_id >= uint64_t( INT64_MAX ) || *home_base_id >= SIZE_MAX ) ) || ( render_morale_mode != SmacMoraleMode::Mode0 && render_morale_mode != SmacMoraleMode::Mode1 ) ) {
		throw std::runtime_error( "Unsupported conventional unit identity, home or render morale mode" );
	}
}

void ConventionalUnitState::Serialize( types::Buffer& buffer ) const {
	Validate();
	buffer.WriteInt( design_id ); buffer.WriteInt( owner_simulation_id );
	buffer.WriteInt( stored_morale ); buffer.WriteInt( damage ); buffer.WriteInt( spent );
	buffer.WriteBool( failed_move ); buffer.WriteInt( static_cast< int32_t >( home ) );
	buffer.WriteInt( static_cast< int32_t >( render_morale_mode ) );
	if ( home_base_id ) { buffer.WriteInt( static_cast< int64_t >( *home_base_id ) ); }
}

ConventionalUnitState ConventionalUnitState::Deserialize( types::Buffer& buffer ) {
	const auto design = Integer( buffer, 0, 511 ), owner = Integer( buffer, 1, 7 );
	const auto morale = Integer( buffer, 0, 255 ), damage = Integer( buffer, 0, 255 ), spent = Integer( buffer, 0, 255 );
	const bool failed = buffer.ReadBool();
	const auto home = Integer( buffer, 0, 1 ), mode = Integer( buffer, 0, 1 );
	std::optional< uint64_t > home_id;
	if ( home ) { const auto id = buffer.ReadInt(); if ( id <= 0 ) { throw std::runtime_error( "Invalid conventional home ID" ); } home_id = static_cast< uint64_t >( id ); }
	ConventionalUnitState result{ design, owner, static_cast< uint8_t >( morale ), static_cast< uint8_t >( damage ), static_cast< uint8_t >( spent ),
		failed, static_cast< Home >( home ), static_cast< SmacMoraleMode >( mode ), home_id };
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
		std::max( full - static_cast< int32_t >( state.spent ), 0 ), 10 * context.reactor - state.damage, 10 * context.reactor };
}

}
