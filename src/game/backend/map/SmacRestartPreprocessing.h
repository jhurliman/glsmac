#pragma once
#include "SmacSiteGeometry.h"
#include <functional>

namespace game::backend::map::smac_restart {
using DetailQuery=std::function<uint8_t(smac_site::Coordinate)>;
// Native map initialization/load producer: floor sqrt(parity-valid tile count).
int32_t ScaleFromGeometry(smac_site::Geometry geometry);
// Native eight-ring, first-hit Q scan. Reads the raw shared-detail byte only:
// 1..127 qualifies; zero/suppressed values do not. Registry/membership unused.
int32_t LandmarkBucket(smac_site::Geometry geometry,smac_site::Coordinate center,const DetailQuery& detail);
}
