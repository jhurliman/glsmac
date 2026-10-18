#pragma once

#include <cstdint>
#include <optional>
#include "types/Buffer.h"
#include "SmacWorkerAllocationRules.h"

namespace game::backend::base {

struct SmacConquestState {
	int32_t former_owner;
	uint8_t assimilation;
	bool operator==(const SmacConquestState& other) const {return former_owner==other.former_owner && assimilation==other.assimilation;}
};

// Persistent inputs belonging to one canonical Base. These are not a cached
// yield, a social rating, a population allocation, or a governor decision.
struct SmacBaseResourceContext {
	static constexpr uint32_t MAX_WIRE_BYTES = 9 * 14 + 8 * 7;
	static constexpr uint32_t GOVERNOR_ACTIVE = 0x80000000u;
	static constexpr uint32_t MANAGE_CITIZENS = 0x40u;
	static constexpr uint32_t STARTUP_AUTOMATION = 0x40000000u;
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
	// Absent in ordinary never-captured port records and pre-capture V2 saves.
	// Native capture owns this history independently of current owner/size.
	std::optional<SmacConquestState> conquest;
	bool recent_defender_loss = false; // native status mask8; cleared at Base upkeep tail
	// Full native word, independent of named effects. Legacy records retain
	// absence; an update to known named bits must preserve every other bit.
	std::optional<uint32_t> retained_status_word;
	// Fresh original creation writes former=current/assimilation0. Keep this
	// independent of the port's first-capture capability marker above. Earlier
	// saves do not gain invented history; conquest is authoritative once present.
	std::optional<int32_t> uncaptured_former_owner;
	// Explicit port migration answer, never inferred from the current owner.
	bool former_owner_migration = false;
	SmacBaseResourceContext WithFormerOwnerRepair(int32_t former) const;
	std::optional<int32_t> FormerOwner() const noexcept {return conquest ? std::optional<int32_t>(conquest->former_owner):uncaptured_former_owner;}
	void SyncKnownStatusBits() noexcept;
	void SyncKnownHurryBit(bool paid) noexcept {if(retained_status_word) {*retained_status_word=(*retained_status_word&~0x40000000u)|(paid?0x40000000u:0u);}}

	void Validate() const;
	SmacBaseResourceContext AfterUpkeep() const;
	// Global startup difficulty precedes stored owner difficulty in the original.
	// Only actual fresh owner initialization produces this default. New Bases
	// copy the owner remembered template; saved Base options remain independent.
	static uint32_t NewGameGovernorOptions( int32_t global_difficulty );
	static constexpr std::array<uint32_t,19> GOVERNOR_CONTROL_MASKS{{
		0x80000000u,0x00200000u,0x00000080u,0x00000040u,0x00000001u,
		0x00400000u,0x00000200u,0x00000400u,0x00000800u,0x00001000u,
		0x00002000u,0x00080000u,0x00800000u,0x00100000u,0x00008000u,
		0x00020000u,0x00010000u,0x00040000u,0x00000020u
	}};
	static uint32_t GovernorControls(uint32_t options) noexcept;
	static uint32_t GovernorOptions(uint32_t previous,uint32_t controls);
	SmacWorkerPriorityContext WorkerPriorities( const std::array<int32_t,5>& faction ) const;
	bool GovernorActive() const { return ( governor_options & GOVERNOR_ACTIVE ) != 0; }
	bool ManageCitizens() const { return ( governor_options & MANAGE_CITIZENS ) != 0; }
	bool HasEvent( Event event ) const { return ( resource_events & event ) != 0; }
	bool operator==( const SmacBaseResourceContext& other ) const;
};

// Port V5 inner record (retains full status when known), with V2-V4 migration.
// Absent outer context means legacy/unavailable, never neutral SMAC.
types::Buffer SerializeSmacBaseResourceContext( const std::optional< SmacBaseResourceContext >& value );
std::optional< SmacBaseResourceContext > DeserializeSmacBaseResourceContext( types::Buffer buffer );

}
