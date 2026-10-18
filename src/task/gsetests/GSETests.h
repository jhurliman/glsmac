#pragma once

#include <functional>
#include <cstdint>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "common/Task.h"

namespace gse {
class GSE;
namespace context {
class Context;
}
}

namespace task {
namespace gsetests {

typedef std::function< std::string( gse::GSE* gse, gse::context::Context* ctx ) > gse_test_t;
#define GT( ... ) [ __VA_ARGS__ ]( gse::GSE* gse, gse::context::Context* ctx ) -> std::string

#define GT_LOG( _text ) { \
    task->LogTest( _text );\
}

#define GT_OK() {\
    return "";\
}

#define GT_FAIL( _text ) {\
    return (std::string)__FILE__ + ":" + std::to_string(__LINE__) + ": " + ( _text );\
}

#define GT_ASSERT( _condition, ... ) {\
    if ( !( _condition ) ) {\
        GT_FAIL( (std::string) "assertion failed [ " # _condition " ]: " __VA_ARGS__ );\
    }\
}

CLASS( GSETests, common::Task )
	void Start() override;
	void Stop() override;
	void Iterate() override;

    // Capture the registration call site without changing existing AddTest callers.
    // Other compilers retain coverage conservatively when provenance is unavailable.
#if defined( __clang__ ) || defined( __GNUC__ )
    void AddTest( const std::string& name, const gse_test_t test, const char* source = __builtin_FILE() );
#else
    void AddTest( const std::string& name, const gse_test_t test, const char* source = nullptr );
#endif
	void LogTest( const std::string& text, bool is_debug = false );

private:
	size_t current_test_index = 0;
    std::optional< std::regex > m_native_filter;
    std::optional< std::regex > m_source_filter;
    uint32_t m_shard_index = 0, m_shard_count = 1;
    struct Test { std::string name; gse_test_t run; std::string source; };
	std::vector< Test > m_tests = {};

	struct {
		size_t passed = 0;
		size_t failed = 0;
	} m_stats = {};
};

}
}
