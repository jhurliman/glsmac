#include "SmacWorkerMapFrame.h"

#include "game/backend/base/SmacWorkerAvailability.h"
#include <set>
#include <stdexcept>
#include <utility>

namespace game::backend::map {
namespace {
using Frame = SmacWorkerMapFrame;
constexpr size_t HEADER_BYTES = 56, TILE_BYTES = 3, BASE_BYTES = 13, UNIT_BYTES = 11;
void Require( bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}
size_t TileCount( int32_t width, int32_t height ) {
	Require( width >= 2 && width <= 512 && !( width & 1 ) && height > 0 && height <= 256,
		"Worker frame dimensions exceed the bounded parity grid" );
	return static_cast< size_t >( width ) * static_cast< size_t >( height ) / 2;
}
size_t Index( const Frame::Data& data, const Frame::Coordinate& coordinate ) {
	const auto x = coordinate[0], y = coordinate[1];
	Require( x >= 0 && x < data.width && y >= 0 && y < data.height && ( x & 1 ) == ( y & 1 ),
		"Worker frame record has an invalid parity coordinate" );
	return static_cast< size_t >( y ) * static_cast< size_t >( data.width / 2 ) + static_cast< size_t >( x / 2 );
}
void Owner( int32_t owner, bool native ) {
	Require( owner >= ( native ? 0 : 1 ) && owner <= 7, "Worker frame has an invalid owner" );
}
size_t WireSize( size_t tiles, size_t bases, size_t units ) {
	size_t size = HEADER_BYTES;
	const auto add = [ & ]( size_t count, size_t stride ) {
		Require( count <= ( Frame::MAX_WIRE_BYTES - size ) / stride, "Worker frame exceeds aggregate wire budget" );
		size += count * stride;
	};
	add( tiles, TILE_BYTES ); add( bases, BASE_BYTES ); add( units, UNIT_BYTES );
	return size;
}
void Validate( const Frame::Data& data ) {
	Require( data.tiles.size() == TileCount( data.width, data.height ), "Worker frame tile count differs from dimensions" );
	Owner( data.observer, false );
	Require( data.ordinary_local_mode, "Worker frame requires the ordinary local worker query" );
	Require( !data.observer_relations[0].foreign_unit_exemption, "Native identity cannot have a worker diplomacy exemption" );
	(void)WireSize( data.tiles.size(), data.bases.size(), data.units.size() );
	for ( const auto& tile : data.tiles ) { if ( tile.claimant ) { Owner( *tile.claimant, true ); } }
	std::set< size_t > centers;
	for ( const auto& base : data.bases ) {
		Owner( base.owner, false );
		Require( !( base.selected_radius & ~Frame::RADIUS_MASK ), "Worker frame cannot truncate full selected32 authority" );
		Require( centers.insert( Index( data, base.coordinate ) ).second, "Worker frame has duplicate Base centers" );
	}
	for ( const auto& unit : data.units ) { Owner( unit.owner, true ); (void)Index( data, unit.coordinate ); }
}
uint32_t ReadWord( const std::string& wire, size_t offset ) {
	uint32_t value = 0;
	for ( unsigned i = 0; i < 4; ++i ) { value |= uint32_t( static_cast< unsigned char >( wire[offset+i] ) ) << ( i * 8 ); }
	return value;
}
void WriteWord( std::string& wire, size_t offset, uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { wire[offset+i] = static_cast< char >( ( value >> ( i * 8 ) ) & 255 ); }
}
uint8_t Byte( const std::string& wire, size_t offset ) { return static_cast< uint8_t >( wire[offset] ); }
Frame::Coordinate ReadCoordinate( const std::string& wire, size_t offset, int32_t width, int32_t height ) {
	const auto x = ReadWord( wire, offset ), y = ReadWord( wire, offset + 4 );
	Require( x < static_cast< uint32_t >( width ) && y < static_cast< uint32_t >( height ), "Worker frame coordinate is out of bounds" );
	return { static_cast< int32_t >( x ), static_cast< int32_t >( y ) };
}
}

bool SmacWorkerMapFrame::Tile::operator==( const Tile& o ) const noexcept { return sea == o.sea && explored_by == o.explored_by && claimant == o.claimant; }
bool SmacWorkerMapFrame::Relation::operator==( const Relation& o ) const noexcept { return foreign_unit_exemption == o.foreign_unit_exemption && territory_recognition == o.territory_recognition; }
bool SmacWorkerMapFrame::Base::operator==( const Base& o ) const noexcept { return owner == o.owner && coordinate == o.coordinate && selected_radius == o.selected_radius; }
bool SmacWorkerMapFrame::Unit::operator==( const Unit& o ) const noexcept { return owner == o.owner && coordinate == o.coordinate && visible_to == o.visible_to && active_convoy == o.active_convoy; }
bool SmacWorkerMapFrame::Data::operator==( const Data& o ) const noexcept {
	return width == o.width && height == o.height && observer == o.observer && horizontal_wrap == o.horizontal_wrap
		&& ordinary_local_mode == o.ordinary_local_mode && foreign_disclosure == o.foreign_disclosure && all_map_revealed == o.all_map_revealed
		&& observer_relations == o.observer_relations && tiles == o.tiles && bases == o.bases && units == o.units;
}
bool SmacWorkerMapFrame::ObservedSquare::operator==( const ObservedSquare& o ) const noexcept {
	return coordinate == o.coordinate && explored == o.explored && claimant == o.claimant && recognized_owner == o.recognized_owner;
}
bool SmacWorkerMapFrame::Evaluation::operator==( const Evaluation& o ) const noexcept { return selected_radius == o.selected_radius && unavailable == o.unavailable && squares == o.squares; }
SmacWorkerMapFrame::SmacWorkerMapFrame( Data data ) : m_data( std::move( data ) ) {}
SmacWorkerMapFrame SmacWorkerMapFrame::Create( const Data& data ) { Validate( data ); return SmacWorkerMapFrame( data ); }
SmacWorkerMapFrame::Data SmacWorkerMapFrame::CopyData() const { return m_data; }
bool SmacWorkerMapFrame::operator==( const SmacWorkerMapFrame& o ) const noexcept { return m_data == o.m_data; }

SmacWorkerMapFrame::Evaluation SmacWorkerMapFrame::Evaluate( size_t base_index ) const {
	Require( base_index < m_data.bases.size(), "Worker frame Base index is unavailable" );
	const auto& current = m_data.bases[base_index];
	Require( current.owner == m_data.observer, "Worker frame observer does not own the evaluating Base" );
	base::SmacWorkerAvailabilityContext context{ base_index, current.owner,
		{current.coordinate,m_data.width,m_data.height,m_data.horizontal_wrap}, bool( m_data.all_map_revealed & ( 1u << m_data.observer ) ), {}, {} };
	// The builder ORs Pact/Treaty. This frame supplies their resolved exemption,
	// without claiming either underlying diplomacy producer or conflating recognition.
	for ( size_t i = 0; i < 8; ++i ) { context.pact[i] = m_data.observer_relations[i].foreign_unit_exemption; }
	std::vector< base::SmacWorkerOtherBase > bases;
	bases.reserve( m_data.bases.size() );
	for ( size_t i = 0; i < m_data.bases.size(); ++i ) {
		const auto& item = m_data.bases[i]; bases.push_back( {i,item.owner,item.coordinate,item.selected_radius} );
	}
	base::SmacWorkerAvailability::Squares squares;
	Evaluation result{current.selected_radius,0,{}};
	for ( int32_t i = 0; i <= 20; ++i ) {
		const auto coordinate = base::SmacWorkedSquareCollection::Resolve( context.map, i );
		if ( !coordinate ) { continue; }
		const auto& tile = m_data.tiles[Index( m_data, *coordinate )];
		std::optional< int32_t > recognized;
		if ( tile.claimant && *tile.claimant > 0 && ( *tile.claimant == m_data.observer || m_data.foreign_disclosure
			|| m_data.observer_relations[*tile.claimant].territory_recognition ) ) { recognized = tile.claimant; }
		const bool explored = ( tile.explored_by & ( 1u << m_data.observer ) ) != 0;
		base::SmacWorkerAvailabilitySquare square{explored,false,tile.sea,recognized,{}};
		for ( const auto& item : m_data.bases ) { square.base_marker |= item.coordinate == *coordinate; }
		for ( const auto& unit : m_data.units ) {
			if ( unit.coordinate == *coordinate ) { square.units.push_back( {unit.owner,bool(unit.visible_to & (1u << m_data.observer)),unit.active_convoy} ); }
		}
		squares[i] = std::move( square ); result.squares[i] = ObservedSquare{*coordinate,explored,tile.claimant,recognized};
	}
	result.unavailable = base::SmacWorkerAvailability::Build( context, squares, bases );
	return result;
}

std::string SmacWorkerMapFrame::Serialize() const {
	std::string wire( "SWMF" ); wire.resize( WireSize( m_data.tiles.size(), m_data.bases.size(), m_data.units.size() ) );
	WriteWord( wire, 4, 1 ); WriteWord( wire, 8, m_data.width ); WriteWord( wire, 12, m_data.height );
	WriteWord( wire, 16, m_data.horizontal_wrap ); WriteWord( wire, 20, m_data.observer ); WriteWord( wire, 24, m_data.ordinary_local_mode );
	WriteWord( wire, 28, m_data.foreign_disclosure ); WriteWord( wire, 32, m_data.all_map_revealed );
	uint32_t exemption = 0, recognition = 0;
	for ( size_t i = 0; i < 8; ++i ) {
		if ( m_data.observer_relations[i].foreign_unit_exemption ) { exemption |= 1u << i; }
		if ( m_data.observer_relations[i].territory_recognition ) { recognition |= 1u << i; }
	}
	WriteWord( wire, 36, exemption ); WriteWord( wire, 40, recognition );
	WriteWord( wire, 44, static_cast< uint32_t >( m_data.tiles.size() ) );
	WriteWord( wire, 48, static_cast< uint32_t >( m_data.bases.size() ) ); WriteWord( wire, 52, static_cast< uint32_t >( m_data.units.size() ) );
	size_t offset = HEADER_BYTES;
	for ( const auto& tile : m_data.tiles ) {
		wire[offset++] = tile.sea; wire[offset++] = static_cast< char >( tile.explored_by ); wire[offset++] = static_cast< char >( tile.claimant.value_or( 255 ) );
	}
	for ( const auto& item : m_data.bases ) {
		wire[offset++] = static_cast< char >( item.owner ); WriteWord( wire, offset, item.coordinate[0] ); WriteWord( wire, offset+4, item.coordinate[1] );
		WriteWord( wire, offset+8, item.selected_radius ); offset += 12;
	}
	for ( const auto& unit : m_data.units ) {
		wire[offset++] = static_cast< char >( unit.owner ); WriteWord( wire, offset, unit.coordinate[0] ); WriteWord( wire, offset+4, unit.coordinate[1] ); offset += 8;
		wire[offset++] = static_cast< char >( unit.visible_to ); wire[offset++] = unit.active_convoy;
	}
	return wire;
}

SmacWorkerMapFrame SmacWorkerMapFrame::Deserialize( const std::string& wire ) {
	Require( wire.size() >= HEADER_BYTES && wire.size() <= MAX_WIRE_BYTES, "Worker frame wire length exceeds budget" );
	Require( wire.compare( 0, 4, "SWMF" ) == 0 && ReadWord( wire, 4 ) == 1, "Unsupported worker frame version" );
	const auto width = ReadWord( wire, 8 ), height = ReadWord( wire, 12 );
	Require( width <= 512 && height <= 256, "Worker frame dimensions exceed budget" );
	const auto tile_count = TileCount( static_cast< int32_t >( width ), static_cast< int32_t >( height ) );
	const auto base_count = ReadWord( wire, 48 ), unit_count = ReadWord( wire, 52 );
	Require( ReadWord( wire, 44 ) == tile_count && WireSize( tile_count, base_count, unit_count ) == wire.size(), "Worker frame has inconsistent counts or trailing bytes" );
	Require( ReadWord( wire, 16 ) <= 1 && ReadWord( wire, 20 ) >= 1 && ReadWord( wire, 20 ) <= 7
		&& ReadWord( wire, 24 ) == 1 && ReadWord( wire, 28 ) <= 1 && ReadWord( wire, 32 ) <= 255
		&& ReadWord( wire, 36 ) <= 255 && ReadWord( wire, 40 ) <= 255, "Worker frame has unsupported mode or flag bits" );
	Require( !( ReadWord( wire, 36 ) & 1u ), "Native identity cannot have a worker diplomacy exemption" );
	// Exact bounded framing and header checked before any owned record allocation.
	Data data{static_cast<int32_t>(width),static_cast<int32_t>(height),static_cast<int32_t>(ReadWord(wire,20)),
		bool(ReadWord(wire,16)),true,bool(ReadWord(wire,28)),static_cast<uint8_t>(ReadWord(wire,32)),{},{},{},{}};
	for ( size_t i = 0; i < 8; ++i ) { data.observer_relations[i] = {bool(ReadWord(wire,36)&(1u<<i)),bool(ReadWord(wire,40)&(1u<<i))}; }
	data.tiles.reserve( tile_count ); data.bases.reserve( base_count ); data.units.reserve( unit_count );
	size_t offset = HEADER_BYTES;
	for ( size_t i = 0; i < tile_count; ++i ) {
		const auto sea = Byte(wire,offset++), explored = Byte(wire,offset++), claimant = Byte(wire,offset++);
		Require( sea <= 1 && ( claimant <= 7 || claimant == 255 ), "Worker frame has invalid tile flags or claim" );
		data.tiles.push_back( {bool(sea),explored,claimant == 255 ? std::nullopt : std::optional<int32_t>(claimant)} );
	}
	for ( size_t i = 0; i < base_count; ++i ) {
		const auto owner = Byte(wire,offset++); const auto coordinate = ReadCoordinate(wire,offset,data.width,data.height);
		data.bases.push_back( {owner,coordinate,ReadWord(wire,offset+8)} ); offset += 12;
	}
	for ( size_t i = 0; i < unit_count; ++i ) {
		const auto owner = Byte(wire,offset++); const auto coordinate = ReadCoordinate(wire,offset,data.width,data.height); offset += 8;
		const auto visible = Byte(wire,offset++), convoy = Byte(wire,offset++);
		Require( convoy <= 1, "Worker frame has invalid convoy flag" ); data.units.push_back( {owner,coordinate,visible,bool(convoy)} );
	}
	Validate( data ); return SmacWorkerMapFrame( std::move( data ) );
}

}
