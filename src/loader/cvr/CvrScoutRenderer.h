#pragma once

#include "CvrIndexedRasterizer.h"

#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <optional>

namespace types::texture { class Texture; }
namespace gse::tests { class CvrRenderPolicyFixture; }

namespace loader::cvr {

namespace scout {struct ShadowScene;struct View;}
struct CvrActorMaterial;
struct CvrFrameAssembly;
class CvrSpartanRoverRenderer;

struct CvrScoutViewInput {
	int32_t facing; // Supported flat idle facing -1 or0..7; -1 selects0.
	int32_t zoom;
	int32_t scale_numerator, scale_denominator; // Positive numerator overrides zoom.
	std::array< int32_t, 2 > anchor;
};

struct CvrScoutTextureDelete { void operator()( types::texture::Texture* texture ) const; };
using CvrScoutTexture = std::unique_ptr< types::texture::Texture, CvrScoutTextureDelete >;

struct CvrScoutImage {
	CvrIndexedSurface indexed;
	std::vector< uint8_t > rgba; // Host conversion keys index9 (ordinary) or24 (translucent Workshop), independent of depth/RGB.
	std::array< int32_t, 2 > target_anchor;
	size_t draw_count, processed_records, written_samples;
	// A fresh tracked owner; callers must keep it alive through all borrowed
	// scene actors. No global/per-view cache or actor lifetime is implied here.
	CvrScoutTexture CreateTexture() const;
};

struct CvrStripImage {
    CvrScoutImage body;CvrShadowResult shadow;
    // Original key0 shadow lookup followed by key9 body copy. Origin and
    // half-open clip belong to the caller's console canvas; validate before stores.
    size_t Composite(std::vector<uint8_t>& target,uint32_t width,uint32_t height,uint32_t pitch,
        const std::array<uint8_t,256>& lookup,const std::array<int32_t,2>& origin,
        const std::array<int32_t,4>& clip) const;
};

class CvrScoutRenderer {
public:
	// Own and decode explicit user-supplied VI/viptr00 assets. No executable or
	// captured palette/window/frame bytes are runtime inputs. This entry point
	// does not infer design identity or locate files in an installation.
	static CvrScoutRenderer Load( std::string_view infantry, std::string_view reactor );
	// Explicit initialized main-palette/light path; no faction tint or remap.
	static CvrScoutRenderer LoadInitialized( std::string_view infantry, std::string_view reactor,
		std::string_view palette_pcx, std::string_view system );
	// Reactor1 flat idle/frame0 adapter. Unsupported geometry/frame/group flags,
	// arithmetic, footprint and out-of-surface samples reject with invalid_argument.
	// Produces a256-square internal image; final target clipping/blitting and
	// gameplay unit selection are separate. Input anchors affect metadata only.
	CvrScoutImage Render( const CvrScoutViewInput& view ) const;
    // Original Workshop composition: optional Synthmetal is inserted between
    // Infantry and Fission reactor. Camera/bounds differ from map/card policy.
    static CvrScoutRenderer LoadWorkshop(std::string_view infantry,std::string_view armor,
        std::string_view reactor,std::string_view palette_pcx,std::string_view system);
    struct WorkshopAnimation {uint32_t phase=0;bool visible=false;};
    // Explicit shared phase. Compositions reuse decoded geometry; Copter body
    // advances phase only when the Workshop hierarchy is visible. Animated
    // component selection uses the typed LoadWorkshopDesign recipe.
    CvrScoutImage RenderWorkshop(uint32_t yaw_bits=0,WorkshopAnimation* animation=nullptr) const;
    using WorkshopReader=std::function<std::string(const std::string&)>;
    // Asset-derived conventional compositions. kind=-1 is the full model;
    // 0..3 builds the separate chassis/weapon/shield/reactor scene.
    static CvrScoutRenderer LoadWorkshopDesign(const WorkshopReader& read,std::string_view faction_pcx,
        int chassis,int weapon,int armor,int reactor,int kind=-1);
    struct WorkshopDefinition {int weapon_power,weapon_mode,armor_strength;bool independent_probe=false;uint32_t abilities=0;};
    // Stock SMAC composer with explicit per-part animation selection. Definition
    // fields come from the active installed rules; abilities retain their source composition order and own materials.
    static CvrScoutRenderer LoadWorkshopDesign(const WorkshopReader& read,std::string_view faction_pcx,
        int chassis,int weapon,int armor,int reactor,int kind,const WorkshopDefinition& definition);
    // Generic chooser4d8e40(owner,-1,weapon), distinct from main side cards.
    static CvrScoutRenderer LoadWorkshopPickerWeapon(const WorkshopReader& read,std::string_view faction_pcx,
        int weapon,const WorkshopDefinition& definition);
    // Ordinary idle/facing2/zoom-10 scene used by the saved Workshop strip.
    // Body and shadow remain separate indexed sources for the native keyed copy.
    static CvrScoutRenderer LoadStripDesign(const WorkshopReader& read,std::string_view faction_pcx,
        int chassis,int weapon,int armor,int reactor,const WorkshopDefinition& definition);
    CvrStripImage RenderStrip() const;
    // Same ordinary composition, with the live map/card caller's view rather
    // than the saved Workshop strip's fixed facing2/zoom-10 scratch view.
    static CvrScoutRenderer LoadOrdinaryDesign(const WorkshopReader& read,std::string_view faction_pcx,
        int chassis,int weapon,int armor,int reactor,const WorkshopDefinition& definition);
    CvrScoutImage RenderOrdinaryBody(const CvrScoutViewInput& view) const;
    CvrScoutImage RenderWorkshopCard(int kind,std::array<uint32_t,3>& position,WorkshopAnimation* animation=nullptr) const;
    CvrScoutImage RenderWorkshopPicker(int kind,int ordinal,bool available,std::array<uint32_t,3>& position) const;
    static CvrScoutRenderer LoadWorkshopPart(std::string_view part,std::string_view palette,std::string_view system,
        const std::array<uint8_t,24*256>* shades=nullptr,bool direct_locations=false);
    CvrScoutImage RenderWorkshopPart(uint32_t scale,bool centered,std::array<int32_t,2> anchor,
        bool unavailable,bool translucent,std::array<uint32_t,3>& position) const;
    // Explicit orthographic scene producer, not inferred initialized map light.
    // On demand: normal color/card rendering does not generate unused masks.
    CvrShadowResult RenderShadow(const scout::ShadowScene& scene) const;

private:
	friend class CvrSpartanRoverRenderer;
	friend class gse::tests::CvrRenderPolicyFixture;
	// Fixed composition policy is independent of optional palette/material data.
	enum class Profile { Infantry, SpartanSpeeder, InitializedInfantry, Workshop, WorkshopPart, WorkshopWeapon, StripInfantry, StripBody, StripSmallInfantry };
    enum class WorkshopFramePhase {Fixed,Modulo15,Raw};
    struct WorkshopAttachment {std::array<float,3> offset;bool direct;std::optional<std::array<uint8_t,24*256>> shades;
        WorkshopFramePhase phase=WorkshopFramePhase::Fixed;bool advance_phase=false;};
    CvrFrameAssembly WorkshopFrames(WorkshopAnimation& animation) const;
    static CvrScoutRenderer LoadDesign(const WorkshopReader& read,std::string_view faction_pcx,
        int chassis,int weapon,int armor,int reactor,int kind,const WorkshopDefinition& definition,Profile profile,bool independent_weapon=false);
	static CvrScoutRenderer LoadParts( const std::vector< std::string_view >& parts, Profile profile, const CvrActorMaterial* material,
        const std::vector<WorkshopAttachment>* attachments=nullptr,int workshop_weapon_mode=0,int picker_weapon=-1);
	CvrScoutImage RenderView(const scout::View& view,uint8_t clear=9,bool unavailable=false,
        std::optional<std::array<int32_t,4>> clip=std::nullopt,const CvrFrameAssembly* assembly=nullptr) const;
    std::array<uint32_t,3> WorkshopPosition(const std::array<uint32_t,9>& rotation,const std::array<uint32_t,3>& previous={0,0,0},const CvrFrameAssembly* assembly=nullptr) const;
	struct Assets;
	explicit CvrScoutRenderer( std::shared_ptr< const Assets > assets );
	std::shared_ptr< const Assets > m_assets;
};

class CvrWorkshopComponents {
public:
    using Reader=std::function<std::string(const std::string&)>;
    static CvrWorkshopComponents Load(const Reader& read);
    std::optional<CvrScoutImage> Render(int kind,int id,bool picker,bool available,std::array<uint32_t,3>& position,int ordinal=-1) const;
    uint8_t Composite(uint8_t source,uint8_t destination) const;
    uint8_t CompositeBackdrop(uint8_t source,size_t x,size_t y) const;
    const std::array<std::array<uint8_t,3>,256>& Colors() const {return m_colors;}
private:
    std::array<std::optional<CvrScoutRenderer>,4> m_side;
    std::array<std::optional<CvrScoutRenderer>,10> m_armor;
    std::array<std::array<uint8_t,3>,256> m_colors;
    std::array<uint8_t,24*256> m_translucency;
    std::vector<uint8_t> m_backdrop;
};

}
