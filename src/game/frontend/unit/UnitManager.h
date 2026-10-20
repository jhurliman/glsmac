#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <atomic>

#include "common/Common.h"

#include "game/backend/unit/Types.h"
#include "game/UnitVisibility.h"

#include "types/Vec2.h"
#include "types/Vec3.h"

namespace game::backend::unit {
class Def;
}
namespace gse::tests { class BackendFixture; }

namespace game {
namespace frontend {

class Game;

namespace sprite {
class InstancedSpriteManager;
}

namespace tile {
class Tile;
}

namespace faction {
class Faction;
}

namespace unit {

class Unit;
class UnitDef;
class BadgeDefs;
class SlotBadges;

CLASS( UnitManager, common::Class )

	UnitManager( Game* game );
	~UnitManager();

	void Iterate();
    int GetNativeBodyZoom() const {return m_native_body_zoom;}
    void PrepareNativeBodyZoom(int zoom);
    void SetNativeBodyZoom(int zoom);
    int GetStackChassisOffset(const Unit* unit) const;
    bool IsSelectionBlinkAdmitted(const Unit* unit) const;
    const actor_visibility::UnitPlan* GetTilePresentation(const tile::Tile& tile) const;
    bool IsBodyAdmitted(const Unit* unit) const;
    bool IsStackBadgeAdmitted(const Unit* unit) const;
    void RefreshVisibility();

	Unit* GetUnitById( const size_t id ) const;

	void DefineUnit( const backend::unit::Def* def );
	void UndefineUnit( const std::string& id );
	void SpawnUnit(
		const size_t unit_id,
		const std::string& unitdef_name,
		const size_t slot_index,
		const ::types::Vec2< size_t >& tile_coords,
		const ::types::Vec3& render_coords,
		const backend::unit::movement_t movement,
		const backend::unit::morale_t morale,
		const std::string& morale_string,
		const backend::unit::health_t health,
		bool conventional = false, bool has_moves = false, backend::unit::movement_t full_movement = 0, bool native = false, std::optional<uint8_t> facing = {}
	);
	void DespawnUnit( const size_t unit_id );
	void RefreshUnit( Unit* unit );
	void MoveUnit( Unit* unit, tile::Tile* dst_tile, const size_t animation_id );
	void RelocateConventionalUnit( Unit* unit, tile::Tile* dst_tile );

	Unit* GetSelectedUnit() const;
	void SelectUnit( Unit* unit, const bool actually_select_unit );
	void DeselectUnit();
	Unit* GetPreviouslyDeselectedUnit() const;
	const bool SelectNextUnitMaybe( bool automatic = false );
	void SelectNextUnitOrSwitchToTileSelection();

	SlotBadges* GetSlotBadges( const size_t slot_index ) const;
	SlotBadges* GetNativeBadges();
	void DefineSlotBadges( const size_t slot_index, const faction::Faction* faction, uint8_t native_palette_slot=255 );

	const types::Vec3 GetCloserCoords( const types::Vec3& coords, const types::Vec3& ref_coords ) const;

private:
	friend class gse::tests::BackendFixture;

	Game* m_game;
    int m_native_body_zoom=0;
    mutable const actor_visibility::Snapshot* m_visibility_snapshot=nullptr;
    mutable std::optional<uint64_t> m_visibility_selected;
    mutable bool m_visibility_active=false;
    mutable std::map<std::array<int32_t,2>,std::vector<uint64_t>> m_visibility_members;
    mutable std::map<std::array<int32_t,2>,actor_visibility::UnitPlan> m_visibility_plans;
    mutable std::map<int32_t,int> m_chassis_flag_offsets;
    const actor_visibility::UnitPlan m_hidden_plan;

	sprite::InstancedSpriteManager* m_ism;

	Unit* m_selected_unit = nullptr;

	std::unordered_map< std::string, UnitDef* > m_unitdefs = {};
	std::unordered_map< size_t, Unit* > m_units = {};
	std::vector< Unit* > m_selectable_units = {};

	SlotBadges* m_native_badges = nullptr;
	BadgeDefs* m_badge_defs = nullptr;
	std::unordered_map< size_t, SlotBadges* > m_slot_badges = {};

	struct moving_unit_info_t {
		tile::Tile* tile;
		size_t animation_id;
	};
	std::unordered_map< Unit*, moving_unit_info_t > m_moving_units = {};

	Unit* m_previously_deselected_unit = nullptr;

	struct WorkVisit { size_t unit_id; size_t turn_id; };
	std::shared_ptr< const WorkVisit > m_work_visit;
	void PumpSelectedWork( std::optional< size_t > selected );
	struct SelectionVisit {
		size_t turn_id;
		std::optional<size_t> current;
		types::Vec2<size_t> focus;
		std::optional<size_t> selected;
		std::atomic<bool> ready{false};
	};
	std::shared_ptr<SelectionVisit> m_selection_visit;
	std::optional<size_t> m_controller_selection;
	bool QueueAutomaticSelection();
	void ReceiveAutomaticSelection();
	void QueueAutomaticSelectionAt( types::Vec2<size_t> focus );

	void AddSelectable( Unit* unit );
	void RemoveSelectable( Unit* unit );
	void UpdateSelectable( Unit* unit );
	Unit* GetNextSelectable();

};

}
}
}
