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
void Write32(std::string& wire,size_t offset,int32_t value) {
	const auto bits=static_cast<uint32_t>(value);for(size_t i=0;i<4;++i) {wire[offset+i]=static_cast<char>((bits>>(8*i))&255);}
}
int32_t Read32(const std::string& wire,size_t offset) {
	uint32_t bits=0;for(size_t i=0;i<4;++i) {bits|=uint32_t(static_cast<uint8_t>(wire[offset+i]))<<(8*i);}
	return static_cast<int32_t>(bits<=INT32_MAX ? int64_t(bits) : int64_t(bits)-(int64_t(1)<<32));
}
constexpr std::array<std::array<int32_t,2>,20> footprint{{
	{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}},
	{{2,-2}},{{2,2}},{{-2,2}},{{-2,-2}},{{1,-3}},{{3,-1}},{{3,1}},{{1,3}},{{-1,3}},{{-3,1}},{{-3,-1}},{{-1,-3}}
}};
}

GlsmacTerritoryTopology::GlsmacTerritoryTopology( int32_t width, int32_t height, bool wrap, std::vector< uint8_t > regions )
	: m_width(width), m_height(height), m_wrap(wrap), m_regions(std::move(regions)) {}

GlsmacTerritoryTopology GlsmacTerritoryTopology::Build( int32_t width, int32_t height, bool wrap, const std::vector< bool >& sea ) {
	return BuildWithAreas(width,height,wrap,sea,{});
}
GlsmacTerritoryTopology GlsmacTerritoryTopology::BuildWithAreas(int32_t width,int32_t height,bool wrap,const std::vector<bool>& sea,RegionValues areas) {
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
		areas[region]=static_cast<int32_t>(queue.size());
	} }
	for (const int32_t y : {0,height-1}) { for (int32_t x = y & 1; x < width; x += 2) {
		const auto index = Index(width,x,y); regions[index] = sea[index] ? 127 : 63;
		auto& area=areas[regions[index]];Require(area<INT32_MAX,"Polar region area overflow is not admitted");++area;
	} }
	auto result=GlsmacTerritoryTopology(width,height,wrap,std::move(regions));result.m_areas=areas;return result;
}

GlsmacTerritoryTopology GlsmacTerritoryTopology::Rebuild(const std::vector<bool>& sea) const {
	Require(m_areas.has_value(),"Region rebuild requires retained area history");
	auto result=BuildWithAreas(m_width,m_height,m_wrap,sea,*m_areas);
	result.m_postpass=m_postpass; // region fill alone does not refresh F or coastal flags
	return result;
}

GlsmacTerritoryTopology GlsmacTerritoryTopology::WithTerrainPostpass(const std::vector<bool>& sea,const std::vector<bool>& fertile) const {
	Require(m_areas && sea.size()==m_regions.size() && fertile.size()==sea.size(),"Terrain postpass requires owned areas and exact terrain extents");
	TerrainPostpass pass;pass.coastal.resize(sea.size(),0);
	for(int32_t y=0;y<m_height;++y) {for(int32_t x=y&1;x<m_width;x+=2) {
		const auto index=Index(m_width,x,y);const auto region=m_regions[index];
		Require(!fertile[index] || !sea[index],"Fertility qualifier cannot identify water");
		if(fertile[index]) {++pass.fertility[region];}
		if(sea[index]) {continue;}
		std::optional<bool> previous;int inner=0,outer=0;bool distinct=false;
		for(size_t step=0;step<footprint.size();++step) {
			auto nx=x+footprint[step][0];const auto ny=y+footprint[step][1];
			if(m_wrap) {nx=(nx%m_width+m_width)%m_width;}
			if(nx<0 || nx>=m_width || ny<0 || ny>=m_height) {continue;}
			const auto next=Index(m_width,nx,ny);const bool water=sea[next];
			if(step<8) {if(!previous || *previous!=water) {++inner;}}
			else {
				if(step==8 || (previous && *previous!=water)) {++outer;}
				const auto other=m_regions[next];
				if(ny>4 && ny<m_height-4 && other<64 && other!=region && (*m_areas)[region]>=40 && (*m_areas)[other]>=40) {distinct=true;}
			}
			previous=water;
		}
		pass.coastal[index]=static_cast<uint8_t>(((*m_areas)[region]>=80 && inner>=4 && outer>=4 ? 1 : 0) | (distinct ? 2 : 0));
	}}
	auto result=*this;result.m_postpass=std::move(pass);return result;
}

void GlsmacTerritoryTopology::ValidateTerrain( const std::vector<bool>& sea ) const {
	Require( m_regions == Build(m_width,m_height,m_wrap,sea).m_regions, "Saved territory topology differs from current terrain" );
}
bool GlsmacTerritoryTopology::operator==( const GlsmacTerritoryTopology& other ) const {
	return m_width == other.m_width && m_height == other.m_height && m_wrap == other.m_wrap && m_regions == other.m_regions
		&& m_areas==other.m_areas && m_postpass==other.m_postpass;
}
std::string GlsmacTerritoryTopology::Serialize() const {
	const auto count=m_regions.size();
	std::string wire(16+count+(m_areas ? 512 : 0)+(m_postpass ? 512+count : 0),'\0');wire.replace(0,8,"GLSTERR2");
	Write16(wire,8,static_cast<uint16_t>(m_width)); Write16(wire,10,static_cast<uint16_t>(m_height)); wire[12] = m_wrap ? 1 : 0;
	wire[13]=m_areas ? 1 : 0;wire[14]=m_postpass ? 1 : 0;
	for (size_t i = 0; i < m_regions.size(); ++i) { wire[16+i] = static_cast<char>(m_regions[i]); }
	size_t offset=16+count;
	if(m_areas) {for(const auto value:*m_areas) {Write32(wire,offset,value);offset+=4;}}
	if(m_postpass) {
		for(const auto value:m_postpass->fertility) {Write32(wire,offset,value);offset+=4;}
		for(const auto value:m_postpass->coastal) {wire[offset++]=static_cast<char>(value);}
	}
	return wire;
}
GlsmacTerritoryTopology GlsmacTerritoryTopology::Deserialize( const std::string& wire ) {
	Require( wire.size() >= 16 && wire.size() <= MAX_WIRE_BYTES && (wire.compare(0,8,"GLSTERR1") == 0 || wire.compare(0,8,"GLSTERR2") == 0),
		"Invalid territory topology header or size" );
	const auto width = Read16(wire,8), height = Read16(wire,10); const auto count = Count(width,height);
	const bool legacy=wire[7]=='1';
	Require((wire[13]==0 || (!legacy && wire[13]==1)) && (wire[14]==0 || (!legacy && wire[14]==1))
		&& (!wire[14] || wire[13]),"Invalid territory metadata flags");
	const bool areas=wire[13]==1,postpass=wire[14]==1;
	Require( wire.size() == 16+count+(areas ? 512 : 0)+(postpass ? 512+count : 0) && (wire[12] == 0 || wire[12] == 1) && !wire[15],
		"Invalid territory topology framing" );
	// Validate every byte before allocating the owned result.
	for (size_t i = 0; i < count; ++i) {
		const auto region = static_cast<uint8_t>(wire[16+i]); const auto y = i/(width/2);
		Require( y == 0 || y == height-1 ? region == 63 || region == 127 : (region >= 1 && region <= 61) || (region >= 65 && region <= 125),
			"Invalid territory topology region" );
	}
	if(postpass) {for(size_t i=16+count+1024;i<wire.size();++i) {Require(static_cast<uint8_t>(wire[i])<=3,"Invalid coastal metadata flags");}}
	std::vector<uint8_t> regions; regions.reserve(count);
	for (size_t i = 0; i < count; ++i) { regions.push_back(static_cast<uint8_t>(wire[16+i])); }
	auto result=GlsmacTerritoryTopology(width,height,wire[12] != 0,std::move(regions));size_t offset=16+count;
	if(areas) {result.m_areas=RegionValues{};for(auto& value:*result.m_areas) {value=Read32(wire,offset);offset+=4;}}
	if(postpass) {
		result.m_postpass=TerrainPostpass{};for(auto& value:result.m_postpass->fertility) {value=Read32(wire,offset);offset+=4;}
		result.m_postpass->coastal.reserve(count);for(size_t i=0;i<count;++i) {result.m_postpass->coastal.push_back(static_cast<uint8_t>(wire[offset++]));}
	}
	return result;
}

}
