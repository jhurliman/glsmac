#include "SmacGroundEdgeCost.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::unit::smac_edge {
namespace {
constexpr uint32_t FEATURES = Base | Road | Tube | Fungus | River | Forest;

void ValidateTile( const Tile& tile, int32_t width, int32_t height ) {
	if ( tile.x < 0 || tile.x >= width || tile.y < 0 || tile.y >= height ||
		( tile.x & 1 ) != ( tile.y & 1 ) || tile.altitude_band < 3 || tile.altitude_band > 7 ||
		tile.rockiness < 0 || tile.rockiness > 2 || ( tile.features & ~FEATURES ) != 0 ) {
		throw std::invalid_argument( "SMAC ground edge cost requires supported land tiles and doubled coordinates" );
	}
}

void Validate( const Query& q ) {
	const auto& d = q.design;
	const bool combat = d.weapon_offense > 0 && d.weapon_mode == 0 && d.plan >= 0 && d.plan <= 3;
	const bool former = d.chassis == Chassis::Infantry && d.weapon_offense == 0 && d.weapon_mode == 9 && d.plan == 9;
	const bool colony = d.chassis == Chassis::Infantry && d.weapon_offense == 0 && d.weapon_mode == 8 && d.plan == 8
		&& !( ( q.source.features | q.destination.features ) & Fungus );
	if ( q.quantum < 1 || q.quantum > 100 || q.faction < 1 || q.faction > 7 ||
		q.width < 2 || ( q.width & 1 ) != 0 || q.height < 1 ||
		d.id < 0 || d.id == 8 || d.id == 9 ||
		( d.chassis != Chassis::Infantry && d.chassis != Chassis::Speeder ) ||
		d.domain != 0 || ( !combat && !former && !colony ) || d.abilities != 0 ||
		d.basic_allowance < q.quantum || int64_t( d.basic_allowance ) > int64_t( q.quantum ) * 99 ||
		d.basic_allowance % q.quantum != 0 ) {
		throw std::invalid_argument( "SMAC ground edge cost requires an admitted ordinary design and bounded query" );
	}
	ValidateTile( q.source, q.width, q.height );
	ValidateTile( q.destination, q.width, q.height );
}
}

int32_t QueryGroundEdgeCost( const Query& q ) {
	Validate( q );
	const auto source = q.source.features;
	const auto destination = q.destination.features;
	if ( ( source & ( Base | Tube ) ) && ( destination & ( Base | Tube ) ) ) { return 0; }
	if ( ( ( source & ( Base | Road ) ) || ( q.owns_xenoempathy && ( source & Fungus ) ) ) &&
		( destination & ( Base | Road ) ) ) { return 1; }
	if ( q.owns_xenoempathy && ( destination & Fungus ) ) { return 1; }
	int64_t dx = int64_t( q.source.x ) - q.destination.x;
	int64_t dy = int64_t( q.source.y ) - q.destination.y;
	dx = dx < 0 ? -dx : dx;
	dy = dy < 0 ? -dy : dy;
	if ( q.horizontal_wrap ) { dx = std::min( dx, int64_t( q.width ) - dx ); }
	if ( ( source & River ) && ( destination & River ) && dx == 1 && dy == 1 ) { return 1; }
	int64_t cost = q.quantum;
	if ( !q.suppress_terrain_charges ) {
		if ( q.destination.rockiness > 1 ) { cost += q.quantum; }
		if ( destination & Forest ) { cost += q.quantum; }
	}
	if ( destination & Fungus ) {
		if ( q.design.plan == 9 || q.retained_planet > 0 ) { cost = std::max( cost, int64_t( q.design.basic_allowance ) ); }
		else { cost += int64_t( q.quantum ) * 2; }
	}
	// The admitted domain bounds this to max(5Q,99Q), at most 9900.
	return static_cast< int32_t >( cost );
}

}
