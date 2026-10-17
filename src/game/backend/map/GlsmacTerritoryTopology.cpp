#include "GlsmacTerritoryTopology.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace game::backend::map {
namespace {
void Require( bool value, const char* message ) {
	if ( !value ) { throw std::runtime_error( message ); }
}
size_t Count( int32_t width, int32_t height ) {
	Require( width >= 2 && width <= 512 && !(width & 1) && height >= 2 && height <= 256,
		"Territory topology requires a bounded parity grid with distinct polar rows" );
	return static_cast< size_t >(width) * height / 2;
}
constexpr std::array< std::array< int32_t, 2 >, 8 > neighbors{{
	{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}
}};
size_t Index( int32_t width, int32_t x, int32_t y ) { return static_cast< size_t >(y) * (width/2) + x/2; }
uint16_t Read16( const std::string& wire, size_t offset ) {
	return static_cast< uint8_t >(wire[offset]) | (uint16_t(static_cast< uint8_t >(wire[offset+1])) << 8);
}
void Write16( std::string& wire, size_t offset, uint16_t value ) {
	wire[offset] = static_cast<char>(value & 255); wire[offset+1] = static_cast<char>(value >> 8);
}
}

GlsmacTerritoryTopology::GlsmacTerritoryTopology( int32_t width, int32_t height, bool wrap, std::vector< uint8_t > regions )
	: m_width(width), m_height(height), m_wrap(wrap), m_regions(std::move(regions)) {}

GlsmacTerritoryTopology GlsmacTerritoryTopology::Build( int32_t width, int32_t height, bool wrap, const std::vector< bool >& sea ) {
	const auto count = Count(width,height);
	Require( sea.size() == count, "Territory topology sea extent differs from geometry" );
	std::vector<uint8_t> regions(count,0);
	std::vector<size_t> queue; queue.reserve(count);
	uint8_t last_land = 0, last_sea = 64;
	for ( int32_t y = 1; y < height-1; ++y ) { for ( int32_t x = y & 1; x < width; x += 2 ) {
		const auto seed = Index(width,x,y); if (regions[seed]) { continue; }
		auto& last = sea[seed] ? last_sea : last_land;
		// The receipt explicitly excludes original capacity merging. Do not
		// alias disconnected components or silently change polar identities.
		Require( last < (sea[seed] ? 125 : 61), "Territory region capacity merging is unsupported" );
		const auto region = ++last;
		queue.clear(); queue.push_back(seed); regions[seed] = region;
		for ( size_t head = 0; head < queue.size(); ++head ) {
			const auto current = queue[head];
			const auto py = static_cast<int32_t>(current / (width/2));
			const auto px = static_cast<int32_t>(current % (width/2))*2 + (py & 1);
			if (py == 0 || py == height-1) { continue; }
			for (const auto& delta : neighbors) {
				auto nx = px + delta[0]; const auto ny = py + delta[1];
				if (wrap) { if (nx < 0) { nx += width; } else if (nx >= width) { nx -= width; } }
				if (nx < 0 || nx >= width || ny < 0 || ny >= height) { continue; }
				const auto next = Index(width,nx,ny);
				if (regions[next] || sea[next] != sea[seed]) { continue; }
				regions[next] = region; queue.push_back(next);
			}
		}
	} }
	for (const int32_t y : {0,height-1}) { for (int32_t x = y & 1; x < width; x += 2) {
		const auto index = Index(width,x,y); regions[index] = sea[index] ? 127 : 63;
	} }
	return GlsmacTerritoryTopology(width,height,wrap,std::move(regions));
}

void GlsmacTerritoryTopology::ValidateTerrain( const std::vector<bool>& sea ) const {
	Require( *this == Build(m_width,m_height,m_wrap,sea), "Saved territory topology differs from current terrain" );
}
bool GlsmacTerritoryTopology::operator==( const GlsmacTerritoryTopology& other ) const {
	return m_width == other.m_width && m_height == other.m_height && m_wrap == other.m_wrap && m_regions == other.m_regions;
}
std::string GlsmacTerritoryTopology::Serialize() const {
	std::string wire(16 + m_regions.size(), '\0'); wire.replace(0,8,"GLSTERR1");
	Write16(wire,8,static_cast<uint16_t>(m_width)); Write16(wire,10,static_cast<uint16_t>(m_height)); wire[12] = m_wrap ? 1 : 0;
	for (size_t i = 0; i < m_regions.size(); ++i) { wire[16+i] = static_cast<char>(m_regions[i]); }
	return wire;
}
GlsmacTerritoryTopology GlsmacTerritoryTopology::Deserialize( const std::string& wire ) {
	Require( wire.size() >= 16 && wire.size() <= MAX_WIRE_BYTES && wire.compare(0,8,"GLSTERR1") == 0,
		"Invalid territory topology header or size" );
	const auto width = Read16(wire,8), height = Read16(wire,10); const auto count = Count(width,height);
	Require( wire.size() == 16+count && (wire[12] == 0 || wire[12] == 1) && !wire[13] && !wire[14] && !wire[15],
		"Invalid territory topology framing" );
	// Validate every byte before allocating the owned result.
	for (size_t i = 0; i < count; ++i) {
		const auto region = static_cast<uint8_t>(wire[16+i]); const auto y = i/(width/2);
		Require( y == 0 || y == height-1 ? region == 63 || region == 127 : (region >= 1 && region <= 61) || (region >= 65 && region <= 125),
			"Invalid territory topology region" );
	}
	std::vector<uint8_t> regions; regions.reserve(count);
	for (size_t i = 0; i < count; ++i) { regions.push_back(static_cast<uint8_t>(wire[16+i])); }
	return GlsmacTerritoryTopology(width,height,wire[12] != 0,std::move(regions));
}

}
