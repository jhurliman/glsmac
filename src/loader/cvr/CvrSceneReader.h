#pragma once

#include "CvrContainerReader.h"

#include <optional>
#include <string>

namespace loader::cvr {

struct CvrFrame {
	uint8_t flags; // Preserved bits; no visibility or other flag semantics applied.
	std::array< float, 3 > location;
	std::array< float, 9 > matrix;
};

struct CvrSceneObject {
	std::string name;
	std::optional< std::vector< uint8_t > > voxel_data;
	std::vector< CvrFrame > frames;
};

struct CvrScene {
	std::string name;
	uint32_t frame_count;
	std::vector< CvrSceneObject > objects; // Source order; names need not be unique.
};

class CvrSceneReader {
public:
	// Decode the observed nested scene-record profile from chunk 4. The public
	// container's tags, nonempty payloads, tag-1 width and total uint32 size
	// budget are checked even when it did not come from CvrContainerReader.
	// Scene/object names are owned byte strings; object names may be empty.
	// Each object has optional opaque
	// voxel data and exactly frame_count flags, locations and matrices. Floats
	// are finite IEEE binary32, decoded little-endian in file component order.
	// No axis conversion, matrix application, attachment lookup, geometry or
	// palette interpretation occurs. Other payload contents remain opaque.
	// Invalid/unsupported records throw std::invalid_argument. The entire scene
	// is validated before result storage is allocated; allocation errors propagate.
	// The caller must keep the container unchanged throughout the call.
	static CvrScene Read( const CvrContainer& container );
};

}
