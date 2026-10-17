#include "InstancedSpriteManager.h"

#include "InstancedSprite.h"
#include "ScoutSpriteCache.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "engine/Engine.h"
#include <memory>
#include "scene/Scene.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sprite.h"
#include "types/texture/Texture.h"

namespace game {
namespace frontend {
namespace sprite {

InstancedSpriteManager::InstancedSpriteManager( scene::Scene* scene )
	: m_scene( scene ) {}

InstancedSpriteManager::~InstancedSpriteManager() {
	for ( auto& sprite : m_instanced_sprites ) {
		m_scene->RemoveActor( sprite.second.actor );
		DELETE( sprite.second.actor );
	}
	for ( auto& sprite : m_repainted_instanced_sprites ) {
		m_scene->RemoveActor( sprite.second.actor );
		DELETE( sprite.second.actor );
	}
	m_instanced_sprites.clear();

	for ( const auto& it : m_repainted_textures ) {
		DELETE( it.second );
	}
}

InstancedSprite* InstancedSpriteManager::GetScoutSprite( const std::string& name, const backend::unit::CvrScoutRender& descriptor ) {
	const auto key = "Scout:" + std::to_string( name.size() ) + ":" + name + ":" + descriptor.PlacementKey();
	const auto existing = m_instanced_sprites.find( key );
	if ( existing != m_instanced_sprites.end() ) { return &existing->second; }
	if ( !m_scout_cache ) { m_scout_cache = std::make_unique< ScoutSpriteCache >(); }
	auto texture = m_scout_cache->Get( *g_engine->GetResourceManager(), descriptor );
	const auto delete_sprite = []( scene::actor::Sprite* value ) { DELETE( value ); };
	const auto delete_actor = []( scene::actor::Instanced* value ) { DELETE( value ); };
	NEWV( sprite, scene::actor::Sprite, name, descriptor.size, texture.get(), { { 0, 0 }, { 1, 1 } }, descriptor.Offset() );
	std::unique_ptr< scene::actor::Sprite, decltype( delete_sprite ) > sprite_owner( sprite, delete_sprite );
	NEWV( actor, scene::actor::Instanced, sprite );
	sprite_owner.release();
	std::unique_ptr< scene::actor::Instanced, decltype( delete_actor ) > actor_owner( actor, delete_actor );
	actor->SetZIndex( s_zlevel_map.at( ZL_UNITS ) );
	// Publish the owned value before scene registration; roll back both on failure.
	auto inserted = m_instanced_sprites.emplace( key, InstancedSprite{ key, name, { 0, 0 }, { 256, 256 }, { 128, 128 }, actor, texture } ).first;
	try { m_scene->AddActor( actor ); }
	catch ( ... ) { m_scene->RemoveActor( actor ); m_instanced_sprites.erase( inserted ); throw; }
	actor_owner.release();
	return &inserted->second;
}

InstancedSprite* InstancedSpriteManager::GetInstancedSprite(
	const std::string& name,
	types::texture::Texture* texture,
	const backend::map::pcx_texture_coordinates_t& src_xy,
	const backend::map::pcx_texture_coordinates_t& src_wh,
	const backend::map::pcx_texture_coordinates_t& src_cxy,
	const types::Vec2< float > dst_wh,
	const z_level_t z_level,
	const float z_index_adjustment
) {

	ASSERT( s_zlevel_map.find( z_level ) != s_zlevel_map.end(), "unknown z level " + std::to_string( z_level ) );
	ASSERT( z_index_adjustment >= -MAX_ZINDEX_ADJUSTMENT && z_index_adjustment <= MAX_ZINDEX_ADJUSTMENT, "z index adjustment too large" );

	const auto key = name + " " + src_xy.ToString() + " " + src_wh.ToString();

	auto it = m_instanced_sprites.find( key );
	if ( it == m_instanced_sprites.end() ) {

		Log( "Creating instanced sprite: " + key );

		const auto tw = texture->GetWidth();
		const auto th = texture->GetHeight();

		NEWV(
			sprite,
			scene::actor::Sprite,
			name,
			{
				dst_wh.x,
				dst_wh.y
			},
			texture,
			{
				{
					(float)1.0f / tw * ( src_xy.x ),
					(float)1.0f / th * ( src_xy.y )
				},
				{
					(float)1.0f / tw * ( src_xy.x + src_wh.x ),
					(float)1.0f / th * ( src_xy.y + src_wh.y )
				}
			},
			{
				dst_wh.x * ( ( (float)( src_cxy.x - src_xy.x ) / src_wh.x ) - 0.5f ),
				dst_wh.y * ( ( (float)( src_cxy.y - src_xy.y ) / src_wh.y ) - 0.5f )
			}
		);
		NEWV( instanced, scene::actor::Instanced, sprite );
		instanced->SetZIndex( s_zlevel_map.at( z_level ) + z_index_adjustment );
		m_scene->AddActor( instanced );
		it = m_instanced_sprites.insert(
			{
				key,
				{
					key,
					name,
					src_xy,
					src_wh,
					src_cxy,
					instanced
				}
			}
		).first;
	}
	return &it->second;
}

InstancedSprite* InstancedSpriteManager::GetInstancedSpriteByKey( const std::string& key ) {
	const auto& it = m_instanced_sprites.find( key );
	ASSERT( it != m_instanced_sprites.end(), "sprite actor '" + key + "' not found" );
	return &it->second;
}

InstancedSprite* InstancedSpriteManager::GetRepaintedInstancedSprite( const std::string& name, const InstancedSprite* original, const types::texture::repaint_rules_t& rules ) {
	const auto& key = name;

	auto it = m_repainted_instanced_sprites.find( key );
	if ( it == m_repainted_instanced_sprites.end() ) {

		Log( "Creating repainted instanced sprite: " + key );

		const auto* original_sprite = original->actor->GetSpriteActor();
		auto* texture = GetRepaintedSourceTexture( name, original_sprite->GetTexture(), rules );

		NEWV(
			sprite,
			scene::actor::Sprite,
			name,
			original_sprite->GetDimensions(),
			texture,
			original_sprite->GetTexCoords(),
			original_sprite->GetDstOffsets()
		);
		sprite->SetRenderFlags( original_sprite->GetRenderFlags() );
		NEWV( instanced, scene::actor::Instanced, sprite );
		instanced->SetZIndex( original->actor->GetZIndex() );
		m_scene->AddActor( instanced );
		it = m_repainted_instanced_sprites.insert(
			{
				key,
				{
					key,
					name,
					original->xy,
					original->wh,
					original->cxy,
					instanced
				}
			}
		).first;
	}
	return &it->second;
}

void InstancedSpriteManager::RemoveInstancedSpriteByKey( const std::string& key ) {
	const auto& it = m_instanced_sprites.find( key );
	ASSERT( it != m_instanced_sprites.end(), "instanced sprite not found: " + key );
	Log( "Removing instanced sprite: " + key );
	const auto& sprite = it->second;
	m_scene->RemoveActor( sprite.actor );
	DELETE( sprite.actor );
	m_instanced_sprites.erase( it );
}

void InstancedSpriteManager::RemoveRepaintedInstancedSpriteByKey( const std::string& key ) {
	const auto& it = m_repainted_instanced_sprites.find( key );
	ASSERT( it != m_repainted_instanced_sprites.end(), "instanced sprite not found: " + key );
	Log( "Removing instanced sprite: " + key );
	const auto& sprite = it->second;
	m_scene->RemoveActor( sprite.actor );
	DELETE( sprite.actor );
	m_repainted_instanced_sprites.erase( it );
}

types::texture::Texture* InstancedSpriteManager::GetRepaintedSourceTexture( const std::string& name, const types::texture::Texture* original, const types::texture::repaint_rules_t& rules ) {
	const auto it = m_repainted_textures.find( name );
	if ( it != m_repainted_textures.end() ) {
		return it->second;
	}
	NEWV( texture, types::texture::Texture, original->GetFilename(), original->GetWidth(), original->GetHeight() );
	texture->RepaintFrom( original, rules );
	m_repainted_textures.insert(
		{
			name,
			texture
		}
	);
	return texture;
}

}
}
}
