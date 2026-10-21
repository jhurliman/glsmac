#include "CvrActorMaterial.h"
#include "loader/txt/TXTLoader.h"
#include "util/String.h"
#include <charconv>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace loader::cvr {
namespace {

struct Pcx {
	size_t width, height;
	std::vector< uint8_t > pixels;
	std::array< CvrPalette::Color, 256 > palette;
};

Pcx ReadPcx( const std::string_view bytes ) {
	constexpr size_t HEADER = 128, PALETTE = 769, MAX_BYTES = 16 * 1024 * 1024;
	if ( bytes.size() < HEADER + PALETTE || bytes.size() > MAX_BYTES ) { throw std::invalid_argument( "CVR actor PCX: invalid file size" ); }
	const auto byte = [ & ]( const size_t i ) { return static_cast< uint8_t >( bytes[ i ] ); };
	const auto word = [ & ]( const size_t i ) { return uint32_t( byte( i ) ) | ( uint32_t( byte( i + 1 ) ) << 8 ); };
	if ( byte( 0 ) != 10 || byte( 1 ) != 5 || byte( 2 ) != 1 || byte( 3 ) != 8 || byte( 65 ) != 1 || word( 4 ) != 0 || word( 6 ) != 0 ) {
		throw std::invalid_argument( "CVR actor PCX: unsupported format" );
	}
	const size_t width = word( 8 ) + 1, height = word( 10 ) + 1, stride = word( 66 );
	if ( width > 2048 || height > 2048 || stride < width || stride > 2048 || ( stride & 1 ) != 0 || byte( bytes.size() - PALETTE ) != 12 ) {
		throw std::invalid_argument( "CVR actor PCX: unsupported dimensions or palette" );
	}
	const size_t end = bytes.size() - PALETTE;
	// Validate every RLE row and exact framing before allocating decoded pixels.
	const auto decode = [ & ]( std::vector< uint8_t >* output ) {
		size_t cursor = HEADER;
		for ( size_t y = 0; y < height; ++y ) {
			size_t x = 0;
			while ( x < stride ) {
				if ( cursor == end ) { throw std::invalid_argument( "CVR actor PCX: truncated pixels" ); }
				uint8_t value = byte( cursor++ ); size_t count = 1;
				if ( ( value & 0xc0 ) == 0xc0 ) {
					count = value & 0x3f;
					if ( count == 0 || cursor == end ) { throw std::invalid_argument( "CVR actor PCX: invalid run" ); }
					value = byte( cursor++ );
				}
				if ( count > stride - x ) { throw std::invalid_argument( "CVR actor PCX: run crosses scanline" ); }
				if ( output && x < width ) {
					std::fill_n( output->begin() + static_cast< ptrdiff_t >( y * width + x ), std::min( count, width - x ), value );
				}
				x += count;
			}
		}
		if ( cursor != end ) { throw std::invalid_argument( "CVR actor PCX: trailing pixel data" ); }
	};
	decode( nullptr );
	Pcx result{ width, height, std::vector< uint8_t >( width * height ), {} };
	decode( &result.pixels );
	for ( size_t i = 0; i < 256; ++i ) {
		for ( size_t channel = 0; channel < 3; ++channel ) { result.palette[ i ][ channel ] = byte( end + 1 + i * 3 + channel ); }
	}
	return result;
}

}

std::vector<uint8_t> CvrActorMaterial::ReadWorkshopBackdrop(std::string_view deswin_pcx) {
    auto image=ReadPcx(deswin_pcx);
    if(image.width!=623 || image.height!=368) {throw std::invalid_argument("Workshop requires its original623x368 backdrop");}
    for(auto& pixel:image.pixels) {pixel=uint8_t(pixel+10);}return std::move(image.pixels);
}
std::array<std::vector<uint8_t>,10> CvrActorMaterial::ReadWorkshopWeaponSprites(std::string_view units_pcx) {
    const auto image=ReadPcx(units_pcx);std::array<std::vector<uint8_t>,10> result;
    if(image.width<816 || image.height<693) {throw std::invalid_argument("Incomplete Workshop weapon sprite sheet");}
    for(size_t id=0;id<10;++id) {
        const size_t left=id==9?716:2,top=id==9?233:2+77*id;auto& sprite=result[id];sprite.reserve(7500);
        for(size_t y=0;y<75;++y) {for(size_t x=0;x<100;++x) {
            auto pixel=uint8_t(image.pixels[(top+y)*image.width+left+x]+10);
            if(pixel==6 || pixel==7) {pixel=9;}sprite.push_back(pixel);
        }}
    }return result;
}
std::array<std::vector<CvrActorMaterial::WorkshopSprite>,10> CvrActorMaterial::ReadWorkshopUnitSprites(std::string_view units_pcx) {
    const auto image=ReadPcx(units_pcx);std::array<std::vector<WorkshopSprite>,10> result;
    if(image.width<816 || image.height<693) {throw std::invalid_argument("Incomplete Workshop unit sprite sheet");}
    for(size_t icon=0;icon<10;++icon) {for(size_t variant=0;variant<(icon==3 ? 7u:1u);++variant) {
        const size_t left=icon==9 ? 716:2+102*variant,top=icon==9 ? 233:2+77*icon;
        std::vector<uint8_t> cell(7500);int l=100,t=75,r=0,b=0;
        for(int y=0;y<75;++y) {for(int x=0;x<100;++x) {
            auto pixel=uint8_t(image.pixels[(top+size_t(y))*image.width+left+size_t(x)]+10);
            if(pixel==6 || pixel==7) {pixel=9;}cell[size_t(y)*100+x]=pixel;
            if(pixel!=9) {l=std::min(l,x);r=std::max(r,x+1);t=std::min(t,y);b=std::max(b,y+1);}
        }}
        WorkshopSprite sprite;
        if(l<r && t<b) {
            sprite.width=r-l;sprite.height=b-t;sprite.offset={l,t};sprite.pixels.reserve(size_t(sprite.width)*sprite.height);
            for(int y=t;y<b;++y) {sprite.pixels.insert(sprite.pixels.end(),cell.begin()+y*100+l,cell.begin()+y*100+r);}
        }
        result[icon].push_back(std::move(sprite));
    }}return result;
}
void CvrActorMaterial::PaintWorkshopWeaponSprite(std::vector<uint8_t>& dst,size_t width,size_t height,
    const std::vector<uint8_t>& sprite,int ordinal) {
    if(&dst==&sprite || width==0 || height==0 || width>2048 || height>2048 || dst.size()!=width*height
        || sprite.size()!=7500 || ordinal<0 || ordinal>=20) {throw std::invalid_argument("Invalid Workshop weapon underlay surface");}
    const int left=27+108*(ordinal%5),top=4+94*(ordinal/5);
    // Keep the logical100x75 origin: an untrimmed keyed copy is equivalent
    // to preserving the native trim offsets, including the extra x129 pixel.
    for(int y=0;y<75;++y) {for(int x=0;x<100;++x) {
        const int px=left+3+x,py=top+8+y;const auto pixel=sprite[size_t(y)*100+x];
        if(pixel!=9 && px>=left && px<left+105 && py>=top && py<top+91 && px>=0 && py>=0
            && size_t(px)<width && size_t(py)<height) {dst[size_t(py)*width+size_t(px)]=pixel;}
    }}
}

std::array< CvrPalette::Color, 256 > CvrActorMaterial::ReadSoftwareColors( const std::string_view palette_pcx ) {
	const auto palette = ReadPcx( palette_pcx );
	std::array< CvrPalette::Color, 256 > colors{};
	static constexpr std::array< CvrPalette::Color, 20 > RESERVED{{
		{ 0, 0, 0 }, { 128, 0, 0 }, { 0, 128, 0 }, { 128, 128, 0 },
		{ 0, 0, 128 }, { 128, 0, 128 }, { 0, 128, 128 }, { 192, 192, 192 },
		{ 8, 0, 0 }, { 9, 0, 0 }, { 245, 0, 0 }, { 246, 0, 0 },
		{ 128, 128, 128 }, { 255, 0, 0 }, { 0, 255, 0 }, { 255, 255, 0 },
		{ 0, 0, 255 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 }
	}};
	for ( size_t i = 0; i < 10; ++i ) { colors[ i ] = RESERVED[ i ]; colors[ i + 246 ] = RESERVED[ i + 10 ]; }
	std::copy_n( palette.palette.begin(), 236, colors.begin() + 10 );
	return colors;
}

CvrActorMaterial CvrActorMaterial::Read( const std::string_view palette_pcx, const std::string_view faction_pcx,
	const std::array< uint8_t, 24 * 256 >& body_shades ) {
	CvrActorMaterial result{};
	result.colors = ReadSoftwareColors( palette_pcx );
	const auto faction = ReadPcx( faction_pcx );
	if ( faction.width <= 435 || faction.height <= 745 ) { throw std::invalid_argument( "CVR actor PCX: tint sample unavailable" ); }
	result.tint_index = static_cast< uint8_t >( uint32_t( faction.pixels[ 745 * faction.width + 435 ] ) + 10 );
    result.shades=Tint(result.colors,body_shades,result.tint_index);
	return result;
}


std::array<uint8_t,24*256> CvrActorMaterial::Tint(const std::array<CvrPalette::Color,256>& colors,
    const std::array<uint8_t,24*256>& source,const uint8_t tint_index) {
    std::array<uint8_t,24*256> result;
	const auto& tint = colors[tint_index];
	std::array< uint8_t, 256 > nearest{};
	for ( size_t i = 0; i < 256; ++i ) {
		std::array< int32_t, 3 > mixed;
		for ( size_t channel = 0; channel < 3; ++channel ) {
			// The binary32 1/255 constant is exactly representable as this double;
			// the uint8 products and product with it are exact (at most40 bits).
			mixed[ channel ] = static_cast< int32_t >( int32_t( colors[ i ][ channel ] ) * tint[ channel ] * 0.0039215688593685626983642578125 );
		}
		int32_t best = std::numeric_limits< int32_t >::max();
		for ( size_t candidate = 0; candidate < 256; ++candidate ) {
			int32_t distance = 0;
			for ( size_t channel = 0; channel < 3; ++channel ) {
				const int32_t difference = mixed[ channel ] - colors[ candidate ][ channel ];
				distance += difference * difference;
			}
			if ( distance < best ) { best = distance; nearest[ i ] = static_cast< uint8_t >( candidate ); }
		}
	}
	for ( size_t i = 0; i < source.size(); ++i ) { result[ i ] = nearest[ source[ i ] ]; }
    return result;
}
std::array<CvrPalette::Color,256> CvrActorMaterial::ReadGamma100Colors(std::string_view palette_pcx) {
    auto colors=ReadSoftwareColors(palette_pcx);
    for(size_t i=10;i<246;++i) {for(auto& channel:colors[i]) {channel=std::min<uint8_t>(channel,235);}}
    return colors;
}
CvrWorkshopMaterials CvrActorMaterial::ReadWorkshop(std::string_view palette_pcx,std::string_view vbp,std::string_view system) {
    const auto armor=txt::TXTLoader::ReadUniqueSection(std::string(system),"ARMORCOLORING");
    const auto transparent=txt::TXTLoader::ReadUniqueSection(std::string(system),"ARMORCOLORING_TRANS");
    if(armor.size()!=4 || transparent.size()!=1) {throw std::invalid_argument("Workshop armor coloring requires four tints and one composite rule");}
    const auto integer=[](std::string_view field) {
        const auto text=util::String::TrimCopy(std::string(field));int32_t value;
        const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
        if(parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size()) {throw std::invalid_argument("Invalid Workshop armor color integer");}return value;
    };
    CvrWorkshopMaterials result{};result.colors=ReadGamma100Colors(palette_pcx);
    const auto source=CvrPaletteReader::Read(CvrContainerReader::Read(vbp)).shades;
    for(size_t i=0;i<4;++i) {
        const auto value=integer(std::string_view(armor[i]).substr(0,armor[i].find(',')));
        if(value<0 || value>245) {throw std::invalid_argument("Workshop armor tint outside admitted relative palette");}
        result.armor[i]=Tint(result.colors,source,uint8_t(value+10));
    }
    std::array<int32_t,3> rule;std::string_view text=transparent[0];
    for(auto& value:rule) {const auto end=text.find(',');if(end==std::string_view::npos) {throw std::invalid_argument("Incomplete Workshop translucency rule");}
        value=integer(text.substr(0,end));text.remove_prefix(end+1);}
    if(rule[0]<0 || rule[0]>245 || rule[1]<0 || rule[1]>100 || rule[2]<0 || rule[2]>100) {throw std::invalid_argument("Workshop translucency outside admitted color/percentage domain");}
    for(int row=0;row<24;++row) {
        const auto weight=(24*rule[1]+(rule[2]-rule[1])*row)/24;
        for(int destination=0;destination<256;++destination) {
            int chosen=destination;
            if(destination>=10 && destination<=245) {
                std::array<int,3> mixed;for(size_t channel=0;channel<3;++channel) {mixed[channel]=((100-weight)*result.colors[destination][channel]+weight*result.colors[rule[0]+10][channel])/100;}
                int best=INT32_MAX;for(int candidate=10;candidate<=245;++candidate) {
                    int distance=0;for(size_t channel=0;channel<3;++channel) {const int d=mixed[channel]-result.colors[candidate][channel];distance+=d*d;}
                    if(distance<best) {best=distance;chosen=candidate;}
                }
            }
            result.translucency[row*256+destination]=uint8_t(chosen);
        }
    }return result;
}

}
