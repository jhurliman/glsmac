#include "CvrSpartanRoverRenderer.h"

#include <charconv>
#include <stdexcept>
#include <utility>

namespace loader::cvr {
namespace {

std::string_view Trim( std::string_view value ) {
	const auto whitespace = []( const char c ) { return c == ' ' || c == '\t' || c == '\r'; };
	while ( !value.empty() && whitespace( value.front() ) ) { value.remove_prefix( 1 ); }
	while ( !value.empty() && whitespace( value.back() ) ) { value.remove_suffix( 1 ); }
	return value;
}

void ValidateMapLight( const std::string_view source ) {
	if ( source.size() > 64 * 1024 || source.find( '\0' ) != std::string_view::npos ) { throw std::invalid_argument( "CVR Rover: invalid system source" ); }
	bool active = false, found = false, have_row = false;
	std::array< int32_t, 3 > light{};
	for ( size_t start = 0; start < source.size(); ) {
		auto end = source.find( '\n', start ); if ( end == std::string_view::npos ) { end = source.size(); }
		auto line = Trim( source.substr( start, end - start ) ); start = end == source.size() ? end : end + 1;
		line = Trim( line.substr( 0, line.find( ';' ) ) );
		if ( line.empty() ) { continue; }
		if ( line.front() == '#' ) {
			active = line == "#VEH_LIGHT_DIR_MAP";
			if ( active && found ) { throw std::invalid_argument( "CVR Rover: duplicate map light section" ); }
			found = found || active; continue;
		}
		if ( !active ) { continue; }
		if ( have_row ) { throw std::invalid_argument( "CVR Rover: extra map light row" ); }
		for ( size_t i = 0; i < light.size(); ++i ) {
			const auto comma = line.find( ',' );
			if ( ( i < 2 ) != ( comma != std::string_view::npos ) ) { throw std::invalid_argument( "CVR Rover: map light requires three integers" ); }
			const auto field = Trim( line.substr( 0, comma ) );
			const auto parsed = std::from_chars( field.data(), field.data() + field.size(), light[ i ] );
			if ( parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size() ) { throw std::invalid_argument( "CVR Rover: invalid map light integer" ); }
			if ( comma != std::string_view::npos ) { line.remove_prefix( comma + 1 ); }
		}
		have_row = true;
	}
	if ( !have_row || light != std::array< int32_t, 3 >{ 6, 4, 15 } ) { throw std::invalid_argument( "CVR Rover: map light outside the admitted system profile" ); }
}

}

const std::array< const char*, 10 >& CvrSpartanRoverRenderer::PartNames() {
	static const std::array< const char*, 10 > names{ "VRCP00.cvr", "VGMC.CVR", "VGMCP.CVR", "VB.CVR", "VBP.CVR", "VSP.cvr", "vsptb.cvr", "vsptf.cvr", "VLIGHTS.CVR", "VR00.cvr" };
	return names;
}

CvrSpartanRoverRenderer::CvrSpartanRoverRenderer( CvrScoutRenderer renderer, CvrActorMaterial material )
	: m_renderer( std::move( renderer ) ), m_material( std::move( material ) ) {}

CvrSpartanRoverRenderer CvrSpartanRoverRenderer::Load( const AssetReader& read ) {
	if ( !read ) { throw std::invalid_argument( "CVR Rover: missing installed asset reader" ); }
	ValidateMapLight( read( "system.txt" ) );
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
