#include "Render.h"

#include "SpriteRender.h"
#include "CvrScoutRender.h"

namespace game {
namespace backend {
namespace unit {

Render::Render( const render_type_t type )
	: m_type( type ) {
	//
}

void Render::Serialize( types::Buffer& buf, const Render* render ) {
	buf.WriteInt( render->m_type );
	switch ( render->m_type ) {
		case RT_SPRITE: {
			SpriteRender::Serialize( buf, (SpriteRender*)render );
			break;
		}
		case RT_CVR_SCOUT:
		case RT_CVR_SPARTAN_ROVER:
		case RT_CVR_INITIALIZED_SCOUT:
			buf.WriteString( static_cast< const CvrScoutRender* >( render )->Encode() );
			break;
		default:
			THROW( "unknown render type on write: " + std::to_string( render->m_type ) );
	}
}

Render* Render::Deserialize( types::Buffer& buf ) {
	const auto render_type = buf.ReadInt();
	switch ( render_type ) {
		case RT_SPRITE:
			return SpriteRender::Deserialize( buf );
		case RT_CVR_SCOUT:
		case RT_CVR_SPARTAN_ROVER:
		case RT_CVR_INITIALIZED_SCOUT:
			return new CvrScoutRender( CvrScoutRender::Decode( buf.ReadString( CvrScoutRender::MAX_WIRE_BYTES ), static_cast< render_type_t >( render_type ) ) );
		default:
			THROW( "unknown render type on read: " + std::to_string( render_type ) );
	}
}

}
}
}
