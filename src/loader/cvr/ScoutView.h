#pragma once

#include "CvrScoutRenderer.h"

namespace loader::cvr::scout {

using Matrix = std::array< uint32_t, 9 >;
using Vector = std::array< uint32_t, 3 >;

Matrix MultiplyMatrix( const Matrix& a, const Matrix& b );
Vector MultiplyVector( const Matrix& matrix, const Vector& vector );
Vector AddVector( const Vector& a, const Vector& b );

struct View {
	Matrix projection, auxiliary;
	Vector translation, light;
	std::array< uint32_t, 2 > scales;
	std::array< int32_t, 2 > target_anchor;
};

View MakeView( const CvrScoutViewInput& input );

}
