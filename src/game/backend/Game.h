#pragma once

#include <memory>
#include <functional>
#include <array>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <mutex>

#include "common/MTModule.h"
#include "gse/GCWrappable.h"

#include "Types.h"
#include "game/backend/map/Types.h"

#include "types/Exception.h"
#include "gse/Exception.h"
#include "game/FrontendRequest.h"
#include "game/BackendRequest.h"
#include "game/backend/turn/Turn.h"
#include "util/random/Types.h"
#include "WorldSnapshotTurn.h"
#include "map/GlsmacResourceMapPolicy.h"
#include "map/SmacWorkerKnowledge.h"
#include "map/GlsmacTerritoryTopology.h"
#include "faction/SmacSimulationState.h"
#include "unit/ConventionalUnitState.h"
#include "base/ProductionState.h"
#include "State.h"

// TODO: remove those
#include "game/backend/map/tile/Tile.h"
#include "game/backend/map/tile/TileState.h"

class GLSMAC;

namespace gse::tests {
class BackendFixture;
class EngineShutdownFixture;
}

namespace types {
namespace texture {
class Texture;
}
namespace mesh {
class Render;

class Data;
}
}

namespace game {
namespace backend {

class Random;

namespace animation {
class Def;
}

namespace faction {
class Faction;
}

namespace map {
namespace tile {
class TileManager;
}
}

namespace resource {
class ResourceManager;
}

namespace unit {
class UnitManager;
}

namespace base {
class BaseManager;
}

namespace animation {
class AnimationManager;
}

namespace event {
class Event;
}

class State;

class Player;

namespace connection {
class Connection;
class Client;
}

namespace bindings {
class Bindings;
}

namespace map {
class Map;
}

namespace slot {
class Slot;
}

namespace event {
class Event;
class EventHandler;
}

enum op_t {
	OP_NONE,
	OP_PING,
	OP_INIT,
	OP_GET_MAP_DATA,
	OP_RESET,
	OP_SAVE_MAP,
	OP_GET_FRONTEND_REQUESTS,
	OP_SEND_BACKEND_REQUESTS,
	OP_ADD_EVENT,
#ifdef DEBUG
	OP_SAVE_DUMP,
	OP_LOAD_DUMP,
#endif
};

enum result_t {
	R_NONE,
	R_SUCCESS,
	R_ABORTED,
	R_ERROR,
	R_PENDING,
};

using world_ticket_t = uint64_t;
struct pending_initialization_t {
	common::mt_id_t request = 0;
	world_ticket_t ticket = 0;
};

typedef std::vector< types::mesh::Render* > data_tile_meshes_t;

typedef union {
	struct {
		bool try_next_unit;
		bool no_scroll_after;
	} tile_select;
	struct {
		size_t unit_id;
		bool no_scroll_after;
	} unit_select;
} tile_query_metadata_t;

struct MT_Request {
	op_t op;
	world_ticket_t world_ticket = 0;
	union {
		struct {
			world_ticket_t pending_initialization;
			bool controller;
		} reset;
		struct {
			tile_query_purpose_t purpose;
			size_t tile_x;
			size_t tile_y;
			map::tile::direction_t tile_direction;
			tile_query_metadata_t metadata;
		} select_tile;
		struct {
			std::string* path;
		} save_map;
		struct {
			std::string* message;
		} chat;
#ifdef DEBUG
		struct {
			std::string* path;
		} dump;
#endif
		struct {
			std::string* serialized_event;
		} add_event;
		struct {
			std::vector< BackendRequest >* requests;
		} send_backend_requests;
	} data;
};

// can't use types::Vec* because only simple structs can be used in union
struct vec3_t {
	float x;
	float y;
	float z;

	vec3_t operator=( const types::Vec3& source ) {
		x = source.x;
		y = source.y;
		z = source.z;
		return *this;
	};

	void operator-=( const vec3_t rhs ) {
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
	};
};

struct vec2_t {
	float x;
	float y;
};

struct vertices_t {
	vec3_t center;
	vec3_t left;
	vec3_t top;
	vec3_t right;
	vec3_t bottom;
};

struct response_map_data_t {
	size_t map_width;
	size_t map_height;
	types::texture::Texture* terrain_texture;
	types::mesh::Render* terrain_mesh;
	types::mesh::Data* terrain_data_mesh;
	std::string* path;
	struct {
		std::unordered_map< std::string, map::sprite_actor_t >* actors;
		std::unordered_map< size_t, std::pair< std::string, types::Vec3 > >* instances;
	} sprites;
	const std::vector< map::tile::Tile >* tiles;
	const std::vector< map::tile::TileState >* tile_states;
	~response_map_data_t();
};

struct MT_Response {
	op_t op;
	result_t result;
	world_ticket_t world_ticket = 0;
	union {
		struct {
			size_t slot_index;
		} init;
		response_map_data_t* get_map_data;
		struct {
			const std::string* error_text;
		} error;
		struct {
			tile_query_purpose_t purpose;
			size_t tile_x;
			size_t tile_y;
			vec2_t coords;
			struct {
				map::tile::elevation_t center;
			} elevation;
			vertices_t selection_coords;
			data_tile_meshes_t* preview_meshes;
			std::vector< std::string >* preview_lines;
			bool scroll_adaptively;
			std::vector< std::string >* sprites;
			std::vector< size_t >* unit_ids;
			tile_query_metadata_t metadata;
		} select_tile;
		struct {
			std::string* path;
		} save_map;
		struct {
			std::vector< FrontendRequest >* requests;
		} get_frontend_requests;
	} data;
};

typedef common::MTModule< MT_Request, MT_Response > MTModule;

CLASS2( Game, MTModule, gse::GCWrappable )

	Game();

	// returns success as soon as this thread is ready (not busy with previous requests)
	common::mt_id_t MT_Ping();

	// initialize map and other things
	pending_initialization_t MT_Init( State* state, world_ticket_t previous_world = 0 );
	pending_initialization_t MT_LoadMap( world_ticket_t world, const std::string& path );
	MT_Response MT_GetResponse( common::mt_id_t request );
	MT_Response MT_WaitResponse( common::mt_id_t request );
	void MT_Cancel( common::mt_id_t request );

	// get map data for display
	common::mt_id_t MT_GetMapData( world_ticket_t world );

	// deinitialize everything
	common::mt_id_t MT_Reset( world_ticket_t world, world_ticket_t pending_initialization = 0 );
	void MT_RetireWorld( world_ticket_t world, world_ticket_t pending_initialization = 0 );
	common::mt_id_t MT_ResetController();
	world_ticket_t MT_BeginResetController();
	common::mt_id_t MT_SubmitResetController( world_ticket_t barrier );
	void MT_AbandonResetController( world_ticket_t barrier );
	// Keep startup packet/message payloads owned until a post-network owner checkpoint.
	void WithWorldIngress( State* state, gc::Object* owner, const State::game_func_t& callback );

	// saves current map into file
	common::mt_id_t MT_SaveMap( world_ticket_t world, const std::string& path );

	// get all pending frontend requests (will be cleared after)
	common::mt_id_t MT_GetFrontendRequests( world_ticket_t world );

	// send backend requests for processing
	common::mt_id_t MT_SendBackendRequests( world_ticket_t world, const std::vector< BackendRequest >& requests );

#ifdef DEBUG

	common::mt_id_t MT_SaveDump( const std::string& path );
	common::mt_id_t MT_LoadDump( const std::string& path );

#endif

	void Start() override;
	void Stop() override;
	void Iterate() override;

	const bool IsStarted() const;
	Random* GetRandom() const;
	map::Map* GetMap() const;
	State* GetState() const;
	const Player* GetPlayer() const;
	const size_t GetSlotNum() const;

	void ShowLoader( const std::string& text );
	void SetLoaderText( const std::string& text );
	void HideLoader();

	void Event( GSE_CALLABLE, const std::string& name, const gse::value::object_properties_t& args );
	void AddEvent( event::Event* const event );
	void AddEventResponse( const std::string& event_id, const bool result, gse::Value* const resolved );

	void ClearEvents();

	WRAPDEFS_PTR( Game )

	void GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) override;

protected:

	const MT_Response ProcessRequest( const MT_Request& request, MT_CANCELABLE ) override;
	void DestroyRequest( const MT_Request& request ) override;
	void DestroyResponse( const MT_Response& response ) override;

public:
	typedef std::function< void() > cb_oncomplete;

	// for bindings etc
	void Message( const std::string& text );
	void Quit( const std::string& reason );
	void OnError( std::runtime_error& err );
	void OnGSEError( const gse::Exception& e );
	const size_t GetTurnId() const;
	const bool IsTurnCompleted( const size_t slot_num ) const;
	const bool IsTurnChecksumValid( const util::crc32::crc_t checksum ) const;
	void CompleteTurn( GSE_CALLABLE, const size_t slot_num );
	void UncompleteTurn( const size_t slot_num );
	void AdvanceTurn( const size_t turn_id );

	void GlobalFinalizeTurn( GSE_CALLABLE );
	void FirstTurn( GSE_CALLABLE );

	faction::Faction* GetFaction( const std::string& id ) const;

	map::tile::TileManager* GetTM() const;
	resource::ResourceManager* GetRM() const;
	// Explicit native SMAC query: no mutation, RNG draw, event maintenance or
	// fallback to legacy callback inputs. Tile's UI/intake caller uses Ordinary.
	enum class ResourceQueryPhase { Ordinary, Upkeep };
	std::array< int64_t, 3 > GetCanonicalTileResources( const map::tile::Tile& tile, const base::Base& base, ResourceQueryPhase phase ) const;
	int64_t GetCanonicalNutrientDemand( const base::Base& base ) const;
	std::array< int32_t, 3 > GetCanonicalOrbitalIntake( const base::Base& base ) const;
	int32_t GetCanonicalMineralSupport( const base::Base& base ) const;
	// Positional source identity is explicit; opaque IDs are never parsed.
	base::ProductionItem MakeSmacProductionItem( const slot::Slot* owner, base::ProductionItemKind kind, int32_t source_id ) const;
	void ValidateSmacProductionState( const slot::Slot* owner, const base::ProductionState& state ) const;
	int32_t GetSmacProductionCost( const slot::Slot* owner, const base::ProductionItem& item ) const;
	// Only the ordinary prerequisite query, not active/role/coastal buildability.
	bool HasSmacProductionPrerequisite( const slot::Slot* owner, const base::ProductionItem& item ) const;
	// Explicit local human-controlled port roster. No AI controller is inferred.
	void RequireSmacLocalHumanOwner( const slot::Slot* owner ) const;
	// Explicit snapshot admission until sight/territory producers are integrated.
	// No knowledge is inferred from current units, tiles or a transport slot.
	void PublishSmacWorkerKnowledge( GSE_CALLABLE, const map::SmacWorkerKnowledge& knowledge );
	map::SmacWorkerMapFrame::Evaluation GetSmacWorkerAvailability( const base::Base& base ) const;
	void RequireWorkerKnowledgeMutation() const;
	unit::UnitManager* GetUM() const;
	base::BaseManager* GetBM() const;
	animation::AnimationManager* GetAM() const;

	gc::Space* const GetGCSpace() const;

	void CheckRW( GSE_CALLABLE ) const;

	void SetSlotNum( const size_t slotnum ); // not thread-safe, make sure to call from game thread only

private:
	friend class gse::tests::BackendFixture;

	const types::Vec3 GetTileRenderCoords( const map::tile::Tile* tile );

	map::tile::TileManager* m_tm = nullptr;
	resource::ResourceManager* m_rm = nullptr;
	unit::UnitManager* m_um = nullptr;
	base::BaseManager* m_bm = nullptr;
	animation::AnimationManager* m_am = nullptr;

	enum game_state_t {
		GS_NONE,
		GS_PREPARING_MAP,
		GS_INITIALIZING,
		GS_STARTING,
		GS_RUNNING,
	};
	std::atomic< game_state_t > m_game_state = GS_NONE;
	common::mt_flag_t m_init_cancel = false;
	std::string m_initialization_error = "";

	Player* m_player = nullptr;
	bool m_is_master = true;
	size_t m_slot_num = 0;
	slot::Slot* m_slot = nullptr;

	response_map_data_t* m_response_map_data = nullptr;

	util::crc32::crc_t m_turn_checksum = 0;
	std::unordered_set< size_t > m_verified_turn_checksum_slots = {};

	std::vector< FrontendRequest >* m_pending_frontend_requests = nullptr;

	enum class StartupStage { Complete, InitializeConfigure, PrepareBindings, ConfigureEvents, AcquireMap, AwaitDownload, InitializeMap, Start, Failure, Ingress };
	struct WorldAdmission {
		enum Kind { INITIALIZATION, CONTROLLER_RESET };
		WorldAdmission( world_ticket_t ticket, Kind kind, State* state, State::game_session_t binding, world_ticket_t previous = 0, std::optional< std::string > map_path = {} )
			: ticket( ticket ), kind( kind ), state( state ), binding( std::move( binding ) ), previous( previous ), map_path( std::move( map_path ) ) {}
		const world_ticket_t ticket;
		const Kind kind;
		State* const state;
		const State::game_session_t binding;
		const world_ticket_t previous;
		const std::optional< std::string > map_path;
		common::mt_id_t request = 0;
		State::game_session_t admitted_binding;
		bool cancelled = false;
		// Set only after this admission's actual Generate call succeeds. File,
		// debug-dump and downloaded map paths never infer fresh placement history.
		bool generated_map = false;
		StartupStage stage = StartupStage::Complete;
		connection::Connection* connection = nullptr;
		size_t slot = 0;
		std::string error;
		struct Ingress { gc::Object* owner; State::game_func_t callback; };
		std::vector< Ingress > ingress;
	};
	struct EventBatchHandoff;
	struct OwnerTransition : gc::Object {
		enum Phase { Pending, Running, Finished, Cancelled };
		OwnerTransition( std::shared_ptr< WorldAdmission > admission, StartupStage stage, std::vector< WorldAdmission::Ingress > ingress );
		void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override;
		const std::shared_ptr< WorldAdmission > admission;
		const StartupStage stage;
		const std::vector< WorldAdmission::Ingress > ingress;
		std::unique_ptr< EventBatchHandoff > configure_events;
		Phase phase = Pending;
		std::thread::id thread;
	};
	mutable std::mutex m_admission_mutex;
	std::unordered_map< world_ticket_t, std::shared_ptr< WorldAdmission > > m_admissions;
	std::shared_ptr< WorldAdmission > m_active_admission;
	std::shared_ptr< WorldAdmission > m_failed_admission;
	std::shared_ptr< OwnerTransition > m_owner_transition;
	std::vector< std::shared_ptr< OwnerTransition > > m_owner_transition_handoffs;
	std::string m_failed_admission_error;
	static world_ticket_t ReserveWorldTicket();
	pending_initialization_t EnqueueInitialization( State* state, world_ticket_t previous, std::optional< std::string > map_path = {} ); // admission mutex held
	void CancelAdmission( const std::shared_ptr< WorldAdmission >& admission ); // admission mutex held
	void RetireWorld( world_ticket_t world, world_ticket_t pending_initialization ); // admission mutex held
	void ReleaseAdmissionRequest( common::mt_id_t request );
	bool MatchesActiveAdmission( world_ticket_t ticket ) const; // admission mutex held
	void InitGame( const std::shared_ptr< WorldAdmission >& admission, MT_Response& response, MT_CANCELABLE );
	bool OwnerBlocksNative() const;
	bool HasInitializationFailure() const;
	bool PublishInitializationFailure();
	bool AdvanceInitialization();
	void PublishOwnerTransition( const std::shared_ptr< WorldAdmission >& admission, StartupStage stage );
	void RunOwnerTransition( GSE_CALLABLE, const std::shared_ptr< OwnerTransition >& transition );
	void FinishOwnerTransition( const std::shared_ptr< OwnerTransition >& transition );
	void SetStartupStage( const std::shared_ptr< WorldAdmission >& admission, StartupStage stage );
	void RefreshInitializationContext( const std::shared_ptr< WorldAdmission >& admission );
	void InitializeConfigure( GSE_CALLABLE );
	void PrepareInitializationBindings();
	void AcquireInitializationMap();
	void PrepareInitializationMap();
	void PublishInitializationMap( map::Map*& candidate );
	void DiscardInitializationMap( map::Map*& candidate );
	void InitializeMap();
	void StartInitializedWorld( GSE_CALLABLE );
	bool DecodeInitializationDownload( GSE_CALLABLE, connection::Client* connection, const std::string& bytes );
	std::string SerializeWorldSnapshot() const;
	void ConfigureWorldDownload( connection::Client* connection );
	struct BoundSimulation {
		faction::SmacSimulationState value;
		roster_epoch_t epoch;
		State::game_session_t session;
		std::vector< player_identity_t > players; // in value.roster order
	};
	// Accessed only under the existing owner/native exclusion. Identity tokens
	// own no Player and require no GC roots; retirement makes them invalid.
	std::optional< BoundSimulation > m_simulation, m_pending_simulation;
	// Injectable word source for the GLSMAC new-world policy; never used by restore.
	std::function< uint32_t() > m_gameplay_entropy_word_source;
	BoundSimulation BindSimulation( faction::SmacSimulationState value ) const;
	void ValidateSimulation( const BoundSimulation& value ) const;
	unit::SmacCustomDesign ResolveConventionalDesign( const unit::ConventionalUnitState& state, const BoundSimulation& simulation ) const;
	void PrepareNewSimulation();
	void PublishPendingSimulation();
	void ValidateCurrentSimulation() const;
	unit::ConventionalUnitContext ResolveConventionalUnit( const unit::ConventionalUnitState& unit, const slot::Slot* owner ) const;
	unit::ConventionalUnitContext ResolveConventionalUnit( const unit::ConventionalUnitState& unit, const slot::Slot* owner, const BoundSimulation& simulation, bool defer_home = false ) const;
	void WriteSimulationState( types::Buffer& buffer ) const;
	std::optional< BoundSimulation > ReadSimulationState( types::Buffer& buffer, size_t turn ) const;
	void RefreshSimulationForTurn( size_t turn );
	void RequireConventionalStartupClaim() const;
	unit::ConventionalUnitState PrepareStartingConventionalUnit( const slot::Slot* owner, unit::SmacMoraleMode mode ) const;
	unit::ConventionalUnitContext ResolveConventionalIdentity( const unit::ConventionalUnitState& unit, const slot::Slot* owner, const BoundSimulation& simulation ) const;
	const faction::SmacSimulationParticipant& RequireSimulationParticipant( const Player* player ) const;
	const faction::SmacSimulationParticipant& RequireSmacProductionOwner( const slot::Slot* owner ) const;
	gse::Value* CopyTechnologyState( GSE_CALLABLE, const Player* player ) const;
	gse::Value* CopySocialState( GSE_CALLABLE, const Player* player ) const;
	gse::Value* CopyStartupDesigns( GSE_CALLABLE, const Player* player ) const;
	void DeserializeWorldSnapshotBody( GSE_CALLABLE, types::Buffer& body, const std::string& map_payload, const util::random::state_t& random_state, const world_snapshot::TurnState& turn_state, std::optional< BoundSimulation > simulation, std::optional< map::GlsmacResourceMapPolicy > map_policy, std::optional< map::SmacWorkerKnowledge > worker_knowledge, std::optional< map::GlsmacTerritoryTopology > territory = std::nullopt );
	map::SmacWorkerMapFrame ProjectSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge, int32_t observer, const base::Base* pending_base = nullptr, bool startup = false ) const;
	std::optional< map::SmacWorkerKnowledge > PrepareBaseTerritory( const base::Base* added, const base::Base* removed, bool attached ) const;
	void CommitBaseTerritory( std::optional< map::SmacWorkerKnowledge >& prepared ) noexcept;
	void ValidateSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge ) const;
	void CompleteSmacWorkerKnowledgeRestore();
	void RequireConventionalSightWorld() const;
	std::optional< map::SmacWorkerKnowledge > PrepareConventionalZoc( const unit::Unit& unit, const map::tile::Tile* destination ) const;
	std::optional< map::SmacWorkerKnowledge > PrepareConventionalSight( uint64_t id, int32_t owner, const map::tile::Tile* destination, const map::tile::Tile* source, bool startup = false ) const;
	void CommitConventionalSight( std::optional< map::SmacWorkerKnowledge >& prepared ) noexcept;
	bool HandleConventionalMoveEvent( GSE_CALLABLE, const std::string& name, const gse::value::object_properties_t& args, size_t caller, bool apply );
	void ResetGame();
	void ClearUntransferredMapData();
	struct DetachedMapPresentation;
	void DetachMapPresentation( map::Map* map, response_map_data_t*& response, DetachedMapPresentation& detached );
	void RetireMap( map::Map*& map, DetachedMapPresentation& detached );
	void RetireOldMap();
	void WithNativeRetirement( const std::function< void() >& f );

	// seed needs to be consistent during session (to prevent save-scumming and for easier reproduction of bugs)
	Random* m_random = nullptr;
	// Install only after map presentation and deferred instance reconstruction.
	std::optional< util::random::state_t > m_pending_snapshot_random;
	struct PendingSnapshotTurn {
		world_snapshot::TurnState state;
		struct Binding { slot::Slot* slot; gse::value::Object* player; };
		std::vector< Binding > bindings;
	};
	mutable std::mutex m_snapshot_turn_mutex;
	std::optional< PendingSnapshotTurn > m_pending_snapshot_turn;
	void StageSnapshotTurn( GSE_CALLABLE, const world_snapshot::TurnState& state );
	void DiscardSnapshotTurn();
	void ValidateSnapshotBindings( const PendingSnapshotTurn& pending ) const;
	void ValidatePendingSnapshotTurn();
	std::optional< size_t > ApplySnapshotTurn();
	State* m_state = nullptr;
	// Borrowed until Stop has invalidated every native wrapper, even when an
	// initialization failure has cleared State and retained an old Map.
	gc::Space* m_world_space = nullptr;

	map::Map* m_map = nullptr;
	map::Map* m_old_map = nullptr; // to restore state, for example if loading of another map failed

	turn::Turn m_current_turn = {};

	bool m_is_turn_complete = false;
	turn::turn_status_t m_turn_status = turn::TS_PLEASE_WAIT;
	bool IsLocalTurnComplete() const;
	void CheckTurnComplete();

	std::unordered_map< std::string, event::EventHandler* > m_event_handlers = {};
	common::Mutex m_event_handlers_mutex;

	std::vector< event::Event* > m_pending_events = {};
	// Bridges extraction to the Space queue without holding this mutex while queueing.
	std::vector< std::shared_ptr< gc::Object > > m_event_batch_handoffs;
	common::Mutex m_pending_events_mutex;

	struct event_waiting_for_response_t {
		event::Event* event;
		bool was_applied;
		gse::Value* rollback_data;
	};
	std::unordered_map< std::string, event_waiting_for_response_t > m_events_waiting_for_responses = {};
	common::Mutex m_events_waiting_for_responses_mutex;

	struct pending_event_response_t {
		std::string event_id;
		bool is_accepted;
		gse::Value* resolved;
	};
	std::vector< pending_event_response_t > m_pending_event_responses = {};
	common::Mutex m_pending_event_responses_mutex;

	std::atomic< uint64_t > m_next_event_id = 0;

	std::atomic< size_t > m_rw_counter = 0;
	void WithRW( const std::function< void() >& f );

	void InitComplete( GSE_CALLABLE );
	void InitFailed( const std::string& error_text );

	void SetTurnStatus( const backend::turn::turn_status_t status );
	const std::string& GetTurnStatusString() const;

	void ProcessEvents();
	bool HandleSmacWorkerEvent( GSE_CALLABLE, const std::string& name, const gse::value::object_properties_t& args, size_t caller, bool apply );
	class EventBatchRoots;
	struct EventBatchHandoff {
		EventBatchHandoff( Game* game, State* state, State::game_session_t session, std::shared_ptr< EventBatchRoots > roots );
		~EventBatchHandoff();
		EventBatchHandoff( const EventBatchHandoff& ) = delete;
		EventBatchHandoff& operator=( const EventBatchHandoff& ) = delete;
		Game* const game;
		State* const state;
		const State::game_session_t session;
		const std::shared_ptr< EventBatchRoots > roots;
		bool published = false;
	};
	std::unique_ptr< EventBatchHandoff > ExtractEventBatch( State* state, const State::game_session_t& session );
	void DispatchEventBatch( std::unique_ptr< EventBatchHandoff > handoff );

private:
	friend class map::tile::TileManager;
	friend class resource::ResourceManager;
	friend class unit::UnitManager;
	friend class base::BaseManager;
	friend class base::Base;
	friend class animation::AnimationManager;
	void AddFrontendRequest( const FrontendRequest& request );

	const bool IsRunning() const;
	const bool IsMapReady() const;

private:
	friend class event::Event;
	const std::string GenerateEventId();

private:
	friend class ::GLSMAC;
	friend class gse::tests::EngineShutdownFixture;
	void SetState( State* const state );

};

}
}
