#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "gse/Value.h"

namespace task::gsetests { class GSETests; }
namespace game { class FrontendRequest; namespace backend { class Game; } }
namespace gse::tests {
class BackendFixture;
// Small adapter to the existing asset-free backend/runtime fixture. Assertions
// live in the dedicated test TU; no alternative gameplay implementation.
class ConventionalUnitFixture {
public:
	ConventionalUnitFixture();
	~ConventionalUnitFixture();
	void Configure( bool refreshed = true, int32_t road_rate = 3 );
	game::backend::Game* Game() const;
	std::string Run( const std::function< std::string( GSE_CALLABLE ) >& check );
	Value* Script( GSE_CALLABLE, const std::string& path );
	const std::vector< game::FrontendRequest >& Updates();
	std::string Snapshot() const;
	void Restore( GSE_CALLABLE, const std::string& snapshot );
	void RetainedMorale( int32_t retained, int32_t current );
	void Early();
private:
	std::unique_ptr< BackendFixture > fixture;
};
void AddConventionalUnitTests( task::gsetests::GSETests* task );
}
