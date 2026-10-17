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
		&& failed_move == other.failed_move && home == other.home && render_morale_mode == other.render_morale_mode;
}

void ConventionalUnitState::Validate() const {
	if ( design_id < 0 || design_id >= 512 || owner_simulation_id < 1 || owner_simulation_id > 7
		|| home != Home::None || ( render_morale_mode != SmacMoraleMode::Mode0 && render_morale_mode != SmacMoraleMode::Mode1 ) ) {
		throw std::runtime_error( "Unsupported conventional unit identity, home or render morale mode" );
	}
}

void ConventionalUnitState::Serialize( types::Buffer& buffer ) const {
	Validate();
	buffer.WriteInt( design_id ); buffer.WriteInt( owner_simulation_id );
	buffer.WriteInt( stored_morale ); buffer.WriteInt( damage ); buffer.WriteInt( spent );
	buffer.WriteBool( failed_move ); buffer.WriteInt( static_cast< int32_t >( home ) );
	buffer.WriteInt( static_cast< int32_t >( render_morale_mode ) );
}

ConventionalUnitState ConventionalUnitState::Deserialize( types::Buffer& buffer ) {
	const auto design = Integer( buffer, 0, 511 ), owner = Integer( buffer, 1, 7 );
	const auto morale = Integer( buffer, 0, 255 ), damage = Integer( buffer, 0, 255 ), spent = Integer( buffer, 0, 255 );
	const bool failed = buffer.ReadBool();
	const auto home = Integer( buffer, 0, 0 ), mode = Integer( buffer, 0, 1 );
	return { design, owner, static_cast< uint8_t >( morale ), static_cast< uint8_t >( damage ), static_cast< uint8_t >( spent ),
		failed, static_cast< Home >( home ), static_cast< SmacMoraleMode >( mode ) };
}

ConventionalUnitStats QueryConventionalUnit( const ConventionalUnitState& state, const ConventionalUnitContext& context, const SmacMoraleMode mode ) {
	state.Validate();
	using Chassis = settings::SmacMovementRules::GroundChassis;
	if ( ( context.chassis != Chassis::Infantry && context.chassis != Chassis::Speeder )
		|| context.quantum < 1 || context.quantum > 100 || context.basic_allowance < 0 || context.basic_allowance > 9900
		|| context.reactor < 1 || context.reactor > 4 || state.damage >= 10 * context.reactor ) {
		throw std::runtime_error( "Unsupported conventional living-unit context" );
	}
	// The producer validates the complete supported faction profile: direct
	// MORALE is absent (0/OFF), including raw records after the eight-slot cap.
	const auto morale = [ & ]( SmacMoraleMode requested ) {
		return SmacConventionalMorale( { state.stored_morale, context.retained_morale, 0, false, {}, requested } );
	};
	const auto allowance_morale = morale( SmacMoraleMode::Mode1 );
	const auto full = SmacConventionalAllowance( { context.basic_allowance, context.quantum, SmacMovementDomain::Land,
		context.reactor, state.damage, allowance_morale, false, false } );
	return { morale( mode ), allowance_morale, context.quantum, context.basic_allowance, full,
		std::max( full - static_cast< int32_t >( state.spent ), 0 ), 10 * context.reactor - state.damage, 10 * context.reactor };
}

}
