#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <functional>

namespace game::backend::map {
class SmacWorkerKnowledge;
namespace smac_territory {

using Coordinate = std::array< int32_t, 2 >;
struct Tile {
	bool sea;
	uint8_t region; // supplied current topology: land1..63, sea65..127
};
struct Base {
	Coordinate coordinate;
	int32_t owner; // playable1..7; native territory is outside this receipt
};
struct Input {
	int32_t width, height;
	bool horizontal_wrap;
	int32_t border_distance; // nonnegative signed32; sea uses floor(value/2)
	// Y ascending, X from row parity in steps of two, including polar rows.
	std::vector< Tile > tiles;
	// Explicit current record order. Later records win equal-distance ties.
	// This is neither inferred creation age nor a sorted owner/port-ID order.
	std::vector< Base > bases;
};
using Claims = std::vector< std::optional< int32_t > >;

// Bounded to the existing worker grid (even width2..512, height1..256), one
// Base per tile. Regions must already exist; this does not flood-fill them.
// Returns every claim, including unclaimed. No old claim is retained.
Claims Rebuild( const Input& input );

// Owned-value composition only: exact geometry must match. Keeps exploration,
// observer relations and Unit observations byte-for-byte. No live publication,
// event scheduling or wire-format change is implied by this operation.
SmacWorkerKnowledge ReplaceClaims( const SmacWorkerKnowledge& knowledge, const Input& input );

// Abstract regional land routing, not movement of an ordinary Unit. Territory
// is signed: negative unowned differs from native faction slot0.
struct RegionalRouteTile { bool sea; int8_t territory; uint8_t region; };
using RegionalRelations = std::array<std::array<uint32_t,8>,8>;
struct RegionalRouteMap {
	int32_t width, height;
	bool horizontal_wrap;
	std::vector<RegionalRouteTile> tiles;
	RegionalRelations relations;
	uint8_t human_membership;
};
struct RegionalRouteQuery { Coordinate source, target; int32_t owner, peer, mode; };
// Transient partial distance field, deliberately excluded from saves. The
// caller resolves BOTH prototype plans from current definitions on a hit.
class RegionalLandWorkspace {
public:
	using PlanResolver=std::function<int32_t(int32_t)>;
	RegionalLandWorkspace(int32_t width,int32_t height);
	bool Query(const RegionalRouteMap& map,const RegionalRouteQuery& query,int32_t prototype,const PlanResolver& current_plan);
	void Invalidate() noexcept {m_destination={{-1,-1}};}
	const Coordinate& Destination() const noexcept {return m_destination;}
	const std::vector<uint32_t>& Distances() const noexcept {return m_distances;}
private:
	int32_t m_width,m_height,m_peer=0,m_prototype=0;
	uint32_t m_options=0;
	Coordinate m_destination{{-1,-1}};
	std::vector<uint32_t> m_distances;
};
// A fresh workspace, searched backward from target. The target is seeded
// without territory admission; source and entered tiles must pass it.
bool RegionalLandReachable(const RegionalRouteMap& map,const RegionalRouteQuery& query);
// Forward Base record order and native land-component representative marking.
// Counts are retained inputs, not rebuilt from the supplied live Base list.
bool RegionalBasesReachable(const RegionalRouteMap& map,const std::vector<Base>& bases,
	int32_t owner,int32_t peer,uint8_t region,int32_t mode,uint8_t owner_bases,uint8_t peer_bases);
bool RegionalBasesReachable(const RegionalRouteMap& map,const std::vector<Base>& bases,
	int32_t owner,int32_t peer,uint8_t region,int32_t mode,uint8_t owner_bases,uint8_t peer_bases,
	RegionalLandWorkspace& workspace,const RegionalLandWorkspace::PlanResolver& current_plan);
RegionalRouteMap ProjectRegionalRoutes(const SmacWorkerKnowledge& knowledge,const std::vector<bool>& sea,
	const std::vector<uint8_t>& regions,RegionalRelations relations,uint8_t human_membership);
// First qualifying direction1..8; center and off-map neighbors are excluded.
uint8_t ImmediateSeaNeighbor(int32_t width,int32_t height,bool horizontal_wrap,
	const std::vector<bool>& sea,const Coordinate& center);

}
}
