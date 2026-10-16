#include "Pop.h"

#include <limits>

#include "game/backend/Game.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/BaseManager.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/callable/Native.h"

namespace game {
namespace backend {
namespace base {

Pop::Pop( Base* const base, const size_t id, const PopDef* def, const uint8_t variant, map::tile::Tile* const worked_tile )
	: m_base( base )
	, m_id( id )
	, m_def( def )
	, m_variant( variant ) {
	//
}

void Pop::Serialize( types::Buffer& buf ) const {
	ASSERT( m_base, "pop base is null" );
	ASSERT( m_def, "pop def is null" );

	buf.WriteInt( m_id );
	buf.WriteString( m_def->m_id );
	buf.WriteInt( m_variant );
}

void Pop::Deserialize( types::Buffer& buf, Game* game ) {
	ASSERT( !m_base, "pop base is not null" );
	ASSERT( !m_def, "pop def is not null" );

	auto* bm = game->GetBM();
	ASSERT( bm, "bm is null" );

	m_id = buf.ReadInt();
	const auto pop_id = buf.ReadString();
	m_def = bm->GetPopDef( pop_id );
	if ( !m_def ) {
		THROW( "can't deserialize pop - def not found: " + pop_id );
	}
	m_variant = buf.ReadInt();
}

void Pop::SetBase( Base* const base ) {
	ASSERT( !m_base, "pop base already set" );
	m_base = base;
}

WRAPIMPL_SERIALIZE( Pop )
	if ( !obj || !obj->m_base ) {
		THROW( "can't serialize pop reference - population has no base" );
	}
	const auto member = obj->m_base->m_pops.find( obj->m_id );
	if ( member == obj->m_base->m_pops.end() || &member->second != obj ) {
		THROW( "can't serialize pop reference - population is not a member of its base" );
	}
	if ( obj->m_base->m_id > std::numeric_limits< long long >::max() || obj->m_id > std::numeric_limits< long long >::max() ) {
		THROW( "can't serialize pop reference - ID is out of range" );
	}
	// Population IDs are local to a base, not unique across the game.
	buf->WriteInt( obj->m_base->m_id );
	buf->WriteInt( obj->m_id );
}

WRAPIMPL_DESERIALIZE( Pop )
	const auto base_id = buf->ReadInt();
	const auto pop_id = buf->ReadInt();
	if ( base_id < 0 || pop_id < 0 || static_cast< unsigned long long >( base_id ) > std::numeric_limits< size_t >::max()
		|| static_cast< unsigned long long >( pop_id ) > std::numeric_limits< size_t >::max() ) {
		THROW( "can't deserialize pop reference - ID is out of range" );
	}
	if ( !game || !game->GetBM() ) {
		THROW( "can't deserialize pop reference - game has no base manager" );
	}
	auto* base = game->GetBM()->GetBase( static_cast< size_t >( base_id ) );
	if ( !base ) {
		THROW( "can't deserialize pop reference - base not found: " + std::to_string( base_id ) );
	}
	const auto member = base->m_pops.find( static_cast< size_t >( pop_id ) );
	if ( member == base->m_pops.end() ) {
		THROW( "can't deserialize pop reference - population not found: " + std::to_string( pop_id ) );
	}
	return member->second.Wrap( GSE_CALL );
}

WRAPIMPL_BEGIN( Pop )
	WRAPIMPL_PROPS
	WRAPIMPL_CUSTOM_SETTERS
		{
			"id",
			VALUE( gse::value::Int,, m_id ),
		},
		{
			"get_type",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return VALUE( gse::value::String, , m_def->m_id );
			} )
		},
		{
			"variant",
			VALUE( gse::value::Int,, m_variant )
		},
		{
			"get_base",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				ASSERT( m_base, "pop has no base" );
				return m_base->Wrap( GSE_CALL );
			} )
		},
		{
			"set_type",
			NATIVE_CALL( this ) {

				m_base->GetGame()->CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );

				m_base->ChangePopType( GSE_CALL, m_id, id );

				return VALUE( gse::value::Undefined );
			} )
		},
	};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Pop )

}
}
}
