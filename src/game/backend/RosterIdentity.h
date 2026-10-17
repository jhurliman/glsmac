#pragma once
#include <atomic>
#include <memory>
namespace game::backend {
class State;
// Identity only: access still requires owner accumulation/native exclusion.
struct RosterEpoch {
	explicit RosterEpoch( State* state )
		: state( state ) {}
	State* const state;
	std::atomic< bool > active{ true };
};
using roster_epoch_t = std::shared_ptr< const RosterEpoch >;
struct PlayerIdentity {
	std::atomic< bool > active{ true };
};
using player_identity_t = std::shared_ptr< const PlayerIdentity >;
}// namespace game::backend
