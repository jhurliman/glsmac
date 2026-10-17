#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "gse/Value.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "game/backend/faction/SmacSimulationState.h"

namespace task::gsetests { class GSETests; }
namespace game { class FrontendRequest; namespace backend { class Game; namespace unit { class Unit; } namespace map::tile { class Tile; } } }
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
	void SelectStartup( int32_t simulation, int32_t design );
	void CopyRoverDesign( int32_t simulation, int32_t design, int32_t source_simulation = 2 );
	game::backend::SmacGameplayRandomState GameplayRandom() const;
	void GameplayRandom( game::backend::SmacGameplayRandomState value );
	void RetainedPlanet( int32_t retained, int32_t current );
	void MovementCapability( game::backend::faction::SmacSimulationState::MovementCapability capability );
	void MoveEvent( GSE_CALLABLE, game::backend::unit::Unit* unit, game::backend::map::tile::Tile* tile, size_t caller = 0 );
	game::backend::Game* Game() const;
	std::string Run( const std::function< std::string( GSE_CALLABLE ) >& check );
	Value* Script( GSE_CALLABLE, const std::string& path );
	const std::vector< game::FrontendRequest >& Updates();
	const std::vector< game::FrontendRequest >& QueuedFrontendRequests() const;
	void PrepareBaseCallbacks();
	std::string Snapshot() const;
	void Restore( GSE_CALLABLE, const std::string& snapshot, bool no_current_simulation = false );
	void RetainedMorale( int32_t retained, int32_t current );
	void Early();
	std::string CheckEarlyStartup( int scenario );
private:
	std::unique_ptr< BackendFixture > fixture;
};
void AddConventionalUnitTests( task::gsetests::GSETests* task );
}
