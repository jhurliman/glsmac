#include "SmacRestartPreprocessing.h"
#include <stdexcept>

namespace game::backend::map::smac_restart {
namespace {
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
void Geometry(smac_site::Geometry geometry) {
	Require(geometry.width>=2 && geometry.width<=512 && !(geometry.width&1) && geometry.height>=2 && geometry.height<=256,
		"Restart preprocessing requires a bounded parity grid");
}
#include "SmacRestartProximityOffsets.h"
constexpr std::array<size_t,8> ends{{1,9,25,49,81,121,169,225}};
}
int32_t ScaleFromGeometry(smac_site::Geometry geometry) {
	Geometry(geometry);const int32_t count=(geometry.width/2)*geometry.height;int32_t result=0;
	while((result+1)*(result+1)<=count) {++result;}return result;
}
int32_t LandmarkBucket(smac_site::Geometry geometry,smac_site::Coordinate center,const DetailQuery& detail) {
	Geometry(geometry);Require(center[0]>=0 && center[0]<geometry.width && center[1]>=0 && center[1]<geometry.height
		&& !((center[0]^center[1])&1) && bool(detail),"Invalid landmark scan center or detail query");
	size_t bucket=0;
	for(size_t offset=0;offset<proximity.size();++offset) {
		if(offset==ends[bucket]) {++bucket;}
		smac_site::Coordinate point{{center[0]+proximity[offset][0],center[1]+proximity[offset][1]}};
		if(geometry.wrap) {if(point[0]<0) {point[0]+=geometry.width;}else if(point[0]>=geometry.width) {point[0]-=geometry.width;}}
		if(point[0]<0 || point[0]>=geometry.width || point[1]<0 || point[1]>=geometry.height) {continue;}
		const auto raw=detail(point);if(raw && raw<128) {return static_cast<int32_t>(bucket);}
	}
	return 32;
}
}
