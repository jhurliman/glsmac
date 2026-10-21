#pragma once

#include "CvrIndexedRasterizer.h"

#include <memory>
#include <string_view>
#include <functional>
#include <optional>

namespace types::texture { class Texture; }
namespace gse::tests { class CvrRenderPolicyFixture; }

namespace loader::cvr {

namespace scout {struct ShadowScene;struct View;}
struct CvrActorMaterial;
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
    CvrScoutImage RenderWorkshop(uint32_t yaw_bits=0) const;
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
	enum class Profile { Infantry, SpartanSpeeder, InitializedInfantry, Workshop, WorkshopPart, WorkshopWeapon };
	static CvrScoutRenderer LoadParts( const std::vector< std::string_view >& parts, Profile profile, const CvrActorMaterial* material );
	CvrScoutImage RenderView(const scout::View& view,uint8_t clear=9,bool unavailable=false) const;
    std::array<uint32_t,3> WorkshopPosition(const std::array<uint32_t,9>& rotation,const std::array<uint32_t,3>& previous={0,0,0}) const;
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
    const std::array<std::array<uint8_t,3>,256>& Colors() const {return m_colors;}
private:
    std::array<std::optional<CvrScoutRenderer>,4> m_side;
    std::array<std::optional<CvrScoutRenderer>,10> m_armor;
    std::array<std::array<uint8_t,3>,256> m_colors;
    std::array<uint8_t,24*256> m_translucency;
};

}
