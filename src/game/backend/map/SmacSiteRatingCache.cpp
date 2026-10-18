#include "SmacSiteRatingCache.h"
#include <optional>
#include <stdexcept>
#include <utility>

namespace game::backend::map::smac_site {
namespace {
// RE-SMAC-TERRITORY-CACHE-REFILL-001 ordered native radius table.
constexpr std::array<Coordinate,49> radius{{
	{{0,0}},{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}},
	{{2,-2}},{{2,2}},{{-2,2}},{{-2,-2}},{{1,-3}},{{3,-1}},{{3,1}},{{1,3}},{{-1,3}},{{-3,1}},{{-3,-1}},{{-1,-3}},
	{{4,0}},{{-4,0}},{{0,4}},{{0,-4}},{{1,-5}},{{2,-4}},{{3,-3}},{{4,-2}},{{5,-1}},{{5,1}},{{4,2}},{{3,3}},{{2,4}},{{1,5}},
	{{-1,5}},{{-2,4}},{{-3,3}},{{-4,2}},{{-5,1}},{{-5,-1}},{{-4,-2}},{{-3,-3}},{{-2,-4}},{{-1,-5}},{{0,6}},{{6,0}},{{0,-6}},{{-6,0}}
}};
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
size_t Count(Geometry geometry) {
	Require(geometry.width>=2 && geometry.width<=512 && !(geometry.width&1) && geometry.height>=2 && geometry.height<=256,
		"Site cache requires a bounded parity grid");
	return size_t(geometry.width)*geometry.height/2;
}
std::optional<Coordinate> Resolve(Geometry geometry,Coordinate center,Coordinate delta) {
	Coordinate point{{center[0]+delta[0],center[1]+delta[1]}};
	if(geometry.wrap) {if(point[0]<0) {point[0]+=geometry.width;} else if(point[0]>=geometry.width) {point[0]-=geometry.width;}}
	if(point[0]<0 || point[0]>=geometry.width || point[1]<0 || point[1]>=geometry.height) {return {};}
	return point;
}
}
RatingCache::RatingCache(Geometry geometry,std::vector<uint8_t> values):m_geometry(geometry),m_values(std::move(values)) {}
RatingCache RatingCache::Fresh(Geometry geometry) {return RatingCache(geometry,std::vector<uint8_t>(Count(geometry),0));}
RatingCache RatingCache::Restore(Geometry geometry,const std::string& packed) {
	Require(packed.size()==Count(geometry),"Saved site-cache extent differs from geometry");
	for(const unsigned char value:packed) {Require(value<=31,"Invalid saved site-cache flags");}
	return RatingCache(geometry,std::vector<uint8_t>(packed.begin(),packed.end()));
}
std::string RatingCache::Serialize() const {return std::string(m_values.begin(),m_values.end());}
size_t RatingCache::Index(Coordinate point) const {
	Require(point[0]>=0 && point[0]<m_geometry.width && point[1]>=0 && point[1]<m_geometry.height && !((point[0]^point[1])&1),
		"Site-cache coordinate is not a current parity tile");
	return size_t(point[1])*(m_geometry.width/2)+point[0]/2;
}
uint8_t RatingCache::Rating(Coordinate point) const {return m_values[Index(point)]&15;}
bool RatingCache::Excluded(Coordinate point) const {return bool(m_values[Index(point)]&16);}
RatingCache RatingCache::ClearRatings() const {
	auto result=*this;for(auto& value:result.m_values) {value&=16;}return result;
}
RatingCache RatingCache::InvalidateLocal(Coordinate center) const {
	(void)Index(center);auto result=*this;
	for(size_t i=0;i<21;++i) {const auto point=Resolve(m_geometry,center,radius[i]);if(point) {result.m_values[Index(*point)]&=16;}}
	return result;
}
void RatingCache::BaseMark(Coordinate center) {
	(void)Index(center);
	for(size_t i=0;i<radius.size();++i) {
		const auto point=Resolve(m_geometry,center,radius[i]);if(!point) {continue;}
		auto& value=m_values[Index(*point)];if(i<21) {value|=16;} else {value&=16;}
	}
}
RatingCache RatingCache::FoundBase(Coordinate center) const {auto result=*this;result.BaseMark(center);return result;}
RatingCache RatingCache::RemoveBase(Coordinate center,const std::vector<Coordinate>& surviving_centers) const {
	(void)Index(center);auto result=*this;
	for(size_t i=0;i<radius.size();++i) {
		const auto point=Resolve(m_geometry,center,radius[i]);if(point) {auto& value=result.m_values[Index(*point)];value=i<21 ? 0 : value&16;}
	}
	for(const auto survivor:surviving_centers) {result.BaseMark(survivor);}return result;
}
RatingCache RatingCache::Refill(const RatingQuery& query) const {
	Require(bool(query),"Site-cache refill requires its native score query");auto result=*this;
	for(int32_t y=0;y<m_geometry.height;++y) {for(int32_t x=y&1;x<m_geometry.width;x+=2) {
		auto& value=result.m_values[Index({{x,y}})];if(value) {continue;}
		const auto rating=query({{x,y}});Require(rating<=15,"Site query returned an invalid rating");value=rating;
	}}
	return result;
}
bool RatingCache::operator==(const RatingCache& other) const {
	return m_geometry.width==other.m_geometry.width && m_geometry.height==other.m_geometry.height && m_geometry.wrap==other.m_geometry.wrap && m_values==other.m_values;
}
}
