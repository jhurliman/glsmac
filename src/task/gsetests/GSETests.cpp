#include "GSETests.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "config/Config.h"

#include "gse/GSE.h"
#include "gse/Async.h"
#include "gse/tests/Tests.h"
#include "engine/Engine.h"
#include "gse/program/Program.h"
#include "util/LogHelper.h"
#include "gse/context/GlobalContext.h"
#include "gc/Space.h"

namespace gse::tests {
const gse::program::Program* GetTestProgram( gc::Space* const gc_space );
}

namespace task {
namespace gsetests {

void GSETests::Start() {
	const auto* c = g_engine->GetConfig();
	if ( c->HasDebugFlag( config::Config::DF_GSE_TESTS_NATIVE_FILTER ) ) {
		try {
			m_native_filter.emplace( c->GetGSETestsNativeFilter() );
		}
		catch ( const std::regex_error& error ) {
			LogTest( std::string( "Invalid native test filter: " ) + error.what() );
			m_stats.failed++;
			return;
		}
	}
    const auto* shard = std::getenv( "GLSMAC_GSE_TEST_SHARD" );
    const auto* source = std::getenv( "GLSMAC_GSE_TEST_SOURCE_FILTER" );
    if ( shard || source ) {
        if ( !m_native_filter ) {
            LogTest( "Native test sharding/source selection requires --gse-tests-native-filter" );
            m_stats.failed++;
            return;
        }
        try {
            if ( shard ) {
                std::cmatch parts;
                if ( !std::regex_match( shard, parts, std::regex( "([0-9]{1,2})/([0-9]{1,2})" ) ) ) {
                    throw std::runtime_error( "expected INDEX/COUNT" );
                }
                m_shard_index = std::stoul( parts[ 1 ].str() );
                m_shard_count = std::stoul( parts[ 2 ].str() );
                if ( m_shard_count == 0 || m_shard_count > 64 || m_shard_index >= m_shard_count ) {
                    throw std::runtime_error( "require 0 <= INDEX < COUNT <= 64" );
                }
            }
            if ( source ) { m_source_filter.emplace( source ); }
        }
        catch ( const std::exception& error ) {
            LogTest( std::string( "Invalid native test selection: " ) + error.what() );
            m_stats.failed++;
            return;
        }
    }
    gse::tests::AddTests( this );
    if ( shard ) { LogTest( "Native shard " + std::string( shard ) + ": " + std::to_string( m_tests.size() ) + " groups" ); }
	if ( m_native_filter && m_tests.empty() ) {
		LogTest( "No native tests matched filter: " + c->GetGSETestsNativeFilter() );
		m_stats.failed++;
	}
}

void GSETests::Stop() {
	if ( m_stats.failed == 0 ) {
		LogTest( "All tests passed." );
	}
	else {
		LogTest( "Testing complete, passed: " + std::to_string( m_stats.passed ) + ", failed: " + std::to_string( m_stats.failed ) );
	}
	m_tests.clear();
	m_stats.passed = 0;
	m_stats.failed = 0;
}

void GSETests::Iterate() {
	if ( current_test_index < m_tests.size() ) {
        const auto& it = m_tests[ current_test_index++ ];
        LogTest( "  " + it.name + "..." );
        const auto started = std::chrono::steady_clock::now();
        std::string errmsg;
        {
            gse::GSE gse;
            gse::context::GlobalContext* ctx;
            gse.GetGCSpace()->Accumulate(
                nullptr,
                [ &gse, &ctx ]() { ctx = gse.CreateGlobalContext(); }
            );
            errmsg = it.run( &gse, ctx );
        } // Include collector/async destruction, where shutdown hangs can occur.
        const auto elapsed = std::chrono::duration< double, std::milli >( std::chrono::steady_clock::now() - started ).count();
        std::ostringstream timing;
        timing << "[GSE timing] " << std::fixed << std::setprecision( 3 ) << elapsed << " ms | " << it.source << " | " << it.name;
        LogTest( timing.str() );
        if ( errmsg.empty() ) { m_stats.passed++; }
        else { m_stats.failed++; LogTest( "    !!! TEST FAILED: " + errmsg ); }
	}
	else if ( current_test_index == m_tests.size() ) {
		current_test_index++;
		g_engine->ShutDown( m_stats.failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE );
	}
}

void GSETests::AddTest( const std::string& name, const gse_test_t test, const char* source ) {
    if ( m_native_filter && !std::regex_search( name, *m_native_filter ) ) { return; }
    std::string path = source ? source : "<unknown registration source>";
    std::replace( path.begin(), path.end(), '\\', '/' );
    // Missing compiler provenance must broaden coverage, never omit tests.
    if ( m_source_filter && source && !std::regex_search( path, *m_source_filter ) ) { return; }
    // FNV-1a is stable across compilers/hosts. Every name belongs to exactly one
    // shard; selection never depends on registration order or std::hash's ABI.
    uint32_t hash = 2166136261u;
    for ( const unsigned char byte : name ) { hash = ( hash ^ byte ) * 16777619u; }
    if ( hash % m_shard_count != m_shard_index ) { return; }
    m_tests.push_back( { name, test, path } );
}

void GSETests::LogTest( const std::string& text, bool is_debug ) {
	if ( is_debug ) {
		Log( text );
	}
	else {
		util::LogHelper::Println( text );
	}
}

}
}
