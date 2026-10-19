#pragma once

#include "CvrScoutRenderer.h"

namespace loader::cvr::scout {

using Matrix = std::array< uint32_t, 9 >;
using Vector = std::array< uint32_t, 3 >;

Matrix MultiplyMatrix( const Matrix& a, const Matrix& b );
Vector MultiplyVector( const Matrix& matrix, const Vector& vector );
Vector AddVector( const Vector& a, const Vector& b );

struct View {
	Matrix projection, auxiliary, orientation;
	Vector translation, light;
	std::array< uint32_t, 2 > scales;
	std::array< int32_t, 2 > target_anchor;
};

// Explicit finite binary32 scene inputs for the orthographic shadow producer.
struct ShadowScene {
    Matrix auxiliary,orientation;
    Vector auxiliary_position,position,direction;
    uint32_t auxiliary_scale,object_scale,ground;
    std::array<uint32_t,2> xy_scale,offset,pixel_offset;
    uint32_t width,height;
};
struct ShadowView {Matrix projection;Vector translation;};
ShadowScene FlatScoutShadowScene(const CvrScoutViewInput& input);
ShadowView MakeShadowView(const ShadowScene& scene);

enum class IdleProfile { Infantry, SpartanSpeeder, InitializedInfantry };
// Fixed measured installed map-light profile, shared by initialized compositions.
void ValidateMapLight( std::string_view system );
View MakeView( const CvrScoutViewInput& input, IdleProfile profile = IdleProfile::Infantry );

}
