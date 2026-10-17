#pragma once

#include <string>
#include <variant>

#include "gse/Wrappable.h"
#include "game/backend/MapObject.h"

#include "Types.h"
#include "ConventionalUnitState.h"

#include "types/Buffer.h"

namespace game {
namespace backend {

namespace slot {
class Slot;
}
namespace map::tile {
class Tile;
}

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
		const bool moved_this_turn
	);
	virtual ~Unit() = default;

	const size_t m_id;
	Def* m_def;
	slot::Slot* m_owner;

	struct LegacyState { movement_t movement; morale_t morale; health_t health; bool moved_this_turn; };
	struct Presentation { movement_t movement; morale_t morale; health_t health; movement_t full_movement = 0; };
	bool IsConventional() const;
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
	static Unit* Deserialize( GSE_CALLABLE, types::Buffer& buf, UnitManager* um, const ConventionalUnitResolver& resolver = {} );

	WRAPDEFS_DYNAMIC( Unit );

	WRAPDEF_SERIALIZABLE;

private:
	friend class UnitManager;
	Unit( GSE_CALLABLE, UnitManager* um, size_t id, Def* def, slot::Slot* owner, map::tile::Tile* tile, ConventionalUnitState state );
	void Publish( GSE_CALLABLE, map::tile::Tile* tile );
	// Called only after manager validation and membership/update allocation.
	void CommitConventionalMove( const ConventionalUnitState& state, map::tile::Tile* tile ) noexcept;
	std::variant< LegacyState, ConventionalUnitState > m_state;
	UnitManager* const m_um;

};

}
}
}
