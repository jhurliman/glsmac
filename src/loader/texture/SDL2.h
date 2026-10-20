#pragma once

#include <unordered_map>
#include <memory>

#define SDL_MAIN_HANDLED 1
#include <SDL_image.h>

#include "TextureLoader.h"

namespace game::smac_detail {struct Sprite;}
namespace loader {
namespace texture {

struct SmacMenuSource;
struct SmacNativeBadgeSource;
struct SmacBaseCardSource;
struct SmacUnitBadgeSource;

CLASS( SDL2, TextureLoader )
	virtual ~SDL2();

	types::texture::Texture* CreateSmacNativeBadgeTexture(int level,int variant) override;
    std::shared_ptr<const SmacUnitBadge> GetSmacUnitBadge(int owner,int level,int variant) override;
    int GetSmacChassisFlagOffset(int chassis) override;
    std::shared_ptr<const SmacBaseCard> GetSmacNativeBaseCard(const std::string& faction,size_t x,size_t y,bool roster) override;
    std::shared_ptr<const game::smac_detail::Sprite> GetSmacDetailBaseSprite(const std::string& faction,size_t frame_y,size_t column=0) override;
    std::shared_ptr<const game::frontend::native_terrain::Source> GetSmacTerrainSource(const std::array<std::string,8>& faction_files={}) override;
    std::shared_ptr<types::texture::Texture> GetSmacEffectTexture(bool death) override;
    std::shared_ptr<const SmacTurnFrames> GetSmacTurnFrames() override;
    std::shared_ptr<const SmacDialogSource> GetSmacDialogSource() override;
    std::shared_ptr<const SmacGraphPalette> GetSmacGraphPalette() override;
	void Start() override;
	void Stop() override;
	void Iterate() override;
	types::texture::Texture* CreateSmacMenuTexture( const std::string& animation, const std::string& background, size_t frame, size_t rows, size_t viewport_width, size_t viewport_height ) override;

protected:

	types::texture::Texture* LoadTextureImpl( const std::string& filename, const types::texture::texture_flag_t flags ) override;
	types::texture::Texture* LoadTextureImpl( const std::string& name, const size_t x1, const size_t y1, const size_t x2, const size_t y2, const uint8_t flags, const float value, const types::texture::texture_flag_t texture_flags ) override;

	// cache all textures for future use
	typedef std::unordered_map< std::string, types::texture::Texture* > texture_map_t;
	texture_map_t m_textures = {};
	texture_map_t m_subtextures = {};

private:
    std::unordered_map<std::string,std::shared_ptr<const SmacDialogSource>> m_dialog_sources;
	std::unordered_map< std::string, std::shared_ptr< SmacMenuSource > > m_smac_menu_sources;
	std::unordered_map<std::string,std::shared_ptr<SmacNativeBadgeSource>> m_native_badge_sources;
    std::unordered_map<std::string,std::shared_ptr<SmacUnitBadgeSource>> m_unit_badge_sources;
    std::unordered_map<std::string,std::shared_ptr<const game::frontend::native_terrain::Source>> m_native_terrain_sources;
    std::unordered_map<bool,std::weak_ptr<types::texture::Texture>> m_effect_sources;
    std::unordered_map<std::string,std::shared_ptr<SmacBaseCardSource>> m_base_card_sources;
    std::unordered_map<std::string,std::shared_ptr<const SmacBaseCard>> m_base_cards;
    std::unordered_map<std::string,std::shared_ptr<const game::smac_detail::Sprite>> m_detail_bases;
    std::unordered_map<std::string,std::shared_ptr<const SmacTurnFrames>> m_turn_frames;
    std::unordered_map<std::string,std::shared_ptr<const SmacGraphPalette>> m_graph_palettes;
	void FixTransparency( types::texture::Texture* texture ) const;
	void FixYellowShadows( types::texture::Texture* texture ) const;
	void FixTexture( types::texture::Texture* texture ) const;

};

}
}
