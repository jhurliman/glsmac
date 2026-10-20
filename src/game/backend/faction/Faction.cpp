#include "Faction.h"

#include "types/texture/Texture.h"
#include "gse/value/String.h"
#include "gse/value/Bool.h"
#include "gse/value/Undefined.h"
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
constexpr long long snapshot_version = 3;
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
	if (m_display_identity) { m_display_identity->Validate(); }
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
	buf.WriteBool(m_display_identity.has_value());
	if (m_display_identity) {
		buf.WriteString(m_display_identity->faction_name);
		buf.WriteString(m_display_identity->leader_title);
		buf.WriteString(m_display_identity->leader_name);
        buf.WriteBool(m_display_identity->setup.has_value());
        if(m_display_identity->setup) {
            const auto& setup=*m_display_identity->setup;
            buf.WriteString(setup.noun);buf.WriteString(setup.adjective);buf.WriteString(setup.description);buf.WriteBool(setup.female);
        }
	}

	return buf;
}

void Faction::Deserialize( types::Buffer buf ) {
	if ( buf.ReadString() != snapshot_marker ) {
		throw std::runtime_error( "Unsupported faction snapshot format" );
	}
	const auto version = buf.ReadInt();
	if (version != 1 && version != 2 && version != snapshot_version) { throw std::runtime_error("Unsupported faction snapshot version"); }

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
	if (version >= 2 && buf.ReadBool()) {
		next.m_display_identity = DisplayIdentity{buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES), buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES), buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES)};
		if(version>=3 && buf.ReadBool()) {
            next.m_display_identity->setup=DisplayIdentity::SetupFields{
                buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES),buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES),
                buf.ReadString(DisplayIdentity::MAX_FIELD_BYTES),buf.ReadBool()};
        }
        next.m_display_identity->Validate();
	}
	if ( buf.lenr != buf.lenw ) { throw std::runtime_error( "Unexpected trailing faction snapshot data" ); }

	// Publish only complete validated fields. Do not replace Wrappable state
	// (callbacks, linked script objects or dependents) with the temporary's.
	m_id.swap( next.m_id );
	m_name.swap( next.m_name );
	m_display_identity.swap(next.m_display_identity);
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
				"display_identity",
				m_display_identity ? static_cast<gse::Value*>(VALUEEXT(gse::value::Object, GSE_CALL, (gse::value::object_properties_t{
					{"faction_name", VALUE(gse::value::String,,m_display_identity->faction_name)},
					{"leader_title", VALUE(gse::value::String,,m_display_identity->leader_title)},
					{"leader_name", VALUE(gse::value::String,,m_display_identity->leader_name)},
                    {"setup", m_display_identity->setup ? static_cast<gse::Value*>(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
                        {"noun",VALUE(gse::value::String,,m_display_identity->setup->noun)},
                        {"adjective",VALUE(gse::value::String,,m_display_identity->setup->adjective)},
                        {"description",VALUE(gse::value::String,,m_display_identity->setup->description)},
                        {"female",VALUE(gse::value::Bool,,m_display_identity->setup->female)}
                    }))) : VALUE(gse::value::Undefined)}
				}))) : VALUE(gse::value::Undefined)
			},
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
