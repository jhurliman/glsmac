#include "CvrSpartanRoverRenderer.h"
#include "CvrScoutRenderer.h"
#include "loader/cvr/CvrSpartanRoverRenderer.h"
#include "loader/cvr/ScoutView.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <stdexcept>
#include <limits>

namespace gse::tests {
namespace {
using namespace loader::cvr;

// Independent PCX encoder: a padded row, constant pixel, and grayscale palette.
// No installed image bytes or original output table are test dependencies.
std::string Pcx( const uint8_t pixel, const uint16_t width = 512, const uint16_t height = 746, const bool distinct_sample = false ) {
	std::string bytes( 128, '\0' );
	const auto word = [ & ]( const size_t offset, const uint16_t value ) { bytes[ offset ] = static_cast< char >( value ); bytes[ offset + 1 ] = static_cast< char >( value >> 8 ); };
	bytes[ 0 ] = 10; bytes[ 1 ] = 5; bytes[ 2 ] = 1; bytes[ 3 ] = 8; bytes[ 65 ] = 1;
	word( 8, static_cast< uint16_t >( width - 1 ) ); word( 10, static_cast< uint16_t >( height - 1 ) );
	const auto stride = static_cast< uint16_t >( ( width + 1 ) & ~1U ); word( 66, stride );
	for ( size_t y = 0; y < height; ++y ) {
		std::vector< uint8_t > row( stride, pixel );
		if ( distinct_sample ) {
			if ( y == 744 ) { row[ 435 ] = 100; }
			if ( y == 745 ) { row[ 434 ] = 20; row[ 435 ] = 245; row[ 436 ] = 80; }
			for ( size_t i = width; i < stride; ++i ) { row[ i ] = 250; }
		}
		for ( size_t x = 0; x < stride; ) {
			size_t count = 1;
			while ( count < 63 && count < stride - x && row[ x + count ] == row[ x ] ) { ++count; }
			bytes.push_back( static_cast< char >( 0xc0 | count ) ); bytes.push_back( static_cast< char >( row[ x ] ) ); x += count;
		}
	}
	bytes.push_back( 12 );
	for ( size_t i = 0; i < 256; ++i ) { for ( size_t channel = 0; channel < 3; ++channel ) { bytes.push_back( static_cast< char >( i ) ); } }
	return bytes;
}

template< typename F > bool Rejects( F&& action ) { try { action(); } catch ( const std::invalid_argument& ) { return true; } return false; }
constexpr CvrScoutViewInput VIEW{ 0, 0, 1, 1, { 128, 128 } };

using Bytes = std::vector< uint8_t >;
void PutWord( Bytes& bytes, const size_t offset, const uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { bytes.at( offset + i ) = static_cast< uint8_t >( value >> ( 8 * i ) ); }
}
Bytes DetailChain( const std::vector< uint32_t >& scalars ) {
	Bytes bytes;
	std::vector< size_t > offsets;
	for ( size_t i = 0; i < scalars.size(); ++i ) {
		const auto offset = bytes.size(), local = 47 + 3 * ( i + 1 ); offsets.push_back( offset );
		bytes.resize( offset + local, 0 );
		PutWord( bytes, offset, i + 1 < scalars.size() ? 1 : 0 ); PutWord( bytes, offset + 8, scalars[ i ] );
		PutWord( bytes, offset + 12, static_cast< uint32_t >( local ) ); PutWord( bytes, offset + 16, static_cast< uint32_t >( local - 1 ) );
		PutWord( bytes, offset + 38, static_cast< uint32_t >( i + 1 ) );
		for ( size_t j = 0; j <= i; ++j ) { bytes[ offset + 42 + j * 3 ] = 13 << 3; bytes[ offset + 44 + j * 3 ] = static_cast< uint8_t >( i + 20 ); }
	}
	for ( const auto offset : offsets ) { PutWord( bytes, offset + 4, static_cast< uint32_t >( bytes.size() - offset ) ); }
	return bytes;
}

}

std::string MakeSpartanRoverTestAsset( const std::string& name ) {
	if ( name == "system.txt" ) { return "#OTHER\n1\n#VEH_LIGHT_DIR_MAP\n6, 4, 15\n#NEXT\n2\n"; }
	if ( name == "PALETTE.PCX" ) { return Pcx( 0, 2, 2 ); }
	if ( name == "spartans.pcx" ) { return Pcx( 245 ); }
	if ( name == "VLIGHTS.CVR" ) { return MakeScoutTestAssetWithGeometry( DetailChain( { 0x41a00000, 0x42200000, 0x42a00000 } ) ); }
	for ( const auto* part : CvrSpartanRoverRenderer::PartNames() ) { if ( name == part ) { return MakeScoutTestAsset( false ); } }
	throw std::invalid_argument( "Unexpected synthetic Rover asset" );
}

void AddCvrSpartanRoverRendererTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR Rover detail chain preserves owned levels and exact threshold ties", GT() {
		auto bytes = DetailChain( { 0x3f800000, 0x40000000, 0x40800000 } ); const auto original = bytes;
		const auto chain = CvrSimpleDetailChain::Read( bytes );
		GT_ASSERT( bytes == original && chain.Levels().size() == 3, "chain decode mutated input or flattened levels" );
		bytes.clear();
		for ( size_t i = 0; i < 3; ++i ) {
			const auto& level = chain.Levels()[ i ];
			GT_ASSERT( level.flags == ( i < 2 ? 1U : 0U ) && level.end_offset == 47 + 3 * ( i + 1 ) &&
				level.end_minus_one + 1 == level.end_offset && level.groups[ 0 ].records.size() == i + 1 &&
				level.groups[ 0 ].records[ 0 ].bytes[ 2 ] == i + 20, "source flags/local extent/record ownership lost" );
		}
		GT_ASSERT( chain.Levels()[ 0 ].payload_size == original.size() && chain.Levels()[ 1 ].payload_size == original.size() - 50, "remaining-chain extents were normalized" );
		// Literal selector outcomes from RE-SMAC-CVR-LOD-001's owned 1/2/4
		// fixture. Our record contents/counts are independent of those queries.
		const std::array< std::array< uint32_t, 2 >, 16 > cases{{
			{ 3212836864U, 2 }, { 0, 2 }, { 1040187392, 2 }, { 1048576000, 2 }, { 1056964608, 1 }, { 1065353216, 0 }, { 1073741824, 0 },
			{ 1068827776, 0 }, { 1068827777, 0 }, { 1068827778, 0 }, { 1060439168, 1 }, { 1060439169, 1 }, { 1060439170, 0 },
			{ 1052050560, 2 }, { 1052050561, 2 }, { 1052050562, 1 }
		}};
		for ( const auto& value : cases ) { GT_ASSERT( chain.Select( value[ 0 ] ) == value[ 1 ], "detail threshold equality or adjacent value differs" ); }
		// Original installed-scalar query inputs/outputs, with self-authored
		// records. Full installed record preservation is an opt-in comparison.
		const auto installed_scalars = CvrSimpleDetailChain::Read( DetailChain( { 1083808154, 1086828053, 1090636481, 1093188295, 1096505653 } ) );
		const std::array< std::array< uint32_t, 2 >, 32 > installed_cases{{
			{ 3212836864U, 4 }, { 0, 4 }, { 981668463, 4 }, { 1008981770, 4 }, { 1028443341, 4 }, { 1032805417, 4 },
			{ 1033476506, 4 }, { 1034147594, 4 }, { 1035489772, 4 }, { 1036831949, 4 }, { 1040187392, 3 }, { 1041865114, 2 },
			{ 1045220557, 1 }, { 1048576000, 0 }, { 1056964608, 0 }, { 1065353216, 0 }, { 1073741824, 0 },
			{ 1050073365, 0 }, { 1050073366, 0 }, { 1050073367, 0 }, { 1047007973, 1 }, { 1047007974, 1 }, { 1047007975, 0 },
			{ 1043498160, 2 }, { 1043498161, 2 }, { 1043498162, 1 }, { 1040798304, 3 }, { 1040798305, 3 }, { 1040798306, 2 },
			{ 1037255593, 4 }, { 1037255594, 4 }, { 1037255595, 3 }
		}};
		for ( const auto& value : installed_cases ) { GT_ASSERT( installed_scalars.Select( value[ 0 ] ) == value[ 1 ], "installed-scalar detail selection differs (threshold or premature binary32 spill)" ); }
		GT_ASSERT( CvrSimpleDetailChain::Read( DetailChain( { 0x3f800000, 0x3f800000 } ) ).Select( 1068827777 ) == 0, "current-level equality failed to stop before following its link" );
		GT_ASSERT( chain.Select( 1 ) == 2 && chain.Select( 0x80000000 ) == 2 && chain.Select( 0x7f7fffff ) == 0, "finite extreme detail scale differs" );
		for ( const auto bits : { 0x7f800000U, 0xff800000U, 0x7fc00000U } ) { GT_ASSERT( Rejects( [ & ] { chain.Select( bits ); } ), "nonfinite detail scale admitted" ); }
		GT_OK();
	} );
	task->AddTest( "CVR Rover detail chain rejects late malformed links records and budgets", GT() {
		const auto good = DetailChain( { 0x3f800000, 0x40000000, 0x40800000 } );
		for ( size_t size = 0; size < good.size(); ++size ) {
			GT_ASSERT( Rejects( [ & ] { CvrSimpleDetailChain::Read( Bytes( good.begin(), good.begin() + size ) ); } ), "truncated detail chain accepted" );
		}
		for ( int scenario = 0; scenario < 15; ++scenario ) {
			auto bad = good; const size_t last = 103;
			switch ( scenario ) {
				case 0: PutWord( bad, 0, 3 ); break;
				case 1: PutWord( bad, 0, 0 ); break;
				case 2: PutWord( bad, 4, 50 ); break;
				case 3: PutWord( bad, 12, 0 ); break;
				case 4: PutWord( bad, 12, UINT32_MAX ); break;
				case 5: PutWord( bad, 16, 50 ); break;
				case 6: PutWord( bad, last, 1 ); break;
				case 7: PutWord( bad, last + 4, 55 ); break;
				case 8: PutWord( bad, last + 8, 0x7f800000 ); break;
				case 9: PutWord( bad, last + 8, 0x80000000 ); break;
				case 10: PutWord( bad, last + 38, 4 ); break;
				case 11: bad[ last + 42 ] = 26 << 3; break;
				case 12: bad.back() = 1; break;
				case 13: bad.push_back( 0 ); break;
				case 14: PutWord( bad, last + 38, CvrSimpleDetailChain::MAX_RECORDS ); break;
			}
			GT_ASSERT( Rejects( [ & ] { CvrSimpleDetailChain::Read( bad ); } ), "malformed detail chain accepted: " + std::to_string( scenario ) );
		}
		GT_ASSERT( CvrSimpleDetailChain::Read( DetailChain( std::vector< uint32_t >( CvrSimpleDetailChain::MAX_LEVELS, 0x3f800000 ) ) ).Levels().size() == CvrSimpleDetailChain::MAX_LEVELS, "exact detail-level cap rejected" );
		GT_ASSERT( Rejects( [] { CvrSimpleDetailChain::Read( DetailChain( std::vector< uint32_t >( CvrSimpleDetailChain::MAX_LEVELS + 1, 0x3f800000 ) ) ); } ), "detail level cap ignored" );
		GT_ASSERT( Rejects( [ & ] { CvrGeometryReader::Read( good ); } ), "single-level profile silently broadened" );
		GT_OK();
	} );
	task->AddTest( "CVR Rover material preserves reserved palette and index-offset ownership", GT() {
		auto palette = Pcx( 0, 2, 2 ); auto faction = Pcx( 245 );
		std::array< uint8_t, 6144 > source{};
		for ( size_t i = 0; i < source.size(); ++i ) { source[ i ] = static_cast< uint8_t >( i ); }
		const auto result = CvrActorMaterial::Read( palette, faction, source );
		palette.clear(); faction.clear(); source.fill( 123 );
		GT_ASSERT( result.tint_index == 255 && result.colors[ 255 ] == CvrPalette::Color( { 255, 255, 255 } ), "sample offset did not wrap in byte domain" );
		GT_ASSERT( result.colors[ 8 ] == CvrPalette::Color( { 8, 0, 0 } ) && result.colors[ 9 ] == CvrPalette::Color( { 9, 0, 0 } ) &&
			result.colors[ 246 ] == CvrPalette::Color( { 245, 0, 0 } ) && result.colors[ 247 ] == CvrPalette::Color( { 246, 0, 0 } ), "reserved original colors replaced by PCX/system approximations" );
		GT_ASSERT( result.colors[ 10 ] == CvrPalette::Color( { 0, 0, 0 } ) && result.colors[ 245 ] == CvrPalette::Color( { 235, 235, 235 } ), "palette range is not first236 at10" );
		GT_ASSERT( result.shades[ 10 ] == 0 && result.shades[ 11 ] == 11 && result.shades[ 8 ] == 8 && result.shades[ 9 ] == 9 && result.shades[ 255 ] == 255,
			"nearest color tie, reserved search, channel truncation or white endpoint differs" );
		for ( size_t i = 256; i < result.shades.size(); ++i ) { GT_ASSERT( result.shades[ i ] == result.shades[ i % 256 ], "shade order changed" ); }
		GT_OK();
	} );
	task->AddTest( "CVR Rover material ignores faction palette and owns padded PCX samples", GT() {
		const auto palette = Pcx( 0, 2, 2 ); auto faction = Pcx( 246, 437, 746 );
		std::fill( faction.end() - 768, faction.end(), static_cast< char >( 255 ) );
		std::array< uint8_t, 6144 > source{}; source.fill( 255 );
		const auto result = CvrActorMaterial::Read( palette, faction, source );
		GT_ASSERT( result.tint_index == 0 && std::all_of( result.shades.begin(), result.shades.end(), []( const uint8_t v ) { return v == 0; } ), "faction palette or scanline padding changed black tint" );
		GT_ASSERT( result.colors[ 11 ] == CvrPalette::Color( { 1, 1, 1 } ), "faction palette replaced game colors" );
		const auto distinct = CvrActorMaterial::Read( palette, Pcx( 246, 437, 746, true ), source );
		GT_ASSERT( distinct.tint_index == 255 && std::all_of( distinct.shades.begin(), distinct.shades.end(), []( const uint8_t v ) { return v == 255; } ), "sample row, column or scanline padding changed the distinct target" );
		GT_OK();
	} );
	task->AddTest( "CVR Rover PCX rejects malformed framing and unsupported domains", GT() {
		const auto palette = Pcx( 0, 2, 2 ), faction = Pcx( 245 ); std::array< uint8_t, 6144 > shades{};
		for ( size_t length : { size_t( 0 ), size_t( 127 ), size_t( 128 ), palette.size() - 1 } ) {
			GT_ASSERT( Rejects( [ & ] { CvrActorMaterial::Read( palette.substr( 0, length ), faction, shades ); } ), "truncated PCX accepted" );
		}
		for ( int scenario = 0; scenario < 13; ++scenario ) {
			auto invalid = palette;
			switch ( scenario ) {
				case 0: invalid[ 0 ] = 0; break;
				case 1: invalid[ 1 ] = 3; break;
				case 2: invalid[ 2 ] = 0; break;
				case 3: invalid[ 3 ] = 4; break;
				case 4: invalid[ 65 ] = 3; break;
				case 5: invalid[ 4 ] = 1; break;
				case 6: invalid[ 66 ] = 1; break;
				case 7: invalid[ 129 ] = '\0'; invalid[ 128 ] = static_cast< char >( 0xc0 ); break;
				case 8: invalid[ 128 ] = static_cast< char >( 0xc3 ); break;
				case 9: invalid[ invalid.size() - 769 ] = 0; break;
				case 10: invalid.insert( invalid.size() - 769, 1, 'x' ); break;
				case 11: invalid[ 9 ] = 8; break;
				case 12: invalid[ 11 ] = 8; break;
			}
			GT_ASSERT( Rejects( [ & ] { CvrActorMaterial::Read( invalid, faction, shades ); } ), "malformed PCX accepted: " + std::to_string( scenario ) );
		}
		GT_ASSERT( Rejects( [ & ] { CvrActorMaterial::Read( palette, palette, shades ); } ), "missing faction sample accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR Rover fixed composition owns all decoded assets after reader destruction", GT() {
		std::vector< std::string > reads;
		const auto renderer = CvrSpartanRoverRenderer::Load( [ & ]( const std::string& name ) {
			reads.push_back( name );
			return MakeSpartanRoverTestAsset( name );
		} );
		GT_ASSERT( reads.size() == 13 && reads.front() == "system.txt" && reads[ 11 ] == "PALETTE.PCX" && reads[ 12 ] == "spartans.pcx", "unexpected asset reads" );
		for ( size_t i = 0; i < 10; ++i ) { GT_ASSERT( reads[ i + 1 ] == CvrSpartanRoverRenderer::PartNames()[ i ], "composition order differs" ); }
		reads.clear();
		const auto first = renderer.Render( VIEW ); const auto copy = renderer; const auto second = copy.Render( VIEW );
		GT_ASSERT( first.draw_count == 10 && first.processed_records == 10 && first.written_samples > 0 && first.indexed.color == second.indexed.color && first.rgba == second.rgba, "owned composition lost records or borrows reader storage" );
		const auto half = renderer.Render( { 0, 0, 1, 2, { 128, 128 } } ), quarter = renderer.Render( { 0, 0, 1, 4, { 128, 128 } } );
		GT_ASSERT( half.draw_count == 10 && half.processed_records == 11 && quarter.processed_records == 12, "actual draw failed to select VLIGHTS from current view scale" );
		for ( size_t i = 0; i < first.indexed.color.size(); ++i ) { GT_ASSERT( first.rgba[ i * 4 + 3 ] == ( first.indexed.color[ i ] == 9 ? 0 : 255 ), "host alpha does not use key9" ); }
		GT_OK();
	} );
	task->AddTest( "CVR Rover view omits Infantry scale and derives admitted system lighting", GT() {
		const auto infantry = scout::MakeView( VIEW ), rover = scout::MakeView( VIEW, scout::IdleProfile::SpartanSpeeder );
		const std::array< int32_t, 2 > expected_anchor{ 178, 178 };
		GT_ASSERT( rover.scales[ 0 ] == 0x3d8f5c29 && infantry.scales[ 0 ] != rover.scales[ 0 ] && rover.target_anchor == expected_anchor, "Speeder scale or common anchor differs" );
		GT_ASSERT( rover.light == scout::Vector( { 0x3eb8941e, 0x3e761ad3, 0x3f66b926 } ), "system map light normalization differs" );
		for ( const auto source : { "", "#VEH_LIGHT_DIR_MAP\n6,4,14", "#VEH_LIGHT_DIR_MAP\n6,4,15\n6,4,15", "#VEH_LIGHT_DIR_MAP\n6,4,15\n#VEH_LIGHT_DIR_MAP\n6,4,15", "#VEH_LIGHT_DIR_MAP\n6,4,15junk", "#VEH_LIGHT_DIR_MAP\n6,4,15," } ) {
			GT_ASSERT( Rejects( [ & ] { CvrSpartanRoverRenderer::Load( [ & ]( const std::string& ) { return std::string( source ); } ); } ), "unsupported map light source accepted" );
		}
		GT_OK();
	} );
}

}
