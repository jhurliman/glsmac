#include "GLSMAC.h"

#include "util/FS.h"
#include "engine/Engine.h"
#include "config/Config.h"
#include "resource/ResourceManager.h"
#include "scheduler/Scheduler.h"
#include "util/LogHelper.h"
#include "gc/Space.h"
#include "console/Console.h"
#include "ui/UI.h"

#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/Async.h"
#include "gse/context/GlobalContext.h"
#include "gse/callable/Native.h"
#include "gse/Exception.h"
#include "gse/value/Undefined.h"

#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include "game/backend/connection/Server.h"
#include "game/backend/connection/Client.h"

#include "game/frontend/Game.h"

static GLSMAC* s_glsmac = nullptr;

GLSMAC::GLSMAC()
	: gse::GCWrappable( nullptr ) {

	// there can only be one GLSMAC
	ASSERT( !s_glsmac, "GLSMAC already defined" );
	s_glsmac = this;

	Log( "Creating global state" );

	// scripting stuff
	NEW( m_gse, gse::GSE );
	m_gse->AddRootObject( this );
	m_gse->AddBindings( this );
	m_ctx = m_gse->CreateGlobalContext();
	m_gc_space = m_gse->GetGCSpace();

	const auto& c = g_engine->GetConfig();

	const auto entry_script =
		util::FS::GeneratePath(
			{
				c->GetDataPath(),
				"default", // only 'default' mod for now
				c->GetMainScript() // script name (extension is appended automatically)
			}, gse::GSE::PATH_SEPARATOR
		)
	;

	m_gc_space->Accumulate( this, [ this ]() {
		gse::ExecutionPointer ep;
		// init UI
		m_ui = new ui::UI( m_gc_space, m_ctx, { "" }, ep );
	}, nullptr, true);

	// init console
	InitConsole();

	m_gc_space->Accumulate( this, [ this, &entry_script ]() {
		gse::ExecutionPointer ep;
		// load main scripts (will be run in RunMain())
		m_gse->RunScript( m_gc_space, m_ctx, {}, ep, entry_script );
		for ( const auto& mod_path : g_engine->GetConfig()->GetModPaths() ) {
			m_gse->RunScript(
				m_gc_space, m_ctx, {}, ep, util::FS::GeneratePath(
					{
						mod_path,
						"main"
					}
				)
			);
		}
	}, nullptr, true );

}

GLSMAC::GLSMAC( gse::GSE* gse )
	: gse::GCWrappable( nullptr )
	, m_gse( gse )
	, m_gc_space( gse->GetGCSpace() ) {
	m_gc_space->Accumulate( this, [ this ]() {
		m_ctx = m_gse->CreateGlobalContext();
	} );
}

GLSMAC::~GLSMAC() {

	if ( m_load_thread ) {
		Log( "Waiting for load thread to stop" );
		m_load_thread.load()->join();
		delete m_load_thread;
	}

	if ( m_is_loader_shown ) {
		HideLoader();
	}

	Log( "Destroying global state" );

	if ( m_game ) {
		m_game->Stop();
		delete m_game;
	}

	m_gse->GetAsync()->StopTimers();

	if ( m_console ) {
		m_console->Stop();
		delete m_console;
	}

	if ( m_ui ) {
		m_gc_space->Accumulate( this, [ this ](){
			gse::ExecutionPointer ep;
			m_ui->Destroy( m_gc_space, m_ctx, { "" }, ep );
		}, nullptr, true);
	}

	DELETE( m_gse );

	if ( s_glsmac == this ) {
		s_glsmac = nullptr;
	}
}

void GLSMAC::Iterate() {
	if ( ProcessReset() ) { return; }
	{
		bool ticked = false;
		while ( m_loader_dots_timer.HasTicked() ) {
			m_loader_dots++;
			if ( m_loader_dots > 3 ) {
				m_loader_dots = 1;
			}
			ticked = true;
		}
		if ( ticked ) {
			UpdateLoaderText();
			if ( ProcessReset() ) { return; }
		}
	}
	if ( m_load_thread ) {
		if ( !m_is_loading ) {
			ASSERT( m_f_after_load, "f_after_load not set" );
			m_load_thread.load()->join();
			delete m_load_thread;
			m_load_thread = nullptr;
			m_gc_space->Accumulate( this, [ this ](){
				HideLoader();
				m_f_after_load();
			});
			m_f_after_load = nullptr;
			if ( ProcessReset() ) { return; }
		}
	}
	m_gse->Iterate();
	if ( ProcessReset() ) { return; }
	m_ui->Iterate();
	if ( ProcessReset() ) { return; }
	if ( m_game ) {
		m_game->Iterate();
		if ( ProcessReset() ) { return; }
	}
	if ( m_console ) {
		m_console->Iterate();
	}
}

bool GLSMAC::ProcessReset() {
	if ( !m_reset_needed ) { return false; }
	if ( m_reset_phase == ResetPhase::FAILED ) { return true; }
	auto* game = g_engine->GetGame();
	if ( m_reset_phase == ResetPhase::IDLE ) {
		try {
			// Retire intent before any blocking cancellation. The caller is
			// outside accumulation, so native work can finish independently.
			m_reset_barrier = game->MT_BeginResetController();
			if ( m_game ) { m_game->RetireBackendRequests(); }
			m_reset_request = game->MT_SubmitResetController( m_reset_barrier );
			m_reset_phase = ResetPhase::WAITING;
		}
		catch ( ... ) {
			m_reset_phase = ResetPhase::FAILED;
			if ( m_reset_barrier ) { game->MT_AbandonResetController( m_reset_barrier ); }
			m_reset_barrier = 0;
			m_reset_request = 0;
			throw;
		}
		return true;
	}
	const auto response = game->MT_GetResponse( m_reset_request );
	if ( response.result == game::backend::R_NONE ) { return true; }
	const auto barrier = m_reset_barrier;
	// Terminal retrieval also releases the controller admission. Do not
	// abandon it again. Backend Stop owns any still-unconsumed MT payloads.
	m_reset_request = 0;
	m_reset_barrier = 0;
	{
		struct ResponseOwner {
			game::backend::Game* backend;
			const game::backend::MT_Response& response;
			~ResponseOwner() { backend->MT_DestroyResponse( response ); }
		} destroy_response{ game, response };
		if ( response.op != game::backend::OP_RESET || response.world_ticket != barrier || response.result != game::backend::R_SUCCESS ) {
			m_reset_phase = ResetPhase::FAILED;
			THROW( "failed to reset game thread" );
		}
	}
	m_reset_phase = ResetPhase::IDLE;
	m_is_game_running = false;
	m_reset_needed = false;
	RunMain();
	return true;
}

WRAPIMPL_BEGIN( GLSMAC )
	auto* game = g_engine->GetGame();
	WRAPIMPL_PROPS
	WRAPIMPL_TRIGGERS
		{
			"config",
			g_engine->GetConfig()->Wrap( GSE_CALL ),
		},
		{
			"ui",
			m_ui->Wrap( GSE_CALL )
		},
		{
			"run",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				S_Init( GSE_CALL, {} );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"exit",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				g_engine->ShutDown();
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"mainmenu", NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				S_MainMenu( GSE_CALL );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"deinit",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				DeinitGameState( GSE_CALL );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"select_ruleset",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( id, 0, String );
				if ( !m_state || m_is_game_running || m_state->m_connection ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Select the gameplay ruleset before initializing a session" );
				}
				try { m_state->m_settings.global.SelectRuleset( id ); }
				catch ( const std::runtime_error& e ) { GSE_ERROR( gse::EC.GAME_ERROR, e.what() ); }
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"init",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				InitGameState( GSE_CALL );
				return VALUE( gse::value::Undefined );
			} ),
		},
		{
			"reset",
			NATIVE_CALL( this ) {
				S_Reset( GSE_CALL );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"game",
			game->Wrap( GSE_CALL ),
		},
		{
			"connect",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				if ( !m_state ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game not initialized" );
				}
				if ( m_state->GetConnection() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Connection is already established" );
				}
				if ( m_is_game_running ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game is already running" );
				}
				game::backend::connection::Connection* connection = nullptr;
				switch ( m_state->m_settings.local.network_role ) {
					case game::backend::settings::LocalSettings::NR_SERVER: {
						NEW( connection, game::backend::connection::Server, gc_space, &m_state->m_settings.local );
						break;
					}
					case game::backend::settings::LocalSettings::NR_CLIENT: {
						NEW( connection, game::backend::connection::Client, gc_space, &m_state->m_settings.local );
						break;
					}
					default:
						GSE_ERROR( gse::EC.GAME_ERROR, "Network role not set" );
				}
				m_state->SetConnection( connection );
				return connection->Wrap( GSE_CALL );
			} ),
		},
		{
			"start_game",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS_MIN_MAX( 0, 1 );
				std::optional< std::string > faction_id;
				if ( !arguments.empty() ) {
					N_GETVALUE( id, 0, String );
					faction_id = id;
				}
				if ( !m_state ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game not initialized" );
				}
				if ( m_is_game_running ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game is already running" );
				}
				if ( m_is_loading ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game still initializing" );
				}
				if ( faction_id || m_state->m_slots->GetSlots().empty() ) {
					try { AddSinglePlayerSlot( faction_id ); }
					catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
				}
				StartGame( GSE_CALL );
				return VALUE( gse::value::Undefined );
			} )
		},
	};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( GLSMAC )

void GLSMAC::ShowLoader( const std::string& text ) {
	if ( !m_is_loader_shown ) {
		m_is_loader_shown = true;
		m_loader_text = text;
		m_loader_dots = 1;
		m_loader_dots_timer.SetInterval( 100 );
		m_gc_space->Accumulate( this, [ this ] () {
			try {
				TriggerObject( m_ui, "loader_show" );
			} catch ( const gse::Exception& e ) {
				// tmp fix gdb race conditions // TODO
			}
		});
	}
	UpdateLoaderText();
}

void GLSMAC::SetLoaderText( const std::string& text ) {
	if ( !m_is_loader_shown ) {
		ShowLoader( text );
	}
	else {
		m_loader_text = text;
		UpdateLoaderText();
	}
}

void GLSMAC::HideLoader() {
	if ( m_is_loader_shown ) {
		m_is_loader_shown = false;
		m_loader_dots_timer.Stop();
		m_gc_space->Accumulate( this, [ this ](){
			try {
				TriggerObject( m_ui, "loader_hide" );
			} catch ( const gse::Exception& e ) {
				// tmp fix gdb race conditions // TODO
			}
		});
	}
}

void GLSMAC::ShowError( const std::string& text, const std::function< void() >& on_close ) {
	Log( text );
	m_gc_space->Accumulate( this, [ this ](){
		TriggerObject( m_ui, "error_popup" );
	});
}

gse::Value* const GLSMAC::TriggerObject( gse::GCWrappable* object, const std::string& event, const f_args_t& f_args ) {
	gse::ExecutionPointer ep;
	return object->Trigger( m_gc_space, m_ctx, {}, ep, event, f_args );
}

void GLSMAC::WithGSE( const std::function<void( GSE_CALLABLE )>& f ) {
	m_state->WithGSE( this, f );
}

void GLSMAC::GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) {
	gse::GCWrappable::GetReachableObjects( reachable_objects );

	g_engine->GetGame()->GetReachableObjects( reachable_objects );

	if ( m_state ) {
		GC_DEBUG_BEGIN( "state" );
		GC_REACHABLE( m_state );
		GC_DEBUG_END();
	}

	if ( m_wrapobj ) {
		GC_DEBUG_BEGIN( "wrapobj" );
		GC_REACHABLE( m_wrapobj );
		GC_DEBUG_END();
	}

	if ( m_ui ) {
		GC_DEBUG_BEGIN( "ui" );
		GC_REACHABLE( m_ui );
		GC_DEBUG_END();
	}

	GC_DEBUG_BEGIN( "main callables" );
	for ( const auto& m : m_main_callables ) {
		GC_REACHABLE( m );
	}
	GC_DEBUG_END();

}

void GLSMAC::AsyncLoad( const std::string& text, const std::function< void() >& f, const std::function< void() >& f_after_load ) {
	ASSERT( !m_load_thread, "load thread already set" );
	ASSERT( !m_f_after_load, "f_after load already set" );
	m_is_loading = true;
	m_f_after_load = f_after_load;
	ShowLoader( text );
	m_load_thread = new std::thread( [ this, f ] {
		f();
		m_is_loading = false;
	});
}

void GLSMAC::S_Init( GSE_CALLABLE, const std::optional< std::string >& path ) {
	if ( m_is_game_running || m_reset_needed ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Cannot run while a game or reset is active" );
	}
	const auto& c = g_engine->GetConfig();
	auto* r = g_engine->GetResourceManager();
	if ( !m_state || r->GetDetectedSMACType() == config::ST_UNKNOWN ) {
		if ( !m_state ) {
			m_state = new game::backend::State( m_gc_space, m_ctx, this );
			m_state->m_settings.global.Initialize();
		}
		try {
			if ( path.has_value() ) {
				r->Init( { path.value() }, config::ST_AUTO );
			}
			else {
				r->Init( c->GetPossibleSMACPaths(), c->GetSMACType() );
			}
		} catch ( const std::runtime_error& e ) {
			gse::value::object_properties_t args = {
				{
					"set_smacpath", NATIVE_CALL( this ) {
						N_EXPECT_ARGS( 1 );
						N_GETVALUE( path, 0, String );
						S_Init( GSE_CALL, path );
						return VALUE( gse::value::Undefined );
					} )
				}
			};
			if ( path.has_value() ) {
				args.insert({ "last_failed_path", VALUE( gse::value::String,, path.value() ) } );
			}
			TriggerObject( this, "smacpath_prompt", ARGS( args ) );
			return;
		}
		if ( path.has_value() ) {
			c->SetSMACPath( path.value() );
		}
	}
	Reset( GSE_CALL );
}

void GLSMAC::S_Reset( GSE_CALLABLE ) {
	ClearHandlers();
	m_ui->Clear( GSE_CALL );
	auto* game = g_engine->GetGame();
	game->ClearHandlers();
	game->ClearEvents();
	m_reset_needed = true;
}

void GLSMAC::S_Intro( GSE_CALLABLE ) {
	TriggerObject( this, "intro" );
}

void GLSMAC::S_MainMenu( GSE_CALLABLE ) {
	TriggerObject( this, "mainmenu_show", ARGS_F( this ) {
		{
			"settings",
			m_state->m_settings.Wrap( GSE_CALL ),
		}
	}; } );
}

void GLSMAC::S_Game( GSE_CALLABLE ) {
	TriggerObject( this, "mainmenu_hide", {} );
	m_game->Start();
}

void GLSMAC::UpdateLoaderText() {
	ASSERT( m_is_loader_shown, "loader not shown" );
	std::string text = m_loader_text + std::string( m_loader_dots, '.' ) + std::string( 3 - m_loader_dots, ' ' );
	m_gc_space->Accumulate( this, [ this, &text ]() {
		try {
			TriggerObject( m_ui, "loader_text", ARGS_F( this, &text ) {
				{
					"text", VALUEEXT( gse::value::String, m_gc_space, text )
				}
			}; } );
		} catch ( const gse::Exception& e ) {
			// tmp fix gdb race conditions // TODO
		}
	});
}

void GLSMAC::Reset( GSE_CALLABLE ) {
	const auto& c = g_engine->GetConfig();

	HideLoader();

	if ( m_game ) {
		m_game->Stop();
	}

	if ( c->HasLaunchFlag( config::Config::LF_QUICKSTART ) ) {
		DeinitGameState( GSE_CALL );
		m_state->m_settings.local.game_mode = game::backend::settings::LocalSettings::GM_SINGLEPLAYER;
		m_state->m_settings.local.network_role = game::backend::settings::LocalSettings::NR_NONE;
		m_state->m_settings.global.SelectRuleset( c->GetRuleset() );
		InitGameState( GSE_CALL );
		if ( c->GetRuleset() == "smac" ) {
			try {
				if ( c->GetQuickstartWorldSize() >= 0 ) { m_state->m_settings.global.SelectSmacWorldSize( c->GetQuickstartWorldSize() ); }
				else {
					const auto size = c->GetQuickstartMapSize();
					m_state->m_settings.global.SelectSmacCustomSize( size.x, size.y );
				}
			} catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
		}
		m_state->m_settings.global.Initialize();
		AddSinglePlayerSlot( c->HasLaunchFlag( config::Config::LF_QUICKSTART_FACTION )
			? std::optional< std::string >( c->GetQuickstartFaction() ) : std::nullopt );
		auto ep2 = ep;
		StartGame( m_gc_space, ctx, si, ep2 );
	}
	else if (
		c->HasLaunchFlag( config::Config::LF_SKIPINTRO ) ||
			g_engine->GetResourceManager()->GetDetectedSMACType() == config::ST_LEGACY // it doesn't have firaxis logo image
		) {
		S_MainMenu( GSE_CALL );
	}
	else {
		S_Intro( GSE_CALL );
	}
}

void GLSMAC::DeinitGameState( GSE_CALLABLE ) {
	if ( m_state ) {
		if ( m_is_game_running ) {
			GSE_ERROR( gse::EC.GAME_ERROR, "Game is still running" );
		}
		m_state->Reset();
		m_state->m_settings.global.ResetRuleset();
	}
}

void GLSMAC::InitGameState( GSE_CALLABLE ) {
	if ( m_is_game_running ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Game is already running" );
	}
	const bool joining = m_state->m_settings.local.network_role == game::backend::settings::LocalSettings::NR_CLIENT;
	try {
		m_state->m_settings.PrepareRuleset( []( const std::string& filename ) {
			return util::FS::ReadTextFile( g_engine->GetResourceManager()->GetCustomPath( filename ) );
		} );
	}
	catch ( const std::exception& e ) { GSE_ERROR( gse::EC.GAME_ERROR, e.what() ); }
	auto* game = g_engine->GetGame();
	m_state->Reset();
	m_state->SetGame( game );
	game->SetState( m_state );
	m_state->GetFM()->Clear();
	// Joining clients receive their faction definitions with the host's settings.
	if ( joining ) { return; }
	m_state->WithGSE( this, [ this ]( GSE_CALLABLE ) {
		TriggerObject( this, "configure_state", ARGS_F( this ) {
			{
				"fm",
				m_state->GetFM()->Wrap( GSE_CALL ),
			}, {
				"ruleset", VALUE( gse::value::String,, m_state->m_settings.global.GetRulesetId() )
			}
		}; } );
		// Configure-state supplies the faction metadata. Validate the complete
		// settings packet before making this host setup available to the lobby.
		try { m_state->Serialize(); }
		catch ( const std::exception& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
	});
}

void GLSMAC::RandomizeSettings( GSE_CALLABLE ) {
	if ( !m_state ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Game is not initialized" );
	}
	if ( m_is_game_running ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Game is already running" );
	}
}

void GLSMAC::AddSinglePlayerSlot( const std::optional< std::string >& faction_id ) {
	if ( !m_state || m_state->m_connection || !m_state->m_slots->GetSlots().empty()
		|| m_state->m_settings.local.game_mode != game::backend::settings::LocalSettings::GM_SINGLEPLAYER
		|| m_state->m_settings.local.network_role != game::backend::settings::LocalSettings::NR_NONE ) {
		THROW( "Choose a single-player faction before creating the local roster" );
	}
	if ( faction_id && !m_state->GetFM()->Get( *faction_id ) ) {
		THROW( "Unknown faction for the configured ruleset: " + *faction_id );
	}
    const auto& rules = m_state->m_settings.global.rules;
    m_state->m_settings.local.player_name = "Player";
    m_state->CreatePlayerSlot(0, {m_state->m_settings.local.player_name, game::backend::roster::Role::Single,
        faction_id, rules.GetDefaultDifficultyLevel()},
        m_state->m_settings.local.account.GetGSID(), 0, "", false);
    m_state->m_slots->GetSlot(0).SetPlayerFlag(game::backend::slot::PF_READY);

}

void GLSMAC::StartGame( GSE_CALLABLE ) {
	if ( m_state->m_connection && m_state->m_connection->IsClient()
		&& !m_state->m_connection->AsClient()->HasGlobalSettings() ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Waiting for the host's gameplay settings" );
	}
	try { m_state->m_settings.global.LockRuleset(); m_state->m_settings.global.LockWorldSize(); }
	catch ( const std::runtime_error& e ) { GSE_ERROR( gse::EC.GAME_ERROR, e.what() ); }
	// real state belongs to game task now
	// save it as backup, then make temporary shallow copy (no connection, players etc)
	//   just for the sake of passing settings to previous menu
	auto* real_state = m_state;

	//m_state = nullptr; // TODO: why?

	m_is_game_running = true;

	auto* game = g_engine->GetGame();
	ASSERT( game, "game not set" );

	if ( m_game ) {
		m_game->Stop();
		delete m_game;
	}
	m_game = new ::game::frontend::Game(
		this, real_state, m_ui, [ this, si, ep ] () {
			m_gc_space->Accumulate(
				this, [ this ]() {
					TriggerObject( this, "start_game" );
				}
			);
		}, [] () {
			// THROW( "TODO: cancel" );
		}
	);

	S_Game( GSE_CALL );

	TriggerObject( this, "configure_game", ARGS_F( &game ) {
		{
			"game",
			game->Wrap( GSE_CALL ),
		}
	}; } );
}

void GLSMAC::InitConsole() {
	ASSERT( !m_console, "console already initialized" );
	m_console = new console::Console();
	m_console->Start();
}

void GLSMAC::RunMain() {
	ASSERT( m_gc_space, "gc space is null" );
	for ( const auto& main : m_main_callables ) {
		ASSERT( main->type == gse::VT_CALLABLE, "main not callable" );
		m_gc_space->Accumulate( this, [ this, &main ] (){
			gse::ExecutionPointer ep;
			if ( !m_wrapobj ) {
				m_wrapobj = Wrap( m_gc_space, m_ctx, {}, ep );
			}
			( (gse::value::Callable*)main )->Run( m_gc_space, m_ctx, {}, ep, {
				m_wrapobj
			} );
		});
	}
}

void GLSMAC::AddToContext( gc::Space* const gc_space, gse::context::Context* ctx, gse::ExecutionPointer& ep ) {
	ctx->CreateBuiltin( "main", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 1 );
		const auto& main = arguments.at(0);
		N_CHECKARG( main, 0, Callable );
		m_main_callables.push_back( main );
		return VALUE( gse::value::Undefined );
	} ), ep );
}
