#pragma once

#include <string>
#include <vector>
#include <optional>

#include "RenderInfo.h"
#include "types/Buffer.h"

namespace game {
namespace backend {
namespace base {

class PopDef {
public:

	typedef uint8_t pop_flags_t;
	static constexpr pop_flags_t PF_NONE = 0;
	static constexpr pop_flags_t PF_TILE_WORKER = 1 << 0;

	PopDef(
		const std::string& id,
		const std::string& name,
		const render_infos_t& renders_human,
		const render_infos_t& renders_progenitor,
		const pop_flags_t flags,
		const std::optional< uint8_t > smac_specialist_id = std::nullopt
	);
	virtual ~PopDef() = default;

	const std::string m_id;
	const std::string m_name;
	const render_infos_t m_renders_human;
	const render_infos_t m_renders_progenitor;
	const pop_flags_t m_flags;
	// Definition role, not a display-name lookup or a second Pop assignment.
	const std::optional< uint8_t > m_smac_specialist_id;

	const std::string ToString( const std::string& prefix = "" ) const;

	static const types::Buffer Serialize( const PopDef* def );
	static PopDef* Deserialize( types::Buffer& buf );

private:
	const std::string InfosToString( const std::string& prefix, const std::string& name, const render_infos_t& infos ) const;

};

}
}
}
