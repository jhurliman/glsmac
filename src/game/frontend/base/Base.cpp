#include "Base.h"
#include <stdexcept>

#include "game/frontend/Game.h"
#include "game/frontend/faction/Faction.h"
#include "game/frontend/tile/Tile.h"
#include "game/frontend/Slot.h"
#include "game/frontend/sprite/Sprite.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "game/frontend/text/InstancedText.h"
#include "scene/actor/Instanced.h"
#include "types/mesh/Rectangle.h"
#include "scene/actor/Sprite.h"
#include "types/texture/Texture.h"
#include "BaseManager.h"
#include "SlotBadges.h"

namespace game {
namespace frontend {
namespace base {

static const std::vector< uint8_t > s_base_render_population_thresholds = {
	1,
	4,
	8,
	15,
};

Base::Base(
	BaseManager* bm,
	const size_t id,
	const std::string& name,
	Slot* slot,
	tile::Tile* tile,
	const bool is_owned,
	const types::Vec3& render_coords,
	text::InstancedText* render_name_sprite
)
	: TileObject( TOT_BASE, tile )
	, m_bm( bm )
	, m_id( id )
	, m_name( name )
	, m_faction( slot->GetFaction() )
	, m_render(
		{
			render_coords,
			nullptr,
			render_name_sprite,
			false,
			0,
		}
	)
	, m_is_owned( is_owned )
	, m_is_guarded( !m_tile->GetUnits().empty() )
	, m_slot_badges( m_bm->GetSlotBadges( slot->GetIndex() ) )
	, m_owner_slot( slot->GetIndex() ) {
	UpdateMeshTex( m_render_data.base, GetSprite()->instanced_sprite );
	m_render.badge.def = m_slot_badges->GetBaseBadgeSprite( m_render.badge.pops_count, m_is_guarded );
	UpdateMeshTex( m_render_data.badge, m_render.badge.def->instanced_sprite );
	m_tile->SetBase( this );
}

Base::~Base() {
	Hide();
	m_tile->UnsetBase( this );
	delete m_render.name_sprite;
	if ( m_render_data.base.mesh ) {
		DELETE( m_render_data.base.mesh );
	}
	if ( m_render_data.badge.mesh ) {
		DELETE( m_render_data.badge.mesh );
	}
}

const size_t Base::GetId() const {
	return m_id;
}

const std::string& Base::GetName() const {
	return m_name;
}

void Base::SetIdentity( Slot* owner, faction::Faction* faction, std::string name,
	bool is_owned, std::unique_ptr<text::InstancedText> label ) {
	// Prepare new render resources while the old instances are still valid.
	auto* badges=m_bm->GetSlotBadges(owner->GetIndex());
	auto* badge=badges->GetBaseBadgeSprite(GetPopulation(),m_is_guarded);
	meshtex_t next_base{},next_badge{};
	const auto destroy_mesh=[](const types::mesh::Render* mesh) {if(mesh) {DELETE(mesh);}};
	UpdateMeshTex(next_base,GetSpriteFor(faction)->instanced_sprite);
	std::unique_ptr<const types::mesh::Render,decltype(destroy_mesh)> base_mesh(next_base.mesh,destroy_mesh);
	UpdateMeshTex(next_badge,badge->instanced_sprite);
	std::unique_ptr<const types::mesh::Render,decltype(destroy_mesh)> badge_mesh(next_badge.mesh,destroy_mesh);
	const bool shown=m_render.is_rendered;Hide();
	delete m_render.name_sprite;
	if(m_render_data.base.mesh) {DELETE(m_render_data.base.mesh);}
	if(m_render_data.badge.mesh) {DELETE(m_render_data.badge.mesh);}
	m_owner_slot=owner->GetIndex();m_slot_badges=badges;m_faction=faction;
	m_name=std::move(name);m_is_owned=is_owned;
	m_render.name_sprite=label.release();m_render.instance_id=0;
	m_render.badge.instance_id=0;m_render.badge.def=badge;m_render.badge.pops_count=GetPopulation();
	m_render_data={next_base,next_badge};base_mesh.release();badge_mesh.release();
	if(shown) {Show();}
}

faction::Faction* const Base::GetFaction() const {
	return m_faction;
}

const bool Base::IsOwned() const {
	return m_is_owned;
}

tile::Tile* Base::GetTile() const {
	return m_tile;
}

sprite::Sprite* Base::GetSprite() const {
	return GetSpriteFor(m_faction);
}

sprite::Sprite* Base::GetSpriteFor( faction::Faction* faction ) const {
    if(m_native_body && m_bm->UsesNativeMapProjection()) {
        auto* source=m_native_body_faction?m_native_body_faction:faction;
        return source->GetBaseSprite(m_native_body->row>=3,uint8_t(m_native_body->column),uint8_t(m_native_body->row%3),true);
    }
	uint8_t size = 0;
	const auto population = GetPopulation();
	for ( uint8_t i = 0 ; i < s_base_render_population_thresholds.size() ; i++ ) {
		if ( s_base_render_population_thresholds.at( i ) > population ) {
			break;
		}
		size = i;
	}
	return faction->GetBaseSprite( m_tile->IsWater(), size, 0, m_bm->UsesNativeMapProjection() ); // TODO: perimeter
}

void Base::SetNativeBody(std::optional<::game::smac_base_body::Selection> selection, faction::Faction* source) {
    if(!selection) {source=nullptr;}
    if(selection && (selection->column<0 || selection->column>3 || selection->row<0 || selection->row>5)) {throw std::invalid_argument("Invalid main-map Base body");}
    if(source==m_native_body_faction && bool(selection)==bool(m_native_body) && (!selection || (selection->column==m_native_body->column && selection->row==m_native_body->row))) {return;}
    const bool shown=m_render.is_rendered;Hide();m_native_body=selection;m_native_body_faction=source;
    // Instance IDs belong to the selected sprite actor, not the Base globally.
    m_render.instance_id=0;UpdateMeshTex(m_render_data.base,GetSprite()->instanced_sprite);
    if(shown) {Show();}
}

void Base::Show() {
	if ( !m_render.is_rendered ) {

		if ( m_render.badge.pops_count != GetPopulation() ) {
			m_render.badge.pops_count = GetPopulation();
			UpdateMeshTex( m_render_data.base, GetSprite()->instanced_sprite );
			m_render.badge.def = m_slot_badges->GetBaseBadgeSprite( m_render.badge.pops_count, m_is_guarded );
			UpdateMeshTex( m_render_data.badge, m_render.badge.def->instanced_sprite );
		}

		const auto& c = m_render.coords;

		m_render.sprite = GetSprite();

		if ( !m_render.instance_id ) {
			m_render.instance_id = m_render.sprite->next_instance_id++;
		}
		m_render.sprite->instanced_sprite->actor->SetInstance(
			m_render.instance_id, {
				c.x,
				c.y,
				c.z
			}
		);

        if ( m_bm->m_smac_population ) {
            const auto& anchor=m_tile->GetRenderData().name_anchor;
            m_render.name_sprite->ShowAt(anchor?anchor->InvertY():c);
        }
		else { m_render.name_sprite->ShowAt(
			{
				m_render.coords.x,
				m_render.coords.y - 0.25f,
				m_render.coords.z - 0.25f
			}
		); }

		ShowBadge();

		m_render.is_rendered = true;
	}
}

void Base::Hide() {
	if ( m_render.is_rendered ) {
		m_render.sprite->instanced_sprite->actor->RemoveInstance( m_render.instance_id );
		m_render.name_sprite->Hide();
		HideBadge();
		m_render.is_rendered = false;
	}
}

void Base::Update() {
	const auto is_guarded = !m_tile->GetUnits().empty();
	if ( is_guarded != m_is_guarded ) {
		if ( m_render.is_rendered ) {
			HideBadge();
			m_render.badge.instance_id = 0;
		}
		m_is_guarded = is_guarded;
		m_render.badge.def = m_slot_badges->GetBaseBadgeSprite( GetPopulation(), m_is_guarded );
		UpdateMeshTex( m_render_data.badge, m_render.badge.def->instanced_sprite );
		if ( m_render.is_rendered ) {
			ShowBadge();
		}
	}
}

const Base::render_data_t& Base::GetRenderData() const {
	return m_render_data;
}

const Base::pops_t& Base::GetPops() const {
	return m_pops;
}

void Base::SetPops( const pops_t& pops ) { SetPops(pops,pops.size()); }
void Base::SetPops( const pops_t& pops, size_t population ) {
	m_pops = pops; // retained worker/specialist projection may precede population
	m_population = population;
}

void Base::SetRenderCoords( const types::Vec3& coords ) {
	Hide();
	m_render.coords = coords;
	Show();
}

void Base::ShowBadge() {
	if ( !m_render.badge.instance_id ) {
		m_render.badge.instance_id = m_render.badge.def->next_instance_id++;
	}
	m_render.badge.def->instanced_sprite->actor->SetInstance( m_render.badge.instance_id, m_slot_badges->GetRenderedBadgeCoords( m_render.coords ) );
}

void Base::HideBadge() {
	m_render.badge.def->instanced_sprite->actor->RemoveInstance( m_render.badge.instance_id );
}

void Base::UpdateMeshTex( meshtex_t& meshtex, const sprite::InstancedSprite* sprite ) {
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
}

}
}
}
