#include "SmacGroundEdgeCost.h"

#include "game/backend/unit/SmacGroundEdgeCost.h"
#include "fixtures/SmacGroundEdgeCost.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace edge = game::backend::unit::smac_edge;
edge::Query Ordinary() {
	return { { 64, edge::Chassis::Infantry, 0, 1, 0, 0, 0, 3 },
		3, 1, 0, false, false, 16, 16, true, { 8, 8, 3, 0, 0 }, { 9, 9, 3, 0, 0 } };
}
template< typename F > bool Rejects( F f ) {
	try { f(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
}

void AddSmacGroundEdgeCostTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC ground edge cost matches admitted original terrain and infrastructure queries", GT() {
		for ( const auto& c : fixtures::smac_ground_edge::CASES ) {
			auto q = Ordinary();
			q.design.chassis = static_cast< edge::Chassis >( c.chassis );
			q.quantum = c.quantum; q.design.basic_allowance = c.quantum * ( c.chassis + 1 );
			q.retained_planet = c.planet; q.owns_xenoempathy = c.xeno != 0;
			q.suppress_terrain_charges = c.terrain_switch != 0; q.horizontal_wrap = c.wrap != 0;
			q.source = { c.sx, c.sy, 3, 0, c.source_features };
			q.destination = { c.dx, c.dy, 3, c.rockiness, c.destination_features };
			GT_ASSERT( edge::QueryGroundEdgeCost( q ) == c.cost, "original edge cost differs: " + std::string( c.name ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC ground edge cost preserves competing early-return precedence", GT() {
		auto q = Ordinary();
		q.owns_xenoempathy = true;
		q.source.features = edge::Base | edge::Road | edge::Tube | edge::River | edge::Fungus;
		q.destination.features = edge::Base | edge::Road | edge::Tube | edge::River | edge::Forest | edge::Fungus;
		q.destination.rockiness = 2;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 0, "tube/base must precede roads, rivers and cheap fungus" );
		q.source.features = edge::Fungus;
		q.destination.features = edge::Road | edge::Forest;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 1, "owning project source fungus must supply the source road endpoint" );
		q.owns_xenoempathy = false;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 9, "destination-only road must not bypass terrain" );
		q.source.features = edge::River;
		q.destination.features = edge::River | edge::Fungus | edge::Forest;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 1, "diagonal river must precede terrain and fungus penalties" );
		q.destination.x = 10; q.destination.y = 8;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 15, "horizontal river must not receive a diagonal discount" );
		q.suppress_terrain_charges = true;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 9, "terrain switch must not suppress ordinary fungus surcharge" );
		q.owns_xenoempathy = true;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 1, "owning project destination fungus must precede terrain" );
		GT_OK();
	} );
	task->AddTest( "SMAC ground edge cost uses periodic doubled-coordinate river geometry", GT() {
		auto q = Ordinary(); q.source = { 0, 8, 3, 0, edge::River }; q.destination = { 15, 9, 3, 0, edge::River };
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 1, "wrapped X seam lost diagonal river connection" );
		q.horizontal_wrap = false;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 3, "flat-map diagnostic query incorrectly wrapped" );
		q.horizontal_wrap = true; q.source = { 0, 0, 3, 0, edge::River }; q.destination = { 1, 15, 3, 0, edge::River };
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 3, "Y must not wrap" );
		q.width = std::numeric_limits< int32_t >::max() - 1; q.height = std::numeric_limits< int32_t >::max();
		q.source = { 0, 0, 3, 0, edge::River }; q.destination = { q.width - 1, 1, 3, 0, edge::River };
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 1, "wide admitted seam arithmetic overflowed" );
		q.destination.y = q.height - 2;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 3, "large Y difference overflowed into adjacency" );
		GT_OK();
	} );
	task->AddTest( "SMAC ground edge cost uses supplied basic allowance and retained PLANET", GT() {
		auto q = Ordinary(); q.design.chassis = edge::Chassis::Speeder; q.design.basic_allowance = 6;
		q.destination.features = edge::Fungus; q.retained_planet = 1;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 6, "positive retained PLANET must use basic design allowance" );
		q.destination.features |= edge::Forest; q.destination.rockiness = 2;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 9, "positive PLANET must retain greater terrain cost" );
		q.retained_planet = 0;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 15, "zero PLANET must pay terrain plus two Q" );
		q.retained_planet = std::numeric_limits< int32_t >::min();
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 15, "negative retained PLANET must not become unsigned positive" );
		q.retained_planet = std::numeric_limits< int32_t >::max(); q.quantum = 100; q.design.basic_allowance = 9900;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 9900, "explicit port upper allowance boundary differs" );
		q.quantum = 1; q.design.basic_allowance = 1; q.retained_planet = 0;
		GT_ASSERT( edge::QueryGroundEdgeCost( q ) == 5, "explicit Q1 stacked terrain/fungus differs" );
		const auto copy = q; const auto cost = edge::QueryGroundEdgeCost( q );
		q.destination.features = 0;
		GT_ASSERT( cost == 5 && copy.destination.features == ( edge::Fungus | edge::Forest ) && edge::QueryGroundEdgeCost( copy ) == 5, "query/result retained a mutable input alias" );
		GT_OK();
	} );
	task->AddTest( "SMAC ground edge cost rejects unsupported designs and malformed query fields", GT() {
		for ( int field = 0; field < 28; field++ ) {
			auto q = Ordinary();
			switch ( field ) {
				case 0: q.quantum = 0; break;
				case 1: q.quantum = 101; break;
				case 2: q.design.basic_allowance = 0; break;
				case 3: q.design.basic_allowance = 298; break;
				case 4: q.design.basic_allowance = 4; break;
				case 5: q.faction = 0; break;
				case 6: q.faction = 8; break;
				case 7: q.design.id = -1; break;
				case 8: q.design.id = 8; break;
				case 9: q.design.id = 9; break;
				case 10: q.design.chassis = static_cast< edge::Chassis >( 2 ); break;
				case 11: q.design.domain = 1; break;
				case 12: q.design.weapon_offense = -1; break;
				case 13: q.design.weapon_offense = 0; break;
				case 14: q.design.weapon_mode = 7; break;
				case 15: q.design.plan = 9; break;
				case 16: q.design.plan = 12; break;
				case 17: q.design.abilities = 1u << 10; break;
				case 18: q.width = 1; break;
				case 19: q.width = 15; break;
				case 20: q.height = 0; break;
				case 21: q.source.x = -2; break;
				case 22: q.destination.x = 17; break;
				case 23: q.source.y = 16; break;
				case 24: q.destination.y = 8; break;
				case 25: q.source.altitude_band = 2; break;
				case 26: q.destination.rockiness = 3; break;
				case 27: q.destination.features = 0x80000000u; break;
			}
			// Unsupported inputs must reject even when infrastructure would return0.
			q.source.features |= edge::Tube; q.destination.features |= edge::Tube;
			GT_ASSERT( Rejects( [ & ]() { edge::QueryGroundEdgeCost( q ); } ), "unsupported edge-cost query accepted: " + std::to_string( field ) );
		}
		GT_OK();
	} );
}

}
