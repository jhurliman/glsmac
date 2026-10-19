#include "UnitManager.h"
#include "game/frontend/MapProjection.h"

#include <algorithm>
#include <cmath>

#include "Unit.h"
#include "UnitDef.h"
#include "SlotBadges.h"
#include "game/backend/unit/Def.h"
#include "game/frontend/Game.h"
#include "game/frontend/tile/TileManager.h"
#include "BadgeDefs.h"
#include "types/mesh/Rectangle.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/map/Map.h"
#include "gse/value/Int.h"

namespace game {
namespace frontend {
namespace unit {

UnitManager::UnitManager( Game* game )
	: m_game( game )
	, m_ism( game->GetISM() ) {
	NEW( m_badge_defs, BadgeDefs, m_ism );
}

UnitManager::~UnitManager() {
	m_work_visit.reset();
	m_selection_visit.reset();
	for ( const auto& it : m_units ) {
		delete it.second;
	}
	for ( const auto& it : m_unitdefs ) {
		delete it.second;
	}
	for ( const auto& it : m_slot_badges ) {
		delete it.second;
	}
	delete m_native_badges;
	DELETE( m_badge_defs );
}

bool UnitManager::IsSelectionBlinkAdmitted(const Unit* unit) const {
    return unit && unit==m_selected_unit && unit->IsOwned() && unit->IsActive() && !unit->IsMoving() && m_game->IsTurnActive();
}

void UnitManager::Iterate() {
    m_badge_defs->AdvanceSelectionBlink(SelectionBlink::Clock::now(),IsSelectionBlinkAdmitted(m_selected_unit));
	ReceiveAutomaticSelection();
	if ( m_selected_unit ) {
		m_selected_unit->Iterate();
	}
	PumpSelectedWork( m_selected_unit && m_selected_unit->IsOwned() && m_selected_unit->IsActive() && !m_selected_unit->IsMoving()
		? std::optional< size_t >( m_selected_unit->GetId() ) : std::nullopt );

	auto it = m_moving_units.begin();
	while ( it != m_moving_units.end() ) {
		const auto& unit = it->first;
		if ( unit != m_selected_unit ) { // already iterated
			unit->Iterate();
		}
		if ( !unit->IsMoving() ) {
			const auto& tile = it->second.tile;
			if ( unit == m_selected_unit ) {
				m_game->SetSelectedTile( tile );
			}
			unit->SetTile( tile );
			m_game->RefreshSelectedTileIf( tile, m_selected_unit );
			m_game->SendAnimationFinished( it->second.animation_id );
			it = m_moving_units.erase( it );
		}
		else {
			it++;
		}
	}
}

void UnitManager::QueueAutomaticSelectionAt( types::Vec2<size_t> focus ) {
	const auto turn = m_game->m_turn_id;
	if ( m_selection_visit && m_selection_visit->turn_id == turn && m_selection_visit->current == m_controller_selection
		&& m_selection_visit->focus == focus ) { return; }
	auto visit = std::make_shared<SelectionVisit>();
	visit->turn_id = turn; visit->current = m_controller_selection; visit->focus = focus;
	m_selection_visit = visit;
	const std::weak_ptr<SelectionVisit> weak = visit;
	// The frontend relinquishes its startup State after initialization. The
	// backend owns the live State; queued work remains bound to its session.
	auto* state = m_game->GetGame()->GetState();
	state->WithGameSession( state->CaptureGameSession(), state, [ weak ]( backend::Game* const game, GSE_CALLABLE ) {
		if ( const auto request = weak.lock() ) {
			request->selected = game->SelectLocalConventionalUnit( request->turn_id, request->current, request->current, request->focus, true );
			request->ready.store( true, std::memory_order_release );
		}
	} );
}

bool UnitManager::QueueAutomaticSelection() {
	const bool canonical = std::any_of( m_units.begin(), m_units.end(), []( const auto& entry ) {
		return entry.second->IsOwned() && entry.second->IsConventional();
	} );
	if ( !canonical ) { return false; }
	// Camera-to-tile projection is the port's view-focus input. Native selection
	// owns the original integer distance and record-order semantics.
	const auto width = m_game->m_map_data.width, height = m_game->m_map_data.height;
	if ( !width || !height ) { return false; }
	const auto& camera = m_game->m_camera_position;
	const auto& view = m_game->m_viewport;
	if ( camera.z <= 0 || view.window_aspect_ratio <= 0 || view.ratio.y <= 0 ) { return false; }
	const double x = 1.0 - m_game->m_map_data.range.percent_to_absolute.x.Unclamp( camera.x / camera.z / view.window_aspect_ratio );
	const double y = 1.0 - m_game->m_map_data.range.percent_to_absolute.y.Unclamp( camera.y / camera.z / view.ratio.y / map_projection::Vertical(m_game->m_map_data.native_projection) );
	if ( !std::isfinite(x) || !std::isfinite(y) ) { return false; }
	const auto cy = static_cast<size_t>(std::clamp(y,0.0,1.0)*double(height-1));
	auto cx = static_cast<size_t>(std::clamp(x,0.0,1.0)*double(width-1));
	if ( (cx^cy)&1 ) { cx = cx+1<width ? cx+1 : cx-1; }
	QueueAutomaticSelectionAt( {cx,cy} );
	return true;
}

void UnitManager::ReceiveAutomaticSelection() {
	if ( !m_selection_visit ) { return; }
	if ( !m_game->IsTurnActive() || m_game->m_world_retired || m_selection_visit->turn_id != m_game->m_turn_id ) {
		m_selection_visit.reset(); return;
	}
	if ( !m_selection_visit->ready.load(std::memory_order_acquire) ) { return; }
	const auto selected = m_selection_visit->selected;
	if ( selected ) {
		auto* unit = GetUnitById(*selected);
		// Backend updates may precede their frontend records. Keep this result
		// until the corresponding ready presentation arrives; never choose by FR order.
		if ( !unit || !unit->IsActive() ) { return; }
		m_selection_visit.reset();
		SelectUnit( unit, true );
		m_game->ScrollToTile( unit->GetTile(), true );
	} else {
		m_selection_visit.reset();
		const auto retained = m_controller_selection;
		m_game->SelectUnitOrSelectedTile( m_selected_unit );
		m_selected_unit = nullptr;
		m_controller_selection = retained;
	}
}

void UnitManager::PumpSelectedWork( std::optional< size_t > selected ) {
	if ( !selected || !m_game->m_is_initialized || m_game->m_world_retired || !m_game->IsTurnActive()
		|| m_game->m_mt_ids.init || m_game->m_mt_ids.get_map_data ) {
		m_work_visit.reset();
		return;
	}
	const auto turn = m_game->m_turn_id;
	if ( m_work_visit && m_work_visit->unit_id == *selected && m_work_visit->turn_id == turn ) { return; }
	m_work_visit = std::make_shared< const WorkVisit >( WorkVisit{ *selected, turn } );
	const std::weak_ptr< const WorkVisit > visit = m_work_visit;
	auto* state = m_game->GetGame()->GetState();
	state->WithGameSession( state->CaptureGameSession(), state, [ visit ]( backend::Game* const game, GSE_CALLABLE ) {
		// Selection/update/teardown retires this visit before a delayed callback.
		// Only copied IDs cross threads; session retirement cancels old-world work.
		if ( const auto current = visit.lock() ) {
			game->QueueSelectedConventionalWork( GSE_CALL, current->unit_id, current->turn_id );
		}
	} );
}

Unit* UnitManager::GetUnitById( const size_t id ) const {
	const auto& it = m_units.find( id );
	return it != m_units.end()
		? it->second
		: nullptr;
}

void UnitManager::DefineUnit( const backend::unit::Def* def ) {
	ASSERT( m_unitdefs.find( def->m_id ) == m_unitdefs.end(), "unit def already exists" );

	Log( "Defining unit: " + def->m_id );

	m_unitdefs.insert(
		{
			def->m_id,
			new UnitDef(
				m_ism,
				def,
                m_game->UsesNativeMapProjection()
			)
		}
	);
}

void UnitManager::UndefineUnit( const std::string& id ) {
	ASSERT( m_unitdefs.find( id ) != m_unitdefs.end(), "unit def not found" );

	Log( "Undefining unit: " + id );

	m_unitdefs.erase( id );
}

void UnitManager::SpawnUnit(
	const size_t unit_id,
	const std::string& unitdef_id,
	const size_t slot_index,
	const types::Vec2< size_t >& tile_coords,
	const types::Vec3& render_coords,
	const backend::unit::movement_t movement,
	const backend::unit::morale_t morale,
	const std::string& morale_string,
	const backend::unit::health_t health,
	const bool conventional, const bool has_moves, const backend::unit::movement_t full_movement, const bool native
) {

	ASSERT( m_unitdefs.find( unitdef_id ) != m_unitdefs.end(), "unitdef not found" );

	if ( m_units.find( unit_id ) != m_units.end() ) {
		// maybe unit was spawned, then despawned and then despawn rolled back
		return;
	}

	auto* unitdef = m_unitdefs.at( unitdef_id );
	if(native && conventional) {THROW("Native unit cannot also be conventional");}
	auto* slot = native ? nullptr : m_game->GetSlot( slot_index );
	auto* tile = m_game->GetTM()->GetTile( tile_coords );

	auto* unit = m_units.insert(
		{
			unit_id,
			new Unit(
				this,
				m_badge_defs,
				unit_id,
				unitdef,
				slot,
				tile,
				{
					render_coords.x,
					render_coords.y,
					render_coords.z
				},
				!native && slot_index == m_game->GetMySlotIndex(),
				movement,
				morale,
				morale_string,
				health, conventional, has_moves, full_movement, native
			)
		}
	).first->second;

	types::mesh::Rectangle* mesh = nullptr;
	types::texture::Texture* texture = nullptr;

	m_game->RenderTile( tile, m_selected_unit );

	if ( unit->IsActive() ) {
		AddSelectable( unit );
	}
}

void UnitManager::DespawnUnit( const size_t unit_id ) {
	const auto& it = m_units.find( unit_id );
	ASSERT( it != m_units.end(), "unit id not found" );

	auto* unit = it->second;

	m_units.erase( it );

	if ( m_controller_selection == unit_id ) { m_controller_selection.reset(); }
	const bool pending_selection = m_selection_visit != nullptr;
	m_selection_visit.reset();
	const bool was_selected = m_selected_unit == unit;
	m_selectable_units.erase( std::remove( m_selectable_units.begin(), m_selectable_units.end(), unit ), m_selectable_units.end() );
	if ( was_selected ) { DeselectUnit(); }
	if ( m_previously_deselected_unit == unit ) { m_previously_deselected_unit = nullptr; }
	// Destruction removes tile membership before choosing another selection.
	delete unit;
	// Rebuild same-tile UI entries before selecting a surviving Unit. The
	// backend wrapper has already retired, so publish only its stable ID.
	m_game->Trigger( m_game->GetGame(), "unit_despawn", ARGS_F( unit_id ) {
		{ "unit_id", VALUE( gse::value::Int,, unit_id ) },
	}; } );
	if ( was_selected || pending_selection ) { SelectNextUnitOrSwitchToTileSelection(); }
	m_game->RefreshSelectedTile( m_selected_unit );

}

void UnitManager::RefreshUnit( Unit* unit ) {
	const bool pending_selection = m_selection_visit != nullptr;
	m_selection_visit.reset();
	if ( m_work_visit && m_work_visit->unit_id == unit->GetId() ) { m_work_visit.reset(); }
	const auto was_active = unit->IsActive();
	UpdateSelectable( unit );
	m_game->RenderTile( unit->GetTile(), m_selected_unit );
	if ( m_selected_unit == unit && was_active ) {
		if ( !unit->IsActive() ) {
			SelectNextUnitOrSwitchToTileSelection();
		}
		else {
			unit->StartBadgeBlink();
		}
	}
	m_game->UpdateRelatedWidgets( ui::WT_UNIT_PREVIEW, unit->GetId(), unit );
	if ( pending_selection ) { SelectNextUnitMaybe( true ); }
}

void UnitManager::RelocateConventionalUnit( Unit* unit, tile::Tile* dst_tile ) {
	if ( m_moving_units.count( unit ) || unit->IsMoving() ) { THROW( "conventional relocation cannot own a legacy animation" ); }
	auto* source = unit->GetTile();
	unit->SetTile( dst_tile );
	if ( unit == m_selected_unit ) { m_game->SetSelectedTile( dst_tile ); }
	m_game->RefreshSelectedTileIf( source, m_selected_unit );
	m_game->RefreshSelectedTileIf( dst_tile, m_selected_unit );
	RefreshUnit( unit );
}

void UnitManager::MoveUnit( Unit* unit, tile::Tile* dst_tile, const size_t animation_id ) {
	auto* src_tile = unit->GetTile();
	const auto& it = m_moving_units.find( unit );
	if ( it != m_moving_units.end() ) {
		const auto& tile = it->second.tile;
		if ( unit == m_selected_unit ) {
			m_game->SetSelectedTile( tile );
		}
		unit->SetTile( tile );
		m_game->RefreshSelectedTileIf( tile, m_selected_unit );
		m_game->SendAnimationFinished( it->second.animation_id );
		m_moving_units.erase( it );
	}
	m_moving_units.insert(
		{
			unit,
			{
				dst_tile,
				animation_id
			}
		}
	);
	src_tile->RemoveUnit( unit );
	m_game->RefreshSelectedTileIf( src_tile, m_selected_unit );
	m_game->SetSelectedTile( dst_tile );
	unit->MoveToTile( dst_tile );
}

Unit* UnitManager::GetSelectedUnit() const {
	return m_selected_unit;
}

void UnitManager::SelectUnit( Unit* unit, const bool actually_select_unit ) {
	Log( "Selecting unit " + std::to_string( unit->GetId() ) );
	//m_tile_at_query_purpose = backend::TQP_UNIT_SELECT; // ???
	if ( m_selected_unit != unit ) {
		m_game->DeselectTileOrUnit();

		unit->SetActiveOnTile();

		m_selected_unit = unit;
		m_selected_unit->Show();

		m_game->SetSelectedTile( m_selected_unit->GetTile() );
		m_game->RefreshSelectedTile( m_selected_unit );
	}
	m_controller_selection = unit->GetId();
	m_selection_visit.reset();
	if ( actually_select_unit && (m_selected_unit->IsActive() || (m_selected_unit->IsOwned() && m_selected_unit->IsHeld())) && m_game->IsTurnActive() ) {
		auto* most_important_unit = m_game->GetSelectedTileMostImportantUnit();
		if ( most_important_unit && m_selected_unit != most_important_unit ) {
			most_important_unit->Hide();
		}
		if(m_selected_unit->IsActive()) {m_selected_unit->StartBadgeBlink();}
		const auto id = m_selected_unit->GetId();
		Log( "Selected unit " + std::to_string( id ) );
        auto* state=m_game->GetGame()->GetState();
        state->WithGameSession(state->CaptureGameSession(),state,[id](backend::Game* const game,GSE_CALLABLE) {
            auto* actor=game->GetUM()->GetUnit(id);
            if(!actor) {return;}
            game->GetState()->TriggerObject(game->GetMap(),"unit_preview",ARGS_F(actor){{"unit",actor->Wrap(GSE_CALL)}};});
            // Preview listeners can retire a Unit. Resolve the stable ID again;
            // neither callback borrows this frontend stack frame or a raw session.
            actor=game->GetUM()->GetUnit(id);
            if(!actor) {return;}
            game->GetState()->TriggerObject(game,"unit_select",ARGS_F(actor){{"unit",actor->Wrap(GSE_CALL)}};});
        });
	}
}

void UnitManager::DeselectUnit() {
	m_work_visit.reset();
	m_selection_visit.reset();
	m_controller_selection.reset();
	m_previously_deselected_unit = m_selected_unit;
	if ( m_selected_unit ) {
		auto* most_important_unit = m_game->GetSelectedTileMostImportantUnit();
		if ( !most_important_unit || m_selected_unit == most_important_unit ) {
			m_selected_unit->StopBadgeBlink( true );
		}
		else if ( most_important_unit ) {
			m_selected_unit->StopBadgeBlink( false );
			m_selected_unit->Hide();
			most_important_unit->Show();
		}
		m_selected_unit = nullptr;
	}
}

Unit* UnitManager::GetPreviouslyDeselectedUnit() const {
	return m_previously_deselected_unit;
}

SlotBadges* UnitManager::GetSlotBadges( const size_t slot_index ) const {
	ASSERT( m_slot_badges.find( slot_index ) != m_slot_badges.end(), "slot unit badges for index " + std::to_string( slot_index ) + " not defined" );
	return m_slot_badges.at( slot_index );
}

SlotBadges* UnitManager::GetNativeBadges() {
    if(!m_native_badges) {m_native_badges=new SlotBadges(m_badge_defs,m_ism,0,nullptr);}
    return m_native_badges;
}

void UnitManager::DefineSlotBadges( const size_t slot_index, const faction::Faction* faction ) {
	ASSERT( m_slot_badges.find( slot_index ) == m_slot_badges.end(), "slot unit badges for index " + std::to_string( slot_index ) + " already defined" );
	m_slot_badges.insert(
		{
			slot_index,
			new SlotBadges( m_badge_defs, m_ism, slot_index, faction )
		}
	);
}

const types::Vec3 UnitManager::GetCloserCoords( const types::Vec3& coords, const types::Vec3& ref_coords ) const {
	return m_game->GetCloserCoords( coords, ref_coords );
}

void UnitManager::AddSelectable( Unit* unit ) {
	const auto& it = std::find( m_selectable_units.begin(), m_selectable_units.end(), unit );
	if ( it == m_selectable_units.end() ) {
		const bool was_empty = m_selectable_units.empty();
		m_selectable_units.push_back( unit );
		if ( was_empty ) {
			SelectNextUnitMaybe( true );
		}
	}
}

void UnitManager::RemoveSelectable( Unit* unit ) {
	const auto& it = std::find( m_selectable_units.begin(), m_selectable_units.end(), unit );
	if ( it != m_selectable_units.end() ) {
		m_selectable_units.erase( it );
		if ( m_previously_deselected_unit == unit ) {
			m_previously_deselected_unit = nullptr;
		}
		if ( m_selected_unit == unit ) {
			SelectNextUnitOrSwitchToTileSelection();
		}
	}
}

void UnitManager::UpdateSelectable( Unit* unit ) {
	if ( unit->IsOwned() ) {
		if ( unit->IsActive() ) {
			AddSelectable( unit );
		}
		else {
			RemoveSelectable( unit );
			if ( m_selected_unit == unit ) {
				SelectNextUnitOrSwitchToTileSelection();
			}
		}
	}
}

Unit* UnitManager::GetNextSelectable() {
	if ( !m_selectable_units.empty() ) {
		if ( !m_selected_unit ) {
			return m_selectable_units.front();
		}
		Unit* selected_unit = nullptr;
		float selected_distance = 0;
		// can be optimized, but probably no need
		for ( const auto& unit : m_selectable_units ) {
			if ( unit == m_selected_unit ) {
				continue;
			}
			const auto& a = unit->GetTile()->GetCoords();
			const auto& b = m_selected_unit->GetTile()->GetCoords();
			float distance = sqrtf( pow( (float)a.x - b.x, 2 ) + pow( (float)a.y - b.y, 2 ) );
			if ( !selected_unit || distance < selected_distance ) {
				selected_unit = unit;
				selected_distance = distance;
			}
		}
		if ( !selected_unit ) {
			selected_unit = m_selected_unit; // the only unit left
		}
		else {
			auto* tile = selected_unit->GetTile();
			if ( tile == m_selected_unit->GetTile() ) {
				const auto tile_units = tile->GetOrderedUnits();
				ASSERT( !tile_units.empty(), "tile units empty" );
				bool searching = false;
				bool found = false;
				Unit* first_good_candidate = nullptr;
				for ( size_t i = 0 ; i < tile_units.size() ; i++ ) {
					if ( tile_units.at( i ) == m_selected_unit ) {
						searching = true;
					}
					else {
						auto* tile_unit = tile_units.at( i );
						if ( searching ) {
							if ( tile_unit->IsActive() ) {
								selected_unit = tile_unit;
								found = true;
								break;
							}
						}
						else if ( !first_good_candidate && tile_unit->IsActive() ) {
							first_good_candidate = tile_unit;
						}
					}
				}
				if ( searching && !found ) {
					selected_unit = first_good_candidate;
				}
			}
		}
		return selected_unit;
	}
	return nullptr;
}

const bool UnitManager::SelectNextUnitMaybe( bool automatic ) {
	if ( !m_game->IsTurnActive() ) {
		return false;
	}
	if ( automatic && QueueAutomaticSelection() ) { return true; }
	auto* selected_unit = GetNextSelectable();
	if ( selected_unit ) {
		SelectUnit( selected_unit, true );
		m_game->ScrollToTile( m_selected_unit->GetTile(), true );
		return true;
	}
	else {
		m_selected_unit = nullptr;
		return false;
	}
}

void UnitManager::SelectNextUnitOrSwitchToTileSelection() {
	if ( !SelectNextUnitMaybe( true ) ) {
		m_game->SelectUnitOrSelectedTile( m_selected_unit );
		m_selected_unit = nullptr;
	}
}

}
}
}
