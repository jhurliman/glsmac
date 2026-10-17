#include <atomic>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include "GLSMAC.h"
#include "config/Config.h"
#include "engine/Engine.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "gse/GSE.h"
#include "gse/context/GlobalContext.h"
#include "gc/Space.h"
#include "gc/GC.h"
#include "scheduler/Simple.h"
#include "common/Task.h"
#include "loader/font/Null.h"
#include "loader/texture/Null.h"
#include "loader/sound/Null.h"
#include "input/Null.h"
#include "graphics/Null.h"
#include "audio/Null.h"
#include "network/simpletcp/SimpleTCP.h"
#include "error_handler/Stdout.h"
#include "debug/MemoryWatcher.h"
#include "util/LogHelper.h"
#include "util/UUID.h"
#include "util/FS.h"

debug::MemoryWatcher memory_watcher;
static std::atomic<bool> state_alive{ false };
static std::atomic<bool> wrong_order{ false };
static std::atomic<unsigned> states_destroyed{ 0 };
static std::atomic<unsigned> backend_stops{ 0 };
static std::mutex trace_mutex;
static void Trace( const char* text ) {
	std::lock_guard guard( trace_mutex );
	std::cout << text << std::endl;
}

namespace gse::tests {
class EngineShutdownFixture {
public:
	static GLSMAC* Create( game::backend::Game& game ) {
		NEWV( runtime, gse::GSE );
		NEWV( app, GLSMAC, runtime );
		runtime->GetGCSpace()->Accumulate( runtime, [ & ]() {
			class ObservedState final : public game::backend::State {
			public:
				ObservedState( gc::Space* space, gse::context::Context* context, GLSMAC* app ) : State( space, context, app ) { state_alive = true; }
				~ObservedState() override { Trace( "state.destroy" ); state_alive = false; ++states_destroyed; }
			};
			app->m_state = new ObservedState( runtime->GetGCSpace(), app->m_ctx, app );
			app->m_state->SetGame( &game );
			game.SetState( app->m_state );
			runtime->AddRootObject( app );
		} );
		return app;
	}
	static void AvoidRetiredStateDereference( game::backend::Game& game ) { game.m_state = nullptr; }
};
}

class ObservedGame final : public game::backend::Game {
public:
	std::promise<void> started;
	void Start() override { game::backend::Game::Start(); started.set_value(); }
	// No world is initialized: keep native iteration out of the fixture's
	// setup while retaining the actual Engine/module Start and Stop paths.
	void Iterate() override {}
	void Stop() override {
		++backend_stops;
		Trace( state_alive ? "backend.stop state_alive=1" : "backend.stop state_alive=0" );
		if ( !state_alive ) {
			wrong_order = true;
			// The safe ordering probe records the failed invariant and removes
			// only the dead pointer so actual Stop can release its other owners.
			gse::tests::EngineShutdownFixture::AvoidRetiredStateDereference( *this );
		}
		game::backend::Game::Stop();
		Trace( "backend.stop.done" );
	}
};

class RuntimeTask final : public common::Task {
public:
	explicit RuntimeTask( ObservedGame& game ) : m_game( game ) {}
	void Start() override {
		m_game.started.get_future().get();
		m_app = gse::tests::EngineShutdownFixture::Create( m_game );
		Trace( "runtime.ready" );
		g_engine->ShutDown();
	}
	void Stop() override {
		Trace( "runtime.stop" );
		DELETE( m_app );
		m_app = nullptr;
		Trace( "runtime.stop.done" );
	}
private:
	ObservedGame& m_game;
	GLSMAC* m_app = nullptr;
};

int main( int argc, char* argv[] ) {
	config::Config config( argv[ 0 ] );
	config.Init( argc, argv );
	common::Mutex::SetMutexMode( config.HasLaunchFlag( config::Config::LF_SINGLE_THREAD ) ? common::MM_NONE : common::MM_DEFAULT );
	if ( config.HasDebugFlag( config::Config::DF_MEMORYDEBUG ) ) { memory_watcher.EnableMemoryDebug(); }
	gc::GC::DebugInit();
	memory_watcher.Init();
	util::LogHelper::Init();
	util::UUID::Init();
	util::FS::CreateDirectoryIfNotExists( config.GetPrefix() );
	error_handler::Stdout error_handler;
	loader::font::Null font_loader;
	loader::texture::Null texture_loader;
	loader::sound::Null sound_loader;
	input::Null input;
	graphics::Null graphics;
	audio::Null audio;
	network::simpletcp::SimpleTCP network;
	ObservedGame game;
	scheduler::Simple scheduler;
	NEWV( task, RuntimeTask, game );
	scheduler.AddTask( task );
	int result;
	{
		engine::Engine engine( &config, &error_handler, {}, nullptr, &font_loader,
			&texture_loader, &sound_loader, nullptr, &scheduler, &input, &graphics,
			&audio, &network, &game );
		result = engine.Run();
	}
	std::cout << "result=" << result << " wrong_order=" << wrong_order << " state_destructors=" << states_destroyed << " backend_stops=" << backend_stops << std::endl;
	return result || wrong_order || states_destroyed != 1 || backend_stops != 1 ? EXIT_FAILURE : EXIT_SUCCESS;
}
