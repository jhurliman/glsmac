#include "MenuCompositor.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace loader::flic {

MenuImage ComposeMenu( const Frame& frame, const std::vector< uint8_t >& background,
    size_t background_width, size_t background_height,
    const std::array< std::array< uint8_t, 3 >, 256 >& common_palette, size_t rows ) {
    if ( !frame.width || !frame.height || !rows || !background_width || !background_height ||
        background_width > std::numeric_limits< size_t >::max() / background_height ||
        background.size() != background_width * background_height ||
        frame.pixels.size() != size_t( frame.width ) * frame.height ||
        frame.width > background_width || rows > background_height / frame.height ) {
        throw std::invalid_argument( "SMAC menu dimensions exceed the background" );
    }
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
    const size_t left = background_width - result.width, top = background_height - result.height;
    for ( size_t y = 0; y < result.height; ++y ) {
        for ( size_t x = 0; x < result.width; ++x ) {
            auto index = frame.pixels[ ( y % frame.height ) * frame.width + x ];
            auto* pixel = result.rgba.data() + ( y * result.width + x ) * 4;
            if ( index == 9 ) { continue; }
            if ( index == 0 ) {
                const auto source = background[ ( top + y ) * background_width + left + x ];
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
