#pragma once

#include <string>
#include <variant>

#include "gse/Wrappable.h"
#include "game/backend/MapObject.h"

#include "Types.h"
#include "ConventionalUnitState.h"
#include "SmacNativeUnitRecord.h"

#include "types/Buffer.h"

namespace game {
namespace backend {

namespace slot {
class Slot;
}
namespace map::tile {
class Tile;
}

namespace base { class BaseManager; }

namespace unit {

class Def;
class UnitManager;

class Unit : public gse::Wrappable, public MapObject {
public:

	static const size_t GetNextId();
	static const void SetNextId( const size_t id );

	Unit(
		GSE_CALLABLE,
		UnitManager* um,
		const size_t id,
		Def* def,
		slot::Slot* owner,
		map::tile::Tile* tile,
		const movement_t movement,
		const morale_t morale,
		const health_t health,
		const bool moved_this_turn,
		bool record_placement = true
	);
	virtual ~Unit() = default;

	const size_t m_id;
	Def* m_def;
	slot::Slot* m_owner;

	struct LegacyState { movement_t movement; morale_t morale; health_t health; bool moved_this_turn; };
	struct Presentation { movement_t movement; morale_t morale; health_t health; movement_t full_movement = 0; };
	bool IsConventional() const;
	bool IsNative() const;
	const SmacNativeUnitRecord& Native() const;
	int32_t SimulationOwner() const;
	bool IsConventionalPending() const;
	void RequireConventionalReady() const;
	LegacyState& Legacy();
	const LegacyState& Legacy() const;
	const ConventionalUnitState& Conventional() const;
	ConventionalUnitStats QueryStats( SmacMoraleMode mode ) const;
	Presentation GetPresentation() const;

	size_t m_animation_id = 0;

	static const movement_t MINIMUM_MOVEMENT_TO_KEEP;
	static const movement_t MINIMUM_HEALTH_TO_KEEP;
	const bool HasMovesLeft() const;

	const std::string& GetMoraleString() const;

	void SetTile( GSE_CALLABLE, map::tile::Tile* tile );

	static const types::Buffer Serialize( const Unit* unit );
	static Unit* Deserialize( GSE_CALLABLE, types::Buffer& buf, UnitManager* um, const ConventionalUnitResolver& resolver = {}, bool defer_home = false );

	WRAPDEFS_DYNAMIC( Unit );

	WRAPDEF_SERIALIZABLE;

private:
	friend class UnitManager;
	friend class base::BaseManager; // staged home repair before canonical Base removal
	struct Unplaced {};
	Unit( Unplaced, UnitManager* um, size_t id, Def* def, slot::Slot* owner, ConventionalUnitState state );
	Unit( Unplaced, UnitManager* um, size_t id, Def* def, map::tile::Tile* tile, SmacNativeUnitRecord state );
	Unit( GSE_CALLABLE, UnitManager* um, size_t id, Def* def, map::tile::Tile* tile, SmacNativeUnitRecord state );
	void CommitInitialPlacement( map::tile::Tile* tile ) noexcept;
	Unit( GSE_CALLABLE, UnitManager* um, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, ConventionalUnitState state, bool early_startup = false, bool record_placement = true, bool pending_home = false );
	void Publish( GSE_CALLABLE, map::tile::Tile* tile, bool record_placement );
	void Place( GSE_CALLABLE, map::tile::Tile* tile, bool record_placement );
	// Called only after manager validation and membership/update allocation.
	void CommitConventionalMove( const ConventionalUnitState& state, map::tile::Tile* tile ) noexcept;
	std::variant< LegacyState, ConventionalUnitState, SmacNativeUnitRecord > m_state;
	UnitManager* const m_um;
	// Transient and deliberately not serializable. Only the claimed new-world
	// Start producer may create it; the first-turn preflight seals all at once.
	bool m_early_startup = false;
	bool m_pending_home = false; // restored scalar ID awaiting all Base records

};

}
}
}
