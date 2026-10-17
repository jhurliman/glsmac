#include "CanonicalResourceIntake.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotWorkerKnowledge.h"
#include "game/backend/WorldSnapshotTerritory.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/map/tile/TileManager.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/backend/slot/Slots.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include "ui/UI.h"
#include "util/FinallyGuard.h"
#include <array>
#include <memory>
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::map::tile;
using Yield = std::array< int64_t, 3 >;
using Phase = Game::ResourceQueryPhase;

bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
Yield Triple( Value* value ) {
	const auto& object = static_cast< gse::value::Object* >( value )->value;
	return { static_cast< gse::value::Int* >( object.at( "NUTRIENTS" ) )->value,
		static_cast< gse::value::Int* >( object.at( "MINERALS" ) )->value,
		static_cast< gse::value::Int* >( object.at( "ENERGY" ) )->value };
}
struct SourceOptions {
	std::array< game::backend::resource::smac::RuleTriple, 9 > rows{{ { 1, 0, 0 }, { 2, 1, 1 }, { 2, 2, 2 }, { 1, 2, 1 },
		{ 1, 1, 1 }, { 1, 0, 0 }, { 2, 1, 3 }, { 2, 2, 2 }, { 0, 6, 6 } }};
	int32_t demand = 2;
	std::array< std::string, 4 > refs{ "Tech49", "Tech60", "Tech59", "Tech62" };
	int32_t changed_technology = -1;
	std::array< std::string, 2 > prerequisites{ "None", "None" };
};
std::function< std::string( const std::string& ) > Source( const SourceOptions& options = {} ) {
	return [ options ]( const std::string& filename ) {
		if ( filename != "alpha.txt" ) { return smac_startup_fixture::Source( filename ); }
		std::istringstream input( smac_startup_fixture::Source( filename ) );
		std::string result, line, section; size_t position = 0;
		while ( std::getline( input, line ) ) {
			if ( !line.empty() && line.front() == '#' ) { section = line; position = 0; result += line + "\n"; continue; }
			if ( section == "#RULES" ) {
				const auto replacement = position == 1 ? std::to_string( options.demand ) : position == 8 ? "1" : position == 9 ? "-1"
					: position == 53 ? options.refs[ 0 ] : position == 54 ? options.refs[ 1 ] : position == 55 ? options.refs[ 2 ] : position == 57 ? options.refs[ 3 ] : "";
				if ( !replacement.empty() ) { line.replace( 0, line.find( ',' ), replacement ); }
			}
			else if ( section == "#RESOURCEINFO" && position < 9 ) {
				line = "Repeated label"; for ( const auto value : options.rows[ position ] ) { line += "," + std::to_string( value ); }
			}
			else if ( section == "#TECHNOLOGY" ) {
				// Owned semantic source, not an installed-file substitute: only the
				// startup Ecology position has the measured fungal nutrient flag.
				line.replace( line.rfind( ',' ) + 1, std::string::npos, position == 6 ? "100000000" : "000000000" );
				if ( int32_t( position ) == options.changed_technology ) {
					std::vector< std::string > fields; std::istringstream row( line ); std::string field;
					while ( std::getline( row, field, ',' ) ) { fields.push_back( field ); }
					fields.at( 6 ) = options.prerequisites[ 0 ]; fields.at( 7 ) = options.prerequisites[ 1 ]; line.clear();
					for ( const auto& part : fields ) { if ( !line.empty() ) { line += ','; } line += part; }
				}
			}
			result += line + "\n"; ++position;
		}
		return result;
	};
}
void Terrain( Tile* tile, const elevation_t elevation = 0 ) {
	for ( auto* corner : tile->elevation.corners ) { *corner = elevation; }
	tile->Update(); tile->moisture = MOISTURE_ARID; tile->rockiness = ROCKINESS_FLAT;
	tile->features = 0; tile->terraforming = 0; tile->bonus = BONUS_NONE;
}
void Define( Game* game ) {
	for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) { game->GetRM()->DefineResource( new game::backend::resource::Resource( id, "Misleading display name", {} ) ); }
	auto* bm = game->GetBM();
	bm->DefinePop( new PopDef( "WORK", "Not an identity", {}, {}, PopDef::PF_TILE_WORKER ) );
	bm->DefinePop( new PopDef( "SPECIAL", "Not a Doctor", {}, {}, PopDef::PF_NONE, 1 ) );
	for ( const int32_t role : { 1, 3, 4, 8, 13, 14, 26, 34, 38, 39 } ) {
		bm->DefineFacility( new FacilityDef( "ROLE" + std::to_string( role ), "Recycling Tanks", "", 0, {}, role ) );
	}
	bm->DefineFacility( new FacilityDef( "HEADQUARTERS", "Headquarters", "", 0, {} ) );
}
Base* MakeBase( GSE_CALLABLE, Game* game, const size_t id = 701, Tile* center = nullptr, const bool worker = true, const bool specialist = false ) {
	auto* bm = game->GetBM(); auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
	if ( !center ) { center = game->GetMap()->GetTile( 2, 2 ); }
	Base::pops_t pops;
	if ( worker ) { pops.emplace( 1, Pop( nullptr, 1, bm->GetPopDef( "WORK" ), 0, game->GetMap()->GetTile( 4, 2 ) ) ); }
	if ( specialist ) { pops.emplace( 2, Pop( nullptr, 2, bm->GetPopDef( "SPECIAL" ), 0 ) ); }
	auto* base = new Base( game, id, owner, owner->GetPlayer()->GetFaction(), center, "Resource query", { { "ROLE1", bm->GetFacilityDef( "ROLE1" ) } },
		pops, pops.empty() ? 1 : pops.rbegin()->first + 1, 13, {}, SmacBaseResourceContext{ 1, false, 0, 0, 0x00dfbe41u, false } );
	bm->SpawnBase( GSE_CALL, base ); return base;
}
void SupportedUnit( GSE_CALLABLE, ConventionalUnitFixture& fixture, Base* base, const bool homed ) {
    using namespace game::backend::unit;
    auto* game = fixture.Game(); auto* um = game->GetUM();
    MoraleSet::morale_values_t names; for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
    um->DefineMoraleSet( new MoraleSet( "resource-morale", names ) );
    um->DefineUnit( new StaticDef( "resource-scout", um->GetMoraleSet( "resource-morale" ), "Scout", MT_LAND, 999,
        new CvrScoutRender( 2, 0, 1, 1, { 1, 1 }, { 128, 128 } ) ) );
    ConventionalUnitState record{ 2, 1, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 };
    if ( homed ) { record.home = ConventionalUnitState::Home::Base; record.home_base_id = base->m_id; }
    Terrain( game->GetMap()->GetTile( 6, 6 ) );
    um->SpawnConventionalUnit( GSE_CALL, 91, um->GetUnitDef( "resource-scout" ), base->m_owner, game->GetMap()->GetTile( 6, 6 ), record );
    fixture.ResourceState( []( auto& state ) { state.social.factions.front().current.at( 2 ) = -4; } );
}
void Callbacks( GSE_CALLABLE, ConventionalUnitFixture& fixture ) {
	fixture.PrepareBaseCallbacks();
	auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_growth" ) );
	setup->Run( GSE_CALL, { fixture.Game()->Wrap( GSE_CALL ) } );
}
Yield Query( Game* game, const Base* base, const Tile* tile, const Phase phase = Phase::Ordinary ) {
	return game->GetCanonicalTileResources( *tile, *base, phase );
}
// These managers use unordered definition maps. Sort complete owned records,
// retaining every byte of each value and every other world frame. No yield,
// authority field or payload tail is removed from the comparison.
std::string CanonicalResourceSnapshot( Game* game, const std::string& bytes ) {
	types::Buffer world( bytes ); const auto* state = game->GetState();
	world_snapshot::ValidateHeader( world, state->m_settings.global );
	(void)world_snapshot::ReadRandomState( world ); (void)world_snapshot::ReadTurnState( world, state->m_slots->GetSlots() );
	if ( world.ReadBool() ) { (void)world.ReadString(); }
	(void)map::GlsmacResourceMapPolicy::ReadOptional( world, true );
	(void)world_snapshot::ReadWorkerKnowledge( world, true );
	(void)world_snapshot::ReadTerritoryTopology( world, true ); (void)world.ReadString();
	types::Buffer result; result.WriteString( bytes.substr( 0, world.lenr ) );
	const auto named = []( types::Buffer& input, types::Buffer& output ) {
		const auto count = input.ReadInt(); if ( count < 0 || count > 1024 ) { throw std::runtime_error( "invalid fixture definition count" ); }
		std::map< std::string, std::string > records;
		for ( int64_t i = 0; i < count; ++i ) {
			const auto id = input.ReadString(), value = input.ReadString();
			if ( !records.emplace( id, value ).second ) { throw std::runtime_error( "duplicate fixture definition" ); }
		}
		output.WriteInt( count ); for ( const auto& record : records ) { output.WriteString( record.first ); output.WriteString( record.second ); }
	};
	types::Buffer resources( world.ReadString() ), canonical_resources;
	named( resources, canonical_resources ); canonical_resources.WriteString( resources.ToString().substr( resources.lenr ) );
	result.WriteString( canonical_resources.ToString() ); result.WriteString( world.ReadString() ); // complete empty Unit manager
	types::Buffer bases( world.ReadString() ), canonical_bases; canonical_bases.WriteString( bases.ReadString() );
	named( bases, canonical_bases );
	{ // Only facility definitions are unordered. BM9 Base record order is semantic.
		const auto count = bases.ReadInt(); if ( count < 0 || count > 1024 ) { throw std::runtime_error( "invalid fixture base table count" ); }
		std::map< std::string, std::string > records;
		for ( int64_t i = 0; i < count; ++i ) {
			const auto bytes = bases.ReadString(); types::Buffer payload( bytes ); const auto id = payload.ReadString();
			if ( !records.emplace( id, bytes ).second ) { throw std::runtime_error( "duplicate fixture base record" ); }
		}
		canonical_bases.WriteInt( count ); for ( const auto& record : records ) { canonical_bases.WriteString( record.second ); }
	}
	canonical_bases.WriteString( bases.ToString().substr( bases.lenr ) ); result.WriteString( canonical_bases.ToString() );
	result.WriteString( bytes.substr( world.lenr ) ); return result.ToString();
}
}

void AddCanonicalResourceIntakeTests( task::gsetests::GSETests* task ) {
    task->AddTest( "Canonical N M preparation matches admitted original mineral cases through actual Base", []( GSE*, context::Context* ) -> std::string {
        // Literal BASE-M-001,002,005..009,018,031 outputs. Remaining named
        // cases need projects/convoys and are outside this live admission.
        struct Case { int32_t raw, support; std::vector< int32_t > facilities; int32_t gross, surplus; };
        const std::vector< Case > cases{
            {5,0,{},5,5}, {5,2,{},5,3}, {5,2,{22},7,5}, {3,0,{22},4,4},
            {3,0,{22,19},6,6}, {3,0,{22,19,20},7,7}, {5,0,{21},7,7},
            {0,2,{19,20,21,22},0,-2}, {-3,0,{22},-4,-4}
        };
        const auto next = Base::GetNextId(), unit_next = unit::Unit::GetNextId();
        util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); unit::Unit::SetNextId( unit_next ); } );
        for ( const auto& c : cases ) {
            SourceOptions options; options.rows[ 1 ][ 1 ] = c.raw;
            ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
            const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
                auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) );
                auto* base = MakeBase( GSE_CALL, game, 701, nullptr, false, true );
                for ( const auto role : c.facilities ) {
                    const auto id = "MULTIPLIER" + std::to_string( role );
                    game->GetBM()->DefineFacility( new FacilityDef( id, "Misleading facility", "", 0, {}, role ) );
                    base->m_facilities.emplace( id, game->GetBM()->GetFacilityDef( id ) );
                }
                SupportedUnit( GSE_CALL, fixture, base, c.support != 0 );
                const ProductionItem item{ ProductionItemKind::Unit, "Opaque target not consulted by this stage" };
                base->SetProductionState( GSE_CALL, ProductionState{ item, 47, item, 11 } );
                const auto before = fixture.Snapshot(); const auto rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom();
                for ( int repeat = 0; repeat < 2; ++repeat ) {
                    const auto result = base->PrepareSmacResourceCalculation();
                    GT_ASSERT( result.raw_intake[ 1 ] == c.raw && result.minerals.intake == c.gross && result.minerals.demand == c.support
                        && result.minerals.surplus == c.surplus && result.minerals.pre_orbital_surplus == c.surplus, "live mineral stage differs from original literal" );
                }
                GT_ASSERT( fixture.Snapshot() == before && game->GetRandom()->GetStateString() == rng && fixture.GameplayRandom() == pair,
                    "mineral preparation posted or accumulated resources" ); GT_OK();
            } ); if ( !error.empty() ) { return error; }
        }
        GT_OK();
    } );
    task->AddTest( "Canonical N M preparation stages food normalization without collection or query mutation", []( GSE*, context::Context* ) -> std::string {
        const auto next = Base::GetNextId(), unit_next = unit::Unit::GetNextId();
        util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); unit::Unit::SetNextId( unit_next ); } );
        // Original GROW normalization boundaries; no population transition or
        // surplus collection is executed by this preparation.
        const std::array< std::array< int64_t, 4 >, 6 > cases{{
            {{0,0,-2,-1}}, {{0,1,-2,1}}, {{0,-3,-2,-3}}, {{2,-3,0,0}}, {{4,-3,2,0}}, {{4,20,2,20}}
        }}; // center N, stored before, surplus, normalized store
        for ( const auto& c : cases ) {
            SourceOptions options; options.rows[ 1 ][ 0 ] = static_cast< int32_t >( c[ 0 ] );
            ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
            const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
                auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) );
                auto* base = MakeBase( GSE_CALL, game, 701, nullptr, false, true ); base->SetAccumulatedNutrients( c[ 1 ] );
                const auto before = fixture.Snapshot(); const auto queued = fixture.QueuedFrontendRequests().size();
                const auto result = base->PrepareSmacResourceCalculation();
                GT_ASSERT( result.nutrients.intake == c[ 0 ] && result.nutrients.demand == 2 && result.nutrients.surplus == c[ 2 ]
                    && result.nutrients.stored_before == c[ 1 ] && result.nutrients.normalized_stored == c[ 3 ], "wrong staged food normalization" );
                auto* value = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_smac_resource_calculation" ) );
                auto* nutrients = static_cast< value::Object* >( value->Get( "nutrients" ) );
                static_cast< value::Int* >( nutrients->Get( "normalized_stored" ) )->value = 999;
                GT_ASSERT( base->PrepareSmacResourceCalculation().nutrients.normalized_stored == c[ 3 ] && fixture.Snapshot() == before
                    && fixture.QueuedFrontendRequests().size() == queued, "UI query normalized, collected or exposed mutable owned state" ); GT_OK();
            } ); if ( !error.empty() ) { return error; }
        }
        GT_OK();
    } );
    task->AddTest( "Canonical N M preparation composes tile orbital facilities support phase and saved target", []( GSE*, context::Context* ) -> std::string {
        const auto next = Base::GetNextId(), unit_next = unit::Unit::GetNextId();
        util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); unit::Unit::SetNextId( unit_next ); } );
        ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
        return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
            auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) );
            auto* forest = game->GetMap()->GetTile( 4, 2 ); Terrain( forest ); forest->terraforming = TERRAFORMING_FOREST;
            auto* base = MakeBase( GSE_CALL, game, 701, nullptr, true, true ); SupportedUnit( GSE_CALL, fixture, base, true );
            for ( const int role : { 19, 22 } ) {
                const auto id = "MULTIPLIER" + std::to_string( role ); game->GetBM()->DefineFacility( new FacilityDef( id, "Other", "", 0, {}, role ) );
                base->m_facilities.emplace( id, game->GetBM()->GetFacilityDef( id ) );
            }
            fixture.ResourceState( []( auto& state ) {
                state.satellites.at( 1 ).resources = {4,4,0}; state.resource_modifiers.solar_ordinary = true; state.resource_modifiers.solar_upkeep = false;
            } );
            base->SetAccumulatedNutrients( -9 );
            const ProductionItem target{ ProductionItemKind::Orbital, "opaque accepted category" };
            base->SetProductionState( GSE_CALL, ProductionState{ target, 47, { ProductionItemKind::Stockpile, {} }, 11 } );
            const auto before = fixture.Snapshot();
            const auto ordinary = base->PrepareSmacResourceCalculation(), upkeep = base->PrepareSmacResourceCalculation( SmacBaseResourcePhase::Upkeep );
            GT_ASSERT( ordinary.raw_intake[ 0 ] == 5 && ordinary.raw_intake[ 1 ] == 5 && ordinary.nutrients.demand == 4 && ordinary.nutrients.surplus == 1
                && ordinary.nutrients.normalized_stored == 0 && ordinary.minerals.multiplier_steps == 2 && ordinary.minerals.intake == 10
                && ordinary.minerals.demand == 2 && ordinary.minerals.surplus == 8 && ordinary.minerals.pre_orbital_surplus == 8,
                "live tile/orbit/home/facility composition changed" );
            GT_ASSERT( ordinary.raw_intake[ 2 ] != upkeep.raw_intake[ 2 ] && ordinary.raw_intake[ 0 ] == upkeep.raw_intake[ 0 ]
                && ordinary.minerals.intake == upkeep.minerals.intake && fixture.Snapshot() == before, "phase was inferred or resources were posted" );
            GT_OK();
        } );
    } );
    task->AddTest( "Canonical N M preparation rejects missing authority and late overflow atomically", []( GSE*, context::Context* ) -> std::string {
        const auto next = Base::GetNextId(), unit_next = unit::Unit::GetNextId();
        util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); unit::Unit::SetNextId( unit_next ); } );
        SourceOptions options; options.rows[ 1 ][ 1 ] = INT32_MAX;
        ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
        return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
            auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) );
            auto* base = MakeBase( GSE_CALL, game, 701, nullptr, false, true ); base->SetAccumulatedNutrients( -4 );
            const auto before = fixture.Snapshot(), rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom();
            GT_ASSERT( Rejects( [ & ] { base->PrepareSmacResourceCalculation(); } ), "late multiplier arithmetic overflow accepted" );
            GT_ASSERT( Rejects( [ & ] { base->PrepareSmacResourceCalculation( static_cast< SmacBaseResourcePhase >( 99 ) ); } ), "unknown phase accepted" );
            fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable );
            const auto unsupported = fixture.Snapshot();
            GT_ASSERT( Rejects( [ & ] { base->PrepareSmacResourceCalculation(); } ) && fixture.Snapshot() == unsupported, "missing project authority acquired a neutral default" );
            fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain );
            GT_ASSERT( fixture.Snapshot() == before && game->GetRandom()->GetStateString() == rng && fixture.GameplayRandom() == pair
                && base->GetAccumulatedNutrients() == -4, "rejected calculation normalized food or changed world" ); GT_OK();
        } );
    } );

	task->AddTest( "Canonical orbital intake consumes saved owner assets and registered Aerospace role", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		std::string world; std::array< int32_t, 3 > expected{ 3, 4, 0 };
		const auto prepared = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* bm = game->GetBM();
			Terrain( game->GetMap()->GetTile( 2, 2 ) );
			bm->DefineFacility( new FacilityDef( "LAUNCH_SITE", "Misleading display", "", 0, {}, 29 ) );
			auto* owner = &game->GetState()->m_slots->GetSlot( 0 ); Base::pops_t pops;
			for ( size_t i = 1; i <= 4; ++i ) { pops.emplace( i, Pop( nullptr, i, bm->GetPopDef( "SPECIAL" ), 0 ) ); }
			auto* base = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Orbital", {}, pops, 5, 0, {}, SmacBaseResourceContext{ 1, false, 0, 0, 0x00dfbe41u, false } );
			bm->SpawnBase( GSE_CALL, base );
			Callbacks( GSE_CALL, fixture );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			GT_ASSERT( game->GetCanonicalOrbitalIntake( *base ) == ( std::array< int32_t, 3 >{ 0, 0, 0 } ), "actual new-world producer did not initialize satellite assets" );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) ) == ( Yield{ 2, 1, 1 } ), "zero satellites changed actual tile intake" );
			fixture.ResourceState( []( auto& state ) { state.satellites.at( 1 ) = { { 3, 7, 0 } }; } );
			GT_ASSERT( game->GetCanonicalOrbitalIntake( *base ) == ( std::array< int32_t, 3 >{ 2, 4, 0 } ), "actual owner counts or population cap ignored" );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) ) == ( Yield{ 4, 5, 1 } ), "actual Base intake omitted capped orbital income" );
			base->m_facilities.emplace( "LAUNCH_SITE", bm->GetFacilityDef( "LAUNCH_SITE" ) );
			const auto before = Base::Serialize( base ).ToString(); const auto pair = fixture.GameplayRandom();
			GT_ASSERT( game->GetCanonicalOrbitalIntake( *base ) == expected && game->GetCanonicalOrbitalIntake( *base ) == expected, "Aerospace positional role ignored or query consumed assets" );
			for ( size_t query = 0; query < 2; ++query ) {
				GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) ) == ( Yield{ 5, 5, 1 } ), "Base intake did not recompute Aerospace contribution exactly once" );
			}
			base->m_facilities.emplace( "HEADQUARTERS", bm->GetFacilityDef( "HEADQUARTERS" ) );
			const bool rejected = Rejects( [ & ] { game->GetCanonicalOrbitalIntake( *base ); } ); base->m_facilities.erase( "HEADQUARTERS" );
			GT_ASSERT( rejected && Base::Serialize( base ).ToString() == before && fixture.GameplayRandom() == pair, "missing facility role accepted or orbital query mutated authority/RNG" );
			world = fixture.Snapshot(); GT_OK();
		} ); if ( !prepared.empty() ) { return prepared; }
		ConventionalUnitFixture destination; destination.ConfigureResources( Source(), "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.Restore( GSE_CALL, world, true ); auto* base = destination.Game()->GetBM()->GetBase( 701 );
			Callbacks( GSE_CALL, destination );
			const auto before = destination.Snapshot();
			GT_ASSERT( base && destination.Game()->GetCanonicalOrbitalIntake( *base ) == expected, "actual deferred restore reset satellite assets or lost Aerospace role" );
			GT_ASSERT( Triple( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ) ) == ( Yield{ 5, 5, 1 } ), "restored Base intake omitted owned orbital income" );
			GT_ASSERT( destination.Snapshot() == before, "restored orbital query mutated the world" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake complete base screen shows actual values and unavailable forecasts", []( GSE*, context::Context* ) -> std::string {
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		SourceOptions options; options.rows[ 1 ] = { 4, 5, 6 }; options.rows[ 3 ] = { 2, 7, 8 }; options.demand = 7; options.refs.fill( "None" );
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) );
			auto* forest = game->GetMap()->GetTile( 4, 2 ); Terrain( forest ); forest->terraforming = TERRAFORMING_FOREST;
			auto* base = MakeBase( GSE_CALL, game ); Callbacks( GSE_CALL, fixture );
            game->GetBM()->DefineFacility( new FacilityDef( "MINERAL_FACTORY", "No name identity", "", 0, {}, 22 ) );
            base->m_facilities.emplace( "MINERAL_FACTORY", game->GetBM()->GetFacilityDef( "MINERAL_FACTORY" ) );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_consumption" ); } ), "canonical full consumption unexpectedly available" );
			auto* game_wrapper = static_cast< value::Object* >( game->Wrap( GSE_CALL ) );
			// The complete screen uses real Base/Tile/manager methods and production
			// intake callbacks. Its existing harness doubles only DOM/assets/year.
			auto* services = new value::Object( GSE_CALL, { { "get_bm", game_wrapper->Get( "get_bm" ) } } );
			auto* no_forecast = NATIVE_CALL() {
				throw std::runtime_error( "canonical screen called an unsupported forecast or allocation formatter" );
				return VALUE( value::Undefined );
			} );
			auto* ui = new value::Object( GSE_CALL, {
				{ "format_resource_balance", ::ui::UI::CreateResourceBalanceFormatter( GSE_CALL ) },
				{ "format_resource_progress", no_forecast }, { "format_placeholder_energy", no_forecast },
			} );
			auto* initialize = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_screen_resources" ) );
			auto* draw = static_cast< value::Callable* >( initialize->Run( GSE_CALL, { services, wrapper, ui } ) );
			for ( const bool production_present : { false, true, false } ) {
				const ProductionItem stockpile{ ProductionItemKind::Stockpile, {} };
				base->SetProductionState( GSE_CALL, production_present ? std::optional< ProductionState >{ ProductionState{ stockpile, 47, stockpile, 0 } } : std::nullopt );
				base->SetAccumulatedNutrients( production_present ? -4 : 13 );
				const auto before = fixture.Snapshot(); const auto rng = game->GetRandom()->GetStateString();
				const auto pair = fixture.GameplayRandom(); const auto frontend = fixture.QueuedFrontendRequests().size();
				auto* expected = new value::Object( GSE_CALL, {
					{ "canonical", VALUE( value::Bool,, true ) }, { "nutrients", VALUE( value::String,, "-001" ) },
					{ "minerals", VALUE( value::String,, "018" ) }, { "energy", VALUE( value::String,, "15" ) },
					{ "stored", VALUE( value::String,, production_present ? "Stored: -4" : "Stored: 13" ) },
					{ "production", VALUE( value::String,, production_present ? "Stored: 47" : "Stored: unavailable" ) },
				} );
				draw->Run( GSE_CALL, { expected } );
				GT_ASSERT( fixture.Snapshot() == before && game->GetRandom()->GetStateString() == rng && fixture.GameplayRandom() == pair
					&& fixture.QueuedFrontendRequests().size() == frontend, "canonical screen changed world, RNG or frontend queue" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake actual callbacks compose seven first-worker contexts", []( GSE*, context::Context* ) -> std::string {
		// FIRST-WORKER-CONTEXT-001 reduced literal outcomes; synthetic host source
		// and explicit existing assignment feed real Base/Tile callbacks. This is
		// not the original manual allocation/recalculation controller.
		const std::array< int64_t, 7 > center_energy{ 2, 1, 2, 3, 2, 2, 2 };
		const auto next = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( next ); } );
		for ( size_t index = 0; index < 7; ++index ) {
			ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), settings::SmacFactionStartupRules::Bindings()[ index ].port_id );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Define( game );
				auto* center = game->GetMap()->GetTile( 2, 2 ); auto* forest = game->GetMap()->GetTile( 4, 2 ); auto* fungal = game->GetMap()->GetTile( 3, 3 );
				Terrain( center ); Terrain( forest ); Terrain( fungal ); forest->terraforming = TERRAFORMING_FOREST; fungal->features = FEATURE_XENOFUNGUS;
				auto* base = MakeBase( GSE_CALL, game ); Callbacks( GSE_CALL, fixture );
				const auto before = Base::Serialize( base ).ToString(); const auto rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom();
				GT_ASSERT( Query( game, base, center ) == ( Yield{ 2, 1, center_energy[ index ] } ), "produced current ratings/HQ did not reach center yield" );
				GT_ASSERT( Query( game, base, forest ) == ( Yield{ 1, 2, 1 } ) && Query( game, base, fungal ) == ( Yield{ index == 0 ? 2 : 0, 0, 0 } ), "forest/fungal source ownership changed" );
				auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
				auto* intake = Call( GSE_CALL, wrapper, "get_intake" );
				GT_ASSERT( Triple( intake ) == ( Yield{ 3, 3, center_energy[ index ] + 1 } ), "actual callback did not sum center and existing worker" );
				static_cast< value::Int* >( static_cast< value::Object* >( intake )->Get( "NUTRIENTS" ) )->value = 999;
				GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) )[ 0 ] == 3, "intake result aliases authority/cache" );
				GT_ASSERT( static_cast< value::Int* >( Call( GSE_CALL, wrapper, "get_smac_nutrient_demand" ) )->value == 2, "configured nutrient demand not consumed" );
				GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_consumption" ); } ), "unsupported total support/maintenance was invented" );
				GT_ASSERT( Base::Serialize( base ).ToString() == before && game->GetRandom()->GetStateString() == rng && fixture.GameplayRandom() == pair, "query changed allocation/context or an RNG stream" );
				GT_OK();
			} ); if ( !error.empty() ) { return std::to_string( index ) + ": " + error; }
		}
		return {};
	} );
	task->AddTest( "Canonical resource intake uses frozen modified rows and scalar nutrient demand", []( GSE*, context::Context* ) -> std::string {
		SourceOptions options; options.rows[ 1 ] = { 4, 5, 6 }; options.rows[ 3 ] = { 2, 7, 8 }; options.demand = 7; options.refs.fill( "None" );
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* forest = game->GetMap()->GetTile( 4, 2 ); Terrain( forest ); forest->terraforming = TERRAFORMING_FOREST;
			auto* base = MakeBase( GSE_CALL, game ); Callbacks( GSE_CALL, fixture ); auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) ) == ( Yield{ 6, 12, 15 } ), "live path substituted installed rows or capped None technology" );
			GT_ASSERT( static_cast< value::Int* >( Call( GSE_CALL, wrapper, "get_smac_nutrient_demand" ) )->value == 7, "live demand remained hardcoded two" );
			GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake separates current social events council and solar phases", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* center = game->GetMap()->GetTile( 2, 2 ); Terrain( center ); auto* base = MakeBase( GSE_CALL, game, 701, center, false );
			fixture.ResourceState( []( auto& state ) { auto& social = state.social.factions.front(); social.current[ 0 ] = -1; social.retained->at( 0 ) = 4;
				state.resource_modifiers = { -1, false, true, 1 }; } );
			GT_ASSERT( Query( game, base, center ) == ( Yield{ 2, 1, 2 } ) && Query( game, base, center, Phase::Upkeep ) == ( Yield{ 2, 1, 6 } ), "current economy, negative dust, council or phase source changed" );
			auto context = base->GetSmacResourceContext(); context.golden_age = true; context.resource_events = SmacBaseResourceContext::BumperCrop | SmacBaseResourceContext::IndustrialBust | SmacBaseResourceContext::HeatWave;
			base->SetSmacResourceContext( GSE_CALL, context );
			GT_ASSERT( Query( game, base, center ) == ( Yield{ 3, 0, 3 } ), "Golden Age or local event axes conflated" );
			context.golden_age = false; context.resource_events = 0; base->SetSmacResourceContext( GSE_CALL, context );
			auto* second_center = game->GetMap()->GetTile( 6, 6 ); Terrain( second_center ); MakeBase( GSE_CALL, game, 702, second_center, false );
			GT_ASSERT( Query( game, base, center )[ 2 ] == 1, "actual same-owner Base count did not reach negative-economy HQ rule" );
			GT_ASSERT( Rejects( [ & ] { Query( game, base, center, static_cast< Phase >( 9 ) ); } ), "unknown phase selected an implicit solar flag" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake uses positional facility roles independent of names", []( GSE*, context::Context* ) -> std::string {
		SourceOptions options; options.refs.fill( "None" ); ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* center = game->GetMap()->GetTile( 2, 2 ); auto* forest = game->GetMap()->GetTile( 4, 2 ); Terrain( center ); Terrain( forest ); forest->terraforming = TERRAFORMING_FOREST;
			auto* base = MakeBase( GSE_CALL, game ); auto* bm = game->GetBM();
			const auto put = [ & ]( int role ) { const auto id = "ROLE" + std::to_string( role ); base->m_facilities.emplace( id, bm->GetFacilityDef( id ) ); };
			put( 4 ); put( 8 ); GT_ASSERT( Query( game, base, center ) == ( Yield{ 2, 1, 2 } ), "known non-resource local role acquired a name-derived effect" );
			put( 3 ); put( 26 ); GT_ASSERT( Query( game, base, center ) == ( Yield{ 3, 2, 3 } ), "recycling/dome double-counted or ignored" );
			put( 13 ); put( 14 ); GT_ASSERT( Query( game, base, forest ) == ( Yield{ 3, 2, 2 } ), "Tree Farm/Hybrid inputs came from cosmetic IDs" );
			for ( const auto& id : { "HEADQUARTERS", "ROLE34", "ROLE38", "ROLE39" } ) {
				base->m_facilities.emplace( id, bm->GetFacilityDef( id ) ); GT_ASSERT( Rejects( [ & ] { Query( game, base, forest ); } ), "unbound/orbital/stockpile/project facility admitted" ); base->m_facilities.erase( id );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake maps actual terrain bonuses and configured technology", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* base = MakeBase( GSE_CALL, game ); auto* tile = game->GetMap()->GetTile( 4, 2 );
			struct Case { elevation_t elevation; moisture_t moisture; rockiness_t rockiness; feature_t features; terraforming_t improvements; bonus_t bonus; Yield expected; };
			const std::array< Case, 15 > cases{{
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, 0, 0, { 0, 0, 0 } },
				{ 0, MOISTURE_MOIST, ROCKINESS_ROLLING, 0, 0, 0, { 1, 1, 0 } },
				{ 0, MOISTURE_RAINY, ROCKINESS_ROCKY, 0, 0, 0, { 0, 1, 0 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, TERRAFORMING_FOREST, 0, { 1, 2, 1 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, FEATURE_XENOFUNGUS, 0, 0, { 2, 0, 0 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, FEATURE_MONOLITH, 0, 0, { 2, 2, 2 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, TERRAFORMING_BOREHOLE, 0, { 0, 2, 2 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, 0, BONUS_NUTRIENT, { 2, 0, 0 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, 0, BONUS_MINERALS, { 0, 2, 0 } },
				{ 0, MOISTURE_ARID, ROCKINESS_FLAT, 0, 0, BONUS_ENERGY, { 0, 0, 2 } },
				{ 0, MOISTURE_RAINY, ROCKINESS_FLAT, 0, TERRAFORMING_FARM, 0, { 2, 0, 0 } },
				{ 0, MOISTURE_ARID, ROCKINESS_ROCKY, 0, TERRAFORMING_MINE | TERRAFORMING_ROAD, 0, { 0, 2, 0 } },
				{ -1000, MOISTURE_ARID, ROCKINESS_FLAT, 0, TERRAFORMING_FARM | TERRAFORMING_MINE | TERRAFORMING_SOLAR, 0, { 2, 1, 2 } },
				{ -1001, MOISTURE_ARID, ROCKINESS_FLAT, 0, 0, 0, { 1, 0, 0 } },
				{ 3000, MOISTURE_ARID, ROCKINESS_FLAT, FEATURE_RIVER, TERRAFORMING_SOLAR, 0, { 0, 0, 2 } }
			}};
			for ( size_t index = 0; index < cases.size(); ++index ) {
				const auto& input = cases[ index ]; Terrain( tile, input.elevation ); tile->moisture = input.moisture; tile->rockiness = input.rockiness;
				tile->features = input.features; tile->terraforming = input.improvements; tile->bonus = input.bonus;
				GT_ASSERT( Query( game, base, tile ) == input.expected, "terrain projection mismatch at " + std::to_string( index ) );
				const auto actual = tile->GetResources( GSE_CALL, base->m_owner, base );
				GT_ASSERT( ( Yield{ actual.at( "NUTRIENTS" ), actual.at( "MINERALS" ), actual.at( "ENERGY" ) } ) == input.expected, "native Tile callback bypassed canonical projection" );
			}
			Terrain( tile ); tile->features = FEATURE_XENOFUNGUS;
			fixture.ResourceState( []( auto& state ) { state.social.factions.front().current[ 7 ] = -1; state.social.factions.front().retained->at( 7 ) = 3; } );
			GT_ASSERT( Query( game, base, tile )[ 0 ] == 1, "fungus used retained PLANET or positive PLANET bonus" );
			fixture.ResourceState( []( auto& state ) { state.technology.factions.front().owned[ 6 ] = false; } );
			GT_ASSERT( Query( game, base, tile )[ 0 ] == 0, "fungal capability was cached across ownership change" );
			Terrain( tile, -1000 ); tile->terraforming = TERRAFORMING_MINE;
			GT_ASSERT( Query( game, base, tile )[ 1 ] == 1, "advanced platform granted before configured ownership" );
			fixture.ResourceState( []( auto& state ) { state.technology.factions.front().owned[ 62 ] = true; } );
			GT_ASSERT( Query( game, base, tile )[ 1 ] == 2, "advanced platform configured identity ignored" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake preserves configured None disabled and owned availability", []( GSE*, context::Context* ) -> std::string {
		for ( int scenario = 0; scenario < 9; ++scenario ) {
			SourceOptions options; options.refs[ 2 ] = scenario == 0 ? "None" : scenario == 1 ? "Disable" : scenario == 6 ? "Tech88" : "Tech59";
			if ( scenario == 4 || scenario == 5 ) { options.changed_technology = 59; options.prerequisites = scenario == 4 ? std::array< std::string, 2 >{ "Disable", "None" } : std::array< std::string, 2 >{ "None", "Disable" }; }
			if ( scenario >= 7 ) { options.changed_technology = 59; options.prerequisites[ 1 ] = "Unknown"; }
			ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* base = MakeBase( GSE_CALL, game ); auto* tile = game->GetMap()->GetTile( 4, 2 ); Terrain( tile, 3000 ); tile->terraforming = TERRAFORMING_SOLAR;
				fixture.ResourceState( [ & ]( auto& state ) { state.technology.factions.front().owned[ 59 ] = scenario >= 3 && scenario != 8; state.technology.factions.front().owned[ 88 ] = true; } );
				if ( scenario == 7 ) { GT_ASSERT( Rejects( [ & ] { Query( game, base, tile ); } ), "owned unknown prerequisite silently became disabled" ); GT_OK(); }
				GT_ASSERT( Query( game, base, tile )[ 2 ] == ( scenario == 0 || scenario == 3 || scenario == 5 ? 4 : 2 ), "ordinary owned-and-available boundary changed" ); GT_OK();
			} ); if ( !error.empty() ) { return std::to_string( scenario ) + ": " + error; }
		}
		return {};
	} );
	task->AddTest( "Canonical resource intake unsupported inputs reject without authority effects", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* base = MakeBase( GSE_CALL, game ); auto* tile = game->GetMap()->GetTile( 4, 2 ); Terrain( tile ); Callbacks( GSE_CALL, fixture );
			for ( int scenario = 0; scenario < 17; ++scenario ) {
				std::unique_ptr< game::backend::resource::Resource > detached_resource;
				Terrain( tile ); auto* neighbor = tile->E; const auto neighbor_flags = neighbor->terraforming;
				const auto before = Base::Serialize( base ).ToString();
				if ( scenario == 0 ) { tile->moisture = 0; }
				if ( scenario == 1 ) { tile->moisture = 255; }
				if ( scenario == 2 ) { tile->rockiness = 0; }
				if ( scenario == 3 ) { tile->rockiness = 255; }
				if ( scenario == 4 ) { tile->bonus = 255; }
				if ( scenario == 5 ) { Terrain( tile, -1001 ); tile->bonus = BONUS_ENERGY; }
				if ( scenario == 6 ) { *tile->elevation.center = 3501; }
				if ( scenario == 7 ) { tile->features = FEATURE_GARLAND_CRATER; }
				if ( scenario == 8 ) { tile->features = 0x8000; }
				if ( scenario == 9 ) { tile->terraforming = TERRAFORMING_MIRROR; }
				if ( scenario == 10 ) { tile->terraforming = 0x8000; }
				if ( scenario == 11 ) { tile->terraforming = TERRAFORMING_SOLAR; neighbor->terraforming |= TERRAFORMING_MIRROR; }
				if ( scenario == 12 ) { fixture.ResourcePolicy( false ); }
				if ( scenario == 13 ) { fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable ); }
				if ( scenario == 14 ) { fixture.ResourceState( []( auto& state ) { auto& social = state.social.factions.front(); social.phase = faction::SmacFactionSocialState::Phase::Early; social.retained.reset(); } ); }
				if ( scenario == 15 ) { base->m_facilities.emplace( "HEADQUARTERS", game->GetBM()->GetFacilityDef( "HEADQUARTERS" ) ); }
				if ( scenario == 16 ) {
					detached_resource.reset( game->GetRM()->GetDefinedResources().at( "ENERGY" ) );
					game->GetRM()->UndefineResource( "ENERGY" );
				}
				const auto pair = fixture.GameplayRandom(); const auto rng = game->GetRandom()->GetStateString(); const auto queue = fixture.QueuedFrontendRequests().size();
				GT_ASSERT( Rejects( [ & ] { Query( game, base, tile ); } ), "unsupported resource input admitted: " + std::to_string( scenario ) );
				GT_ASSERT( fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == rng && fixture.QueuedFrontendRequests().size() == queue, "rejected resource query mutated state" );
				neighbor->terraforming = neighbor_flags;
				fixture.ResourcePolicy( true ); fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain );
				if ( scenario == 14 ) { fixture.ResourceState( []( auto& state ) { auto& social = state.social.factions.front(); social.phase = faction::SmacFactionSocialState::Phase::Refreshed; social.retained = social.current; } ); }
				base->m_facilities.erase( "HEADQUARTERS" );
				if ( scenario == 16 ) { game->GetRM()->DefineResource( detached_resource.get() ); detached_resource.release(); }
				GT_ASSERT( Base::Serialize( base ).ToString() == before, "rejected resource query changed Base authority" );
			}
			Terrain( tile );
			GT_ASSERT( Rejects( [ & ] { tile->GetResources( GSE_CALL, &game->GetState()->m_slots->GetSlot( 1 ), base ); } ), "evaluating Base accepted a different caller owner" );
			// A coherent Early/turn0 state must reach the query's own phase fence,
			// independently of the malformed phase/turn combination above.
			fixture.Early(); const auto early_base = Base::Serialize( base ).ToString(); const auto early_pair = fixture.GameplayRandom();
			GT_ASSERT( Rejects( [ & ] { Query( game, base, tile ); } ) && Base::Serialize( base ).ToString() == early_base && fixture.GameplayRandom() == early_pair,
				"coherent Early query invented refreshed current resource context" );
			GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake actual world restore retains sources and decisions", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		std::string bytes, world, canonical; Yield expected{}; SmacGameplayRandomState pair( 0, 0 );
		const auto prepared = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* forest = game->GetMap()->GetTile( 4, 2 ); Terrain( forest ); forest->terraforming = TERRAFORMING_FOREST;
			auto* base = MakeBase( GSE_CALL, game ); Callbacks( GSE_CALL, fixture );
			auto context = base->GetSmacResourceContext(); context.resource_events = SmacBaseResourceContext::BumperCrop | SmacBaseResourceContext::HeatWave; context.resource_event_timer = -7; base->SetSmacResourceContext( GSE_CALL, context );
			fixture.ResourceState( []( auto& state ) { state.resource_modifiers = { -1, true, false, 1 }; state.social.factions.front().retained->at( 0 ) = 4; } );
			bytes = Base::Serialize( base ).ToString(); expected = Triple( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ) );
			pair = fixture.GameplayRandom(); world = fixture.Snapshot(); canonical = CanonicalResourceSnapshot( game, world ); GT_OK();
		} ); if ( !prepared.empty() ) { return prepared; }
		ConventionalUnitFixture destination; destination.ConfigureResources( Source(), "GAIANS" ); destination.ResourcePolicy( false );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = destination.Game(); destination.Restore( GSE_CALL, world, true ); auto* base = game->GetBM()->GetBase( 701 ); Callbacks( GSE_CALL, destination );
			const auto before_query = destination.Snapshot();
			GT_ASSERT( base && Base::Serialize( base ).ToString() == bytes && destination.GameplayRandom() == pair, "body restore replayed founding, assignment, event reset or entropy" );
			GT_ASSERT( Triple( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ) ) == expected, "restored map/context/frozen roles did not drive real callback" );
			GT_ASSERT( CanonicalResourceSnapshot( game, before_query ) == canonical, "restore changed complete resource world beyond unordered table order" );
			GT_ASSERT( destination.Snapshot() == before_query, "read-only resource query changed exact restored world bytes" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake rejects other Base centers through retained assignments", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); auto* tile = game->GetMap()->GetTile( 4, 2 ); Terrain( tile );
			auto* base = MakeBase( GSE_CALL, game ); auto* other = MakeBase( GSE_CALL, game, 702, tile, false ); Callbacks( GSE_CALL, fixture );
			const auto before = Base::Serialize( base ).ToString(); const auto other_before = Base::Serialize( other ).ToString(); const auto pair = fixture.GameplayRandom();
			GT_ASSERT( Query( game, other, tile ) == ( Yield{ 2, 1, 2 } ), "own queried Base center was rejected" );
			GT_ASSERT( Rejects( [ & ] { Query( game, base, tile ); } )
				&& Rejects( [ & ] { tile->GetResources( GSE_CALL, base->m_owner, base ); } )
				&& Rejects( [ & ] { Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ); } ), "another Base center fell through to ordinary terrain" );
			GT_ASSERT( Base::Serialize( base ).ToString() == before && Base::Serialize( other ).ToString() == other_before && fixture.GameplayRandom() == pair,
				"cross-context rejection repaired an allocation or changed authority" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake preserves signed wrapping checked tile arithmetic and wide totals", []( GSE*, context::Context* ) -> std::string {
		SourceOptions options; options.rows[ 1 ][ 0 ] = options.rows[ 3 ][ 0 ] = INT32_MAX; options.refs.fill( "None" ); options.demand = INT32_MIN;
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( options ), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* center = game->GetMap()->GetTile( 2, 2 ); Terrain( center ); auto* tile = game->GetMap()->GetTile( 4, 2 ); Terrain( tile ); tile->terraforming = TERRAFORMING_FOREST;
			auto* base = MakeBase( GSE_CALL, game, 701, nullptr, true, true ); Callbacks( GSE_CALL, fixture );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) )[ 0 ] == 4294967294LL, "intake narrowed signed64 aggregate" );
			GT_ASSERT( static_cast< value::Int* >( Call( GSE_CALL, wrapper, "get_smac_nutrient_demand" ) )->value == -4294967296LL, "signed configured demand narrowed or normalized" );
			center->bonus = BONUS_NUTRIENT;
			GT_ASSERT( Query( game, base, center )[ 0 ] == -2, "center ordered signed32 wrap was replaced with tile overflow policy" );
			tile->bonus = BONUS_NUTRIENT; const auto before = Base::Serialize( base ).ToString(); const auto pair = fixture.GameplayRandom();
			GT_ASSERT( Rejects( [ & ] { Query( game, base, tile ); } ) && Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_intake" ); } ), "ordinary overflow accepted after partial intake" );
			GT_ASSERT( Base::Serialize( base ).ToString() == before && fixture.GameplayRandom() == pair, "late arithmetic rejection changed authority" ); GT_OK();
		} );
	} );
	task->AddTest( "Canonical resource intake leaves legacy callback and consumption routes intact", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); Terrain( game->GetMap()->GetTile( 2, 2 ) ); Terrain( game->GetMap()->GetTile( 4, 2 ) );
			auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			auto* base = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Legacy", {},
				{ { 1, Pop( nullptr, 1, game->GetBM()->GetPopDef( "WORK" ), 0, game->GetMap()->GetTile( 4, 2 ) ) } }, 2 );
			game->GetBM()->SpawnBase( GSE_CALL, base ); Callbacks( GSE_CALL, fixture );
			size_t calls = 0;
			const auto callback = game->GetTM()->On( GSE_CALL, "get_tile_resources", NATIVE_CALL( &calls ) {
				++calls; return VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{
					{ "NUTRIENTS", VALUE( value::Int,, 7 ) }, { "MINERALS", VALUE( value::Int,, 8 ) }, { "ENERGY", VALUE( value::Int,, 9 ) } } ) );
			} ) );
			util::FinallyGuard remove_callback( [ & ] { game->GetTM()->Off( GSE_CALL, "get_tile_resources", callback ); } );
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_intake" ) ) == ( Yield{ 14, 16, 18 } ) && calls == 2, "legacy Tile callback replaced by canonical policy" );
			GT_ASSERT( Triple( Call( GSE_CALL, wrapper, "get_consumption" ) ) == ( Yield{ 2, 0, 0 } ), "legacy consumption changed" );
			GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_smac_nutrient_demand" ); } ), "legacy Base acquired canonical profile semantics" );
			GT_OK();
		} );
	} );
}
}
