#pragma once

#include "CvrSceneReader.h"

#include <memory>

namespace loader::cvr {

// Create once from decoded scenes, then share immutable geometry/frame storage
// across assemblies. When moving input, relinquish any previously retained
// mutable element pointers; mutating moved storage through them is unsupported.
class CvrFrameSource {
public:
	static std::shared_ptr< const CvrFrameSource > Create( std::vector< CvrScene > scenes );
	const std::vector< CvrScene >& Scenes() const { return m_scenes; }

private:
	explicit CvrFrameSource( std::vector< CvrScene > scenes );
	const std::vector< CvrScene > m_scenes;
};

struct CvrFrameSelection {
	size_t scene_index;
	int32_t frame;
	std::array< float, 3 > offset; // Explicit file-component order, with no axis conversion.
};

struct CvrAssemblyPosition {
	size_t selection_index;
	size_t object_index;
};

struct CvrAssembledFrame {
	size_t selection_index;
	size_t scene_index;
	size_t object_index; // Geometry identity within source->Scenes(), independent of names.
	uint32_t frame_index;
	CvrFrame value;
};

struct CvrFrameAssembly {
	std::shared_ptr< const CvrFrameSource > source;
	std::vector< CvrAssembledFrame > records;
	// The original routine keeps the first 200 records. Presence marks a partial
	// assembly; an exact 200-record complete request has no omitted position.
	std::optional< CvrAssemblyPosition > first_omitted;
};

class CvrFrameAssembler {
public:
	static constexpr size_t CAPACITY = 200;
	// All selections are validated before result records are allocated, including
	// any beyond capacity. Nonnegative frames use each scene's own frame count.
	// Flags and matrix bits are copied; only location + explicit offset is computed.
	// Arithmetic supports nearest rounding, normal-or-zero binary32 operands and
	// binary64 intermediate sums within the normal-or-zero binary32 range. Unsupported/malformed
	// input throws std::invalid_argument ("CVR assembly:"); allocation errors pass
	// through. Source creation checks counts and finite values in every frame.
	static CvrFrameAssembly Assemble( std::shared_ptr< const CvrFrameSource > source,
		const std::vector< CvrFrameSelection >& selections );
};

}
