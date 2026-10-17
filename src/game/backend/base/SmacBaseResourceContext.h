#pragma once

#include <cstdint>
#include <optional>
#include "types/Buffer.h"

namespace game::backend::base {

// Persistent inputs belonging to one canonical Base. These are not a cached
// yield, a social rating, a population allocation, or a governor decision.
struct SmacBaseResourceContext {
	static constexpr uint32_t MAX_WIRE_BYTES = 5 * 14 + 3 * 7;
	static constexpr uint32_t GOVERNOR_ACTIVE = 0x80000000u;
	static constexpr uint32_t MANAGE_CITIZENS = 0x40u;
	enum Event : uint8_t {
		BumperCrop = 1, Famine = 2, IndustrialBoom = 4,
		IndustrialBust = 8, HeatWave = 16, CloudCover = 32,
	};

	int32_t owner_simulation_id;
	bool golden_age;
	uint8_t resource_events;
	int32_t resource_event_timer;
	uint32_t governor_options; // one authority; preserve all supplied bits
	bool drone_riots; // saved actual status; queries never derive it from population

	void Validate() const;
	bool GovernorActive() const { return ( governor_options & GOVERNOR_ACTIVE ) != 0; }
	bool ManageCitizens() const { return ( governor_options & MANAGE_CITIZENS ) != 0; }
	bool HasEvent( Event event ) const { return ( resource_events & event ) != 0; }
	bool operator==( const SmacBaseResourceContext& other ) const;
};

// Port V2 inner record (adds drone-riot status). Absent means legacy/unavailable, never neutral SMAC.
types::Buffer SerializeSmacBaseResourceContext( const std::optional< SmacBaseResourceContext >& value );
std::optional< SmacBaseResourceContext > DeserializeSmacBaseResourceContext( types::Buffer buffer );

}
