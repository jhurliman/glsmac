#include "MenuCompositor.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace loader::flic {

MenuImage ComposeMenu( const Frame& frame, const std::vector< uint8_t >& background,
    size_t background_width, size_t background_height,
    const std::array< std::array< uint8_t, 3 >, 256 >& common_palette, size_t rows,
    size_t viewport_width, size_t viewport_height ) {
    if ( !frame.width || !frame.height || !rows || !background_width || !background_height ||
        background_width > std::numeric_limits< size_t >::max() / background_height ||
        background.size() != background_width * background_height ||
        frame.pixels.size() != size_t( frame.width ) * frame.height ||
        background_width > UINT32_MAX || background_height > UINT32_MAX ||
        rows > ( 16 * 1024 * 1024 ) / size_t( frame.width ) / frame.height ||
        bool( viewport_width ) != bool( viewport_height ) ) {
        throw std::invalid_argument( "SMAC menu dimensions exceed the background" );
    }
    if ( !viewport_width ) { viewport_width = background_width; viewport_height = background_height; }
    if ( viewport_width > 65535 || viewport_height > 65535 ) { throw std::invalid_argument( "SMAC menu viewport exceeds renderer dimensions" ); }
    std::array< std::array< uint8_t, 3 >, 256 > palette = {};
    for ( size_t i = 0; i < 236; ++i ) {
        for ( size_t c = 0; c < 3; ++c ) { palette[ i + 10 ][ c ] = std::min< uint8_t >( common_palette[ i ][ c ], 235 ); }
    }
    std::array< uint8_t, 236 > blue = {};
    for ( size_t i = 0; i < blue.size(); ++i ) {
        std::array< int, 3 > mixed;
        for ( size_t c = 0; c < 3; ++c ) { mixed[ c ] = ( 20 * palette[ i + 10 ][ c ] + 80 * palette[ 179 ][ c ] ) / 100; }
        int best_distance = std::numeric_limits< int >::max();
        for ( size_t candidate = 10; candidate <= 245; ++candidate ) {
            int distance = 0;
            for ( size_t c = 0; c < 3; ++c ) { const int delta = mixed[ c ] - palette[ candidate ][ c ]; distance += delta * delta; }
            // Strict comparison retains the first game index for equal distances.
            if ( distance < best_distance ) { best_distance = distance; blue[ i ] = candidate; }
        }
    }
    MenuImage result{ frame.width, rows * frame.height, {} };
    if ( result.width > std::numeric_limits< size_t >::max() / result.height / 4 ) {
        throw std::invalid_argument( "SMAC menu output is too large" );
    }
    result.rgba.resize( result.width * result.height * 4 );
    const int64_t left = int64_t( viewport_width ) - int64_t( result.width ), top = int64_t( viewport_height ) - int64_t( result.height );
    for ( size_t y = 0; y < result.height; ++y ) {
        for ( size_t x = 0; x < result.width; ++x ) {
            auto index = frame.pixels[ ( y % frame.height ) * frame.width + x ];
            auto* pixel = result.rgba.data() + ( y * result.width + x ) * 4;
            if ( index == 9 ) { continue; }
            if ( index == 0 ) {
                const auto display_x = left + int64_t( x ), display_y = top + int64_t( y );
                if ( display_x < 0 || display_y < 0 ) { continue; } // clipped portion of a small viewport
                // Match nearest-neighbor sampling of the stretched background
                // at this displayed pixel center, not the unscaled PCX corner.
                const size_t source_x = ( uint64_t( 2 * display_x + 1 ) * background_width ) / ( 2 * viewport_width );
                const size_t source_y = ( uint64_t( 2 * display_y + 1 ) * background_height ) / ( 2 * viewport_height );
                const auto source = background[ source_y * background_width + source_x ];
                if ( source >= blue.size() ) { throw std::invalid_argument( "SMAC menu background uses a reserved PCX index" ); }
                index = blue[ source ];
            }
            else if ( index < 10 || index > 245 ) {
                throw std::invalid_argument( "SMAC menu artwork uses an unsupported reserved game index" );
            }
            std::copy( palette[ index ].begin(), palette[ index ].end(), pixel );
            pixel[ 3 ] = 255;
        }
    }
    return result;
}

}
