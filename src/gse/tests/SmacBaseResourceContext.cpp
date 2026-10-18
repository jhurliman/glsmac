#include "SmacBaseResourceContext.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slots.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Callable.h"
#include "gse/value/Array.h"
#include "gse/callable/Native.h"
#include "util/FinallyGuard.h"
#include <array>
#include <functional>
#include <memory>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;

bool Rejects( const std::function< void() >& f ) { try { f(); } catch ( const std::exception& ) { return true; } return false; }
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}

SmacBaseResourceContext Record() { return { 1, true, 37, -7, 0x8f000041u, true }; }

struct ObservedPopDef final : PopDef {
	ObservedPopDef( size_t& destroyed, const std::string& id, uint8_t role )
		: PopDef( id, "Observed", {}, {}, PF_NONE, role ), destroyed( destroyed ) {}
	~ObservedPopDef() override { ++destroyed; }
	size_t& destroyed;
};

Base* MakeBase( GSE_CALLABLE, Game* game ) {
	auto* bm = game->GetBM();
	// Neither display text nor definition ID is the positional identity.
	bm->DefinePop( new PopDef( "ALPHA", "Not a Doctor", {}, {}, PopDef::PF_NONE, 1 ) );
	bm->DefinePop( new PopDef( "BETA", "Not a Technician", {}, {}, PopDef::PF_NONE, 0 ) );
	bm->DefinePop( new PopDef( "DOCTOR", "Doctor", {}, {}, PopDef::PF_NONE ) );
	bm->DefinePop( new PopDef( "LABOR", "Worker", {}, {}, PopDef::PF_TILE_WORKER ) );
	auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
	auto* result = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Context", {},
		{ { 1, Pop( nullptr, 1, bm->GetPopDef( "ALPHA" ), 0 ) } }, 2, 91, std::nullopt, Record() );
	bm->SpawnBase( GSE_CALL, result ); return result;
}

types::Buffer Frame( int64_t version, int64_t owner, int64_t events, int64_t timer, int64_t governor ) {
	types::Buffer b; b.WriteInt( version ); b.WriteBool( true ); b.WriteInt( owner ); b.WriteBool( true );
	b.WriteInt( events ); b.WriteInt( timer ); b.WriteInt( governor ); b.WriteBool( true ); return b;
}
}

void AddSmacBaseResourceContextTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC Base PopDef ownership transfers only after frontend publication", []( GSE*, context::Context* ) -> std::string {
		size_t destroyed = 0;
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* bm = fixture.Game()->GetBM();
			// The external copied-TU control changes only this expectation and makes
			// the real queue submission throw once for this unique definition ID.
			constexpr bool expect_late_failure = false;
			const std::string id = "LATE_POP_PUBLICATION_CONTROL";
			auto candidate = std::make_unique< ObservedPopDef >( destroyed, id, 1 );
			const auto before = fixture.QueuedFrontendRequests().size();
			const bool failed = Rejects( [ & ] { bm->DefinePop( candidate.get() ); } );
			const bool published = bm->GetPopDef( id ) == candidate.get();
			// On a broken old implementation the manager retained the candidate.
			// Relinquish the test owner before reporting that invariant, so cleanup
			// does not depend on a double delete or allocator reuse to detect it.
			if ( published ) { candidate.release(); }
			GT_ASSERT( failed == expect_late_failure, "late publication control did not reach its expected branch" );
			GT_ASSERT( published == !failed && fixture.QueuedFrontendRequests().size() == before + ( failed ? 0 : 1 ), "failed frontend publication retained manager ownership or queued payload" );
			candidate.reset(); bm->Clear();
			GT_ASSERT( destroyed == 1, "first candidate was not released exactly once" );
			auto retry = std::make_unique< ObservedPopDef >( destroyed, id, 0 );
			bm->DefinePop( retry.get() ); retry.release();
			auto duplicate = std::make_unique< ObservedPopDef >( destroyed, id, 1 );
			const auto count = fixture.QueuedFrontendRequests().size();
			GT_ASSERT( Rejects( [ & ] { bm->DefinePop( duplicate.get() ); } ) && bm->GetPopDef( id )->m_smac_specialist_id == 0
				&& fixture.QueuedFrontendRequests().size() == count, "duplicate definition replaced ownership or emitted a notification" );
			duplicate.reset(); bm->Clear(); GT_ASSERT( destroyed == 3, "retry/duplicate cleanup lost or repeated ownership" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Base resource context preserves status timer and independent governor bits", GT() {
		for ( int32_t owner : { 1, 4, 7 } ) {
			for ( uint8_t flags = 0; flags < 64; ++flags ) {
				for ( int32_t timer : { INT32_MIN, -7, -1, 0, 1, 10, INT32_MAX } ) {
					for ( uint32_t governor : { 0u, 0x40u, 0x80000000u, 0xffffffffu } ) {
						const SmacBaseResourceContext input{ owner, flags % 2 != 0, flags, timer, governor, flags % 3 == 0 };
						const auto bytes = SerializeSmacBaseResourceContext( input );
						GT_ASSERT( bytes.lenw == 112 && SmacBaseResourceContext::MAX_WIRE_BYTES == 154, "context frame extent changed" );
						const auto restored = DeserializeSmacBaseResourceContext( bytes );
						GT_ASSERT( restored && *restored == input && SerializeSmacBaseResourceContext( restored ).ToString() == bytes.ToString(), "restore reset or normalized persistent context" );
						GT_ASSERT( restored->GovernorActive() == ( governor >= 0x80000000u ) && restored->ManageCitizens() == ( governor == 0x40u || governor == 0xffffffffu ), "citizen and active flags became coupled" );
					}
				}
			}
		}
		GT_ASSERT( !DeserializeSmacBaseResourceContext( SerializeSmacBaseResourceContext( std::nullopt ) ), "legacy absence invented neutral context" ); GT_OK();
	} );
	task->AddTest("SMAC Base full status preserves opaque bits and migrates unknown history",GT(){
		auto input=Record();input.recent_defender_loss=true;input.retained_status_word=0xfffffff1u;input.SyncKnownStatusBits();
		const auto wire=SerializeSmacBaseResourceContext(input);
		GT_ASSERT(DeserializeSmacBaseResourceContext(wire)==input,"Full status lost opaque or named bits");
		const auto next=input.AfterUpkeep();GT_ASSERT(next.retained_status_word==(*input.retained_status_word&~8u),"Tail erased unrelated status history");
		auto bad=input;bad.retained_status_word=0;
		GT_ASSERT(Rejects([&]{SerializeSmacBaseResourceContext(bad);}),"Contradictory status history accepted");
		auto legacy=Frame(4,1,37,-7,0x8f000041u);legacy.WriteBool(false);legacy.WriteBool(false);
		GT_ASSERT(DeserializeSmacBaseResourceContext(legacy)==Record(),"V4 invented full status history");GT_OK();
	});
	task->AddTest( "SMAC conquered Base context preserves history and migrates V2 without inventing it", GT() {
		for ( const auto former : { 1, 2, 7 } ) { for ( const auto timer : { 0, 1, 50, 255 } ) {
			auto input = Record(); input.conquest = SmacConquestState{ former, static_cast<uint8_t>(timer) };
			const auto bytes = SerializeSmacBaseResourceContext(input);
			GT_ASSERT(bytes.lenw == SmacBaseResourceContext::MAX_WIRE_BYTES-14 && DeserializeSmacBaseResourceContext(bytes) == input, "conquest history did not round trip");
		} }
		const auto migrated = DeserializeSmacBaseResourceContext(Frame(2,1,37,-7,0x8f000041u));
		GT_ASSERT(migrated == Record() && !migrated->conquest, "V2 migration invented capture history");
		for ( const auto former : { 0, 1, 7, 8 } ) { for ( const auto timer : { -1, 0, 255, 256 } ) {
			auto bytes = Frame(3,1,0,0,0); bytes.WriteBool(true); bytes.WriteInt(former); bytes.WriteInt(timer);
			GT_ASSERT(Rejects([&] { DeserializeSmacBaseResourceContext(bytes); }) == (former == 0 || former == 8 || timer < 0 || timer > 255), "invalid conquest history admission");
		} }
		GT_OK();
	} );
	task->AddTest("SMAC Base defender-loss context survives save and clears only at upkeep tail",GT(){
		auto input=Record();input.recent_defender_loss=true;input.conquest=SmacConquestState{2,50};
		GT_ASSERT(DeserializeSmacBaseResourceContext(SerializeSmacBaseResourceContext(input))==input,"saved defender-loss bit was lost");
		const auto tail=input.AfterUpkeep();
		GT_ASSERT(!tail.recent_defender_loss && input.recent_defender_loss && tail.conquest->assimilation==49
			&& tail.governor_options==input.governor_options && tail.drone_riots==input.drone_riots,"upkeep mutated input or unrelated status");
		auto older=Frame(3,1,37,-7,0x8f000041u);older.WriteBool(false);
		const auto migrated=DeserializeSmacBaseResourceContext(older);
		GT_ASSERT(migrated==Record() && !migrated->recent_defender_loss,"V3 migration invented combat aftermath");GT_OK();
	});
	task->AddTest( "SMAC Base resource context rejects malformed bounded frames before publication", GT() {
		for ( const auto& frame : { Frame( 0, 1, 0, 0, 0 ), Frame( 4, 1, 0, 0, 0 ), Frame( 2, 0, 0, 0, 0 ), Frame( 2, 8, 0, 0, 0 ),
			Frame( 2, 1, -1, 0, 0 ), Frame( 2, 1, 64, 0, 0 ), Frame( 2, 1, 0, INT64_C( -2147483649 ), 0 ), Frame( 2, 1, 0, INT64_C( 2147483648 ), 0 ),
			Frame( 2, 1, 0, 0, -1 ), Frame( 2, 1, 0, 0, INT64_C( 4294967296 ) ) } ) {
			GT_ASSERT( Rejects( [ & ] { DeserializeSmacBaseResourceContext( frame ); } ), "malformed context accepted" );
		}
		const auto bytes = SerializeSmacBaseResourceContext( Record() ).ToString();
		for ( size_t size = 0; size < bytes.size(); ++size ) { GT_ASSERT( Rejects( [ & ] { DeserializeSmacBaseResourceContext( types::Buffer( bytes.substr( 0, size ) ) ); } ), "truncated context accepted" ); }
		GT_ASSERT( Rejects( [ & ] { DeserializeSmacBaseResourceContext( types::Buffer( bytes + "x" ) ); } ), "trailing context accepted" );
		types::Buffer outer; outer.WriteString( std::string(SmacBaseResourceContext::MAX_WIRE_BYTES+1,'x') );
		GT_ASSERT( Rejects( [ & ] { outer.ReadString( SmacBaseResourceContext::MAX_WIRE_BYTES ); } ) && outer.lenr == 0, "oversized context consumed its payload" );
		GT_OK();
	} );
	task->AddTest( "SMAC Base positional specialist roles retain exact source identity", GT() {
		for ( uint8_t id = 0; id < 7; ++id ) {
			const PopDef def( "SAME", "Repeated English text", {}, {}, PopDef::PF_NONE, id );
			auto bytes = PopDef::Serialize( &def ); const auto original = bytes.ToString();
			std::unique_ptr< PopDef > result( PopDef::Deserialize( bytes ) );
			GT_ASSERT( result->m_smac_specialist_id == id && PopDef::Serialize( result.get() ).ToString() == original, "specialist identity inferred from name or lost" );
		}
		GT_ASSERT( Rejects( [] { PopDef invalid( "x", "x", {}, {}, PopDef::PF_TILE_WORKER, 1 ); } ) && Rejects( [] { PopDef invalid( "x", "x", {}, {}, PopDef::PF_NONE, 7 ); } ), "worker/specialist overlap or out-of-range position admitted" ); GT_OK();
	} );
	task->AddTest( "SMAC Base real role definition admits catalog positions before use", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* bm = fixture.Game()->GetBM(); auto* wrapper = static_cast< value::Object* >( bm->Wrap( GSE_CALL ) );
			for ( int64_t position : std::array< int64_t, 6 >{ -1, 0, 1, 6, 7, INT64_MAX } ) {
				auto* def = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
					{ "name", VALUE( value::String,, "Repeated display text" ) }, { "renders_human", VALUE( value::Array ) }, { "renders_progenitor", VALUE( value::Array ) },
					{ "smac_specialist_id", VALUE( value::Int,, position ) } } ) );
				const auto id = "role" + std::to_string( position ); const auto before = fixture.QueuedFrontendRequests().size();
				const bool rejected = Rejects( [ & ] { Call( GSE_CALL, wrapper, "define_pop", { VALUE( value::String,, id ), def } ); } );
				GT_ASSERT( rejected == ( position < 0 || position > 6 ), "role definition did not enforce positional domain" );
				GT_ASSERT( rejected ? !bm->GetPopDef( id ) && fixture.QueuedFrontendRequests().size() == before
					: bm->GetPopDef( id )->m_smac_specialist_id == position && fixture.QueuedFrontendRequests().size() == before + 1, "role rejection/publication had the wrong ownership effects" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC Base invalid owner role and late restore frames leave identity and tile untouched", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			auto* owner = &game->GetState()->m_slots->GetSlot( 0 ); auto* tile = game->GetMap()->GetTile( 4, 4 );
			const auto id = Base::GetNextId(); const auto count = bm->GetBases().size(); const auto queue = fixture.QueuedFrontendRequests().size();
			for ( int scenario = 0; scenario < 3; ++scenario ) {
				auto context = Record(); if ( scenario == 0 ) { context.owner_simulation_id = 2; }
				Base::pops_t pops; if ( scenario != 0 ) { pops.emplace( 1, Pop( nullptr, 1, bm->GetPopDef( scenario == 1 ? "DOCTOR" : "LABOR" ), 0 ) ); }
				GT_ASSERT( Rejects( [ & ] { std::unique_ptr< Base > rejected( new Base( game, id, owner, owner->GetPlayer()->GetFaction(), tile, "Rejected", {}, pops, 2, 0, {}, context ) ); } ), "invalid canonical constructor admitted" );
				GT_ASSERT( !tile->base && Base::GetNextId() == id && bm->GetBases().size() == count && fixture.QueuedFrontendRequests().size() == queue, "rejected context/role constructor published effects" );
			}
			const auto saved = Base::Serialize( base ).ToString(); const auto base_id = base->m_id;
			bm->DespawnBase( GSE_CALL, base_id ); const auto after = Base::GetNextId();
			// The real Base reader reaches its final bounded nested frame after all
			// earlier fields. Neither a valid-sized bad owner nor cap+1 may construct.
			types::Buffer envelope; envelope.WriteString( SerializeSmacBaseResourceContext( Record() ).ToString() );
			types::Buffer allocation_absent; allocation_absent.WriteBool( false );
			types::Buffer cache_absent; cache_absent.WriteString( SerializeSmacBaseUpkeepCache( std::nullopt ).ToString() );
			const auto prefix = saved.substr( 0, saved.size() - envelope.lenw - allocation_absent.lenw - cache_absent.lenw );
			for ( const auto& payload : { Frame( 2, 8, 0, 0, 0 ).ToString(), SerializeSmacBaseResourceContext( Record() ).ToString() + "x" } ) {
				types::Buffer body( prefix ); body.WriteString( payload ); body.WriteBool( false ); body.WriteString( SerializeSmacBaseUpkeepCache( std::nullopt ).ToString() );
				GT_ASSERT( Rejects( [ & ] { std::unique_ptr< Base > rejected( Base::Deserialize( body, game ) ); } ) && !bm->GetBase( base_id )
					&& !game->GetMap()->GetTile( 2, 2 )->base && Base::GetNextId() == after, "late malformed body published a Base" );
				if ( payload.size() > SmacBaseResourceContext::MAX_WIRE_BYTES ) { GT_ASSERT( body.lenr == prefix.size(), "outer cap failure consumed the context frame" ); }
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC Base actual ownership assignments and restore retain canonical decisions", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game ); auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			const auto before = Base::Serialize( base ).ToString(); const auto state = base->GetSmacResourceContext();
			auto* copy = static_cast< value::Object* >( Call( GSE_CALL, wrapper, "get_smac_resource_context" ) );
			static_cast< value::Int* >( copy->Get( "governor_options" ) )->value = 0;
			GT_ASSERT( base->GetSmacResourceContext() == state && Base::Serialize( base ).ToString() == before, "read alias changed persistent state" );
			const auto rng = game->GetRandom()->GetStateString(); auto* bad = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "type", VALUE( value::String,, "DOCTOR" ) } } ) );
			GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, wrapper, "create_pop", { bad } ); } ) && Base::Serialize( base ).ToString() == before && game->GetRandom()->GetStateString() == rng, "untyped specialist changed population or RNG" );
			GT_ASSERT( Rejects( [ & ] { base->SetPopAssignment( GSE_CALL, &base->m_pops.at( 1 ), "LABOR", nullptr ); } ) && Base::Serialize( base ).ToString() == before, "worker without assignment was published" );
			base->SetPopAssignment( GSE_CALL, &base->m_pops.at( 1 ), "LABOR", game->GetMap()->GetTile( 4, 2 ) );
			GT_ASSERT( bm->GetWorkingPop( { 4, 2 } ) == &base->m_pops.at( 1 ) && base->GetSmacResourceContext() == state, "explicit assignment changed context or lookup" );
			base->SetPopAssignment( GSE_CALL, &base->m_pops.at( 1 ), "BETA", nullptr );
			GT_ASSERT( !bm->GetWorkingPop( { 4, 2 } ) && base->m_pops.at( 1 ).m_def->m_smac_specialist_id == 0, "specialist transition inferred English type or retained worker claim" );
			auto changed = state; changed.golden_age = false; changed.resource_event_timer = INT32_MIN; changed.governor_options = 0x40;
			base->SetSmacResourceContext( GSE_CALL, changed );
			auto wrong = changed; wrong.owner_simulation_id = 2;
			GT_ASSERT( Rejects( [ & ] { base->SetSmacResourceContext( GSE_CALL, wrong ); } ) && base->GetSmacResourceContext() == changed, "context setter rebound owner" );
			const auto saved = Base::Serialize( base ).ToString(); const auto id = base->m_id;
			types::Buffer snapshot; bm->Serialize( snapshot ); bm->DespawnBase( GSE_CALL, id ); bm->Clear();
			size_t founded = 0; bm->On( GSE_CALL, "base_spawn", NATIVE_CALL( &founded ) { ++founded; return VALUE( value::Undefined ); } ) );
			bm->Deserialize( GSE_CALL, snapshot ); base = bm->GetBase( id );
			GT_ASSERT( base && !founded && Base::Serialize( base ).ToString() == saved && base->GetSmacResourceContext() == changed && base->m_pops.at( 1 ).m_def->m_smac_specialist_id == 0, "actual manager restore replayed first-base reset/hook or changed specialist" );
			GT_ASSERT( Rejects( [ & ] { bm->UndefinePop( "BETA" ); } ) && bm->GetPopDef( "BETA" ), "canonical role in use was retired" );
			game->GetState()->RetireGameCallbacks();
			GT_ASSERT( Rejects( [ & ] { base->GetSmacResourceContext(); } ), "retired owner session became a current context" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Base canonical legacy growth and worker commands are explicitly inactive", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			fixture.PrepareBaseCallbacks(); auto* game = fixture.Game(); auto* base = MakeBase( GSE_CALL, game );
			auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_growth" ) ); setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ) } );
			game->GetState()->TriggerObject( game, "start", ARGS_F( game ) { { "game", game->Wrap( GSE_CALL ) } }; } );
			const auto before = Base::Serialize( base ).ToString(); game->AdvanceTurn( 2 );
			GT_ASSERT( Base::Serialize( base ).ToString() == before, "legacy growth modified canonical allocation/status" );
			auto* data = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "base", base->Wrap( GSE_CALL ) }, { "pop", base->m_pops.at( 1 ).Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( 4, 2 )->Wrap( GSE_CALL ) } } ) );
			auto* event = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "data", data }, { "caller", VALUE( value::Int,, 0 ) }, { "game", game->Wrap( GSE_CALL ) } } ) );
			for ( const auto* name : { "work_base_tile", "unwork_base_tile" } ) {
				auto* handlers = static_cast< value::Object* >( fixture.Script( GSE_CALL, std::string( "./GLSMAC_data/default/game/event/" ) + name ) );
				GT_ASSERT( Call( GSE_CALL, handlers, "validate", { event } )->type == VT_STRING && Rejects( [ & ] { Call( GSE_CALL, handlers, "apply", { event } ); } ), "legacy edit entered canonical assignment path" );
			}
			GT_ASSERT( Base::Serialize( base ).ToString() == before, "rejected canonical edit changed persistent decisions" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Base actual world body defers canonical restore until simulation publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure();
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		std::string saved, expected;
		const auto prepare = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* base = MakeBase( GSE_CALL, source.Game() ); expected = Base::Serialize( base ).ToString(); saved = source.Snapshot(); GT_OK();
		} );
		GT_ASSERT( prepare.empty(), "source canonical snapshot: " + prepare );
		for ( bool no_current : { false, true } ) {
			ConventionalUnitFixture destination; destination.Configure(); destination.RetainedMorale( 6, 6 );
			const auto result = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				size_t founded = 0; destination.Game()->GetBM()->On( GSE_CALL, "base_spawn", NATIVE_CALL( &founded ) { ++founded; return VALUE( value::Undefined ); } ) );
				destination.Restore( GSE_CALL, saved, no_current ); auto* base = destination.Game()->GetBM()->GetBase( 701 );
				GT_ASSERT( base && !founded && Base::Serialize( base ).ToString() == expected && base->GetSmacResourceContext() == Record(), "actual body restore lost context or replayed founding" ); GT_OK();
			} );
			GT_ASSERT( result.empty(), "deferred canonical restore: " + result );
		}
		GT_OK();
	} );
	for ( bool queued : { false, true } ) {
		task->AddTest( std::string( "SMAC Base first resource producer uses actual claimed " ) + ( queued ? "queued" : "immediate" ) + " Start", [ queued ]( GSE*, context::Context* ) -> std::string { return CheckSmacBaseStartupFixture( queued, 3 ); } );
	}
	task->AddTest( "SMAC Base first resource producer copies global governor defaults at all six difficulties", []( GSE*, context::Context* ) -> std::string {
		for(int32_t difficulty=0;difficulty<6;++difficulty) {
			const auto result=CheckSmacBaseStartupFixture(false,difficulty);if(!result.empty()){return result;}
		} GT_OK();
	} );
}
}
