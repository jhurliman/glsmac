#include "CvrGeometryReader.h"

#include "loader/cvr/CvrGeometryReader.h"
#include "loader/cvr/CvrSceneReader.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using namespace loader::cvr;
using Bytes = std::vector< uint8_t >;

void PutWord( Bytes& bytes, const size_t offset, const uint32_t value ) {
	for ( size_t i = 0 ; i < 4 ; ++i ) { bytes.at( offset + i ) = static_cast< uint8_t >( value >> ( i * 8 ) ); }
}

void AppendWord( Bytes& bytes, const uint32_t value ) {
	const auto offset = bytes.size(); bytes.resize( offset + 4 ); PutWord( bytes, offset, value );
}

template< size_t N >
void AppendVectors( Bytes& bytes, const std::array< CvrGeometryVector, N >& vectors ) {
	for ( const auto& vector : vectors ) {
		for ( const auto component : vector ) {
			const auto word = static_cast< uint16_t >( component );
			bytes.push_back( static_cast< uint8_t >( word ) );
			bytes.push_back( static_cast< uint8_t >( word >> 8 ) );
		}
	}
}

void FixSize( Bytes& bytes ) {
	PutWord( bytes, 4, static_cast< uint32_t >( bytes.size() ) );
	PutWord( bytes, 12, static_cast< uint32_t >( bytes.size() ) );
	PutWord( bytes, 16, static_cast< uint32_t >( bytes.size() - 1 ) );
}

CvrGeometry Fixture( const uint32_t flags ) {
	CvrGeometry result{ flags, 0, 0x4099999a, 0, 0,
		{ { { -32768, 32767, -1 }, { 0, 1, 256 }, { -256, 1234, -4321 } } },
		static_cast< uint32_t >( flags & 2 ? 4 : ( flags & 8 ? 4 : 3 ) ), {} };
	CvrGeometryGroup first{ std::nullopt, { { { 0x27, 0x80, 0xff } }, { { 0x27, 0x80, 0xff } }, { { 0xcf, 0xff, 0 } } } };
	if ( flags & 8 ) { first.records.push_back( { { 0xd5, 0x81, 0xfe } } ); }
	if ( flags & 2 ) {
		first.header = CvrGeometryGroupHeader{ { { { 32767, -32768, -123 }, { 456, -789, 0 }, { 1, 2, 3 }, { -4, -5, -6 } } },
			static_cast< uint32_t >( first.records.size() ), 0 };
	}
	result.groups.push_back( first );
	if ( flags & 2 ) {
		CvrGeometryGroup second{ CvrGeometryGroupHeader{ { { { 9, 8, 7 }, { 6, 5, 4 }, { 3, 2, 1 }, { 0, -1, -2 } } },
			static_cast< uint32_t >( flags & 8 ? 2 : 1 ), 0 }, { { { 0x07, 0xab, 0xcd } } } };
		if ( flags & 8 ) { second.records.push_back( { { 0xd0, 0x12, 0x34 } } ); }
		result.groups.push_back( second );
	}
	return result;
}

Bytes Encode( const CvrGeometry& source ) {
	Bytes bytes;
	for ( const auto value : { source.flags, uint32_t( 0 ), source.scalar_bits, uint32_t( 0 ), uint32_t( 0 ) } ) { AppendWord( bytes, value ); }
	AppendVectors( bytes, source.vectors );
	AppendWord( bytes, source.record_count );
	for ( const auto& group : source.groups ) {
		if ( group.header ) {
			AppendVectors( bytes, group.header->vectors );
			AppendWord( bytes, group.header->record_count ); AppendWord( bytes, group.header->flags );
		}
		for ( const auto& record : group.records ) { bytes.insert( bytes.end(), record.bytes.begin(), record.bytes.end() ); }
	}
	bytes.resize( bytes.size() + 5, 0 );
	FixSize( bytes );
	return bytes;
}

CvrGeometry Expected( const uint32_t flags ) {
	auto result = Fixture( flags );
	result.payload_size = static_cast< uint32_t >( Encode( result ).size() );
	result.end_offset = result.payload_size; result.end_minus_one = result.payload_size - 1;
	return result;
}

bool Equal( const CvrGeometry& a, const CvrGeometry& b ) {
	if ( a.flags != b.flags || a.payload_size != b.payload_size || a.scalar_bits != b.scalar_bits ||
		a.end_offset != b.end_offset || a.end_minus_one != b.end_minus_one || a.vectors != b.vectors ||
		a.record_count != b.record_count || a.groups.size() != b.groups.size() ) { return false; }
	for ( size_t i = 0 ; i < a.groups.size() ; ++i ) {
		const auto& x = a.groups[ i ]; const auto& y = b.groups[ i ];
		if ( x.header.has_value() != y.header.has_value() || x.records.size() != y.records.size() ) { return false; }
		if ( x.header && ( x.header->vectors != y.header->vectors || x.header->record_count != y.header->record_count || x.header->flags != y.header->flags ) ) { return false; }
		for ( size_t j = 0 ; j < x.records.size() ; ++j ) { if ( x.records[ j ].bytes != y.records[ j ].bytes ) { return false; } }
	}
	return true;
}

bool Rejected( const Bytes& bytes, const std::string& message = {} ) {
	try { (void)CvrGeometryReader::Read( bytes ); }
	catch ( const std::invalid_argument& error ) {
		const std::string text( error.what() );
		return text.find( "CVR geometry:" ) == 0 && ( message.empty() || text.find( message ) != std::string::npos );
	}
	return false;
}

}

void AddCvrGeometryReaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR geometry preserves all four profiles and distinct terminal count conventions", GT() {
		for ( const uint32_t flags : { 0, 2, 8, 10 } ) {
			const auto expected = Expected( flags );
			const auto parsed = CvrGeometryReader::Read( Encode( expected ) );
			GT_ASSERT( Equal( parsed, expected ), "header, signed vector, group order, repeated record or terminal bytes changed" );
			GT_ASSERT( parsed.groups[ 0 ].records[ 0 ].Code() == 4 && parsed.groups[ 0 ].records[ 2 ].Code() == 25, "record code bit extraction changed" );
			GT_ASSERT( parsed.record_count == ( flags == 0 ? 3 : 4 ), "simple/grouped header count convention changed" );
		}
		auto minimal = Fixture( 8 );
		minimal.groups[ 0 ].records = { { { 0xd7, 0x80, 0xff } } }; minimal.record_count = 1;
		GT_ASSERT( CvrGeometryReader::Read( Encode( minimal ) ).groups[ 0 ].records[ 0 ].bytes == minimal.groups[ 0 ].records[ 0 ].bytes,
			"terminal-only simple record was discarded or rejected" );
		minimal = Fixture( 10 );
		minimal.groups[ 0 ].records = { { { 0xd7, 0x80, 0xff } } }; minimal.groups[ 0 ].header->record_count = 1;
		minimal.record_count = 1;
		GT_ASSERT( CvrGeometryReader::Read( Encode( minimal ) ).groups.size() == 2, "terminal-only group changed aggregate handling" );
		minimal.groups[ 1 ].records = { { { 0xd0, 0, 0 } } }; minimal.groups[ 1 ].header->record_count = 1;
		GT_ASSERT( Rejected( Encode( minimal ) ), "zero aggregate accepted with a positive header count" );
		minimal.record_count = 0;
		GT_ASSERT( Rejected( Encode( minimal ) ), "zero overall count accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR geometry owns records after source scene destruction copy and move", GT() {
		const auto expected = Expected( 10 );
		CvrGeometry parsed;
		{
			CvrSceneObject source{ "synthetic", Encode( expected ), {} };
			const auto before = *source.voxel_data;
			parsed = CvrGeometryReader::Read( *source.voxel_data );
			GT_ASSERT( *source.voxel_data == before, "reader modified input" );
			source.voxel_data->assign( before.size(), 0xff );
		}
		GT_ASSERT( Equal( parsed, expected ), "result borrowed source storage" );
		auto copy = parsed;
		parsed.groups[ 0 ].records[ 0 ].bytes[ 1 ] ^= 0xff;
		parsed.groups[ 1 ].header->vectors[ 0 ][ 0 ] = -100;
		parsed.groups.clear();
		const auto moved = std::move( copy );
		GT_ASSERT( Equal( moved, expected ), "copy/move retained aliases or lost data" );
		GT_OK();
	} );
	task->AddTest( "CVR geometry preserves exact positive binary32 scalar bits and rejects other classes", GT() {
		for ( const uint32_t bits : { 0x00000001u, 0x007fffffu, 0x00800000u, 0x3f800001u, 0x4099999au, 0x7f7fffffu } ) {
			auto source = Fixture( 0 ); source.scalar_bits = bits;
			GT_ASSERT( CvrGeometryReader::Read( Encode( source ) ).scalar_bits == bits, "scalar normalized or finite positive bits rejected" );
		}
		for ( const uint32_t bits : { 0u, 0x80000000u, 0x80000001u, 0xbf800000u, 0xff7fffffu, 0x7f800000u, 0xff800000u, 0x7f800001u, 0x7fc12345u, 0xffc12345u } ) {
			auto source = Fixture( 0 ); source.scalar_bits = bits;
			GT_ASSERT( Rejected( Encode( source ), "scalar" ), "unsupported scalar class accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR geometry accepts each raw code and rejects missing early repeated or unknown terminals", GT() {
		for ( const uint32_t flags : { 0, 2, 8, 10 } ) {
			auto source = Fixture( flags );
			auto& records = source.groups[ 0 ].records;
			const size_t first_record = flags & 2 ? 74 : 42;
			for ( uint8_t code = 0 ; code < 26 ; ++code ) {
				records[ 0 ].bytes = { static_cast< uint8_t >( ( code << 3 ) | 7 ), 0xab, 0xef };
				const auto parsed = CvrGeometryReader::Read( Encode( source ) );
				GT_ASSERT( parsed.groups[ 0 ].records[ 0 ].Code() == code && parsed.groups[ 0 ].records[ 0 ].bytes == records[ 0 ].bytes, "code or uninterpreted attributes changed" );
			}
			for ( uint8_t code = 26 ; code < 32 ; ++code ) {
				auto bytes = Encode( source ); bytes[ first_record ] = static_cast< uint8_t >( code << 3 );
				GT_ASSERT( Rejected( bytes ), "early terminal or unknown code accepted" );
			}
			if ( flags & 8 ) {
				auto bytes = Encode( source ); bytes[ first_record + ( records.size() - 1 ) * 3 ] = 0;
				GT_ASSERT( Rejected( bytes ), "missing terminal accepted" );
			}
			else {
				auto bytes = Encode( source ); bytes[ first_record + ( records.size() - 1 ) * 3 ] = 0xd0;
				GT_ASSERT( Rejected( bytes ), "terminal accepted without bit 8" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR geometry rejects every truncated prefix and invalid size or end fields", GT() {
		for ( const uint32_t flags : { 0, 2, 8, 10 } ) {
			const auto valid = Encode( Fixture( flags ) );
			for ( size_t length = 0 ; length < valid.size() ; ++length ) {
				Bytes bytes( valid.begin(), valid.begin() + length );
				GT_ASSERT( Rejected( bytes ), "truncated input accepted" );
				if ( length >= 20 ) { FixSize( bytes ); GT_ASSERT( Rejected( bytes ), "truncation accepted after repairing size fields" ); }
			}
			for ( const size_t offset : { 4, 12, 16 } ) {
				for ( const uint32_t word : { 0u, 1u, static_cast< uint32_t >( valid.size() + 1 ), std::numeric_limits< uint32_t >::max() } ) {
					auto bytes = valid; PutWord( bytes, offset, word );
					GT_ASSERT( Rejected( bytes ), "invalid size/end field accepted" );
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR geometry rejects wrong aggregate and group counts before exposing records", GT() {
		for ( const uint32_t flags : { 0, 2, 8, 10 } ) {
			const auto valid = Encode( Fixture( flags ) );
			for ( const uint32_t count : { 0u, 1u, 2u, 5u, 6u, std::numeric_limits< uint32_t >::max() } ) {
				auto bytes = valid; PutWord( bytes, 38, count );
				GT_ASSERT( Rejected( bytes ), "wrong aggregate count accepted" );
			}
			if ( flags & 2 ) {
				const auto second_header = size_t( 74 ) + ( flags & 8 ? 4 : 3 ) * 3;
				for ( const size_t offset : { size_t( 66 ), second_header + 24 } ) {
					for ( const uint32_t count : { 0u, 100u, std::numeric_limits< uint32_t >::max() } ) {
						auto bytes = valid; PutWord( bytes, offset, count );
						GT_ASSERT( Rejected( bytes ), "invalid group count accepted" );
					}
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR geometry rejects unsupported flags including late groups and trailing bytes", GT() {
		for ( const uint32_t flags : { 1u, 3u, 4u, 9u, 11u, 16u, 0x100u, 0x80000000u, 0xffffffffu } ) {
			auto bytes = Encode( Fixture( 0 ) ); PutWord( bytes, 0, flags );
			GT_ASSERT( Rejected( bytes, "unsupported geometry flags" ), "unknown geometry profile accepted" );
		}
		for ( const uint32_t flags : { 2, 10 } ) {
			const auto valid = Encode( Fixture( flags ) );
			const auto second_header = size_t( 74 ) + ( flags & 8 ? 4 : 3 ) * 3;
			for ( const size_t offset : { size_t( 70 ), second_header + 28 } ) {
				for ( const uint32_t group_flags : { 1u, 2u, 0xffffffffu } ) {
					auto bytes = valid; PutWord( bytes, offset, group_flags );
					GT_ASSERT( Rejected( bytes, "unsupported group flags" ), "unknown first/late group profile accepted" );
				}
			}
		}
		for ( const uint32_t flags : { 0, 2, 8, 10 } ) {
			const auto valid = Encode( Fixture( flags ) );
			for ( size_t i = valid.size() - 5 ; i < valid.size() ; ++i ) {
				auto bytes = valid; bytes[ i ] = 1;
				GT_ASSERT( Rejected( bytes ), "nonzero trailer accepted" );
			}
			for ( const uint8_t extra : { 0, 1, 0xff } ) {
				auto bytes = valid; bytes.push_back( extra ); FixSize( bytes );
				GT_ASSERT( Rejected( bytes ), "extra byte accepted after repairing lengths" );
			}
		}
		GT_OK();
	} );
}

}
