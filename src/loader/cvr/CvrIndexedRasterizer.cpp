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

Steps MakeSteps( const CvrIndexedDraw& draw ) {
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
					Signed32( steps[ index ][ row ] * ( row == 2 ? 256 : 1 ) );
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
	const CvrIndexedDraw& draw, CvrIndexedResult* const output ) {
	if ( !draw.records ) { Invalid( " missing records" ); }
	const auto& records = *draw.records;
	if ( records.size() <= size_t( draw.terminal ) ) { Invalid( " empty effective draw" ); }
	if ( draw.footprint != CvrSampleFootprint::Single && draw.footprint != CvrSampleFootprint::Square2 ) { Invalid( " unsupported footprint" ); }
	const auto steps = MakeSteps( draw );
	std::array< int64_t, 3 > position{ draw.initial[ 0 ], draw.initial[ 1 ], draw.initial[ 2 ] };
	Signed32( position[ 2 ] * 256 );
	if ( draw.terminal && records.back().Code() != 26 ) { Invalid( " missing terminal code" ); }
	const auto effective = records.size() - size_t( draw.terminal );
	for ( size_t i = 0 ; i < effective ; ++i ) {
		const auto& record = records[ i ];
		const auto code = record.Code();
		if ( code >= steps.size() ) { Invalid( " unsupported sample code" ); }
		for ( size_t row = 0 ; row < 3 ; ++row ) {
			position[ row ] += steps[ code ][ row ];
			Signed32( position[ row ] * ( row == 2 ? 256 : 1 ) );
		}
		const auto x = Floor256( position[ 0 ] );
		const auto y = Floor256( position[ 1 ] );
		const auto extra = draw.footprint == CvrSampleFootprint::Square2 ? 1 : 0;
		if ( x < 0 || y < 0 || x + extra >= initial.width || y + extra >= initial.height ) { Invalid( " sample outside surface" ); }
		const size_t first_index = ( size_t( record.bytes[ 0 ] & 7 ) << 8 ) | record.bytes[ 1 ];
		const size_t second_index = ( size_t( lookups.first[ first_index ] ) << 8 ) | record.bytes[ 2 ];
		if ( second_index >= lookups.second.size() ) { Invalid( " lookup outside supplied table" ); }
		if ( output ) {
			++output->processed_records;
			const auto depth = static_cast< uint16_t >( Floor256( position[ 2 ] ) );
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

}

CvrIndexedResult CvrIndexedRasterizer::Draw( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
	const std::vector< CvrIndexedDraw >& draws ) {
	const auto cells = uint64_t( initial.pitch ) * initial.height;
	if ( !initial.width || !initial.height || initial.pitch < initial.width || cells > MAX_SURFACE_SAMPLES ||
		initial.color.size() != cells || initial.depth.size() != cells ) { Invalid( " invalid surface dimensions or buffers" ); }
	if ( lookups.second.empty() || lookups.second.size() > 65536 || lookups.second.size() % 256 ) { Invalid( " invalid second lookup extent" ); }
	if ( draws.size() > MAX_DRAWS ) { Invalid( " too many draws" ); }
	size_t count = 0;
	for ( const auto& draw : draws ) {
		if ( !draw.records || draw.records->size() > MAX_RECORDS - count ) { Invalid( " missing or excessive records" ); }
		count += draw.records->size();
		Walk( initial, lookups, draw, nullptr );
	}
	CvrIndexedResult result{ initial, 0, 0 };
	for ( const auto& draw : draws ) { Walk( initial, lookups, draw, &result ); }
	return result;
}

}
