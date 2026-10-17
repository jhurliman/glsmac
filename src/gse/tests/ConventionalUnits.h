#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "gse/Value.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "game/backend/map/SmacWorkerKnowledge.h"
#include "game/backend/faction/SmacSimulationState.h"
#include "game/backend/settings/SmacSpecialistRules.h"

namespace task::gsetests { class GSETests; }
namespace game { class FrontendRequest; namespace backend { class Game; namespace unit { class Unit; } namespace base { class Base; } namespace map::tile { class Tile; } } }
namespace gse::tests {
class BackendFixture;
// Small adapter to the existing asset-free backend/runtime fixture. Assertions
// live in the dedicated test TU; no alternative gameplay implementation.
class ConventionalUnitFixture {
public:
	ConventionalUnitFixture();
	~ConventionalUnitFixture();
	void Configure( bool refreshed = true, int32_t road_rate = 3, bool single = false, bool selected_rover = false );
	void ConfigureMovementCampaign( bool selected_rover = true );
	void ConfigureSpecialists( const game::backend::settings::SmacSpecialistRules& rules );
	void SpecialistKnowledge( int32_t simulation, const std::array< bool, 89 >& owned );
	void ConfigureResources( const std::function< std::string( const std::string& ) >& source, const std::string& faction );
	void ConfigureProduction( const std::function< std::string( const std::string& ) >& source, const std::string& faction );
	void ProductionPresentations( const std::array< std::string, 4 >& roles );
	void ConfigureTwoResourceParticipants( const std::function< std::string( const std::string& ) >& source );
	void ResourceState( const std::function< void( game::backend::faction::SmacSimulationState& ) >& edit );
	void PostEconomy( const game::backend::base::Base& base );
	void SettleEconomy( int32_t owner );
	bool HasTechnologyReveal( int32_t owner ) const;
	std::optional< game::backend::map::SmacWorkerKnowledge > PrepareTechnologyReveal( int32_t owner ) const;
	void CommitTechnologyReveal( std::optional< game::backend::map::SmacWorkerKnowledge > value );
	void ResourcePolicy( bool present );
	void BasePresentationRoles( const std::array<std::string,8>& roles );
	void SelectStartup( int32_t simulation, int32_t design );
	void CopyRoverDesign( int32_t simulation, int32_t design, int32_t source_simulation = 2 );
	game::backend::SmacGameplayRandomState GameplayRandom() const;
	void GameplayRandom( game::backend::SmacGameplayRandomState value );
	void RetainedPlanet( int32_t retained, int32_t current );
	void MovementCapability( game::backend::faction::SmacSimulationState::MovementCapability capability );
	void MovementHistory( std::optional< game::backend::SmacMovementHistory > history );
	std::optional< game::backend::SmacMovementHistory > MovementHistory() const;
	void MoveEvent( GSE_CALLABLE, game::backend::unit::Unit* unit, game::backend::map::tile::Tile* tile, size_t caller = 0 );
	game::backend::Game* Game() const;
	std::string Run( const std::function< std::string( GSE_CALLABLE ) >& check );
	Value* Script( GSE_CALLABLE, const std::string& path );
	const std::vector< game::FrontendRequest >& Updates();
	const std::vector< game::FrontendRequest >& QueuedFrontendRequests() const;
	void PrepareBaseCallbacks();
	std::string Snapshot() const;
	void Restore( GSE_CALLABLE, const std::string& snapshot, bool no_current_simulation = false );
	void RestoreThroughStart( GSE_CALLABLE, const std::string& snapshot, const std::function< void( GSE_CALLABLE ) >& on_start );
	void QueueProbeEvent( GSE_CALLABLE, const std::function< void() >& callback );
	void DrainEvents();
	static std::string WithControllerViewport( const std::function<std::string()>& check );
	std::string CheckSelectedWorkController( game::backend::unit::Unit* actor );
	std::string CheckRetainedWorkSelection( GSE_CALLABLE, game::backend::unit::Unit* actor, game::backend::unit::Unit* scout );
	void RetainedMorale( int32_t retained, int32_t current );
	void Early();
	std::string CheckEarlyStartup( int scenario );
private:
	std::unique_ptr< BackendFixture > fixture;
};
void AddConventionalUnitTests( task::gsetests::GSETests* task );
}
