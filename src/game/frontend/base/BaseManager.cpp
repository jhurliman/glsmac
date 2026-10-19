#include "BaseManager.h"

#include <algorithm>

#include "Base.h"
#include "game/frontend/Game.h"
#include "game/frontend/Slot.h"
#include "game/frontend/unit/UnitManager.h"
#include "game/frontend/tile/TileManager.h"
#include "game/frontend/text/InstancedTextManager.h"
#include "game/frontend/text/InstancedText.h"
#include "game/frontend/faction/Faction.h"
#include "game/backend/base/PopDef.h"
#include "PopDef.h"
#include "types/mesh/Rectangle.h"
#include "engine/Engine.h"
#include "loader/font/FontLoader.h"
#include "SlotBadges.h"
#include "game/backend/Game.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/Base.h"
#include "game/backend/map/Map.h"
#include "gse/value/Int.h"
#include "loader/cvr/CvrActorMaterial.h"
#include "resource/ResourceManager.h"
#include "util/FS.h"

namespace game {
namespace frontend {
namespace base {

BaseManager::BaseManager( Game* game, bool smac_population )
	: m_game( game )
	, m_ism( game->GetISM() )
	, m_name_font( game->GetITM()->GetInstancedFont( smac_population
		? g_engine->GetFontLoader()->LoadCustomFont("arial.ttf",12,loader::font::RasterMode::MONOCHROME)
		: g_engine->GetFontLoader()->LoadFont(::resource::TTF_ARIALN,48),smac_population ) )
	, m_badge_font( game->GetITM()->GetInstancedFont( g_engine->GetFontLoader()->LoadFont( ::resource::TTF_ARIALNB, 48 ) ) )
	, m_smac_population( smac_population ) {
}

BaseManager::~BaseManager() {
	for ( const auto& it : m_bases ) {
		delete it.second;
	}
	for ( const auto& it : m_popdefs ) {
		delete it.second;
	}
	for ( const auto& it : m_slot_badges ) {
		delete it.second;
	}
}

base::Base* BaseManager::GetBaseById( const size_t id ) const {
	const auto& it = m_bases.find( id );
	return it != m_bases.end()
		? it->second
		: nullptr;
}

void BaseManager::DefinePop( const backend::base::PopDef* def ) {
	ASSERT( m_popdefs.find( def->m_id ) == m_popdefs.end(), "popdef already defined: " + def->m_id );
	m_popdefs_order.push_back( def->m_id );
	m_popdefs.insert(
		{
			def->m_id,
			new PopDef(
				def->m_name,
				def->m_renders_human,
				def->m_renders_progenitor
			)
		}
	);
}

void BaseManager::UndefinePop( const std::string& id ) {
	const auto& it = m_popdefs.find( id );
	ASSERT( it != m_popdefs.end(), "popdef does not exist: " + id );
	delete it->second;
	m_popdefs.erase( it );
	for ( auto it = m_popdefs_order.begin() ; it != m_popdefs_order.end() ; it++ ) {
		if ( ( *it ) == id ) {
			m_popdefs_order.erase( it );
			break;
		}
	}
}

const std::vector< std::string >& BaseManager::GetPopDefOrder() const {
	return m_popdefs_order;
}

PopDef* BaseManager::GetPopDef( const std::string& id ) const {
	ASSERT( m_popdefs.find( id ) != m_popdefs.end(), "popdef not defined: " + id );
	return m_popdefs.at( id );
}

void BaseManager::SpawnBase(
	const size_t base_id,
	const size_t slot_index,
	const types::Vec2< size_t >& tile_coords,
	const types::Vec3& render_coords,
	const std::string& name
) {

	ASSERT( m_bases.find( base_id ) == m_bases.end(), "base id already exists" );

	auto* tile = m_game->GetTM()->GetTile( tile_coords );
	auto* slot = m_game->GetSlot( slot_index );
	auto* faction = slot->GetFaction();

	auto* base = new base::Base(
		this,
		base_id,
		name,
		slot,
		tile,
		slot_index == m_game->GetMySlotIndex(),
		render_coords,
		m_game->GetITM()->CreateInstancedText(
			name,
			m_name_font,
			faction->m_colors.text,
			faction->m_colors.text_shadow
		)
	);

	m_bases.insert(
		{
			base_id,
			base
		}
	);
	auto it = m_owner_base_ids.find( slot_index );
	if ( it == m_owner_base_ids.end() ) {
		it = m_owner_base_ids.insert(
			{
				slot_index,
				{}
			}
		).first;
	}
	ASSERT( it->second.find( base_id ) == it->second.end(), "faction base id already exists" );
	it->second.insert( base_id );

	RefreshBase( base );
}

void BaseManager::DespawnBase( const size_t base_id ) {
	const auto& it = m_bases.find( base_id );
	ASSERT( it != m_bases.end(), "base id not found" );

	auto* base = it->second;
	const auto faction = m_owner_base_ids.find( base->GetOwnerSlot() );
	ASSERT( faction != m_owner_base_ids.end() && faction->second.count( base_id ), "base not in owner ids" );
	faction->second.erase( base_id );
	if ( faction->second.empty() ) { m_owner_base_ids.erase( faction ); }

	m_bases.erase( it );

	delete base;
	// The backend wrapper has already retired. Notify UI by stable scalar ID.
	m_game->Trigger( m_game->GetGame(), "base_despawn", ARGS_F( base_id ) {
		{ "base_id", VALUE( gse::value::Int,, base_id ) },
	}; } );

	m_game->RefreshSelectedTile();
}

void BaseManager::RefreshBase( Base* base ) {
	m_game->RenderTile( base->GetTile(), m_game->GetUM()->GetSelectedUnit() );
	m_game->UpdateRelatedWidgets( ui::WT_BASE_PREVIEW, base->GetId(), base );
}

void BaseManager::UpdateBaseIdentity( Base* base, size_t owner_slot, faction::Faction* faction, const std::string& name ) {
	ASSERT(base && GetBaseById(base->GetId())==base && faction,"invalid Base identity update");
	if(base->GetOwnerSlot()==owner_slot && base->GetFaction()==faction && base->GetName()==name) {return;}
	auto* owner=m_game->GetSlot(owner_slot);
	const auto* colors=owner->GetFaction();
	std::unique_ptr<text::InstancedText> label(m_game->GetITM()->CreateInstancedText(name,m_name_font,colors->m_colors.text,colors->m_colors.text_shadow));
	auto records=m_owner_base_ids;
	auto old=records.find(base->GetOwnerSlot());
	ASSERT(old!=records.end() && old->second.count(base->GetId())==1,"missing old Base owner");
	old->second.erase(base->GetId());
	if(old->second.empty()) {records.erase(old);}
	records[owner_slot].insert(base->GetId());
	base->SetIdentity(owner,faction,name,owner_slot==m_game->GetMySlotIndex(),std::move(label));
	m_owner_base_ids.swap(records);
}

/* TODO void BaseManager::DespawnBase( const size_t base_id ) {
	const auto& it = m_units.find( unit_id );
	ASSERT( it != m_units.end(), "unit id not found" );

	auto* unit = it->second;

	m_units.erase( it );

	if ( unit->IsOwned() ) {
		RemoveSelectable( unit );
	}

	delete unit;

	m_game->RefreshSelectedTile( m_selected_unit );

}*/

SlotBadges* BaseManager::GetSlotBadges( const size_t slot_index ) const {
	ASSERT( m_slot_badges.find( slot_index ) != m_slot_badges.end(), "slot base badges for index " + std::to_string( slot_index ) + " not defined" );
	return m_slot_badges.at( slot_index );
}

void BaseManager::DefineSlotBadges( const size_t slot_index, const faction::Faction* faction ) {
	ASSERT( m_slot_badges.find( slot_index ) == m_slot_badges.end(), "slot base badges for index " + std::to_string( slot_index ) + " already defined" );
	m_slot_badges.insert(
		{
			slot_index,
			new SlotBadges( this, m_ism, slot_index, faction )
		}
	);
}

void BaseManager::SelectBase( Base* base ) {
	m_game->UpdateTilePreview( base->GetTile() );
	auto* game = m_game->GetGame();
	auto* const b = game->GetBM()->GetBase( base->GetId() );
	m_game->Trigger(
		game->GetMap(), "base_preview", ARGS_F( &b ) {
			{
				"base",
				b->Wrap( GSE_CALL )
			},
		}; }
	);
	m_game->Trigger(
		game, "base_select", ARGS_F( &b ) {
			{
				"base",
				b->Wrap( GSE_CALL )
			},
		}; }
	);
}

Base* BaseManager::GetBaseBefore( Base* base ) const {
	const auto& ids_it = m_owner_base_ids.find( base->GetOwnerSlot() );
	ASSERT( ids_it != m_owner_base_ids.end(), "owner ids not found" );
	const auto& ids = ids_it->second;
	auto it = ids.find( base->GetId() );
	ASSERT( it != ids.end(), "base not in faction ids" );
	if ( it == ids.begin() ) {
		it = ids.end();
	}
	it--;
	ASSERT( m_bases.find( *it ) != m_bases.end(), "base id not found" );
	return m_bases.at( *it );
}

Base* BaseManager::GetBaseAfter( Base* base ) const {
	const auto& ids_it = m_owner_base_ids.find( base->GetOwnerSlot() );
	ASSERT( ids_it != m_owner_base_ids.end(), "owner ids not found" );
	const auto& ids = ids_it->second;
	auto it = ids.find( base->GetId() );
	ASSERT( it != ids.end(), "base not in faction ids" );
	it++;
	if ( it == ids.end() ) {
		it = ids.begin();
	}
	ASSERT( m_bases.find( *it ) != m_bases.end(), "base id not found" );
	return m_bases.at( *it );
}

text::InstancedFont* BaseManager::GetBadgeFont() const {
	return m_badge_font;
}

const types::Font* BaseManager::GetSmacPopulationFont() {
	if (!m_smac_population_font) {
		m_smac_population_font = g_engine->GetFontLoader()->LoadCustomFont("arialbd.ttf",16,loader::font::RasterMode::MONOCHROME);
	}
	return m_smac_population_font;
}

const std::array<types::Color,256>& BaseManager::GetSmacPopulationPalette() {
	if (!m_smac_population_palette_loaded) {
		std::vector<unsigned char> bytes;
		util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath("PALETTE.PCX"));
		const auto colors=loader::cvr::CvrActorMaterial::ReadSoftwareColors(
			std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
		for(size_t i=0;i<colors.size();++i) {
			m_smac_population_palette[i]=types::Color::FromRGB(colors[i][0],colors[i][1],colors[i][2]);
		}
		m_smac_population_palette_loaded=true;
	}
	return m_smac_population_palette;
}

}
}
}
