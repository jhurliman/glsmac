#pragma once

#include "CvrIndexedRasterizer.h"

#include <memory>
#include <string_view>

namespace types::texture { class Texture; }
namespace gse::tests { class CvrRenderPolicyFixture; }

namespace loader::cvr {

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
	std::vector< uint8_t > rgba; // Host conversion: alpha0 iff color index9, independent of depth/RGB.
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

private:
	friend class CvrSpartanRoverRenderer;
	friend class gse::tests::CvrRenderPolicyFixture;
	// Fixed composition policy is independent of optional palette/material data.
	enum class Profile { Infantry, SpartanSpeeder, InitializedInfantry };
	static CvrScoutRenderer LoadParts( const std::vector< std::string_view >& parts, Profile profile, const CvrActorMaterial* material );
	struct Assets;
	explicit CvrScoutRenderer( std::shared_ptr< const Assets > assets );
	std::shared_ptr< const Assets > m_assets;
};

}
