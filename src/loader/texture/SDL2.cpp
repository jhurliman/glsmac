#include "SDL2.h"
#include "SmacPalette.h"
#include "util/FinallyGuard.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "game/frontend/SmacWaterCoast.h"
#include "SmacEffect.h"
#include "SmacNativeBadge.h"
#include "SmacBaseCard.h"
#include "SmacTurnFrames.h"
#include "SmacGraphPalette.h"
#include "loader/flic/TurnComplete.h"
#include "loader/cvr/CvrActorMaterial.h"
#include "loader/flic/MenuCompositor.h"
#include <algorithm>

#include <memory>

#include "util/FS.h"
#include "types/texture/Texture.h"

namespace loader {
namespace texture {

SDL2::~SDL2() {
	for ( auto& it : m_textures ) {
		DELETE( it.second );
	}
	for ( auto& it : m_subtextures ) {
		DELETE( it.second );
	}
}

void SDL2::Start() {

}

void SDL2::Stop() {

}

void SDL2::Iterate() {

}

SDL_Surface* load_image( const std::vector< unsigned char >& buffer ) {
	SDL_RWops* rw = SDL_RWFromConstMem( buffer.data(), buffer.size() );
	ASSERT( rw, "rw is null" );
	return IMG_Load_RW( rw, 1 );
}

std::shared_ptr<const SmacGraphPalette> SDL2::GetSmacGraphPalette() {
    const auto path=GetCustomFilename("palette.pcx");
    const auto cached=m_graph_palettes.find(path);if(cached!=m_graph_palettes.end()) {return cached->second;}
    std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,path);
    using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
    Surface image(load_image(bytes),SDL_FreeSurface);
    if(!image || !image->format || image->format->BitsPerPixel!=8 || !image->pixels
        || image->w<637 || image->h<32 || image->w>2048 || image->h>2048 || image->pitch<image->w) {
        THROW("Dominance graph palette metadata unavailable");
    }
    auto result=std::make_shared<SmacGraphPalette>();
    const auto colors=MakeSmacSoftwarePalette(image->format->palette);
    for(size_t index=0;index<256;++index) {for(size_t channel=0;channel<3;++channel) {result->colors[index][channel]=colors[index][channel];}}
    for(size_t slot=0;slot<8;++slot) {
        result->normal[slot]=static_cast<uint8_t>(static_cast<const uint8_t*>(image->pixels)[23*size_t(image->pitch)+580+8*slot]+10);
        result->alternate[slot]=static_cast<uint8_t>(static_cast<const uint8_t*>(image->pixels)[31*size_t(image->pitch)+580+8*slot]+10);
    }
    return m_graph_palettes.emplace(path,std::move(result)).first->second;
}

std::shared_ptr<const SmacTurnFrames> SDL2::GetSmacTurnFrames() {
    const auto animation=GetCustomFilename("turn.flc"),palette=GetCustomFilename("palette.pcx");
    const auto key=animation+":"+palette;
    const auto found=m_turn_frames.find(key);if(found!=m_turn_frames.end()) {return found->second;}
    std::vector<uint8_t> bytes;util::FS::ReadFile(bytes,animation);
    const auto indexed=loader::flic::DecodeTurnComplete(bytes);
    util::FS::ReadFile(bytes,palette);
    using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
    Surface common(load_image(bytes),SDL_FreeSurface);
    const auto colors=MakeSmacSoftwarePalette(common && common->format ? common->format->palette : nullptr);
    auto result=std::make_shared<SmacTurnFrames>();
    for(size_t f=0;f<19;++f) {
        auto& rgba=result->rgba[f];rgba.resize(233*23*4);
        for(size_t p=0;p<233*23;++p) {
            const auto index=indexed[f].pixels[p];
            for(size_t c=0;c<3;++c) {rgba[p*4+c]=colors[index][c];}
            // Key0 restores the retained console backing, never palette0 black.
            rgba[p*4+3]=index ? 255 : 0;
        }
    }
    return m_turn_frames.emplace(key,std::move(result)).first->second;
}

struct SmacBaseCardSource {
    size_t width,height;
    std::vector<uint8_t> indexed;
    std::array<types::Color::rgba_t,256> palette;
};
std::shared_ptr<const SmacBaseCard> SDL2::GetSmacNativeBaseCard(const std::string& faction,size_t x,size_t y,bool roster) {
    const auto faction_path=GetCustomFilename(faction),palette_path=GetCustomFilename("palette.pcx");
    const auto source_key=faction_path+":"+palette_path;
    const auto key=source_key+":"+std::to_string(x)+":"+std::to_string(y)+(roster?":roster":":selected");
    const auto cached=m_base_cards.find(key);if(cached!=m_base_cards.end()) {return cached->second;}
    auto source=m_base_card_sources.find(source_key);
    if(source==m_base_card_sources.end()) {
        std::vector<unsigned char> bytes;
        using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
        util::FS::ReadFile(bytes,faction_path);Surface image(load_image(bytes),SDL_FreeSurface);
        if(!image || !image->format || image->format->BitsPerPixel!=8 || !image->pixels
            || image->w<=0 || image->h<=0 || image->w>2048 || image->h>2048 || image->pitch<image->w) {return {};}
        auto decoded=std::make_shared<SmacBaseCardSource>();
        decoded->width=image->w;decoded->height=image->h;decoded->indexed.resize(decoded->width*decoded->height);
        for(size_t row=0;row<decoded->height;++row) {for(size_t col=0;col<decoded->width;++col) {
            const auto raw=static_cast<const uint8_t*>(image->pixels)[row*size_t(image->pitch)+col];
            decoded->indexed[row*decoded->width+col]=static_cast<uint8_t>(raw+10);
        }}
        util::FS::ReadFile(bytes,palette_path);
        Surface common(load_image(bytes),SDL_FreeSurface);
        const auto colors=MakeSmacSoftwarePalette(common && common->format ? common->format->palette : nullptr);
        for(size_t c=0;c<256;++c) {decoded->palette[c]=types::Color::RGB(colors[c][0],colors[c][1],colors[c][2]);}
        source=m_base_card_sources.emplace(source_key,std::move(decoded)).first;
    }
    const auto& data=*source->second;
    auto card=MakeSmacIndexedBaseCard(data.indexed,data.width,data.height,x,y,roster,data.palette);
    if(!card) {return {};}
    return m_base_cards.emplace(key,std::make_shared<const SmacBaseCard>(std::move(*card))).first->second;
}

struct SmacNativeBadgeSource {std::array<SmacNativeBadge,14> badges;};
std::shared_ptr<const game::frontend::native_terrain::Source> SDL2::GetSmacTerrainSource(const std::array<std::string,8>& faction_files) {
    const auto path=GetCustomFilename("texture.pcx"),common_path=GetCustomFilename("palette.pcx");
    const auto rainfall_path=GetCustomFilename("rainfall.pcx"),ter1_path=GetCustomFilename("ter1.pcx");
    const auto system_path=GetCustomFilename("system.txt");
    std::array<std::string,8> faction_paths{};
    auto key=path+":"+common_path+":"+rainfall_path+":"+system_path+":"+ter1_path;
    for(size_t i=0;i<8;++i) {
        if(!faction_files[i].empty()) {faction_paths[i]=GetCustomFilename(faction_files[i]);}
        key+=":"+std::to_string(faction_paths[i].size())+":"+faction_paths[i];
    }
    const auto found=m_native_terrain_sources.find(key);if(found!=m_native_terrain_sources.end()) {return found->second;}
    using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
    std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,path);Surface image(load_image(bytes),SDL_FreeSurface);
    util::FS::ReadFile(bytes,common_path);Surface common(load_image(bytes),SDL_FreeSurface);
    if(!image || image->w!=1024 || image->h!=768) {THROW("Unsupported native terrain atlas dimensions");}
    ApplySmacTerrainPalette(image.get(),common && common->format?common->format->palette:nullptr);
    auto source=std::make_shared<game::frontend::native_terrain::Source>();
    source->loaded={image->w,image->h,std::vector<uint8_t>(static_cast<size_t>(image->w)*static_cast<size_t>(image->h))};
    for(size_t i=0;i<256;++i) {
        const auto color=image->format->palette->colors[static_cast<uint8_t>(i-10)];source->palette[i]={color.r,color.g,color.b};
    }
    if(SDL_LockSurface(image.get())!=0) {THROW(SDL_GetError());}
    util::FinallyGuard unlock([&]{SDL_UnlockSurface(image.get());});
    for(int y=0;y<image->h;++y) {
        const auto* row=static_cast<const uint8_t*>(image->pixels)+y*image->pitch;
        for(int x=0;x<image->w;++x) {source->loaded.pixels[static_cast<size_t>(y)*static_cast<size_t>(image->w)+static_cast<size_t>(x)]=static_cast<uint8_t>(row[x]+10);}
    }
    source->shadow_lookup=game::frontend::smac_developed_overlay::ShadowLookup(source->palette);
    util::FS::ReadFile(bytes,ter1_path);Surface ter1(load_image(bytes),SDL_FreeSurface);
    if(!ter1 || !ter1->format || ter1->format->BitsPerPixel!=8 || ter1->w<1023 || ter1->h<704 ||
        ter1->w>=32767 || ter1->h>=32767 || ter1->pitch<ter1->w) {THROW("Unsupported native developed atlas source");}
    source->ter1={ter1->w,ter1->h,std::vector<uint8_t>(static_cast<size_t>(ter1->w)*static_cast<size_t>(ter1->h))};
    if(SDL_LockSurface(ter1.get())!=0) {THROW(SDL_GetError());}
    util::FinallyGuard unlock_ter1([&]{SDL_UnlockSurface(ter1.get());});
    for(int y=0;y<ter1->h;++y) {
        const auto* row=static_cast<const uint8_t*>(ter1->pixels)+static_cast<size_t>(y)*static_cast<size_t>(ter1->pitch);
        for(int x=0;x<ter1->w;++x) {source->ter1.pixels[static_cast<size_t>(y)*static_cast<size_t>(ter1->w)+static_cast<size_t>(x)]=static_cast<uint8_t>(row[x]+10);}
    }
    // Coast masks depend on source indices, not converted RGBA colors.
    util::FS::ReadFile(bytes,rainfall_path);Surface rainfall(load_image(bytes),SDL_FreeSurface);
    if(!rainfall || !rainfall->format || rainfall->format->BitsPerPixel!=8 || rainfall->w<115 || rainfall->h<389 ||
        rainfall->w>=32767 || rainfall->h>=32767 || rainfall->pitch<rainfall->w) {THROW("Unsupported native coast mask source");}
    game::frontend::smac_terrain_raster::Bitmap loaded_rainfall{rainfall->w,rainfall->h,
        std::vector<uint8_t>(static_cast<size_t>(rainfall->w)*static_cast<size_t>(rainfall->h))};
    if(SDL_LockSurface(rainfall.get())!=0) {THROW(SDL_GetError());}
    util::FinallyGuard unlock_rainfall([&]{SDL_UnlockSurface(rainfall.get());});
    for(int y=0;y<rainfall->h;++y) {
        const auto* row=static_cast<const uint8_t*>(rainfall->pixels)+static_cast<size_t>(y)*static_cast<size_t>(rainfall->pitch);
        for(int x=0;x<rainfall->w;++x) {loaded_rainfall.pixels[static_cast<size_t>(y)*static_cast<size_t>(rainfall->w)+static_cast<size_t>(x)]=static_cast<uint8_t>(row[x]+10);}
    }
    source->coast_mask=game::frontend::smac_water_coast::CoastMasks(loaded_rainfall)[0];
    source->boundary_mask=game::frontend::smac_territory_boundary::Mask(loaded_rainfall);
    source->faction_files=faction_files;
    const auto loaded_index=[](SDL_Surface* surface,int x,int y)->uint8_t {
        if(!surface || !surface->format || surface->format->BitsPerPixel!=8 || !surface->pixels ||
            x<0 || y<0 || x>=surface->w || y>=surface->h || surface->pitch<surface->w) {THROW("Unsupported native boundary metadata image");}
        if(SDL_LockSurface(surface)!=0) {THROW(SDL_GetError());}
        util::FinallyGuard unlock_sample([&]{SDL_UnlockSurface(surface);});
        return static_cast<uint8_t>(static_cast<const uint8_t*>(surface->pixels)[static_cast<size_t>(y)*static_cast<size_t>(surface->pitch)+static_cast<size_t>(x)]+10);
    };
    std::array<uint8_t,8> targets{};
    for(size_t i=0;i<8;++i) {targets[i]=loaded_index(common.get(),580+8*static_cast<int>(i),56);}
    util::FS::ReadFile(bytes,system_path);
    const auto opacities=game::frontend::smac_territory_boundary::ReadOpacities(
        std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
    source->owner_clut=game::frontend::smac_territory_boundary::Rows(source->palette,targets,opacities);
    for(size_t owner=0;owner<8;++owner) {if(!faction_paths[owner].empty()) {
        util::FS::ReadFile(bytes,faction_paths[owner]);Surface faction(load_image(bytes),SDL_FreeSurface);
        // Port-defined Base art may be a small grid or true-color image.
        // Without the native metadata profile, retain SYSTEM's global row.
        if(!faction || !faction->format || faction->format->BitsPerPixel!=8 || !faction->pixels ||
            faction->w<162 || faction->h<758 || faction->pitch<faction->w) {continue;}
        const auto target=loaded_index(faction.get(),161,747),encoded=loaded_index(faction.get(),161,757);
        const int opacity=static_cast<uint8_t>(encoded-10)*100/255;
        source->owner_clut[owner]=game::frontend::smac_territory_boundary::Row(source->palette,target,opacity);
    }}
    m_native_terrain_sources.emplace(key,source);return source;
}
types::texture::Texture* SDL2::CreateSmacNativeBadgeTexture(int level,int variant) {
    if(level<0 || level>6 || variant<0 || variant>1) {THROW("Native badge requires level0..6 and variant0..1");}
    const auto units_path=GetCustomFilename("units.pcx"),palette_path=GetCustomFilename("palette.pcx");
    const auto key=units_path+":"+palette_path;
    auto found=m_native_badge_sources.find(key);
    if(found==m_native_badge_sources.end()) {
        std::vector<unsigned char> bytes;
        using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
        util::FS::ReadFile(bytes,units_path);Surface units(load_image(bytes),SDL_FreeSurface);
        util::FS::ReadFile(bytes,palette_path);Surface palette(load_image(bytes),SDL_FreeSurface);
        auto source=std::make_shared<SmacNativeBadgeSource>();
        for(int m=0;m<7;++m) {for(int v=0;v<2;++v) {
            source->badges[2*m+v]=DecodeSmacNativeBadge(units.get(),palette && palette->format ? palette->format->palette : nullptr,m,v);
        }}
        found=m_native_badge_sources.emplace(key,std::move(source)).first;
    }
    const auto& badge=found->second->badges[2*level+variant];
    auto texture=std::make_unique<types::texture::Texture>("NativeBadge_"+std::to_string(level)+"_"+std::to_string(variant),SmacNativeBadge::Width,SmacNativeBadge::Height);
    std::copy(badge.rgba.begin(),badge.rgba.end(),texture->GetBitmap());texture->FullUpdate();return texture.release();
}

struct SmacMenuSource {
	std::vector< unsigned char > animation;
	std::vector< uint8_t > background;
	std::array< std::array< uint8_t, 3 >, 256 > palette;
	size_t width, height;
	std::unordered_map< size_t, loader::flic::Frame > frames;
};

types::texture::Texture* SDL2::CreateSmacMenuTexture( const std::string& animation, const std::string& background, size_t frame_index, size_t rows, size_t viewport_width, size_t viewport_height ) {
	const auto animation_path = GetCustomFilename( animation ), background_path = GetCustomFilename( background );
	const auto source_key = animation_path + ":" + background_path;
	auto found = m_smac_menu_sources.find( source_key );
	if ( found == m_smac_menu_sources.end() ) {
		auto source = std::make_shared< SmacMenuSource >();
		util::FS::ReadFile( source->animation, animation_path );
		std::vector< unsigned char > bytes;
		using Surface = std::unique_ptr< SDL_Surface, decltype( &SDL_FreeSurface ) >;
		util::FS::ReadFile( bytes, background_path );
		Surface image( load_image( bytes ), SDL_FreeSurface );
		util::FS::ReadFile( bytes, GetCustomFilename( "palette.pcx" ) );
		Surface palette_image( load_image( bytes ), SDL_FreeSurface );
		ApplySmacPalette( image.get(), palette_image ? palette_image->format->palette : nullptr );
		for ( size_t i = 0; i < source->palette.size(); ++i ) {
			const auto& color = palette_image->format->palette->colors[ i ];
			source->palette[ i ] = { color.r, color.g, color.b };
		}
		source->width = image->w; source->height = image->h;
		source->background.resize( source->width * source->height );
		for ( int y = 0; y < image->h; ++y ) {
			std::copy_n( static_cast< const uint8_t* >( image->pixels ) + y * image->pitch, image->w, source->background.data() + size_t( y ) * image->w );
		}
		found = m_smac_menu_sources.emplace( source_key, std::move( source ) ).first;
	}
	auto& source = *found->second;
	auto frame = source.frames.find( frame_index );
	if ( frame == source.frames.end() ) {
		frame = source.frames.emplace( frame_index, loader::flic::FlicReader::ReadFrame( source.animation, frame_index ) ).first;
	}
	const auto composed = loader::flic::ComposeMenu( frame->second, source.background, source.width, source.height, source.palette, rows, viewport_width, viewport_height );
	// Each Surface owns its current composed pixels. Only source data is cached,
	// so dragging a window through many dimensions does not retain all canvases.
	auto texture = std::make_unique< types::texture::Texture >( source_key, composed.width, composed.height );
	memcpy( texture->m_bitmap, composed.rgba.data(), composed.rgba.size() );
	texture->FullUpdate();
	return texture.release();
}

std::shared_ptr<types::texture::Texture> SDL2::GetSmacEffectTexture(bool death) {
    const auto cached=m_effect_sources[death].lock();if(cached) {return cached;}
    using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
    std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,GetCustomFilename(death ? "xf.pcx":"xi.pcx"));
    Surface image(load_image(bytes),SDL_FreeSurface);
    util::FS::ReadFile(bytes,GetCustomFilename("palette.pcx"));Surface common(load_image(bytes),SDL_FreeSurface);
    const int width=death ? 816:720,height=death ? 304:49;
    if(!image || image->format->BitsPerPixel!=8 || image->w<width || image->h<height
        || !common || !common->format->palette || common->format->palette->ncolors!=256) {THROW("Indexed Scout effect assets have incompatible dimensions or palettes");}
    smac_effect::Palette palette{};smac_effect::ApplySmacSoftwareReservedColors(palette);
    for(size_t index=0;index<236;++index) {const auto color=common->format->palette->colors[index];
        palette[index+10]={std::min<uint8_t>(color.r,235),std::min<uint8_t>(color.g,235),std::min<uint8_t>(color.b,235)};}
    util::FS::ReadFile(bytes,GetCustomFilename("system.txt"));
    const auto bank=smac_effect::Generate(palette,smac_effect::ReadRows(std::string(bytes.begin(),bytes.end()),death ? 0:3));
    auto texture=std::make_shared<types::texture::Texture>(death ? "Indexed Scout death":"Indexed Scout hit",width,height+18);
    texture->Clear();auto* out=texture->GetBitmap();
    for(int y=0;y<height;++y) {const auto* source=static_cast<const uint8_t*>(image->pixels)+y*image->pitch;
        for(int x=0;x<width;++x) {auto* pixel=out+(size_t(y)*width+x)*4;const auto index=source[x];pixel[0]=index;
            // The explicit SMAC software startup profile supplies row252 safely.
            // Other non-CLUT atlas annotations are outside the selected crops.
            pixel[3]=(index<16 || index==252) ? 255:0;}}
    for(size_t row=0;row<16;++row) {for(size_t destination=0;destination<256;++destination) {
        auto* pixel=out+((height+row)*width+destination)*4;pixel[0]=bank[row*256+destination];pixel[3]=255;}}
    for(size_t index=0;index<256;++index) {
        auto* pixel=out+((height+16)*width+index)*4;pixel[0]=smac_effect::SmacSoftwareStartupEscape(index);pixel[3]=255;
    }
    for(size_t index=0;index<256;++index) {auto* pixel=out+((height+17)*width+index)*4;
        for(size_t channel=0;channel<3;++channel) {pixel[channel]=palette[index][channel];}pixel[3]=255;}
    texture->FullUpdate();m_effect_sources[death]=texture;return texture;
}

types::texture::Texture* SDL2::LoadTextureImpl( const std::string& filename, const types::texture::texture_flag_t flags ) {

	const auto cache_key = filename + ":flags=" + std::to_string( flags );
	texture_map_t::iterator it = m_textures.find( cache_key );
	if ( it != m_textures.end() ) {
		return it->second;
	}
	else {
		Log( "Loading texture \"" + filename + "\"" );
		std::vector< unsigned char > buffer = {};
		util::FS::ReadFile( buffer, filename );
		using Surface = std::unique_ptr< SDL_Surface, decltype( &SDL_FreeSurface ) >;
		Surface image( load_image( buffer ), SDL_FreeSurface );
		if ( !image ) { THROW( IMG_GetError() ); }
		if ( flags & (types::texture::TF_SMAC_PALETTE | types::texture::TF_SMAC_SOFTWARE_TERRAIN) ) {
            const bool terrain=flags & types::texture::TF_SMAC_SOFTWARE_TERRAIN;
            if(terrain && ((flags & types::texture::TF_SMAC_PALETTE) || m_fix_yellow_shadows)) {
                THROW("Conflicting indexed terrain texture profiles");
            }
			if ( !terrain && (!m_transparent_colors.empty() || m_fix_yellow_shadows) ) {
				THROW( "SMAC display palette currently requires an opaque source" );
			}
			std::vector< unsigned char > palette_buffer;
			util::FS::ReadFile( palette_buffer, GetCustomFilename( "palette.pcx" ) );
			Surface palette_image( load_image( palette_buffer ), SDL_FreeSurface );
            const auto* common=palette_image && palette_image->format ? palette_image->format->palette : nullptr;
            if(terrain) {ApplySmacTerrainPalette(image.get(),common);}
            else {ApplySmacPalette(image.get(),common);}
		}
		if ( image->format->format != SDL_PIXELFORMAT_RGBA32 ) {
			// we must have all images in same format
			auto* converted = SDL_ConvertSurfaceFormat( image.get(), SDL_PIXELFORMAT_RGBA32, 0 );
			if ( !converted ) { THROW( SDL_GetError() ); }
			image.reset( converted );
		}

		NEWV( texture, types::texture::Texture, filename, image->w, image->h, flags );
		texture->SetBitmap( image->pixels );
		ASSERT( texture->m_bpp == image->format->BitsPerPixel / 8, "unsupported texture bpp" );
		memcpy( ptr( texture->m_bitmap, 0, texture->m_bitmap_size ), image->pixels, texture->m_bitmap_size );
		image.reset();

		FixTexture( texture ); // some pcx files have strange artifacts that we need to fix procedurally

		if(!(flags & types::texture::TF_SMAC_SOFTWARE_TERRAIN)) {FixTransparency( texture );}
		FixYellowShadows( texture );

		m_textures.insert(
			{
				cache_key,
				texture
			}
		);

		DEBUG_STAT_INC( textures_loaded );

		return texture;
	}

}

types::texture::Texture* SDL2::LoadTextureImpl( const std::string& name, const size_t x1, const size_t y1, const size_t x2, const size_t y2, const uint8_t flags, const float value, const types::texture::texture_flag_t texture_flags ) {
	ASSERT( x1 <= x2, "LoadTexture x overflow ( " + std::to_string( x1 ) + " > " + std::to_string( x2 ) + " )" );
	ASSERT( y1 <= y2, "LoadTexture y overflow ( " + std::to_string( y1 ) + " > " + std::to_string( y2 ) + " )" );

	const std::string subtexture_key =
		name + ":" +
			std::to_string( x1 ) + ":" +
			std::to_string( y1 ) + ":" +
			std::to_string( x2 ) + ":" +
			std::to_string( y2 ) + ":" +
			std::to_string( flags ) + ":" +
			std::to_string( value ) + ":" + std::to_string( texture_flags );

	texture_map_t::iterator it = m_subtextures.find( subtexture_key );
	if ( it != m_subtextures.end() ) {
		return it->second;
	}
	else {

		auto* full_texture = LoadTextureImpl( name, texture_flags );

		NEWV( subtexture, types::texture::Texture, subtexture_key, x2 - x1 + 1, y2 - y1 + 1 );

		subtexture->AddFrom( full_texture, types::texture::AM_DEFAULT, x1, y1, x2, y2 );
		if ( flags & types::texture::LT_ROTATE ) {
			subtexture->Rotate();
		}
		if ( flags & types::texture::LT_FLIPV ) {
			subtexture->FlipV();
		}

		if ( flags & types::texture::LT_ALPHA ) {
			subtexture->SetAlpha( value );
		}
		if ( flags & types::texture::LT_CONTRAST ) {
			subtexture->SetContrast( value );
		}

		if ( flags & types::texture::LT_TILED ) {
			subtexture->m_is_tiled = true;
		}
		
		// needed?
		if(!(texture_flags & types::texture::TF_SMAC_SOFTWARE_TERRAIN)) {FixTransparency( subtexture );}
		//FixYellowShadows( subtexture );

		m_subtextures[ subtexture_key ] = subtexture;

		return subtexture;
	}
}

void SDL2::FixTransparency( types::texture::Texture* texture ) const {
	if ( !m_transparent_colors.empty() ) {
		void* at = nullptr;
		for ( size_t i = 0 ; i < texture->m_bitmap_size ; i += texture->m_bpp ) {
			at = ptr( texture->m_bitmap, i, texture->m_bpp );
			for ( auto& c : m_transparent_colors ) {
				if ( !memcmp( at, &c, texture->m_bpp ) ) {
					memset( at, 0, texture->m_bpp );
					break;
				}
			}
		}
	}
}

static const types::Color::rgba_t s_yellow_shadow_src = types::Color::RGB( 253, 189, 118 );
static const types::Color::rgba_t s_yellow_shadow_dst = types::Color::RGBA( 0, 0, 0, 127 );

void SDL2::FixYellowShadows( types::texture::Texture* texture ) const {
	if ( m_fix_yellow_shadows ) {
		void* at = nullptr;
		for ( size_t i = 0 ; i < texture->m_bitmap_size ; i += texture->m_bpp ) {
			at = ptr( texture->m_bitmap, i, texture->m_bpp );
			if ( !memcmp( at, &s_yellow_shadow_src, texture->m_bpp ) ) {
				memcpy( at, &s_yellow_shadow_dst, texture->m_bpp );
			}
		}
	}
}

void SDL2::FixTexture( types::texture::Texture* texture ) const {
	if ( texture->m_name == "interface.pcx" ) {
		ASSERT( texture->m_width == 750 && texture->m_height == 900, "unexpected texture.pcx dimensions" );
		// rain icons have weird brown lines on them
		const std::vector< types::Vec2< size_t > > pixels_to_fix = {
			{ 725, 309 },
			{ 724, 310 },
			{ 723, 310 },
			{ 722, 310 },
			{ 721, 311 },
			{ 720, 312 },
			{ 719, 312 },
		};
		for ( auto& v : pixels_to_fix ) {
			texture->SetPixel( v.x, v.y, texture->GetPixel( v.x + 1, v.y ) );
		}
	}
}

}
}
