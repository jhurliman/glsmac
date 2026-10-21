#pragma once

#include "Area.h"
#include "loader/cvr/WorkshopPreviewController.h"
#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include <optional>

namespace ui {class SmacWorkshopCachedPreview;}
namespace task::intro { class Movie; }
namespace loader::cvr {class CvrScoutRenderer;class CvrWorkshopComponents;struct CvrStripImage;struct CvrScoutImage;}
namespace loader::texture {struct SmacDialogSource;struct SmacTurnFrames;struct SmacGraphPalette;}

namespace types {
namespace mesh {
class Rectangle;
}
namespace texture {
class Texture;
}
}

namespace scene::actor {
class Mesh;
}

namespace ui {

class Geometry;
class UI;

namespace dom {

class Surface : public Area {
public:

	Surface( DOM_ARGS_T = "surface" );
	void Destroy( GSE_CALLABLE ) override;
	// Main-thread RGBA upload; updates both the texture and cached actor revision.
	void PresentMovieFrame(int width, int height, const std::vector<uint8_t>& rgba);
    void PresentTurnFrame(const std::vector<uint8_t>& rgba);
    void PresentSolidColor(const types::Color& color);

protected:
	virtual ~Surface();

	struct {
		types::texture::Texture* texture = nullptr;
		bool is_texture_owned = false;
	} m_background = {};

	void CreateTexture();

private:
	types::mesh::Rectangle* m_mesh = nullptr;
	scene::actor::Mesh* m_actor = nullptr;

	void ClearTexture();
    std::shared_ptr<const loader::texture::SmacDialogSource> m_dialog_source;
    std::vector<std::array<int,5>> m_dialog_frames;
    std::array<int,2> m_dialog_size{{-1,-1}},m_dialog_phase{{0,0}};
    void UpdateDialogFrames();
    void EnsureWorkshopComponents();
    gse::Value* WorkshopPickerText(GSE_CALLABLE);
    gse::Value* WorkshopParentText(GSE_CALLABLE);
    gse::Value* WorkshopRenameText(GSE_CALLABLE);
    gse::Value* WorkshopRetireText(GSE_CALLABLE,const std::string& name,int32_t count);
    gse::Value* WorkshopUpgradeText(GSE_CALLABLE,int32_t kind,const std::array<std::string,2>& names,const std::array<int32_t,3>& numbers);
    gse::Value* WorkshopAbilityText(GSE_CALLABLE,const std::string& technology);
    std::optional<std::array<std::string,8>> m_workshop_picker_text;
    std::optional<std::array<std::string,38>> m_workshop_parent_text;
    std::optional<std::array<std::string,9>> m_workshop_retire_text;
    std::optional<std::array<std::string,42>> m_workshop_upgrade_text;
    std::optional<std::array<std::string,4>> m_workshop_rename_text;
    bool m_workshop_console_backing=false;
    std::optional<std::array<std::string,12>> m_workshop_ability_text;
	std::unique_ptr<task::intro::Movie> m_movie;
    std::shared_ptr<const loader::texture::SmacTurnFrames> m_turn_frames;
    std::shared_ptr<const loader::texture::SmacGraphPalette> m_graph_palette;
	std::array<std::shared_ptr<const loader::cvr::CvrScoutRenderer>,2> m_workshop_models;
	std::shared_ptr<const loader::cvr::CvrWorkshopComponents> m_workshop_components;
    loader::cvr::WorkshopPreviewController m_workshop_preview;
    bool m_workshop_preview_iterable=false;
    std::shared_ptr<const ui::SmacWorkshopCachedPreview> m_workshop_cached;
    std::array<int,2> m_workshop_cached_key{{-1,-1}};
    std::shared_ptr<const loader::cvr::CvrScoutRenderer> m_workshop_design;
    uint32_t m_workshop_design_abilities=0;
    std::array<int,8> m_workshop_design_key{{-2,-2,-2,-2,-2,-2,-2,-2}};
    std::string m_workshop_design_faction;
    struct PickerImage {std::array<int,8> key;bool known;std::array<uint32_t,3> before,after;std::shared_ptr<const std::vector<uint8_t>> colors;};
    std::vector<PickerImage> m_workshop_picker_images;
    std::optional<std::array<std::vector<uint8_t>,10>> m_workshop_weapon_sprites;
    std::string m_workshop_picker_faction;
    struct StripImage {std::array<int64_t,8> key;std::shared_ptr<const loader::cvr::CvrStripImage> image;};
    std::vector<StripImage> m_workshop_strip_images;
    std::string m_workshop_strip_faction;
    std::optional<std::array<std::array<uint8_t,3>,256>> m_workshop_strip_colors;
    std::array<uint8_t,256> m_workshop_strip_shadow{};
    std::array<std::string,2> m_workshop_strip_labels;
	void StopMovie();

private:
	friend class Panel;
	types::texture::Texture* GetOwnedTexturePtr();

};

}
}
