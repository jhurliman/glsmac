#include "SmacPopulationRules.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/SmacPopulationStep.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slots.h"
#include <sstream>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;

bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }

std::function< std::string( const std::string& ) > Source( int32_t multiplier = 11, int32_t complex = 7, int32_t dome = 14,
	const std::string& morgan_bonuses = {} ) {
	return [ = ]( const std::string& filename ) {
		auto text = smac_startup_fixture::Source( filename );
		if ( filename == "morgan.txt" && !morgan_bonuses.empty() ) {
			const auto first = text.find( '\n' ); const auto second = text.find( '\n', first + 1 );
			text.replace( second + 1, text.find( '\n', second + 1 ) - second - 1, morgan_bonuses );
		}
		if ( filename != "alpha.txt" ) { return text; }
		std::istringstream input( text ); std::string line, section, output; size_t row = 0;
		while ( std::getline( input, line ) ) {
			if ( !line.empty() && line[0] == '#' ) { section = line; row = 0; }
			else {
				++row;
				if ( section == "#RULES" ) {
					const auto replacement = row == 2 ? "2" : row == 6 ? std::to_string( multiplier )
						: row == 13 ? std::to_string( complex ) : row == 14 ? std::to_string( dome ) : "";
					if ( !replacement.empty() ) { line.replace( 0, line.find( ',' ), replacement ); }
				}
			}
			output += line + '\n';
		}
		return output;
	};
}

void Define( Game* game ) {
	auto* bm = game->GetBM();
	bm->DefinePop( new PopDef( "population-specialist", "Not an identity", {}, {}, PopDef::PF_NONE, 1 ) );
	for ( const int32_t role : { 2, 24, 25, 52, 63 } ) {
		bm->DefineFacility( new FacilityDef( "role" + std::to_string( role ), "Misleading facility name", "", 0, {}, role ) );
	}
	bm->DefineFacility( new FacilityDef( "unresolved", "Children's Creche", "", 0, {} ) );
}

Base* MakeBase( GSE_CALLABLE, Game* game, size_t population, const std::vector< int32_t >& facilities = {},
	size_t id = 701, int32_t x = 2, bool canonical = true ) {
	auto* bm = game->GetBM(); auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
	Base::pops_t pops;
	for ( size_t index = 1; index <= population; ++index ) { pops.emplace( index, Pop( nullptr, index, bm->GetPopDef( "population-specialist" ), 0 ) ); }
	Base::facilities_t installed;
	for ( const auto role : facilities ) { const auto name = "role" + std::to_string( role ); installed.emplace( name, bm->GetFacilityDef( name ) ); }
	const auto context = canonical ? std::optional( SmacBaseResourceContext{ 1, false, 0, 0, 0x00dfbe41u, false } ) : std::nullopt;
	auto* result = new Base( game, id, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( x, 2 ), "Population query",
		installed, pops, population + 1, 17, std::nullopt, context );
	bm->SpawnBase( GSE_CALL, result ); return result;
}
}

void AddSmacPopulationRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC live population rules use current growth local modifiers and raw frozen rules", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "MORGANITES" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* base = MakeBase( GSE_CALL, game, 4 );
			fixture.ResourceState( []( auto& state ) { state.social.factions[0].current[6] = 1; state.social.factions[0].retained->at(6) = -2; } );
			const auto before = Base::Serialize( base ).ToString(); const auto rng = fixture.GameplayRandom(); const auto updates = fixture.QueuedFrontendRequests().size();
			auto rules = game->GetCanonicalPopulationRules( *base );
			// Real source rows: multiplier11, Tiny category0, then full GROWTH1.
			GT_ASSERT( rules.cost_per_step == 8 && rules.effective_growth == 1 && rules.nutrients_per_citizen == 2
				&& !rules.housing_allows_growth && !rules.cloning_vats, "live rules used retained growth, default rules or lost Morgan penalty" );
			GT_ASSERT( Base::Serialize( base ).ToString() == before && fixture.GameplayRandom() == rng && fixture.QueuedFrontendRequests().size() == updates,
				"population rules query mutated Base, RNG or notifications" );
			base->m_facilities.emplace( "role2", game->GetBM()->GetFacilityDef( "role2" ) );
			rules = game->GetCanonicalPopulationRules( *base ); GT_ASSERT( rules.effective_growth == 3 && rules.cost_per_step == 6, "Creche source2 growth missing" );
			auto context = base->GetSmacResourceContext(); context.golden_age = true; base->SetSmacResourceContext( GSE_CALL, context );
			fixture.ResourceState( []( auto& state ) { state.social.factions[0].current[6] = 2; } );
			rules = game->GetCanonicalPopulationRules( *base ); GT_ASSERT( rules.effective_growth == 6 && rules.cost_per_step == 4, "cost clamp replaced full boom growth or lost Golden Age" );
			base->m_facilities.erase( "role2" ); rules = game->GetCanonicalPopulationRules( *base );
			GT_ASSERT( rules.effective_growth == 4 && rules.cost_per_step == 5, "Golden Age was tied to Creche presence" );
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live population housing uses independent facilities and last scalar assignment", []( GSE*, context::Context* ) -> std::string {
		for ( const auto& bonuses : { std::string(), std::string( "TECH,Tech1,POPULATION,-2,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,SOCIAL,+ECONOMY,POPULATION,3" ) } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( 10, 7, 14, bonuses ), "MORGANITES" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); Define( game ); auto* base = MakeBase( GSE_CALL, game, 4, {25} );
				GT_ASSERT( !game->GetCanonicalPopulationRules( *base ).housing_allows_growth, "Dome bypassed Complex or POPULATION was summed/dropped" );
				base->m_facilities.emplace( "role24", game->GetBM()->GetFacilityDef( "role24" ) );
				GT_ASSERT( game->GetCanonicalPopulationRules( *base ).housing_allows_growth, "Complex did not permit Morgan size4" );
				auto* larger = MakeBase( GSE_CALL, game, 11, {24}, 702, 4 );
				GT_ASSERT( !game->GetCanonicalPopulationRules( *larger ).housing_allows_growth, "Complex bypassed Dome at Morgan size11" );
				larger->m_facilities.emplace( "role25", game->GetBM()->GetFacilityDef( "role25" ) );
				GT_ASSERT( game->GetCanonicalPopulationRules( *larger ).housing_allows_growth, "Dome source25 missing" ); GT_OK();
			} );
			if ( !error.empty() ) { return error; }
		}
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source( 10, 8, 15 ), "MORGANITES" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* base = MakeBase( GSE_CALL, game, 4 );
			GT_ASSERT( game->GetCanonicalPopulationRules( *base ).housing_allows_growth, "housing substituted installed limits for frozen source values" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live population rules reject unavailable authority without publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source(), "MORGANITES" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); Define( game ); auto* base = MakeBase( GSE_CALL, game, 4 );
			const auto before = Base::Serialize( base ).ToString(); const auto updates = fixture.QueuedFrontendRequests().size();
			fixture.ResourceState( []( auto& state ) { state.social.factions[0].phase = faction::SmacFactionSocialState::Phase::Early; } );
			GT_ASSERT( Rejects( [ & ] { game->GetCanonicalPopulationRules( *base ); } ), "Early social invented refreshed growth" );
			fixture.ResourceState( []( auto& state ) { state.social.factions[0].phase = faction::SmacFactionSocialState::Phase::Refreshed; } );
			fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::Unavailable );
			GT_ASSERT( Rejects( [ & ] { game->GetCanonicalPopulationRules( *base ); } ), "missing campaign inferred no projects" );
			fixture.MovementCapability( faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain );
			base->m_facilities.emplace( "unresolved", game->GetBM()->GetFacilityDef( "unresolved" ) );
			GT_ASSERT( Rejects( [ & ] { game->GetCanonicalPopulationRules( *base ); } ), "display name supplied a facility role" ); base->m_facilities.erase( "unresolved" );
			for ( const auto role : { 52, 63 } ) {
				const auto name = "role" + std::to_string( role ); base->m_facilities.emplace( name, game->GetBM()->GetFacilityDef( name ) );
				GT_ASSERT( Rejects( [ & ] { game->GetCanonicalPopulationRules( *base ); } ), "project authority silently treated as absent" ); base->m_facilities.erase( name );
			}
			GT_ASSERT( Base::Serialize( base ).ToString() == before && fixture.QueuedFrontendRequests().size() == updates, "rejected query mutated authority" );
			auto* legacy = MakeBase( GSE_CALL, game, 1, {}, 702, 4, false );
			GT_ASSERT( Rejects( [ & ] { game->GetCanonicalPopulationRules( *legacy ); } ), "legacy context invented canonical growth status" ); GT_OK();
		} );
	} );
}
}
