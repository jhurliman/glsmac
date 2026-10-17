#include "SmacFacilityIdentity.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/base/PopDef.h"
#include "gse/value/Object.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include <limits>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
const RenderInfo render{ "fixture", 1, 2, 3, 4 };
bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }
std::unique_ptr< FacilityDef > Decode( const std::string& bytes ) { types::Buffer input( bytes ); return std::unique_ptr< FacilityDef >( FacilityDef::Deserialize( input ) ); }
types::Buffer Prefix() {
	types::Buffer result; result.WriteString( "port-id" ); result.WriteString( "Repeated name" ); result.WriteString( "Description" ); result.WriteInt( 40 );
	result.WriteString( "fixture" ); for ( int i = 1; i <= 4; ++i ) { result.WriteInt( i ); } return result;
}
struct CountedFacility : FacilityDef {
	CountedFacility( int& destroyed, const std::string& id, std::optional< int32_t > source )
		: FacilityDef( id, "Repeated name", "Description", 40, render, source ), destroyed( destroyed ) {}
	~CountedFacility() override { ++destroyed; }
	int& destroyed;
};
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& method, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( method ) )->Run( GSE_CALL, args );
}
Value* Definition( GSE_CALLABLE, Value* source ) {
	return VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
		{ "name", VALUE( value::String,, "Arbitrary display name" ) }, { "description", VALUE( value::String,, "Description" ) },
		{ "cost", VALUE( value::Int,, 40 ) }, { "smac_source_id", source },
		{ "render", VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
			{ "type", VALUE( value::String,, "sprite" ) }, { "file", VALUE( value::String,, "fixture" ) },
			{ "x", VALUE( value::Int,, 1 ) }, { "y", VALUE( value::Int,, 2 ) }, { "w", VALUE( value::Int,, 3 ) }, { "h", VALUE( value::Int,, 4 ) }
		} ) ) }
	} ) );
}
}

void AddSmacFacilityIdentityTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC facility identities preserve all catalog positions and reject malformed suffixes", GT() {
		for ( int32_t id = 0; id <= 71; ++id ) {
			const auto source = id ? std::optional< int32_t >( id ) : std::nullopt;
			FacilityDef original( "port-id", "Repeated name", "Description", 40, render, source );
			auto expected = Prefix(); expected.WriteBool( id != 0 ); if ( id ) { expected.WriteInt( id ); }
			const auto bytes = FacilityDef::Serialize( &original ).ToString(); const auto decoded = Decode( bytes );
			GT_ASSERT( bytes == expected.ToString() && decoded->m_smac_source_id == source && decoded->m_id == "port-id"
				&& FacilityDef::Serialize( decoded.get() ).ToString() == bytes, "positional identity or legacy absence changed" );
		}
		auto valid = Prefix(); valid.WriteBool( true ); valid.WriteInt( 26 ); const auto bytes = valid.ToString();
		for ( size_t end = 0; end < bytes.size(); ++end ) { GT_ASSERT( Rejects( [ & ] { Decode( bytes.substr( 0, end ) ); } ), "truncated facility accepted" ); }
		for ( const auto id : { int64_t( -1 ), int64_t( 0 ), int64_t( 72 ), int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1, std::numeric_limits< int64_t >::max() } ) {
			auto bad = Prefix(); bad.WriteBool( true ); bad.WriteInt( id );
			GT_ASSERT( Rejects( [ & ] { Decode( bad.ToString() ); } ), "invalid source narrowed or accepted" );
		}
		for ( const auto id : { INT32_MIN, -1, 0, 72, INT32_MAX } ) { GT_ASSERT( Rejects( [ & ] { FacilityDef bad( "x", "x", "x", 0, render, id ); } ), "constructor accepted invalid source" ); }
		types::Buffer invalid_bool; invalid_bool.WriteBool( false ); auto flag = invalid_bool.ToString(); flag.at( flag.size() - 2 ) = 2; flag.back() = 2;
		GT_ASSERT( Rejects( [ & ] { Decode( Prefix().ToString() + flag ); } ) && Rejects( [ & ] { Decode( bytes + 'x' ); } ), "invalid boolean or trailing bytes accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC facility manager binds frozen positions with precise ownership and independent names", GT() {
		for ( const auto ruleset : { "unselected", "smac", "smax" } ) {
			ConventionalUnitFixture fixture;
			if ( std::string( ruleset ) != "unselected" ) { auto& settings = fixture.Game()->GetState()->m_settings.global; settings.SelectRuleset( ruleset ); if ( std::string( ruleset ) == "smax" ) { settings.InitializeHostRules( smac_rules_fixture::Source ); } }
			int destroyed = 0; auto candidate = std::make_unique< CountedFacility >( destroyed, "unbound", 1 );
			GT_ASSERT( Rejects( [ & ] { fixture.Game()->GetBM()->DefineFacility( candidate.get() ); } ) && !fixture.Game()->GetBM()->GetFacilityDef( "unbound" ) && destroyed == 0, "missing frozen SMAC profile admitted or ownership consumed" );
		}
		int destroyed = 0; ConventionalUnitFixture fixture; fixture.Configure(); auto* manager = fixture.Game()->GetBM();
		manager->DefinePop( new PopDef( "shared-id", "Worker", {}, {}, PopDef::PF_NONE ) );
		for ( int32_t id = 1; id <= 71; ++id ) {
			const auto name = id == 1 ? "shared-id" : "facility-" + std::to_string( id );
			auto candidate = std::make_unique< CountedFacility >( destroyed, name, id ); manager->DefineFacility( candidate.get() ); candidate.release();
			manager->ValidateSmacFacilityDef( manager->GetFacilityDef( name ) );
		}
		for ( const bool repeated_role : { false, true } ) {
			auto candidate = std::make_unique< CountedFacility >( destroyed, repeated_role ? "other-id" : "shared-id", repeated_role ? std::optional< int32_t >( 1 ) : std::nullopt );
			GT_ASSERT( Rejects( [ & ] { manager->DefineFacility( candidate.get() ); } ) && !manager->GetFacilityDef( "other-id" ) && manager->GetFacilityDef( "shared-id" )->m_smac_source_id == 1, "duplicate ID or source changed owner" );
		}
		GT_ASSERT( destroyed == 2 && manager->GetPopDef( "shared-id" ), "failed definitions leaked or pop namespace collided" );
		for ( int32_t id = 1; id <= 71; ++id ) { manager->UndefineFacility( id == 1 ? "shared-id" : "facility-" + std::to_string( id ) ); }
		GT_ASSERT( destroyed == 73 && Rejects( [ & ] { manager->UndefineFacility( "shared-id" ); } ), "undefine failed exact ownership release" );
		GT_OK();
	} );
	task->AddTest( "SMAC facility native binding rejects wide types without state or RNG changes", GT() {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* manager = fixture.Game()->GetBM(); auto* object = static_cast< value::Object* >( manager->Wrap( GSE_CALL ) );
			const auto rng = fixture.GameplayRandom(); types::Buffer before; manager->Serialize( before );
			const std::vector< Value* > invalid{ VALUE( value::Bool,, true ), VALUE( value::String,, "1" ), VALUE( value::Undefined ),
				VALUE( value::Int,, -1 ), VALUE( value::Int,, 0 ), VALUE( value::Int,, 72 ), VALUE( value::Int,, int64_t( INT32_MAX ) + 1 ) };
			for ( auto* source : invalid ) {
				GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, object, "define_facility", { VALUE( value::String,, "attempt" ), Definition( GSE_CALL, source ) } ); } ), "invalid GSE source accepted" );
				types::Buffer after; manager->Serialize( after ); GT_ASSERT( before.ToString() == after.ToString() && fixture.GameplayRandom() == rng, "rejected definition changed state" );
			}
			Call( GSE_CALL, object, "define_facility", { VALUE( value::String,, "renamed" ), Definition( GSE_CALL, VALUE( value::Int,, 26 ) ) } );
			GT_ASSERT( manager->GetFacilityDef( "renamed" )->m_smac_source_id == 26 && fixture.GameplayRandom() == rng, "native binding inferred a name or consumed RNG" );
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC facility manager restore rejects malformed later definitions without dangling ownership", GT() {
		for ( int scenario = 0; scenario < 5; ++scenario ) {
			ConventionalUnitFixture fixture; fixture.Configure(); const auto rng = fixture.GameplayRandom();
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				FacilityDef first( "first", "Repeated name", "Description", 40, render, 1 );
				auto second = Prefix(); second.WriteBool( true ); second.WriteInt( scenario == 1 ? 0 : 1 );
				auto bytes = second.ToString();
				if ( scenario == 2 ) { bytes += 'x'; }
				if ( scenario == 3 ) { bytes.pop_back(); }
				if ( scenario == 4 ) { FacilityDef duplicate( "first", "Different name", "Description", 40, render ); bytes = FacilityDef::Serialize( &duplicate ).ToString(); }
				types::Buffer input; input.WriteString( "GLSMAC_BASES_V7" ); input.WriteInt( 0 ); input.WriteInt( 2 );
				input.WriteString( FacilityDef::Serialize( &first ).ToString() ); input.WriteString( bytes ); input.WriteInt( 0 ); input.WriteInt( 1 );
				auto* manager = fixture.Game()->GetBM();
				GT_ASSERT( Rejects( [ & ] { manager->Deserialize( GSE_CALL, input ); } ), "malformed or duplicate later facility accepted" );
				GT_ASSERT( manager->GetFacilityDef( "first" ) && manager->GetFacilityDef( "first" )->m_smac_source_id == 1 && !manager->GetFacilityDef( "port-id" )
					&& manager->GetBases().empty() && fixture.GameplayRandom() == rng, "failed later record corrupted earlier ownership or world state" );
				manager->UndefineFacility( "first" );
				GT_OK();
			} ); GT_ASSERT( error.empty(), "late definition restore: " + error );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC facility default producer and actual base restore preserve explicit roles", GT() {
		ConventionalUnitFixture source; source.Configure();
		const auto prepared = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			static_cast< value::Callable* >( source.Script( GSE_CALL, "./GLSMAC_data/tests/_smac_facility_identity" ) )->Run( GSE_CALL, { source.Game()->Wrap( GSE_CALL ) } );
			auto* manager = source.Game()->GetBM();
			GT_ASSERT( manager->GetFacilityDef( "HEADQUARTERS" )->m_smac_source_id == 1 && manager->GetFacilityDef( "RECYCLING_TANKS" )->m_smac_source_id == 3, "actual default producer omitted source roles" );
			auto* owner = &source.Game()->GetState()->m_slots->GetSlot( 0 );
			auto base = std::make_unique< Base >( source.Game(), 701, owner, owner->GetPlayer()->GetFaction(), source.Game()->GetMap()->GetTile( 2, 2 ), "Facility restore",
				Base::facilities_t{ { "HEADQUARTERS", manager->GetFacilityDef( "HEADQUARTERS" ) }, { "RECYCLING_TANKS", manager->GetFacilityDef( "RECYCLING_TANKS" ) } }, Base::pops_t{} );
			manager->SpawnBase( GSE_CALL, base.get() ); base.release();
			GT_ASSERT( Rejects( [ & ] { manager->UndefineFacility( "HEADQUARTERS" ); } ) && manager->GetBase( 701 )->m_facilities.at( "HEADQUARTERS" ) == manager->GetFacilityDef( "HEADQUARTERS" ), "installed facility was invalidated" );
			GT_OK();
		} ); GT_ASSERT( prepared.empty(), "facility setup: " + prepared );
		types::Buffer snapshot; source.Game()->GetBM()->Serialize( snapshot );
		ConventionalUnitFixture restored; restored.Configure();
		const auto rng = restored.GameplayRandom();
		const auto error = restored.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* manager = restored.Game()->GetBM(); types::Buffer input( snapshot.ToString() ); manager->Deserialize( GSE_CALL, input );
			const auto* base = manager->GetBase( 701 ); GT_ASSERT( base && input.lenr == input.lenw, "actual manager did not restore base" );
			GT_ASSERT( base->m_facilities.at( "HEADQUARTERS" ) == manager->GetFacilityDef( "HEADQUARTERS" ) && base->m_facilities.at( "HEADQUARTERS" )->m_smac_source_id == 1
				&& base->m_facilities.at( "RECYCLING_TANKS" )->m_smac_source_id == 3 && restored.GameplayRandom() == rng, "restored facilities lost positional identity or changed RNG" );
			GT_ASSERT( Base::Serialize( base ).ToString() == Base::Serialize( source.Game()->GetBM()->GetBase( 701 ) ).ToString(), "base record changed on restore" );
			manager->DespawnBase( GSE_CALL, 701 ); manager->UndefineFacility( "HEADQUARTERS" ); manager->UndefineFacility( "RECYCLING_TANKS" );
			GT_OK();
		} ); GT_ASSERT( error.empty(), "facility restore: " + error );
		ConventionalUnitFixture expansion; auto& settings = expansion.Game()->GetState()->m_settings.global; settings.SelectRuleset( "smax" ); settings.InitializeHostRules( smac_rules_fixture::Source );
		return expansion.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			static_cast< value::Callable* >( expansion.Script( GSE_CALL, "./GLSMAC_data/tests/_smac_facility_identity" ) )->Run( GSE_CALL, { expansion.Game()->Wrap( GSE_CALL ) } );
			GT_ASSERT( !expansion.Game()->GetBM()->GetFacilityDef( "HEADQUARTERS" )->m_smac_source_id && !expansion.Game()->GetBM()->GetFacilityDef( "RECYCLING_TANKS" )->m_smac_source_id, "SMAX inherited SMAC identities" );
			GT_OK();
		} );
	} );
}
}
