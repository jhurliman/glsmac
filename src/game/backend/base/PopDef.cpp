#include "PopDef.h"
#include <stdexcept>

namespace game {
namespace backend {
namespace base {

PopDef::PopDef(
	const std::string& id,
	const std::string& name,
	const render_infos_t& renders_human,
	const render_infos_t& renders_progenitor,
	const pop_flags_t flags,
	const std::optional< uint8_t > smac_specialist_id
)
	: m_id( id )
	, m_name( name )
	, m_renders_human( renders_human )
	, m_renders_progenitor( renders_progenitor )
	, m_flags( flags )
	, m_smac_specialist_id( smac_specialist_id ) {
	if ( m_smac_specialist_id && ( *m_smac_specialist_id > 6 || ( m_flags & PF_TILE_WORKER ) ) ) {
		throw std::runtime_error( "SMAC specialist role must be a non-worker catalog position 0..6" );
	}
}

const std::string PopDef::ToString( const std::string& prefix ) const {
	return (std::string)
		TS_OBJ_BEGIN( "PopDef" ) +
		TS_OBJ_PROP_STR( "id", m_id ) +
		TS_OBJ_PROP_STR( "name", m_name ) +
		InfosToString( prefix, "renders_human", m_renders_human ) +
		InfosToString( prefix, "renders_progenitor", m_renders_human ) +
		TS_OBJ_PROP_STR( "flags", (
			( m_flags & PF_TILE_WORKER )
				? "tile_worker"
				: ""
		) ) +
		TS_OBJ_END();
}

const types::Buffer PopDef::Serialize( const PopDef* def ) {
	types::Buffer buf;
	buf.WriteString( def->m_id );
	buf.WriteString( def->m_name );
#define X( _r ) \
    buf.WriteInt( def->_r.size() ); \
    for ( const auto& r : def->_r ) { \
        r.Write( buf ); \
    }
	X( m_renders_human )
	X( m_renders_progenitor )
#undef X
	buf.WriteInt( def->m_flags );
	buf.WriteBool( def->m_smac_specialist_id.has_value() );
	if ( def->m_smac_specialist_id ) { buf.WriteInt( *def->m_smac_specialist_id ); }
	return buf;
}

PopDef* PopDef::Deserialize( types::Buffer& buf ) {
	const auto id = buf.ReadString();
	const auto name = buf.ReadString();
#define X( _r ) \
    render_infos_t _r = {}; \
    _r.resize( buf.ReadInt() ); \
    for ( auto& r : _r ) { \
        r.file = buf.ReadString(); \
        r.x = buf.ReadInt(); \
        r.y = buf.ReadInt(); \
        r.width = buf.ReadInt(); \
        r.height = buf.ReadInt(); \
    }
	X( renders_human )
	X( renders_progenitor )
#undef X
	const auto flags = buf.ReadInt();
	if ( flags < 0 || flags > UINT8_MAX ) { throw std::runtime_error( "Population flags are out of range" ); }
	std::optional< uint8_t > specialist;
	if ( buf.ReadBool() ) {
		const auto position = buf.ReadInt();
		if ( position < 0 || position > 6 ) { throw std::runtime_error( "Invalid SMAC specialist position" ); }
		specialist = static_cast< uint8_t >( position );
	}
	if ( buf.lenr != buf.lenw ) { throw std::runtime_error( "Trailing population definition" ); }
	return new PopDef( id, name, renders_human, renders_progenitor, flags, specialist );
}

const std::string PopDef::InfosToString( const std::string& prefix, const std::string& name, const render_infos_t& infos ) const {
	std::string result = TS_ARR_BEGIN( name );
	for ( size_t i = 0 ; i < infos.size() ; i++ ) {
		const auto& info = infos.at( i );
		result += TS_OBJ_BEGIN( std::to_string( i ) ) +
			info.ToString( prefix ) +
			TS_OBJ_END();
	}
	result += TS_ARR_END();
	return result;
}

}
}
}
