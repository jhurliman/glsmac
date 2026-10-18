#include "SmacTerritoryRules.h"
#include "SmacWorkerKnowledge.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace game::backend::map::smac_territory {
namespace {
void Require( bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}
size_t Index( const Input& input, const Coordinate& position ) {
	return static_cast< size_t >( position[1] ) * ( input.width / 2 ) + position[0] / 2;
}
void Validate( const Input& input ) {
	Require( input.width >= 2 && input.width <= 512 && !( input.width & 1 ) && input.height >= 1 && input.height <= 256,
		"Territory requires the bounded doubled-coordinate worker grid" );
	Require( input.border_distance >= 0, "Territory requires a nonnegative border distance" );
	const auto count = static_cast< size_t >( input.width ) * input.height / 2;
	Require( input.tiles.size() == count && input.bases.size() <= count, "Territory map or Base count differs from geometry" );
	for ( const auto& tile : input.tiles ) {
		Require( tile.sea ? tile.region >= 65 && tile.region <= 127 : tile.region >= 1 && tile.region <= 63,
			"Territory requires supplied valid land or sea region IDs" );
	}
	std::set< size_t > centers;
	for ( const auto& base : input.bases ) {
		const auto& p = base.coordinate;
		Require( base.owner >= 1 && base.owner <= 7 && p[0] >= 0 && p[0] < input.width && p[1] >= 0 && p[1] < input.height && !( ( p[0] ^ p[1] ) & 1 ),
			"Territory Base has an unsupported owner or coordinate" );
		Require( centers.insert( Index( input, p ) ).second, "Territory has duplicate Base centers" );
	}
}
int64_t Distance( const Input& input, const Coordinate& from, const Coordinate& to ) {
	int64_t dx = int64_t( from[0] ) - to[0], dy = int64_t( from[1] ) - to[1];
	dx = dx < 0 ? -dx : dx; dy = dy < 0 ? -dy : dy;
	if ( input.horizontal_wrap ) { dx = std::min( dx, int64_t( input.width ) - dx ); }
	const auto difference = dx >= dy ? dx - dy : dy - dx;
	return std::max( dx, dy ) - ( difference + 2 ) / 4;
}
}

Claims Rebuild( const Input& input ) {
	Validate( input );
	Claims result( input.tiles.size() );
	for ( int32_t y = 0; y < input.height; ++y ) { for ( int32_t x = y & 1; x < input.width; x += 2 ) {
		const Coordinate position{x,y}; const auto index = Index( input, position );
		const auto& tile = input.tiles[index];
		const Base* selected = nullptr;
		int64_t nearest = 0;
		for ( const auto& base : input.bases ) {
			if ( input.tiles[Index( input, base.coordinate )].region != tile.region ) { continue; }
			const auto distance = Distance( input, position, base.coordinate );
			if ( !selected || distance <= nearest ) { selected = &base; nearest = distance; }
		}
		const auto cutoff = tile.sea ? input.border_distance / 2 : input.border_distance;
		if ( !selected || nearest >= cutoff ) { continue; }
		bool veto = false;
		if ( tile.sea ) {
			for ( const auto& base : input.bases ) {
				if ( base.owner != selected->owner && !input.tiles[Index( input, base.coordinate )].sea
					&& Distance( input, position, base.coordinate ) <= nearest ) { veto = true; break; }
			}
		}
		if ( !veto ) { result[index] = selected->owner; }
	} }
	return result;
}

SmacWorkerKnowledge ReplaceClaims( const SmacWorkerKnowledge& knowledge, const Input& input ) {
	auto data = knowledge.CopyData();
	Require( data.width == input.width && data.height == input.height && data.horizontal_wrap == input.horizontal_wrap,
		"Territory geometry differs from worker knowledge" );
	const auto claims = Rebuild( input );
	for ( size_t i = 0; i < claims.size(); ++i ) { data.tiles[i].claimant = claims[i]; }
	return SmacWorkerKnowledge::Create( data );
}

namespace {
constexpr std::array<Coordinate,8> ROUTE_NEIGHBORS{{{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}}};
size_t RouteIndex(const RegionalRouteMap& map,const Coordinate& p) {return size_t(p[1])*(map.width/2)+p[0]/2;}
bool RouteCoordinate(const RegionalRouteMap& map,const Coordinate& p) {
	return p[0]>=0 && p[0]<map.width && p[1]>=0 && p[1]<map.height && !((p[0]^p[1])&1);
}
void ValidateRoutes(const RegionalRouteMap& map,int32_t owner,int32_t peer) {
	Require(map.width>=2 && map.width<=512 && !(map.width&1) && map.height>=1 && map.height<=256,
		"Regional routes require the bounded parity map");
	Require(owner>=1 && owner<=7 && peer>=1 && peer<=7 && owner!=peer,"Regional route owners must be distinct positive slots");
	Require(map.tiles.size()==size_t(map.width)*map.height/2,"Regional route extent differs from geometry");
	for(const auto& tile:map.tiles) {Require(tile.territory<=7 && tile.region<128,"Regional route territory or region is invalid");}
}
template<class F> void RouteNeighbors(const RegionalRouteMap& map,size_t index,F visit) {
	const int32_t y=index/(map.width/2),x=2*(index%(map.width/2))+(y&1);
	for(const auto& delta:ROUTE_NEIGHBORS) {
		Coordinate p{{x+delta[0],y+delta[1]}};
		if(map.horizontal_wrap) {p[0]=(p[0]%map.width+map.width)%map.width;}
		if(RouteCoordinate(map,p)) {visit(RouteIndex(map,p));}
	}
}
bool RouteTerritory(const RegionalRouteMap& map,const RegionalRouteQuery& query,int32_t territory) {
	if(territory==query.owner || territory==query.peer) {return true;}
	if(territory<0) {return query.mode==0;}
	if(query.mode==0) {return false;}
	const auto forward=map.relations[query.peer][territory],reverse=map.relations[territory][query.peer];
	if((forward&0x100000u) && !(forward&0x11u)) {return false;}
	return (reverse&0x11u) || !(map.human_membership&(1u<<territory));
}
bool LandRoute(const RegionalRouteMap& map,const RegionalRouteQuery& query) {
	const auto source=RouteIndex(map,query.source),target=RouteIndex(map,query.target);
	std::vector<bool> seen(map.tiles.size());std::vector<size_t> pending{target};seen[target]=true;
	for(size_t cursor=0;cursor<pending.size();++cursor) {
		const auto current=pending[cursor];if(current==source) {return true;}
		RouteNeighbors(map,current,[&](size_t next) {
			if(seen[next]) {return;}
			const auto& tile=map.tiles[next];
			if((tile.sea && next!=source && next!=target) || !RouteTerritory(map,query,tile.territory)) {return;}
			seen[next]=true;pending.push_back(next);
		});
	}
	return false;
}
void MarkRouteComponent(const RegionalRouteMap& map,size_t seed,int32_t owner,std::vector<bool>& marked) {
	if(map.tiles[seed].sea) {return;}
	std::vector<size_t> pending{seed};marked[seed]=true;
	for(size_t cursor=0;cursor<pending.size();++cursor) {RouteNeighbors(map,pending[cursor],[&](size_t next) {
		if(!marked[next] && !map.tiles[next].sea && map.tiles[next].territory==owner) {marked[next]=true;pending.push_back(next);}
	});}
}
}

bool RegionalLandReachable(const RegionalRouteMap& map,const RegionalRouteQuery& query) {
	ValidateRoutes(map,query.owner,query.peer);
	Require(RouteCoordinate(map,query.source) && RouteCoordinate(map,query.target),"Regional route endpoint is invalid");
	return LandRoute(map,query);
}
bool RegionalBasesReachable(const RegionalRouteMap& map,const std::vector<Base>& bases,
	int32_t owner,int32_t peer,uint8_t region,int32_t mode,uint8_t owner_bases,uint8_t peer_bases) {
	ValidateRoutes(map,owner,peer);Require(region<128,"Regional route region is invalid");
	if(region && (!owner_bases || !peer_bases)) {return false;}
	for(const auto& base:bases) {Require(base.owner>=1 && base.owner<=7 && RouteCoordinate(map,base.coordinate),"Regional route Base is invalid");}
	std::vector<bool> marked(map.tiles.size());
	for(const auto& source:bases) {
		const auto si=RouteIndex(map,source.coordinate);
		if(source.owner!=owner || map.tiles[si].region!=region || marked[si]) {continue;}
		MarkRouteComponent(map,si,owner,marked);
		for(const auto& target:bases) {
			const auto ti=RouteIndex(map,target.coordinate);
			if(target.owner!=peer || map.tiles[ti].region!=region || marked[ti]) {continue;}
			MarkRouteComponent(map,ti,peer,marked);
			if(LandRoute(map,{source.coordinate,target.coordinate,owner,peer,mode})) {return true;}
		}
	}
	return false;
}
RegionalRouteMap ProjectRegionalRoutes(const SmacWorkerKnowledge& knowledge,const std::vector<bool>& sea,
	const std::vector<uint8_t>& regions,RegionalRelations relations,uint8_t human_membership) {
	const auto data=knowledge.CopyData();
	Require(sea.size()==data.tiles.size() && regions.size()==data.tiles.size(),"Regional route projection extent differs from knowledge");
	RegionalRouteMap result{data.width,data.height,data.horizontal_wrap,{},std::move(relations),human_membership};
	result.tiles.reserve(sea.size());
	for(size_t i=0;i<sea.size();++i) {result.tiles.push_back({sea[i],static_cast<int8_t>(data.tiles[i].claimant.value_or(-1)),regions[i]});}
	return result;
}

}
