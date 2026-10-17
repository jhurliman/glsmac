#include "SmacProductionTargets.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slots.h"
#include "gse/value/Object.h"
#include "gse/value/Array.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Callable.h"
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
std::string Source( const std::string& filename ) {
	auto source = smac_startup_fixture::Source( filename );
	if ( filename == "alpha.txt" ) {
		const auto begin = source.find( "#RULES\n" );
		const auto at = source.find( "123,", begin ); source.replace( at, 3, "10" );
	}
	return source;
}
}

void AddSmacProductionTargetsTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC production targets resolve frozen identity cost and ordinary prerequisites", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			const auto scout = game->MakeSmacProductionItem( owner, ProductionItemKind::Unit, 2 );
			const auto former = game->MakeSmacProductionItem( owner, ProductionItemKind::Unit, 1 );
			const auto before = fixture.Snapshot();
			GT_ASSERT( game->GetSmacProductionCost( owner, scout ) == 8 && game->GetSmacProductionCost( owner, former ) == 16,
				"actual Tiny/current INDUSTRY/frozen cached costs differ" );
			GT_ASSERT( game->HasSmacProductionPrerequisite( owner, scout ) && game->HasSmacProductionPrerequisite( owner, former ), "actual named ownership was not used" );
			auto renamed = scout; renamed.id = "display text is not identity";
			GT_ASSERT( game->GetSmacProductionCost( owner, renamed ) == 8 && fixture.Snapshot() == before, "opaque ID changed cost or query mutated state" );
			fixture.ResourceState( []( auto& state ) { state.technology.factions[ 0 ].owned[ 6 ] = false; state.social.factions[ 0 ].current[ 9 ] = -1; } );
			GT_ASSERT( !game->HasSmacProductionPrerequisite( owner, former ) && game->GetSmacProductionCost( owner, scout ) == 9,
				"fresh owner technology/current INDUSTRY was cached or rederived" );
			GT_ASSERT( Rejects( [ & ] { game->MakeSmacProductionItem( owner, ProductionItemKind::Unit, 63 ); } )
				&& Rejects( [ & ] { game->MakeSmacProductionItem( owner, ProductionItemKind::Facility, 38 ); } )
				&& Rejects( [ & ] { game->GetSmacProductionCost( owner, { ProductionItemKind::Unit, "smac:1:2" } ); } ),
				"missing/incorrect source identity was inferred" );
			fixture.Early();
			GT_ASSERT( Rejects( [ & ] { game->GetSmacProductionCost( owner, scout ); } ), "unclaimed Early cost was accepted" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC production targets preserve canonical queue through actual Base wrapper and restore", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		std::string snapshot; std::optional< ProductionState > expected;
		const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			const auto scout = game->MakeSmacProductionItem( owner, ProductionItemKind::Unit, 2 );
			const auto former = game->MakeSmacProductionItem( owner, ProductionItemKind::Unit, 1 );
			const ProductionState state{ scout, 7, former, -2, std::vector< ProductionItem >{ former, scout, former } };
			auto* base = new Base( game, 703, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Production", {}, {}, 1, 0, state );
			game->GetBM()->SpawnBase( GSE_CALL, base );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			auto* copy = static_cast< value::Object* >( Call( GSE_CALL, wrapper, "get_production_state" ) );
			Call( GSE_CALL, wrapper, "set_production_state", { copy } );
			GT_ASSERT( base->GetProductionState() == state, "actual wrapper lost source IDs/queue duplicates/baselines" );
			static_cast< value::Object* >( static_cast< value::Array* >( copy->Get( "queue" ) )->value[ 0 ] )->Set( "id", VALUE( value::String,, "detached" ), GSE_CALL );
			GT_ASSERT( base->GetProductionState() == state, "returned queue aliases Base authority" );
			const auto before = Base::Serialize( base ).ToString();
			for ( int scenario = 0; scenario < 5; ++scenario ) {
				auto invalid = state;
				if ( scenario == 0 ) { invalid.queue.reset(); }
				if ( scenario == 1 ) { invalid.queue->push_back( { ProductionItemKind::Unit, "unresolved" } ); }
				if ( scenario == 2 ) { invalid.queue->at( 2 ).smac_source_id = 63; }
				if ( scenario == 3 ) { invalid.queue->resize( 10, scout ); }
				if ( scenario == 4 ) { invalid.baseline_item.smac_source_id = 63; }
				GT_ASSERT( Rejects( [ & ] { base->SetProductionState( GSE_CALL, invalid ); } ) && Base::Serialize( base ).ToString() == before, "late target rejection changed Base" );
			}
			snapshot = fixture.Snapshot(); expected = state; GT_OK();
		} );
		GT_ASSERT( error.empty(), "production source: " + error );
		ConventionalUnitFixture destination; destination.ConfigureResources( Source, "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.Restore( GSE_CALL, snapshot, true );
			GT_ASSERT( destination.Game()->GetBM()->GetBase( 703 )->GetProductionState() == expected, "actual body restore reselected queue or baseline" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC production targets codec bounds queue and distinguishes empty from absent", []( GSE*, context::Context* ) -> std::string {
		const ProductionItem item{ ProductionItemKind::Unit, "opaque", 2 };
		ProductionState state{ item, INT64_MIN, item, INT64_MAX, std::vector< ProductionItem >{} };
		for ( const size_t count : { 0u, 9u } ) {
			state.queue->assign( count, item ); auto wire = SerializeProductionState( state );
			GT_ASSERT( DeserializeProductionState( wire ) == state, "logical queue/sign boundaries changed" );
		}
		const auto unchecked = [ & ]( int64_t count, bool bad_source ) {
			types::Buffer b; b.WriteBool( true );
			for ( int i = 0; i < 2; ++i ) { b.WriteInt( 1 ); b.WriteBool( true ); b.WriteInt( bad_source ? INT64_MAX : 2 ); b.WriteString( "opaque" ); b.WriteInt( 0 ); }
			b.WriteBool( true ); b.WriteInt( count ); return b;
		};
		for ( auto bad : { unchecked( -1, false ), unchecked( 10, false ), unchecked( INT64_MAX, false ), unchecked( 0, true ) } ) {
			GT_ASSERT( Rejects( [ & ] { DeserializeProductionState( bad ); } ), "malformed count/source accepted" );
		}
		auto legacy = SerializeProductionState( ProductionState{ { ProductionItemKind::Unit, "legacy" }, 1, { ProductionItemKind::Stockpile, "" }, 2 } );
		GT_ASSERT( !DeserializeProductionState( legacy )->queue, "legacy absence became an empty canonical queue" ); GT_OK();
	} );
}
}
