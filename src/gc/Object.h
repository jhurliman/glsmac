#pragma once

#include <unordered_set>
#include "common/Mutex.h"
#include <string>

#include "common/Common.h"

#define GC_REACHABLE( _var ) \
    if ( reachable_objects.find( _var ) == reachable_objects.end() ) { \
        (_var)->GetReachableObjects( reachable_objects ); \
    } \
    else { \
        GC_DEBUG( "ref", _var ); \
}

#if ( defined( DEBUG ) || defined( FASTDEBUG ) )
#include "GC.h"
#define GC_LOG( _what ) \
    if ( GC::IsDebugEnabled() ) { \
        Log( _what ); \
    }
#define GC_DEBUG_LOCK() gc::GC::DebugLock()
#define GC_DEBUG_BEGIN( _what ) gc::GC::DebugBegin( _what )
#define GC_DEBUG( _what, _object ) gc::GC::DebugEntry( _what, _object )
#define GC_DEBUG_END() gc::GC::DebugEnd()
#define GC_DEBUG_UNLOCK() gc::GC::DebugUnlock()
#else
#define GC_LOG( _what )
#define GC_DEBUG_LOCK()
#define GC_DEBUG_BEGIN( _what )
#define GC_DEBUG( _what, _object )
#define GC_DEBUG_END()
#define GC_DEBUG_UNLOCK()
#endif

namespace gc {

class Space;

CLASS( Object, common::Class )

	Object( gc::Space* const gc_space );
	virtual ~Object();
	Object( const Object& ) = delete;
	Object( Object&& ) = delete;
	Object& operator=( const Object& ) = delete;
	Object& operator=( Object&& ) = delete;

	virtual void GetReachableObjects( std::unordered_set< Object* >& reachable_objects );

protected:
	void Persist( Object* const obj );
	void Unpersist( Object* const obj );
	const bool IsPersisted( Object* const obj ) const;

private:
	friend class Space;
	// Pending registration belongs to this address. A failed derived constructor
	// cancels it during base destruction; publication clears it before collection.
	Space* m_pending_space = nullptr;
	std::unordered_set< Object* > m_persisted_objects = {};

};

}
