#include "TextureLoader.h"

#include <algorithm>

#include "types/texture/Texture.h"
#include "types/texture/LazyTexture.h"
#include "engine/Engine.h"
#include "resource/ResourceManager.h"
#include "util/FinallyGuard.h"

namespace loader {
namespace texture {

// some pcxs contain special yellow shadows, replace with semi-transparent shadow
static const std::unordered_set< resource::resource_t > s_fix_yellow_shadow = {
	resource::PCX_TER1,
	resource::PCX_GAIANS,
	resource::PCX_HIVE,
	resource::PCX_UNIV,
	resource::PCX_MORGAN,
	resource::PCX_SPARTANS,
	resource::PCX_BELIEVE,
	resource::PCX_PEACE,
	resource::PCX_CYBORG,
	resource::PCX_PIRATES,
	resource::PCX_DRONE,
	resource::PCX_ANGELS,
	resource::PCX_FUNGBOY,
	resource::PCX_CARETAKE,
	resource::PCX_USURPER,
};

// different pcxs use different color for transparency, also some colors should be filtered out
static const auto s_tc_pink = types::Color::RGB( 255, 0, 255 );
static const auto s_tc_purple = types::Color::RGB( 152, 24, 228 );
static const auto s_tc_darkpurple = types::Color::RGB( 100, 16, 156 );
static const auto s_tc_aqua = types::Color::RGB( 24, 184, 228 );
static const auto s_tc_facborder = types::Color::RGB( 77, 156, 176 );
static const std::unordered_map< resource::resource_t, TextureLoader::transparent_colors_t > s_tcs = {
	{
		resource::PCX_TEXTURE,
		{
			types::Color::RGB( 125, 0, 128 ),
		}
	},
	{
		resource::PCX_INTERFACE,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_MOON1,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_MOON2,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_MOON3,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_ICONS,
		{
			s_tc_purple,
		}
	},
	{
		resource::PCX_NEWICONS,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_ALIENCIT,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_CONSOLE_X,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_CONSOLE2,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_CONSOLE2_A,
		{
			s_tc_darkpurple,
		}
	},
	{
		resource::PCX_CONSOLE_X2_A,
		{
			s_tc_pink,
		}
	},
	{
		resource::PCX_JACKAL,
		{
			s_tc_darkpurple,
		}
	},
	{
		resource::PCX_TER1,
		{
			s_tc_purple,
			s_tc_darkpurple, // tile markings
			s_tc_aqua, // borders
		}
	},
	{
		resource::PCX_UNITS,
		{
			s_tc_purple,
			s_tc_darkpurple, // tile markings
			s_tc_aqua, // borders
		}
	},
	{
		resource::PCX_FLAGS,
		{
			types::Color::RGB( 124, 124, 124 ),
			s_tc_aqua, // borders
		}
	},
	{
		resource::PCX_XI,
		{
			types::Color::RGB( 0, 67, 255 ),
			types::Color::RGB( 155, 27, 231 ), // label
			types::Color::RGB( 27, 187, 231 ), // borders
		}
	},
	{
		resource::PCX_XF,
		{
			types::Color::RGB( 0, 0, 0 ),
			types::Color::RGB( 155, 27, 231 ), // tile
			types::Color::RGB( 27, 187, 231 ), // borders
		}
	},
	{
		resource::PCX_GAIANS,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_HIVE,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_UNIV,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_MORGAN,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_SPARTANS,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_BELIEVE,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_PEACE,
		{
			s_tc_pink,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_CYBORG,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_PIRATES,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_DRONE,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_ANGELS,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_FUNGBOY,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_CARETAKE,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
	{
		resource::PCX_USURPER,
		{
			s_tc_darkpurple,
			s_tc_facborder,
		},
	},
};
static const TextureLoader::transparent_colors_t s_no_transparent_colors = {};

TextureLoader::~TextureLoader() {
	for ( const auto& it : m_color_textures ) {
		DELETE( it.second );
	}
	for ( const auto& it : m_lazy_textures ) {
		DELETE( it.second );
	}
}

const TextureLoader::transparent_colors_t& TextureLoader::GetTCs( const resource::resource_t res ) {
	const auto& transparent_colors_it = s_tcs.find( res );
	if ( transparent_colors_it != s_tcs.end() ) {
		return transparent_colors_it->second;
	}
	else {
		return s_no_transparent_colors;
	}
}

types::texture::LazyTexture* TextureLoader::GetLazyTexture( const std::string& filename, const types::texture::texture_flag_t flags ) {
	const auto& it = m_lazy_textures.find( filename );
	if ( it != m_lazy_textures.end() ) {
		return it->second;
	}
	return m_lazy_textures.insert( { filename, new types::texture::LazyTexture( this, filename, flags ) } ).first->second;
}

types::texture::Texture* TextureLoader::LoadTexture( const resource::resource_t res, const types::texture::texture_flag_t flags ) {
	transparent_colors_t colors_old = m_transparent_colors;
	const bool fix_yellow_shadows_old = m_fix_yellow_shadows;
	util::FinallyGuard restore( [ & ] { m_transparent_colors.swap( colors_old ); m_fix_yellow_shadows = fix_yellow_shadows_old; } );
	m_transparent_colors = GetTCs( res );
	m_fix_yellow_shadows = s_fix_yellow_shadow.find( res ) != s_fix_yellow_shadow.end();
	auto* result = LoadTextureImpl( GetPath( res ), flags );
	return result;
}

types::texture::Texture* TextureLoader::TryLoadCustomTexture( const std::string& filename, const types::texture::texture_flag_t flags ) {
	std::string key;
	key.resize( filename.size() );
	std::transform( filename.begin(), filename.end(), key.begin(), ::tolower );
	const auto res = g_engine->GetResourceManager()->GetResource( filename );
	transparent_colors_t colors_old = m_transparent_colors;
	const bool fix_yellow_shadows_old = m_fix_yellow_shadows;
	util::FinallyGuard restore( [ & ] { m_transparent_colors.swap( colors_old ); m_fix_yellow_shadows = fix_yellow_shadows_old; } );
	if ( res != resource::NONE ) {
		m_transparent_colors = GetTCs( res );
		m_fix_yellow_shadows = s_fix_yellow_shadow.find( res ) != s_fix_yellow_shadow.end();
	}
	const auto path = TryGetCustomFilename( filename );
    if(flags & types::texture::TF_SMAC_SOFTWARE_TERRAIN) {
        // Explicit indexed software imports preserve the source key/palette.
        // Named faction sheets otherwise inherit legacy RGB shadow filters.
        m_transparent_colors.clear();
        m_fix_yellow_shadows=false;
    }
	auto* result = path.empty()
		? nullptr
		: LoadTextureImpl( path, flags );
	return result;
}

types::texture::Texture* TextureLoader::LoadCustomTexture( const std::string& filename, const types::texture::texture_flag_t flags ) {
	auto* result = TryLoadCustomTexture( filename, flags );
	if ( !result ) {
		THROW( "could not load texture: " + filename );
	}
	return result;
}

types::texture::Texture* TextureLoader::LoadTexture( const resource::resource_t res, const size_t x1, const size_t y1, const size_t x2, const size_t y2, const uint8_t flags, const float value, const types::texture::texture_flag_t texture_flags ) {
	transparent_colors_t colors_old = m_transparent_colors;
	util::FinallyGuard restore( [ & ] { m_transparent_colors.swap( colors_old ); } );
	m_transparent_colors = GetTCs( res );
	types::texture::Texture* result = LoadTextureImpl( GetPath( res ), x1, y1, x2, y2, flags, value, texture_flags );
	return result;
}

std::shared_ptr<types::texture::Texture> TextureLoader::GetSmacEffectTexture(bool) {THROW("Indexed effects are unsupported by this texture loader");}

std::shared_ptr<const game::smac_detail::Sprite> TextureLoader::GetSmacDetailBaseSprite(const std::string&,size_t,size_t) {throw std::runtime_error("Native Detail Base source unavailable");}
std::shared_ptr<const SmacBaseCard> TextureLoader::GetSmacNativeBaseCard(const std::string&,size_t,size_t,bool) {return {};}

std::shared_ptr<const SmacGraphPalette> TextureLoader::GetSmacGraphPalette() {THROW("Dominance graph colors are unsupported by this texture backend");}

std::shared_ptr<const SmacTurnFrames> TextureLoader::GetSmacTurnFrames() {THROW("Turn Complete decoding is unsupported by this texture backend");}

types::texture::Texture* TextureLoader::CreateSmacNativeBadgeTexture(int,int) {THROW("Native badge decoding is not supported by this texture loader");}
std::shared_ptr<const SmacUnitBadge> TextureLoader::GetSmacUnitBadge(int,int,int) {return {};}
int TextureLoader::GetSmacChassisFlagOffset(int) {return -1;}
std::shared_ptr<const game::frontend::native_terrain::Source> TextureLoader::GetSmacTerrainSource(const std::array<std::string,8>&) {THROW("Indexed terrain is unsupported by this texture loader");}

types::texture::Texture* TextureLoader::CreateSmacMenuTexture( const std::string&, const std::string&, size_t, size_t, size_t, size_t ) {
	THROW( "SMAC menu composition is unsupported by this texture backend" );
}

types::texture::Texture* TextureLoader::GetColorTexture( const types::Color& color ) {
	const types::Color::rgba_t c = color.GetRGBA();
	const auto& it = m_color_textures.find( c );
	if ( it != m_color_textures.end() ) {
		return it->second;
	}
	auto* texture = types::texture::Texture::FromColor( color );
	m_color_textures[ c ] = texture;
	return texture;
}

}
}

namespace loader::texture {
std::shared_ptr<const SmacDialogSource> TextureLoader::GetSmacDialogSource() {return {};}
}
