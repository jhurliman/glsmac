#include "Faction.h"

#include "types/texture/Texture.h"
#include "gse/value/String.h"
#include "gse/value/Bool.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "resource/ResourceManager.h"
#include "util/String.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace game {
namespace backend {
namespace faction {

namespace {

constexpr char snapshot_marker[] = "GLSMAC_FACTION";
constexpr long long snapshot_version = 1;
constexpr auto known_flags = Faction::FF_NAVAL | Faction::FF_PROGENITOR;

void CheckFlags( const long long flags ) {
	if ( flags < 0 || ( flags & ~known_flags ) != 0 ) {
		throw std::runtime_error( "Unsupported faction flags" );
	}
}

void WriteSize( types::Buffer& buf, const size_t value ) {
	if ( value > static_cast< unsigned long long >( std::numeric_limits< long long >::max() ) ) {
		throw std::runtime_error( "Faction size exceeds snapshot integer range" );
	}
	buf.WriteInt( static_cast< long long >( value ) );
}

size_t ReadSize( types::Buffer& buf ) {
	const auto value = buf.ReadInt();
	if ( value < 0 || static_cast< unsigned long long >( value ) > std::numeric_limits< size_t >::max() ) {
		throw std::runtime_error( "Invalid faction size" );
	}
	return static_cast< size_t >( value );
}

void WriteNames( types::Buffer& buf, const std::vector< std::string >& names ) {
	WriteSize( buf, names.size() );
	for ( const auto& name : names ) { buf.WriteString( name ); }
}

std::vector< std::string > ReadNames( types::Buffer& buf ) {
	const auto count = ReadSize( buf );
	// Each framed string consumes bytes, even when empty. Use the remaining
	// input as a conservative count bound; never reserve an untrusted count.
	if ( count > buf.lenw - buf.lenr ) {
		throw std::runtime_error( "Faction name count exceeds remaining snapshot data" );
	}
	std::vector< std::string > names;
	for ( size_t i = 0 ; i < count ; ++i ) { names.push_back( buf.ReadString() ); }
	return names;
}

}

Faction::Faction() {
	//
}

Faction::Faction( const std::string& id, const std::string& name )
	: m_id( id )
	, m_name( name ) {
	//
}

const types::Buffer Faction::Serialize() const {
	CheckFlags( m_flags );
	types::Buffer buf;

	buf.WriteString( snapshot_marker );
	buf.WriteInt( snapshot_version );
	buf.WriteString( m_id );
	buf.WriteString( m_name );
	buf.WriteInt( m_flags );
	WriteNames( buf, m_base_names.land );
	WriteNames( buf, m_base_names.water );

	buf.WriteColor( m_colors.text );
	buf.WriteColor( m_colors.text_shadow );
	buf.WriteColor( m_colors.border );

	buf.WriteString( m_bases_render.file );
	WriteSize( buf, m_bases_render.grid_x );
	WriteSize( buf, m_bases_render.grid_y );
	WriteSize( buf, m_bases_render.cell_width );
	WriteSize( buf, m_bases_render.cell_height );
	WriteSize( buf, m_bases_render.cell_cx );
	WriteSize( buf, m_bases_render.cell_cy );
	WriteSize( buf, m_bases_render.cell_padding );
	buf.WriteFloat( m_bases_render.scale_x );
	buf.WriteFloat( m_bases_render.scale_y );

	return buf;
}

void Faction::Deserialize( types::Buffer buf ) {
	if ( buf.ReadString() != snapshot_marker || buf.ReadInt() != snapshot_version ) {
		throw std::runtime_error( "Unsupported faction snapshot format" );
	}

	Faction next;
	next.m_id = buf.ReadString();
	next.m_name = buf.ReadString();
	const auto flags = buf.ReadInt();
	CheckFlags( flags );
	next.m_flags = static_cast< faction_flag_t >( flags );
	next.m_base_names.land = ReadNames( buf );
	next.m_base_names.water = ReadNames( buf );

	buf.ReadColor( next.m_colors.text );
	buf.ReadColor( next.m_colors.text_shadow );
	buf.ReadColor( next.m_colors.border );

	next.m_bases_render.file = buf.ReadString();
	next.m_bases_render.grid_x = ReadSize( buf );
	next.m_bases_render.grid_y = ReadSize( buf );
	next.m_bases_render.cell_width = ReadSize( buf );
	next.m_bases_render.cell_height = ReadSize( buf );
	next.m_bases_render.cell_cx = ReadSize( buf );
	next.m_bases_render.cell_cy = ReadSize( buf );
	next.m_bases_render.cell_padding = ReadSize( buf );
	next.m_bases_render.scale_x = buf.ReadFloat();
	next.m_bases_render.scale_y = buf.ReadFloat();
	if ( buf.lenr != buf.lenw ) { throw std::runtime_error( "Unexpected trailing faction snapshot data" ); }

	// Publish only complete validated fields. Do not replace Wrappable state
	// (callbacks, linked script objects or dependents) with the temporary's.
	m_id.swap( next.m_id );
	m_name.swap( next.m_name );
	m_flags = next.m_flags;
	m_base_names.land.swap( next.m_base_names.land );
	m_base_names.water.swap( next.m_base_names.water );
	m_colors.text.Set( next.m_colors.text );
	m_colors.text_shadow.Set( next.m_colors.text_shadow );
	m_colors.border.Set( next.m_colors.border );
	std::swap( m_bases_render, next.m_bases_render );
}

WRAPIMPL_BEGIN( Faction )
	WRAPIMPL_PROPS
			{
				"id",
				VALUE( gse::value::String, , m_id )
			},
			{
				"name",
				VALUE( gse::value::String, , m_name )
			},
			{
				"text_color",
				m_colors.text.Wrap( GSE_CALL ),
			},
			{
				"is_naval",
				VALUE( gse::value::Bool, , m_flags & Faction::FF_NAVAL )
			},
			{
				"is_progenitor",
				VALUE( gse::value::Bool, , m_flags & Faction::FF_PROGENITOR )
			},
		};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Faction )

}
}
}
