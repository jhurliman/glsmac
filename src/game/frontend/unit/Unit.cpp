#include "Unit.h"

#include "game/frontend/Game.h"
#include "game/frontend/tile/Tile.h"
#include "util/String.h"
#include "game/backend/unit/Unit.h"
#include "game/frontend/Slot.h"
#include "UnitDef.h"
#include "BadgeDefs.h"
#include "SlotBadges.h"
#include "UnitManager.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "scene/actor/Instanced.h"
#include "types/mesh/Rectangle.h"
#include "scene/actor/Sprite.h"
#include "types/texture/Texture.h"

namespace game {
namespace frontend {
namespace unit {

Unit::Unit(
	UnitManager* um,
	BadgeDefs* badge_defs,
	const size_t id,
	UnitDef* def,
	Slot* slot,
	tile::Tile* tile,
	const types::Vec3& render_coords,
	const bool is_owned,
	const backend::unit::movement_t movement,
	const backend::unit::morale_t morale,
	const std::string& morale_string,
	const backend::unit::health_t health,
	const bool conventional, const bool has_moves, const backend::unit::movement_t full_movement, const bool native, std::optional<uint8_t> facing
)
	: TileObject( TOT_UNIT, tile )
	, m_um( um )
	, m_badge_defs( badge_defs )
	, m_id( id )
	, m_def( def )
	, m_slot_badges( slot ? m_um->GetSlotBadges( slot->GetIndex() ) : m_um->GetNativeBadges() )
	, m_render(
		{
			render_coords,
			false,
			0,
			{},
			{}
		}
	)
	, m_is_owned( is_owned )
	, m_movement( movement )
	, m_morale( morale )
	, m_morale_string( morale_string )
	, m_health( health )
	, m_conventional( conventional ), m_has_moves( has_moves ), m_native(native), m_full_movement( full_movement ), m_facing(facing) {
	m_is_active = ShouldBeActive();
	m_render.badge.def = m_slot_badges->GetUnitBadgeSprite( m_morale, m_is_active || (m_held && m_is_owned && CanMove()), m_order_glyph, m_order_color_index );
	m_render.badge.healthbar.def = m_badge_defs->GetBadgeHealthbarSprite( m_health );
	UpdateMeshTex( m_render_data.unit, GetSprite()->instanced_sprite );
	UpdateMeshTex( m_render_data.badge, GetBadgeSprite()->instanced_sprite );
	UpdateMeshTex( m_render_data.healthbar, GetBadgeHealthbarSprite()->instanced_sprite );
	m_tile->AddUnit( this );
}

Unit::~Unit() {
	Hide();
	m_tile->RemoveUnit( this );
	if ( m_render_data.unit.mesh ) {
		DELETE( m_render_data.unit.mesh );
	}
	if ( m_render_data.badge.mesh ) {
		DELETE( m_render_data.badge.mesh );
	}
	if ( m_render_data.healthbar.mesh ) {
		DELETE( m_render_data.healthbar.mesh );
	}
}

const size_t Unit::GetId() const {
	return m_id;
}

const bool Unit::IsOwned() const {
	return m_is_owned;
}

const bool Unit::IsActive() const {
	return m_is_active;
}

tile::Tile* Unit::GetTile() const {
	return m_tile;
}

const size_t Unit::GetSelectionWeight() const {
	size_t weight = 0;

	// active units have priority
	if ( m_is_active ) {
		weight += 200;
	}

	// non-artillery units have priority
	// TODO: use real unit properties
	if ( m_def->IsArtillery() ) {
		weight += 60;
	}

	// higher morale has priority
	weight += m_morale * 2;

	// health is also somewhat important
	weight += m_health * 8;

	return weight;
}

sprite::Sprite* Unit::GetSprite() const {
	return m_def->GetSprite( m_morale, m_facing );
}

sprite::Sprite* Unit::GetBadgeSprite() const {
	return m_render.badge.def;
}

sprite::Sprite* Unit::GetBadgeHealthbarSprite() const {
	return m_render.badge.healthbar.def;
}

const std::string Unit::GetNameString() const {
	return m_def->GetNameString();
}

const std::string Unit::GetStatsString() const {
	return (m_conventional || m_native) ? "? - ? - " + util::String::ApproximateFloat( m_full_movement ) : m_def->GetStatsString();
}

const std::string& Unit::GetMoraleString() const {
	return m_morale_string;
}

const std::string Unit::GetMovesString() const {
	if ( m_def->IsImmovable() ) {
		return "";
	}
	return "Moves: " + util::String::ApproximateFloat( m_movement );
}

void Unit::Iterate() {
    if(m_render.badge.blink) {
        const auto visible=m_badge_defs->IsSelectionBlinkVisible();
        if(visible!=IsBadgeVisible()) {
            if(visible) {ShowBadge();} else {HideBadge();}
        }
    }
	while ( m_mover.HasTicked() ) {
		SetRenderCoords( m_mover.GetPosition() );
	}
}

void Unit::SetActiveOnTile() {
	m_tile->SetActiveUnit( this );
}

void Unit::Show() {
    if(!m_um->IsBodyAdmitted(this)) {Hide();return;}
	if ( !m_render.is_rendered ) {
		const auto& c = m_render.coords;

		auto* sprite = GetSprite();
        if(m_render.body!=sprite) {m_render.body=sprite;m_render.instance_id=0;}

		if ( !m_render.instance_id ) {
			m_render.instance_id = sprite->next_instance_id++;
		}
		sprite->instanced_sprite->actor->SetInstance(
			m_render.instance_id, {
				c.x,
				c.y,
				c.z
			}
		);

		if ( !m_render.badge.instance_id ) {
			m_render.badge.instance_id = m_render.badge.def->next_instance_id++;
		}
		if ( !m_render.badge.healthbar.instance_id ) {
			m_render.badge.healthbar.instance_id = m_render.badge.healthbar.def->next_instance_id++;
		}

		m_render.is_rendered = true;
		ShowBadge();
	}
}

void Unit::Hide() {
    HideFakeBadge();
	if ( m_render.is_rendered ) {
		m_render.body->instanced_sprite->actor->RemoveInstance( m_render.instance_id );
		HideBadge(); // Tile redraws hide sprites without ending selection.
		m_render.is_rendered = false;
	}
}

const bool Unit::IsBadgeVisible() const {
	return m_render.badge.def->instanced_sprite->actor->HasInstance( m_render.badge.instance_id );
}

void Unit::ShowBadge() {
	if(!m_render.is_rendered || !m_um->IsBodyAdmitted(this) || (m_render.badge.blink && !m_badge_defs->IsSelectionBlinkVisible())) {HideBadge();return;}
	m_render.badge.def->instanced_sprite->actor->SetInstance( m_render.badge.instance_id, GetRenderedBadgeCoords(false) );
	m_render.badge.healthbar.def->instanced_sprite->actor->SetInstance( m_render.badge.healthbar.instance_id, GetRenderedBadgeCoords(true) );
}

void Unit::HideBadge() {
	m_render.badge.def->instanced_sprite->actor->RemoveInstance( m_render.badge.instance_id );
	m_render.badge.healthbar.def->instanced_sprite->actor->RemoveInstance( m_render.badge.healthbar.instance_id );
}

void Unit::ShowFakeBadge( const uint8_t offset ) {
    if(!m_um->IsStackBadgeAdmitted(this)) {HideFakeBadge();return;}
	if ( !m_render.fake_badge.instance_id || m_fake_badge_offset != offset ) {
		if ( m_render.fake_badge.instance_id ) {
			m_slot_badges->HideFakeBadge( m_render.fake_badge.instance_id );
		}
		m_render.fake_badge.instance_id = m_slot_badges->ShowFakeBadge( m_render.coords, offset );
		m_fake_badge_offset = offset;
	}
}

void Unit::HideFakeBadge() {
	if ( m_render.fake_badge.instance_id ) {
		m_slot_badges->HideFakeBadge( m_render.fake_badge.instance_id );
		m_render.fake_badge.instance_id = 0;
		m_fake_badge_offset = 0;
	}
}

void Unit::StartBadgeBlink() {
    m_badge_defs->SetSelectionBlinkAdmission(m_um->IsSelectionBlinkAdmitted(this));
	m_render.badge.blink=true;ShowBadge();
}

void Unit::StopBadgeBlink( const bool is_badge_shown ) {
	m_render.badge.blink=false;
	if ( is_badge_shown ) {
		ShowBadge();
	}
	else {
		HideBadge();
	}
}

void Unit::Refresh() {
	if ( m_need_refresh ) {
		m_need_refresh = false;
        if(m_render.is_rendered && m_render.body!=GetSprite()) {Hide();}
		m_is_active = ShouldBeActive();
		if ( !m_is_active ) {
			StopBadgeBlink( true );
		}
		const bool is_badge_visible = IsBadgeVisible();
		if ( is_badge_visible ) {
			HideBadge();
		}
		{
            auto* newdef=UsesNativeMapOverlay()?m_slot_badges->GetNativeMapBadge(
                m_morale,m_is_owned && !CanMove(),m_order_glyph,m_order_color_index):nullptr;
            if(!newdef) {
                newdef=m_slot_badges->GetUnitBadgeSprite(m_morale,m_is_active || (m_held && m_is_owned && CanMove()),m_order_glyph,m_order_color_index);
                if(UsesNativeMapOverlay()) {newdef=m_badge_defs->GetNativeMapSprite(newdef);}
            }
			if ( m_render.badge.def != newdef ) {
				if ( m_render.badge.instance_id ) {
					m_render.badge.def->instanced_sprite->actor->RemoveInstance( m_render.badge.instance_id );
					m_render.badge.instance_id = 0;
				}
				m_render.badge.def = newdef;
			}
			if ( !m_render.badge.instance_id ) {
				m_render.badge.instance_id = m_render.badge.def->next_instance_id++;
			}
		}
		{
			auto* newdef = m_badge_defs->GetBadgeHealthbarSprite( m_health, m_maximum_hit_points );
            if(UsesNativeMapOverlay()) {newdef=m_badge_defs->GetNativeMapSprite(newdef);}
			if ( m_render.badge.healthbar.def != newdef ) {
				if ( m_render.badge.healthbar.instance_id ) {
					m_render.badge.healthbar.def->instanced_sprite->actor->RemoveInstance( m_render.badge.healthbar.instance_id );
					m_render.badge.healthbar.instance_id = 0;
				}
				m_render.badge.healthbar.def = newdef;
			}
			if ( !m_render.badge.healthbar.instance_id ) {
				m_render.badge.healthbar.instance_id = m_render.badge.healthbar.def->next_instance_id++;
			}
		}
		// TODO: cache/optimize
		UpdateMeshTex( m_render_data.unit, GetSprite()->instanced_sprite );
		UpdateMeshTex( m_render_data.badge, GetBadgeSprite()->instanced_sprite );
		UpdateMeshTex( m_render_data.healthbar, GetBadgeHealthbarSprite()->instanced_sprite );
		if ( is_badge_visible ) {
			ShowBadge();
		}
		m_um->RefreshUnit( this );
	}
}

void Unit::SetFacing(std::optional<uint8_t> facing) {
    if(m_facing!=facing) {m_facing=facing;m_need_refresh=true;}
}

void Unit::SetMovement( const backend::unit::movement_t movement ) {
	if ( movement != m_movement ) {
		m_movement = movement;
		m_need_refresh = true;
	}
}

void Unit::SetHealth( const backend::unit::health_t health ) {
	if ( health != m_health ) {
		m_health = health;
		m_need_refresh = true;
	}
}

void Unit::SetMaximumHitPoints(int32_t maximum) {
    if(maximum!=m_maximum_hit_points) {m_maximum_hit_points=maximum;m_need_refresh=true;}
}

const bool Unit::CanMove() const {
	return m_conventional ? m_has_moves : m_movement >= backend::unit::Unit::MINIMUM_MOVEMENT_TO_KEEP;
}

void Unit::SetOrderGlyph(char glyph,uint8_t color) {if(m_order_glyph!=glyph || m_order_color_index!=color) {m_order_glyph=glyph;m_order_color_index=color;m_need_refresh=true;}}

void Unit::SetHeld(bool held) {if(m_held!=held) {m_held=held;m_need_refresh=true;}}

void Unit::SetActionability( const bool conventional, const bool has_moves, const backend::unit::movement_t full_movement ) {
	if ( m_conventional != conventional || m_has_moves != has_moves || m_full_movement != full_movement ) {
		m_conventional = conventional; m_has_moves = has_moves; m_full_movement = full_movement; m_need_refresh = true;
	}
}

void Unit::SetMorale( const backend::unit::morale_t morale, const std::string& text ) {
	if ( morale != m_morale ) {
		// Sprite definitions may vary with morale. Retire the old actor instance
		// using its old key before selecting the new one.
		const bool shown = m_render.is_rendered;
		if ( shown ) { Hide(); }
		m_render.instance_id = 0;
		m_morale = morale; m_morale_string = text; m_need_refresh = true;
		Refresh();
		if ( shown ) { Show(); }
	}
	else if ( text != m_morale_string ) { m_morale_string = text; m_need_refresh = true; }
}

void Unit::SetTile( tile::Tile* dst_tile ) {
	ASSERT( m_tile, "source tile not set" );
	ASSERT( dst_tile, "destination tile not set" );

	m_tile->RemoveUnit( this );

	m_tile = dst_tile;

	m_tile->AddUnit( this );

	UpdateFromTile();
}

void Unit::MoveToTile( tile::Tile* dst_tile ) {
	m_mover.Stop(); //ASSERT( !m_mover.IsRunning(), "unit already moving" );
	//ASSERT( m_tile != dst_tile, "can't move to same tile" );
	ASSERT( m_tile, "source tile not set" );
	ASSERT( dst_tile, "destination tile not set" );

	StopBadgeBlink( true );
	HideFakeBadge();
	Show();

	auto from = m_tile->GetRenderData().coords.InvertY();
	auto to = dst_tile->GetRenderData().coords.InvertY();
	m_mover.Scroll( from, m_um->GetCloserCoords( to, from ), MOVE_DURATION_MS );
}

const bool Unit::IsMoving() const {
	return m_mover.IsRunning();
}

sprite::InstancedSprite* Unit::GetCardSprite( bool roster ) const {
	return m_def->GetCardSprite( roster, m_facing );
}

std::shared_ptr<types::texture::Texture> Unit::GetCardOverlay() const {
    if((!m_conventional && !m_native) || m_maximum_hit_points<=0) {return {};}
    const bool spent=!m_native && m_is_owned && !CanMove();
    const CardOverlayKey key{m_morale,spent,m_health,m_maximum_hit_points,m_order_glyph,m_order_color_index};
    if(!m_card_overlay_key || *m_card_overlay_key!=key) {
        auto next=m_slot_badges->MakeCardOverlay(m_morale,spent?1:0,m_health,m_maximum_hit_points,m_order_glyph,m_order_color_index);
        if(!next) {return {};}
        m_card_overlay=std::move(next);m_card_overlay_key=key;
    }
    return m_card_overlay;
}

const Unit::render_data_t& Unit::GetRenderData() const {
	return m_render_data;
}

bool Unit::UsesNativeMapOverlay() const {
    return m_conventional && m_maximum_hit_points>0 && m_def->UsesNativeMapPlacement();
}

types::Vec3 Unit::GetRenderedBadgeCoords(bool health) const {
    if(UsesNativeMapOverlay()) {
        // Original flag canvas starts at the dispatcher helper; health starts at(6,6).
        return {m_render.coords.x-0.5f+(health?6.0f/96:0),
            m_render.coords.y+1-(health?6.0f/48:0),m_render.coords.z};
    }
    return health?BadgeDefs::GetBadgeHealthbarCoords(m_render.coords,m_maximum_hit_points>0):BadgeDefs::GetBadgeCoords(m_render.coords);
}

const bool Unit::ShouldBeActive() const {
	return m_is_owned && !m_held && CanMove();
}

void Unit::UpdateMeshTex( meshtex_t& meshtex, const sprite::InstancedSprite* sprite ) {
	auto* texture = sprite->actor->GetSpriteActor()->GetTexture();
	NEWV( mesh, types::mesh::Rectangle );
	mesh->SetCoords(
		{
			0.0f,
			0.0f
		},
		{
			1.0f,
			1.0f
		},
		{
			sprite->xy.x,
			sprite->xy.y
		},
		{
			sprite->xy.x + sprite->wh.x,
			sprite->xy.y + sprite->wh.y
		},
		{
			texture->GetWidth(),
			texture->GetHeight()
		},
		0.8f
	);
	if ( meshtex.mesh ) {
		DELETE( meshtex.mesh );
	}
	meshtex.mesh = mesh->ToRenderMesh();
	meshtex.texture = texture;
	meshtex.texture_owner = sprite->texture_owner;
}

void Unit::SetRenderCoords( const types::Vec3& coords ) {
	m_render.coords = coords;
	if(m_render.is_rendered) {m_render.body->instanced_sprite->actor->UpdateInstance(
		m_render.instance_id, {
			coords.x,
			coords.y,
			coords.z
		}
	);}
	m_render.badge.def->instanced_sprite->actor->UpdateInstance(
		m_render.badge.instance_id,
		GetRenderedBadgeCoords(false)
	);
	m_render.badge.healthbar.def->instanced_sprite->actor->UpdateInstance(
		m_render.badge.healthbar.instance_id,
		GetRenderedBadgeCoords(true)
	);
	if ( m_render.fake_badge.instance_id ) {
		m_slot_badges->HideFakeBadge( m_render.fake_badge.instance_id );
		m_render.fake_badge.instance_id = m_slot_badges->ShowFakeBadge( m_render.coords, m_fake_badge_offset );
	}
	m_need_refresh = true;
}

}
}
}
