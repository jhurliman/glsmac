#include "SmacSiteQuality.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <cstdlib>

namespace game::backend::map::smac_site {
namespace {
constexpr std::array<Coordinate,21> inner{{
	{{0,0}},{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}},
	{{2,-2}},{{2,2}},{{-2,2}},{{-2,-2}},{{1,-3}},{{3,-1}},{{3,1}},{{1,3}},{{-1,3}},{{-3,1}},{{-3,-1}},{{-1,-3}}
}};
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
void Validate(const Tile& tile) {
	Require(tile.elevation<=7 && tile.moisture<=3 && tile.rockiness<=3 && tile.region<128,"Invalid decoded site-quality tile");
}
}

static Score Evaluate(Geometry geometry,Coordinate center,int32_t turn,const Areas& areas,const Query& query,bool require_land) {
	Require(geometry.width>=2 && geometry.width<=512 && !(geometry.width&1) && geometry.height>=2 && geometry.height<=256,
		"Site-quality geometry is not a bounded parity grid");
	Require(center[0]>=0 && center[0]<geometry.width && center[1]>=0 && center[1]<geometry.height
		&& !((center[0]^center[1])&1) && turn>=0 && bool(query),"Unsupported site-quality center, turn or query");
	const auto resolve=[&](Coordinate delta)->std::optional<Coordinate> {
		Coordinate point{{center[0]+delta[0],center[1]+delta[1]}};
		if(geometry.wrap) {point[0]=(point[0]%geometry.width+geometry.width)%geometry.width;}
		if(point[0]<0 || point[0]>=geometry.width || point[1]<0 || point[1]>=geometry.height) {return {};}
		return point;
	};
	Score result{};Tile middle{};bool sea_center=false;int32_t land_fungus=0;
	for(size_t index=0;index<inner.size();++index) {
		const auto point=resolve(inner[index]);if(!point) {continue;}
		const auto tile=query(*point);Validate(tile);if(index==0) {middle=tile;sea_center=tile.elevation<3;Require(!require_land || !sea_center,"Land-site query requires a land center");}
		result.last=*point;result.coastal=tile.coastal;
		if(tile.elevation<3) {
			++result.water;if(tile.elevation<2) {++result.deep_water;}
			if(index && tile.resource && !(tile.elevation>=2 && tile.fungus)) {++result.points;}
			if(index<=8 && areas[tile.region]>=50) {result.large_water=true;}
			continue;
		}
		if(tile.fungus) {++land_fungus;}
		if(index && tile.rockiness>=2) {++result.rocky_neighbors;}
		const bool active=!(tile.landmark_detail&128);
		if(!tile.fungus) {
			if(tile.resource) {result.points+=index ? 2 : -1;}
			if(active) {
				if(tile.planet && tile.landmark_detail<9) {result.points+=index ? 1 : -1;}
				if(tile.crater && tile.landmark_detail<9) {result.points+=index ? 2 : -1;}
				if(tile.uranium) {result.points+=index ? 1 : -3;}
				if(tile.jungle) {result.points+=2;}
			}
			if(index && tile.site) {++result.points;}
		}
		const auto moisture=tile.rockiness>=2 || (active && tile.planet) ? 0 : tile.moisture;
		if(tile.excluded) {continue;}
		const int weight=index ? 1 : -1;
		if(tile.monolith || tile.forest) {
			result.production+=weight;
			if(tile.monolith && !tile.forest && (moisture>1 || tile.elevation>3)) {result.production+=index ? 1 : -2;}
		} else if(tile.rockiness==1 && moisture>0) {
			if(tile.fungus) {result.fungus_production+=weight;} else {result.production+=weight;}
		}
		if(tile.river) {++result.rivers;}
	}
	for(int32_t dy=-6;dy<=6;++dy) {for(int32_t dx=-6;dx<=6;++dx) {
		if((dx+dy)&1 || std::abs(dx)+std::abs(dy)>6
			|| std::find(inner.begin(),inner.end(),Coordinate{{dx,dy}})!=inner.end()) {continue;}
		const auto point=resolve({{dx,dy}});if(!point) {continue;}
		const auto tile=query(*point);Validate(tile);
		if(tile.base) {
			const bool cardinal=(std::abs(dx)==4 && dy==0) || (dx==0 && std::abs(dy)==4);
			result.outer_bases+=cardinal ? (sea_center && tile.elevation<3 ? 4 : 2) : 1;
		}
	}}
	result.raw=result.points+result.rivers/4+4*int(result.large_water)+4*int(result.coastal)
		+(3*result.production+6)/4-(result.water-int(result.large_water))/2+(result.fungus_production+1)/3
		-result.deep_water/2-(turn>150 ? land_fungus/3 : 0)-(sea_center ? 2 : 1)*result.outer_bases-result.rocky_neighbors/4;
	const bool rejected=middle.monolith || (middle.fungus && middle.elevation>=2)
		|| (!sea_center && middle.rockiness>=2) || (sea_center && result.outer_bases>=4);
	result.rating=rejected ? 0 : std::clamp(result.raw,1,15);
	return result;
}
Score LandQuality(Geometry geometry,Coordinate center,int32_t turn,const Areas& areas,const Query& query) {
	return Evaluate(geometry,center,turn,areas,query,true);
}
Score Quality(Geometry geometry,Coordinate center,int32_t turn,const Areas& areas,const Query& query) {
	return Evaluate(geometry,center,turn,areas,query,false);
}
}
