#include "SmacWorkerKnowledge.h"

#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace game::backend::map {
namespace {
using Knowledge = SmacWorkerKnowledge;
constexpr size_t HEADER_BYTES = 52, TILE_BYTES = 2, UNIT_BYTES = 10, BASE_BYTES = 18;
void Require( bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}
size_t TileCount( int32_t width, int32_t height ) {
	Require( width >= 2 && width <= 512 && !( width & 1 ) && height > 0 && height <= 256,
		"Worker knowledge dimensions exceed the bounded parity grid" );
	return static_cast< size_t >( width ) * static_cast< size_t >( height ) / 2;
}
void AddSize( size_t& size, size_t count, size_t stride, size_t cap ) {
	Require( size <= cap && count <= ( cap - size ) / stride, "Worker knowledge exceeds aggregate wire budget" );
	size += count * stride;
}
size_t WireSize( size_t tiles, size_t units ) {
	size_t size = HEADER_BYTES + 1 + 9 + 4; // memory, Pact and Base-memory count
	AddSize( size, tiles, TILE_BYTES, Knowledge::MAX_WIRE_BYTES );
	AddSize( size, units, UNIT_BYTES, Knowledge::MAX_WIRE_BYTES );
	return size;
}
size_t MemorySize( const Knowledge::Data& data ) {
	size_t size = WireSize( data.tiles.size(), data.units.size() );
	for ( const auto& row : data.remembered ) {
		if ( row.empty() ) { continue; }
		Require( row.size() == data.tiles.size(), "Remembered improvement extent differs" );
		AddSize( size, row.size(), 1, Knowledge::MAX_WIRE_BYTES );
		for ( const auto& value : row ) {
			if ( !value ) { continue; }
			Require( !( value->features & ~uint16_t(0x7fff) ) && !( value->terraforming & ~uint16_t(0x1fff) ), "Unknown remembered improvement fields" );
			AddSize( size, 1, 5, Knowledge::MAX_WIRE_BYTES );
		}
	}
	AddSize( size, data.bases.size(), BASE_BYTES, Knowledge::MAX_WIRE_BYTES );
	return size;
}
void ValidateId( uint64_t id ) {
	Require( id > 0 && id <= static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ), "Worker knowledge Unit ID is out of range" );
}
void Validate( const Knowledge::Data& data ) {
	Require( data.tiles.size() == TileCount( data.width, data.height ), "Worker knowledge tile count differs from dimensions" );
	Require( data.ordinary_local_mode, "Worker knowledge requires ordinary local mode" );
	(void)MemorySize( data );
	bool playable = false;
	for ( size_t i = 1; i < 8; ++i ) { playable |= data.observers[i].has_value(); }
	Require( playable, "Worker knowledge requires a playable observer row" );
	for ( const auto& row : data.observers ) {
		Require( !row || !row->relations[0].foreign_unit_exemption, "Native identity cannot have a worker diplomacy exemption" );
	}
	for ( const auto& tile : data.tiles ) {
		Require( !tile.claimant || ( *tile.claimant >= 0 && *tile.claimant <= 7 ), "Worker knowledge has an invalid raw claimant" );
	}
	std::set< uint64_t > base_ids;
	for ( const auto& base : data.bases ) {
		ValidateId( base.id );
		Require( base_ids.insert( base.id ).second, "Worker knowledge has duplicate Base IDs" );
		for ( const auto& population : base.population ) {
			Require( !population || *population <= 127, "Remembered Base population exceeds 127" );
		}
	}
	std::set< uint64_t > ids;
	for ( const auto& unit : data.units ) {
		ValidateId( unit.id );
		Require( ids.insert( unit.id ).second, "Worker knowledge has duplicate Unit IDs" );
	}
}
uint8_t Byte( const std::string& wire, size_t offset ) { return static_cast< uint8_t >( wire[offset] ); }
uint64_t Read( const std::string& wire, size_t offset, unsigned bytes ) {
	uint64_t value = 0;
	for ( unsigned i = 0; i < bytes; ++i ) { value |= uint64_t( Byte( wire, offset+i ) ) << ( i * 8 ); }
	return value;
}
void Write( std::string& wire, size_t offset, uint64_t value, unsigned bytes ) {
	for ( unsigned i = 0; i < bytes; ++i ) { wire[offset+i] = static_cast< char >( ( value >> ( i * 8 ) ) & 255 ); }
}
}

bool SmacWorkerKnowledge::Improvements::operator==( const Improvements& o ) const noexcept {
	return features == o.features && terraforming == o.terraforming && base_present == o.base_present && unit_present == o.unit_present;
}
bool SmacWorkerKnowledge::Tile::operator==( const Tile& o ) const noexcept { return explored_by == o.explored_by && claimant == o.claimant; }
bool SmacWorkerKnowledge::Observer::operator==( const Observer& o ) const noexcept { return foreign_disclosure == o.foreign_disclosure && relations == o.relations && pact_with == o.pact_with; }
bool SmacWorkerKnowledge::UnitObservation::operator==( const UnitObservation& o ) const noexcept { return id == o.id && visible_to == o.visible_to && active_convoy == o.active_convoy; }
bool SmacWorkerKnowledge::BaseObservation::operator==( const BaseObservation& o ) const noexcept { return id == o.id && known_by == o.known_by && population == o.population; }
bool SmacWorkerKnowledge::Data::operator==( const Data& o ) const noexcept {
	return width == o.width && height == o.height && horizontal_wrap == o.horizontal_wrap && ordinary_local_mode == o.ordinary_local_mode
		&& all_map_revealed == o.all_map_revealed && observers == o.observers && tiles == o.tiles && units == o.units && remembered == o.remembered && bases == o.bases;
}
SmacWorkerKnowledge::SmacWorkerKnowledge( Data data ) : m_data( std::move( data ) ) {}
SmacWorkerKnowledge SmacWorkerKnowledge::Create( const Data& data ) { Validate( data ); return SmacWorkerKnowledge( data ); }
SmacWorkerKnowledge::Data SmacWorkerKnowledge::CopyData() const { return m_data; }
bool SmacWorkerKnowledge::operator==( const SmacWorkerKnowledge& o ) const noexcept { return m_data == o.m_data; }

SmacWorkerMapFrame SmacWorkerKnowledge::Project( int32_t observer, const std::vector< bool >& sea,
	const std::vector< SmacWorkerMapFrame::Base >& bases, const std::vector< LiveUnit >& units ) const {
	Require( observer >= 1 && observer <= 7 && m_data.observers[observer].has_value(), "Worker knowledge requested observer is absent" );
	Require( sea.size() == m_data.tiles.size(), "Worker knowledge terrain extent differs" );
	Require( units.size() == m_data.units.size(), "Worker knowledge live Unit count differs" );
	// Bound the larger consumer frame before constructing any of its vectors.
	size_t size = 56;
	AddSize( size, sea.size(), 3, SmacWorkerMapFrame::MAX_WIRE_BYTES );
	AddSize( size, bases.size(), 13, SmacWorkerMapFrame::MAX_WIRE_BYTES );
	AddSize( size, units.size(), 11, SmacWorkerMapFrame::MAX_WIRE_BYTES );
	std::map< uint64_t, const LiveUnit* > live;
	for ( const auto& unit : units ) {
		ValidateId( unit.id );
		Require( live.emplace( unit.id, &unit ).second, "Worker knowledge has duplicate live Unit IDs" );
	}
	const auto& row = *m_data.observers[observer];
	SmacWorkerMapFrame::Data data{m_data.width,m_data.height,observer,m_data.horizontal_wrap,m_data.ordinary_local_mode,
		row.foreign_disclosure,m_data.all_map_revealed,row.relations,{},bases,{}};
	data.tiles.reserve( m_data.tiles.size() ); data.units.reserve( m_data.units.size() );
	for ( size_t i = 0; i < sea.size(); ++i ) { data.tiles.push_back( {sea[i],m_data.tiles[i].explored_by,m_data.tiles[i].claimant} ); }
	for ( const auto& observation : m_data.units ) {
		const auto found = live.find( observation.id );
		Require( found != live.end(), "Worker knowledge observation has no live Unit" );
		const auto& unit = *found->second;
		data.units.push_back( {unit.owner,unit.coordinate,observation.visible_to,observation.active_convoy} );
	}
	// Unique equal-sized lists and a successful match for every observation also
	// exclude extra live IDs. No borrowed pointers leave this synchronous join.
	return SmacWorkerMapFrame::Create( data );
}

std::string SmacWorkerKnowledge::Serialize() const {
	std::string wire( "SWKN" ); wire.resize( MemorySize( m_data ) );
	Write( wire, 4, 4, 4 ); Write( wire, 8, m_data.width, 4 ); Write( wire, 12, m_data.height, 4 );
	wire[16] = m_data.horizontal_wrap; wire[17] = m_data.ordinary_local_mode; wire[18] = static_cast< char >( m_data.all_map_revealed );
	uint8_t presence = 0;
	for ( size_t i = 0; i < 8; ++i ) {
		if ( !m_data.observers[i] ) { continue; }
		presence |= uint8_t( 1u << i );
		const auto& row = *m_data.observers[i]; uint8_t exemption = 0, recognition = 0;
		for ( size_t j = 0; j < 8; ++j ) {
			if ( row.relations[j].foreign_unit_exemption ) { exemption |= uint8_t( 1u << j ); }
			if ( row.relations[j].territory_recognition ) { recognition |= uint8_t( 1u << j ); }
		}
		wire[28+i*3] = row.foreign_disclosure; wire[29+i*3] = static_cast< char >( exemption ); wire[30+i*3] = static_cast< char >( recognition );
	}
	wire[19] = static_cast< char >( presence );
	Write( wire, 20, m_data.tiles.size(), 4 ); Write( wire, 24, m_data.units.size(), 4 );
	size_t offset = HEADER_BYTES;
	for ( const auto& tile : m_data.tiles ) { wire[offset++] = static_cast< char >( tile.explored_by ); wire[offset++] = static_cast< char >( tile.claimant.value_or(255) ); }
	for ( const auto& unit : m_data.units ) {
		Write( wire, offset, unit.id, 8 ); offset += 8; wire[offset++] = static_cast< char >( unit.visible_to ); wire[offset++] = unit.active_convoy;
	}
	uint8_t memory_rows = 0;
	for ( size_t i = 0; i < 8; ++i ) { if ( !m_data.remembered[i].empty() ) { memory_rows |= uint8_t(1u << i); } }
	wire[offset++] = static_cast<char>(memory_rows);
	for ( const auto& row : m_data.remembered ) { for ( const auto& value : row ) {
		wire[offset++] = value.has_value();
		if ( value ) {
			Write(wire,offset,value->features,2); Write(wire,offset+2,value->terraforming,2);
			wire[offset+4] = ( value->base_present ? 1 : 0 ) | ( value->unit_present ? 2 : 0 ); offset += 5;
		}
	} }
	uint8_t pact_rows = 0;
	for ( size_t i = 0; i < 8; ++i ) {
		if ( m_data.observers[i] && m_data.observers[i]->pact_with ) {
			pact_rows |= uint8_t(1u<<i); wire[offset+1+i] = static_cast<char>(*m_data.observers[i]->pact_with);
		}
	}
	wire[offset] = static_cast<char>(pact_rows); offset += 9;
	Write(wire,offset,m_data.bases.size(),4); offset += 4;
	for ( const auto& base : m_data.bases ) {
		Write(wire,offset,base.id,8); wire[offset+8]=static_cast<char>(base.known_by);
		uint8_t present=0;
		for ( size_t i=0;i<8;++i ) { if(base.population[i]) {
			present |= uint8_t(1u<<i); wire[offset+10+i]=static_cast<char>(*base.population[i]);
		} }
		wire[offset+9]=static_cast<char>(present); offset += BASE_BYTES;
	}
	return wire;
}

SmacWorkerKnowledge SmacWorkerKnowledge::Deserialize( const std::string& wire ) {
	Require( wire.size() >= HEADER_BYTES && wire.size() <= MAX_WIRE_BYTES, "Worker knowledge wire length exceeds budget" );
	Require( wire.compare( 0, 4, "SWKN" ) == 0 && Read( wire, 4, 4 ) == 4, "Unsupported worker knowledge version" );
	const auto width = Read( wire, 8, 4 ), height = Read( wire, 12, 4 );
	Require( width <= 512 && height <= 256, "Worker knowledge dimensions exceed budget" );
	const auto tiles = TileCount( static_cast< int32_t >( width ), static_cast< int32_t >( height ) );
	const auto units = static_cast< size_t >( Read( wire, 24, 4 ) );
	Require( Read( wire, 20, 4 ) == tiles && WireSize( tiles, units ) <= wire.size(), "Worker knowledge has inconsistent counts or trailing bytes" );
	Require( Byte(wire,16) <= 1 && Byte(wire,17) == 1 && ( Byte(wire,19) & 254 ), "Worker knowledge has invalid mode or absent playable observers" );
	// Scan the variable suffix before materializing any tile/unit/memory arrays.
	size_t memory_start = HEADER_BYTES + tiles*TILE_BYTES + units*UNIT_BYTES;
	size_t scan = memory_start; const auto memory_rows = Byte(wire,scan++);
	for ( size_t i = 0; i < 8; ++i ) { if ( memory_rows & (1u<<i) ) { for ( size_t j = 0; j < tiles; ++j ) {
		Require( scan < wire.size(), "Truncated remembered improvement presence" );
		const auto present = Byte(wire,scan++); Require( present <= 1, "Invalid remembered improvement presence" );
		if ( present ) {
			Require( wire.size()-scan >= 5, "Truncated remembered improvement fields" );
			Require( !(Read(wire,scan,2)&~uint64_t(0x7fff)) && !(Read(wire,scan+2,2)&~uint64_t(0x1fff)) && Byte(wire,scan+4)<=3, "Invalid remembered improvement fields" ); scan += 5;
		}
	} } }
	Require( wire.size()-scan >= 13, "Truncated directed Pact or Base memory suffix" );
	const auto pact_rows = Byte(wire,scan);
	Require( !(pact_rows & ~Byte(wire,19)), "Directed Pact row has no observer" );
	for ( size_t i=0; i<8; ++i ) {
		Require( (pact_rows & (1u<<i)) || Byte(wire,scan+1+i)==0, "Absent directed Pact row has nonzero fields" );
	}
	const auto base_start=scan+9;
	const auto base_count=static_cast<size_t>(Read(wire,base_start,4));
	Require( base_count <= (wire.size()-base_start-4)/BASE_BYTES && wire.size()-base_start-4 == base_count*BASE_BYTES,
		"Inconsistent Base memory count or trailing bytes" );
	for(size_t i=0;i<base_count;++i) {
		const auto at=base_start+4+i*BASE_BYTES; ValidateId(Read(wire,at,8));
		for(size_t observer=0;observer<8;++observer) {
			const auto population=Byte(wire,at+10+observer);
			Require( population<=127 && ((Byte(wire,at+9)&(1u<<observer)) || population==0), "Invalid remembered Base population" );
		}
	}
	Data data{static_cast<int32_t>(width),static_cast<int32_t>(height),bool(Byte(wire,16)),true,Byte(wire,18),{},{},{}};
	for ( size_t i = 0; i < 8; ++i ) {
		const auto disclosure = Byte(wire,28+i*3), exemption = Byte(wire,29+i*3), recognition = Byte(wire,30+i*3);
		if ( !( Byte(wire,19) & (1u<<i) ) ) {
			Require( disclosure == 0 && exemption == 0 && recognition == 0, "Absent worker observer has nonzero fields" ); continue;
		}
		Require( disclosure <= 1, "Worker observer has invalid disclosure flag" );
		Require( !( exemption & 1u ), "Native identity cannot have a worker diplomacy exemption" );
		Observer row{bool(disclosure),{}};
		for ( size_t j = 0; j < 8; ++j ) { row.relations[j] = {bool(exemption&(1u<<j)),bool(recognition&(1u<<j))}; }
		data.observers[i] = row;
		if ( pact_rows & (1u<<i) ) { data.observers[i]->pact_with = Byte(wire,scan+1+i); }
	}
	// All aggregate extents and fixed header fields precede record allocation.
	data.tiles.reserve( tiles ); data.units.reserve( units ); size_t offset = HEADER_BYTES;
	for ( size_t i = 0; i < tiles; ++i ) {
		const auto explored = Byte(wire,offset++), claimant = Byte(wire,offset++);
		Require( claimant <= 7 || claimant == 255, "Worker knowledge has invalid raw claimant" );
		data.tiles.push_back( {explored,claimant == 255 ? std::nullopt : std::optional<int32_t>(claimant)} );
	}
	for ( size_t i = 0; i < units; ++i ) {
		const auto id = Read(wire,offset,8); offset += 8;
		const auto visible = Byte(wire,offset++), convoy = Byte(wire,offset++);
		Require( convoy <= 1, "Worker knowledge has invalid convoy flag" );
		data.units.push_back( {id,visible,bool(convoy)} );
	}
	++offset; // the already validated remembered-row mask
	for ( size_t i = 0; i < 8; ++i ) { if ( memory_rows & (1u<<i) ) {
		auto& row = data.remembered[i]; row.reserve(tiles);
		for ( size_t j = 0; j < tiles; ++j ) {
			if ( !Byte(wire,offset++) ) { row.emplace_back(); continue; }
			const auto flags=Byte(wire,offset+4);
			row.emplace_back(Improvements{static_cast<uint16_t>(Read(wire,offset,2)),static_cast<uint16_t>(Read(wire,offset+2,2)),bool(flags&1),bool(flags&2)}); offset+=5;
		}
	} }
	data.bases.reserve(base_count);
	for(size_t i=0;i<base_count;++i) {
		const auto at=base_start+4+i*BASE_BYTES;
		BaseObservation base{Read(wire,at,8),Byte(wire,at+8),{}};
		for(size_t observer=0;observer<8;++observer) { if(Byte(wire,at+9)&(1u<<observer)) {base.population[observer]=Byte(wire,at+10+observer);} }
		data.bases.push_back(base);
	}
	Validate( data ); return SmacWorkerKnowledge( std::move( data ) );
}

}
