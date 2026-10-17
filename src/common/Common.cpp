#include <atomic>

#ifdef DEBUG
#include <chrono>
#endif

#include "Common.h"

#include "engine/Engine.h"
#include "logger/Logger.h"

#ifdef DEBUG
debug_stats_t g_debug_stats = {};
#endif

namespace common {

std::atomic< size_t > g_next_object_id;

Class::Class()
	: m_object_id( g_next_object_id++ ) {
	//
}

const std::string Class::GetNamespace() const {
	return "";
}

const std::string Class::GetName() const {
	if ( !m_name.empty() ) {
		return GetNamespace() + "(" + m_name + ")#" + std::to_string( m_object_id );
	}
	else {
		return GetNamespace() + "#" + std::to_string( m_object_id );
	}
}

const std::string& Class::GetLocalName() const {
	return m_name;
}

void Class::Log( const std::string& text ) const {
	if ( g_engine != NULL ) {
#ifdef DEBUG
		// One immutable origin avoids a shared last-message timestamp race.
		// Convert explicitly: the clock's native tick is not necessarily a nanosecond.
		static const auto origin = std::chrono::steady_clock::now();
		const auto duration = std::chrono::duration_cast< std::chrono::nanoseconds >(
			std::chrono::steady_clock::now() - origin
		).count();
#endif
		g_engine->Log(
#ifdef DEBUG
			"[t+" + std::to_string( duration ) + "ns] " +
#endif
				"<" + GetName() + "> " + text
		);
	}
}

#ifdef DEBUG

void Class::SetTesting( const bool testing ) {
	m_is_testing = testing;
}

const bool Class::IsTesting() const {
	return m_is_testing;
}

void Class::TestBreakpoint() {
	if ( IsTesting() ) {
		int a = 5;
		/**** put gdb breakpoint here ****/
		a++;
	}
}

void Class::SetTraceData( const std::string& data ) {
	m_trace_data = data;
}

const std::string& Class::GetTraceData() const {
	return m_trace_data;
}

#endif

}
