#pragma once

#include <unordered_map>
#include <vector>
#include <set>
#include <array>
#include "types/Color.h"

#include "common/Common.h"

#include "game/backend/unit/Types.h"

#include "types/Vec2.h"
#include "types/Vec3.h"

namespace game::backend {
namespace unit {
class Def;
}

namespace base {
class PopDef;
}
}

namespace gse::tests { class BackendFixture; }
namespace types { class Font; }

namespace game {
namespace actor_visibility {struct BaseVisibility;}
namespace frontend {

class Game;
class Slot;

namespace sprite {
class InstancedSpriteManager;
}

namespace text {
class InstancedFont;
class InstancedText;
class InstancedTextManager;
}

namespace tile {
class Tile;
}

namespace faction {
class Faction;
}

namespace base {

class PopDef;
class Base;
class SlotBadges;

CLASS( BaseManager, common::Class )

	BaseManager( Game* game, bool smac_population = false );
	~BaseManager();

	base::Base* GetBaseById( const size_t id ) const;
    bool UsesNativeMapProjection() const;
    actor_visibility::BaseVisibility GetBaseVisibility(size_t id) const;
    void RefreshVisibility();

	void DefinePop( const backend::base::PopDef* def );
	void UndefinePop( const std::string& id );

	const std::vector< std::string >& GetPopDefOrder() const;
	PopDef* GetPopDef( const std::string& id ) const;

	void SpawnBase(
		const size_t base_id,
		const size_t slot_index,
		const ::types::Vec2< size_t >& tile_coords,
		const ::types::Vec3& render_coords,
		const std::string& name
	);
	void DespawnBase( const size_t base_id );
	void RefreshBase( Base* base );
	void UpdateBaseIdentity( Base* base, size_t owner_slot, faction::Faction* faction, const std::string& name );

	SlotBadges* GetSlotBadges( const size_t slot_index ) const;
	void DefineSlotBadges( const size_t slot_index, const faction::Faction* faction );

	void SelectBase( Base* base );
	Base* GetBaseBefore( Base* base ) const;
	Base* GetBaseAfter( Base* base ) const;

private:
	friend class Base;
	friend class gse::tests::BackendFixture;

private:
	friend class SlotBadges;
	text::InstancedFont* GetBadgeFont() const;
	const types::Font* GetSmacPopulationFont();
	const std::array<types::Color,256>& GetSmacPopulationPalette();
	text::InstancedText* CreateNameLabel(const faction::Faction* faction,const std::string& name);

private:

	Game* m_game;
	sprite::InstancedSpriteManager* m_ism;

	text::InstancedFont* m_name_font;
	text::InstancedFont* m_badge_font;
	bool m_smac_population = false;
	const types::Font* m_smac_population_font = nullptr;
	std::array<types::Color,256> m_smac_population_palette{};
	bool m_smac_population_palette_loaded = false;

	std::unordered_map< size_t, SlotBadges* > m_slot_badges = {};

	std::vector< std::string > m_popdefs_order = {};
	std::unordered_map< std::string, PopDef* > m_popdefs = {};

	std::unordered_map< size_t, base::Base* > m_bases = {};
	typedef std::set< size_t > ordered_base_ids_t;
	std::unordered_map< size_t, ordered_base_ids_t > m_owner_base_ids = {};

};

}
}
}
