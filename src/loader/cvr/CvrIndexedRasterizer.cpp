#include "CvrIndexedRasterizer.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace loader::cvr {
namespace {

[[noreturn]] void Invalid( const char* message ) {
	throw std::invalid_argument( std::string( "CVR indexed raster:" ) + message );
}

void Signed32( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		Invalid( " signed32 arithmetic overflow" );
	}
}

int64_t Floor256( const int64_t value ) {
	return value >= 0 ? value / 256 : -( ( -value + 255 ) / 256 );
}

using Steps = std::array< std::array< int64_t, 3 >, 26 >;

Steps MakeSteps( const CvrIndexedDraw& draw, const bool modulo_depth ) {
	for ( size_t row = 0 ; row < 2 ; ++row ) {
		for ( const auto component : draw.basis[ row ] ) {
			if ( component < -32768 || component > 32767 ) { Invalid( " basis row exceeds signed16" ); }
		}
	}
	Steps steps{};
	size_t index = 0;
	for ( int a = -1 ; a <= 1 ; ++a ) {
		for ( int b = -1 ; b <= 1 ; ++b ) {
			for ( int c = -1 ; c <= 1 ; ++c ) {
				if ( !a && !b && !c ) { continue; }
				for ( size_t row = 0 ; row < 3 ; ++row ) {
					steps[ index ][ row ] = int64_t( a ) * draw.basis[ row ][ 0 ] + int64_t( b ) * draw.basis[ row ][ 1 ] + int64_t( c ) * draw.basis[ row ][ 2 ];
					if ( row != 2 || !modulo_depth ) { Signed32( steps[ index ][ row ] * ( row == 2 ? 256 : 1 ) ); }
				}
				++index;
			}
		}
	}
	return steps;
}

// Both passes use checked int64 arithmetic. The first touches no output buffer;
// the second has no allocations and applies only fully prevalidated samples.
void Walk( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
	const CvrIndexedDraw& draw, CvrIndexedResult* const output, const bool modulo_depth ) {
	if ( !draw.records ) { Invalid( " missing records" ); }
	const auto& records = *draw.records;
    if(draw.clip && (draw.footprint!=CvrSampleFootprint::Single || (*draw.clip)[0]<0 || (*draw.clip)[1]<0
        || (*draw.clip)[2]<(*draw.clip)[0] || (*draw.clip)[3]<(*draw.clip)[1]
        || uint32_t((*draw.clip)[2])>initial.width || uint32_t((*draw.clip)[3])>initial.height)) {Invalid(" invalid kernel clip");}
	if ( records.size() <= size_t( draw.terminal ) ) { Invalid( " empty effective draw" ); }
	if ( draw.footprint != CvrSampleFootprint::Single && draw.footprint != CvrSampleFootprint::Square2 ) { Invalid( " unsupported footprint" ); }
	const auto steps = MakeSteps( draw, modulo_depth );
	std::array< int64_t, 3 > position{ draw.initial[ 0 ], draw.initial[ 1 ], draw.initial[ 2 ] };
	uint32_t depth_position = static_cast< uint32_t >( draw.initial[ 2 ] ) << 8;
	if ( !modulo_depth ) { Signed32( position[ 2 ] * 256 ); }
	if ( draw.terminal && records.back().Code() != 26 ) { Invalid( " missing terminal code" ); }
	const auto effective = records.size() - size_t( draw.terminal );
	for ( size_t i = 0 ; i < effective ; ++i ) {
		const auto& record = records[ i ];
		const auto code = record.Code();
		if ( code >= steps.size() ) { Invalid( " unsupported sample code" ); }
		for ( size_t row = 0 ; row < 3 ; ++row ) {
			if ( row == 2 && modulo_depth ) {
				depth_position += static_cast< uint32_t >( steps[ code ][ row ] ) << 8;
				continue;
			}
			position[ row ] += steps[ code ][ row ];
			Signed32( position[ row ] * ( row == 2 ? 256 : 1 ) );
		}
		const auto x = Floor256( position[ 0 ] );
		const auto y = Floor256( position[ 1 ] );
		const auto extra = draw.footprint == CvrSampleFootprint::Square2 ? 1 : 0;
        if(output) {++output->processed_records;}
        if(draw.clip && (x<(*draw.clip)[0] || y<(*draw.clip)[1] || x>=(*draw.clip)[2] || y>=(*draw.clip)[3])) {continue;}
		if ( x < 0 || y < 0 || x + extra >= initial.width || y + extra >= initial.height ) { Invalid( " sample outside surface" ); }
		const size_t first_index = ( size_t( record.bytes[ 0 ] & 7 ) << 8 ) | record.bytes[ 1 ];
		const size_t second_index = ( size_t( lookups.first[ first_index ] ) << 8 ) | record.bytes[ 2 ];
		if ( second_index >= lookups.second.size() ) { Invalid( " lookup outside supplied table" ); }
		if ( output ) {
			const auto depth = static_cast< uint16_t >( modulo_depth ? depth_position >> 16 : Floor256( position[ 2 ] ) );
			const auto color = lookups.second[ second_index ];
			for ( int dy = 0 ; dy <= extra ; ++dy ) {
				for ( int dx = 0 ; dx <= extra ; ++dx ) {
					const auto offset = size_t( y + dy ) * initial.pitch + size_t( x + dx );
					if ( depth < output->surface.depth[ offset ] ) {
						output->surface.depth[ offset ] = depth;
						output->surface.color[ offset ] = color;
						++output->written_samples;
					}
				}
			}
		}
	}
}

CvrIndexedResult DrawBatch( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
	const std::vector< CvrIndexedDraw >& draws, const bool modulo_depth ) {
	const auto cells = uint64_t( initial.pitch ) * initial.height;
	if ( !initial.width || !initial.height || initial.pitch < initial.width || cells > CvrIndexedRasterizer::MAX_SURFACE_SAMPLES ||
		initial.color.size() != cells || initial.depth.size() != cells ) { Invalid( " invalid surface dimensions or buffers" ); }
	if ( lookups.second.empty() || lookups.second.size() > 65536 || lookups.second.size() % 256 ) { Invalid( " invalid second lookup extent" ); }
	if ( draws.size() > CvrIndexedRasterizer::MAX_DRAWS ) { Invalid( " too many draws" ); }
	size_t count = 0;
	for ( const auto& draw : draws ) {
		if ( !draw.records || draw.records->size() > CvrIndexedRasterizer::MAX_RECORDS - count ) { Invalid( " missing or excessive records" ); }
		count += draw.records->size();
		Walk( initial, lookups, draw, nullptr, modulo_depth );
	}
	CvrIndexedResult result{ initial, 0, 0 };
	for ( const auto& draw : draws ) { Walk( initial, lookups, draw, &result, modulo_depth ); }
	return result;
}

}

CvrIndexedResult CvrIndexedRasterizer::Draw( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
	const std::vector< CvrIndexedDraw >& draws ) { return DrawBatch( initial, lookups, draws, false ); }

CvrIndexedResult CvrIndexedRasterizer::DrawModuloDepth( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
	const std::vector< CvrIndexedDraw >& draws ) { return DrawBatch( initial, lookups, draws, true ); }


CvrShadowResult CvrIndexedRasterizer::Shadow(uint32_t width,uint32_t height,const std::vector<CvrIndexedDraw>& draws) {
    if(!width || !height || uint64_t(width)*height>MAX_SURFACE_SAMPLES || draws.size()>MAX_DRAWS) {Invalid(" invalid shadow extent");}
    CvrShadowResult result{width,height,width,std::vector<uint8_t>(size_t(width)*height,0)};
    result.draw_count=draws.size();
    size_t records=0;
    for(const auto& draw:draws) {
        if(!draw.records || (draw.footprint!=CvrSampleFootprint::Single && draw.footprint!=CvrSampleFootprint::Square2) || draw.clip || draw.records->size()<=size_t(draw.terminal)
            || (draw.terminal && draw.records->back().Code()!=26) || draw.records->size()>MAX_RECORDS-records) {Invalid(" unsupported shadow stream");}
        records+=draw.records->size();
        auto coordinates=draw;coordinates.basis[2]={0,0,0};coordinates.initial[2]=0;
        const auto steps=MakeSteps(coordinates,false);
        int64_t x=coordinates.initial[0],y=coordinates.initial[1];
        for(size_t i=0;i<draw.records->size()-size_t(draw.terminal);++i) {
            const auto code=draw.records->at(i).Code();if(code>=26) {Invalid(" invalid shadow direction");}
            x+=steps[code][0];y+=steps[code][1];Signed32(x);Signed32(y);
            const auto px=Floor256(x),py=Floor256(y);
            const int side=draw.footprint==CvrSampleFootprint::Single?1:2;
            if(px<0 || py<0 || px+side>width || py+side>height) {Invalid(" shadow sample outside surface");}
            for(int dy=0;dy<side;++dy) {for(int dx=0;dx<side;++dx) {
                result.mask[size_t(py+dy)*width+size_t(px+dx)]=128;++result.written_samples;
            }}
            ++result.processed_records;
        }
    }
    return result;
}

size_t CvrShadowResult::Blend(std::vector<uint8_t>& target,uint32_t target_width,uint32_t target_height,uint32_t target_pitch,
    const std::array<uint8_t,256>& lookup,const std::array<int32_t,2>& origin,const std::array<int32_t,4>& clip) const {
    if(&target==&mask || !width || !height || pitch<width || uint64_t(pitch)*height!=mask.size()
        || !target_width || !target_height || target_pitch<target_width || uint64_t(target_pitch)*target_height!=target.size()
        || target.size()>CvrIndexedRasterizer::MAX_SURFACE_SAMPLES || mask.size()>CvrIndexedRasterizer::MAX_SURFACE_SAMPLES
        || clip[0]<0 || clip[1]<0 || clip[2]<clip[0] || clip[3]<clip[1]
        || uint32_t(clip[2])>target_width || uint32_t(clip[3])>target_height) {Invalid(" invalid shadow target or clip");}
    size_t stores=0;
    for(uint32_t y=0;y<height;++y) {for(uint32_t x=0;x<width;++x) {
        const int64_t px=int64_t(origin[0])+x,py=int64_t(origin[1])+y;
        if(mask[size_t(y)*pitch+x] && px>=clip[0] && px<clip[2] && py>=clip[1] && py<clip[3]) {
            auto& pixel=target[size_t(py)*target_pitch+size_t(px)];pixel=lookup[pixel];++stores;
        }
    }}
    return stores;
}

}
