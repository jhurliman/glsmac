#include "ScoutSpriteCache.h"

#include "game/backend/unit/CvrScoutRender.h"
#include "resource/ResourceManager.h"
#include "types/texture/Texture.h"

#include <fstream>
#include <stdexcept>

namespace game::frontend::sprite {
namespace {
std::string ReadAsset( const std::string& path ) {
	std::ifstream input( path, std::ios::binary | std::ios::ate );
	if ( !input ) { throw std::runtime_error( "Cannot open Scout asset: " + path ); }
	const auto length = input.tellg();
	if ( length <= 0 || length > 32 * 1024 * 1024 ) { throw std::runtime_error( "Scout asset exceeds bounded input domain" ); }
	std::string bytes( static_cast< size_t >( length ), '\0' );
	input.seekg( 0 ); input.read( bytes.data(), static_cast< std::streamsize >( bytes.size() ) );
	if ( !input || input.peek() != std::char_traits< char >::eof() ) { throw std::runtime_error( "Scout asset changed or could not be read" ); }
	return bytes;
}
}

std::shared_ptr< types::texture::Texture > ScoutSpriteCache::Get( resource::ResourceManager& resources, const backend::unit::CvrScoutRender& descriptor ) {
	const auto key = descriptor.PixelKey();
	const auto existing = m_textures.find( key );
	if ( existing != m_textures.end() ) { return existing->second; }
	if ( m_textures.size() >= MAX_VIEWS ) { throw std::runtime_error( "Scout scene view cache is full" ); }
	if ( descriptor.composition == backend::unit::CvrScoutRender::Composition::SpartanRover ) {
		if ( !m_rover ) {
			// Only fixed composition filenames reach the installed/mod resolver.
			m_rover.emplace( loader::cvr::CvrSpartanRoverRenderer::Load( [ & ]( const std::string& name ) {
				return ReadAsset( resources.GetCustomPath( name ) );
			} ) );
		}
		const auto image = m_rover->Render( descriptor.view );
		std::shared_ptr< types::texture::Texture > texture( image.CreateTexture() );
		return m_textures.emplace( key, std::move( texture ) ).first->second;
	}
	if ( descriptor.composition == backend::unit::CvrScoutRender::Composition::InitializedScout ) {
		if ( !m_initialized ) {
			const auto infantry = ReadAsset( resources.GetCustomPath( "vi.cvr" ) );
			const auto reactor = ReadAsset( resources.GetCustomPath( "viptr00.cvr" ) );
			const auto palette = ReadAsset( resources.GetCustomPath( "PALETTE.PCX" ) );
			const auto system = ReadAsset( resources.GetCustomPath( "system.txt" ) );
			m_initialized.emplace( loader::cvr::CvrScoutRenderer::LoadInitialized( infantry, reactor, palette, system ) );
		}
		const auto image = m_initialized->Render( descriptor.view );
		std::shared_ptr< types::texture::Texture > texture( image.CreateTexture() );
		return m_textures.emplace( key, std::move( texture ) ).first->second;
	}
	if ( !m_renderer ) {
		// The serialized descriptor cannot choose paths. Use normal installed/mod resolution.
		const auto infantry = ReadAsset( resources.GetCustomPath( "vi.cvr" ) );
		const auto reactor = ReadAsset( resources.GetCustomPath( "viptr00.cvr" ) );
		m_renderer.emplace( loader::cvr::CvrScoutRenderer::Load( infantry, reactor ) );
	}
	const auto image = m_renderer->Render( descriptor.view );
	std::shared_ptr< types::texture::Texture > texture( image.CreateTexture() );
	return m_textures.emplace( key, std::move( texture ) ).first->second;
}

}
