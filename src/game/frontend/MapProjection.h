#pragma once

#include "types/Vec3.h"

namespace game::frontend::map_projection {
// Original ordinary zoom0 diamonds are 96x48. The port's fixed camera
// already supplies a sqrt(1/2) Y/Z factor; apply it once more for 2:1.
inline types::Vec3 CameraScale(float zoom,bool native) {
    const float yz=native?zoom*0.7071067811865475244f:zoom;
    return {zoom,yz,yz};
}
inline float Vertical(bool native) {return native?0.5f:0.707f;}
inline float HeightVertical(bool native) {return native?0.5f:1.0f/1.414f;}
inline float FitHeight(bool native) {return native?4.0f:2.82f;}
inline float RangeVertical(bool native) {return native?0.125f:0.1768f;}
}
