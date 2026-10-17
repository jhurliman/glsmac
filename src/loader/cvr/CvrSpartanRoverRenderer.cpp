#include "CvrSpartanRoverRenderer.h"

#include "ScoutView.h"
#include <stdexcept>
#include <utility>

namespace loader::cvr {
const std::array< const char*, 10 >& CvrSpartanRoverRenderer::PartNames() {
	static const std::array< const char*, 10 > names{ "VRCP00.cvr", "VGMC.CVR", "VGMCP.CVR", "VB.CVR", "VBP.CVR", "VSP.cvr", "vsptb.cvr", "vsptf.cvr", "VLIGHTS.CVR", "VR00.cvr" };
	return names;
}

CvrSpartanRoverRenderer::CvrSpartanRoverRenderer( CvrScoutRenderer renderer, CvrActorMaterial material )
	: m_renderer( std::move( renderer ) ), m_material( std::move( material ) ) {}

CvrSpartanRoverRenderer CvrSpartanRoverRenderer::Load( const AssetReader& read ) {
	if ( !read ) { throw std::invalid_argument( "CVR Rover: missing installed asset reader" ); }
	scout::ValidateMapLight( read( "system.txt" ) );
	std::array< std::string, 10 > bytes;
	size_t total = 0;
	for ( size_t i = 0; i < bytes.size(); ++i ) {
		bytes[ i ] = read( PartNames()[ i ] );
		if ( bytes[ i ].size() > 32 * 1024 * 1024 || bytes[ i ].size() > 64 * 1024 * 1024 - total ) { throw std::invalid_argument( "CVR Rover: composition exceeds source budget" ); }
		total += bytes[ i ].size();
	}
	const auto body = CvrPaletteReader::Read( CvrContainerReader::Read( bytes[ 4 ] ) );
	const auto palette = read( "PALETTE.PCX" );
	const auto faction = read( "spartans.pcx" );
	auto material = CvrActorMaterial::Read( palette, faction, body.shades );
	std::vector< std::string_view > parts;
	for ( const auto& part : bytes ) { parts.push_back( part ); }
	auto renderer = CvrScoutRenderer::LoadParts( parts, CvrScoutRenderer::Profile::SpartanSpeeder, &material );
	return CvrSpartanRoverRenderer( std::move( renderer ), std::move( material ) );
}

CvrScoutImage CvrSpartanRoverRenderer::Render( const CvrScoutViewInput& view ) const { return m_renderer.Render( view ); }
const CvrActorMaterial& CvrSpartanRoverRenderer::Material() const { return m_material; }

}
