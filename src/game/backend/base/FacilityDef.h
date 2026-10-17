#pragma once

#include <string>
#include <vector>
#include <optional>

#include "RenderInfo.h"
#include "types/Buffer.h"

namespace game {
namespace backend {
namespace base {

class FacilityDef {
public:

	FacilityDef(
		const std::string& id,
		const std::string& name,
		const std::string& description,
		const size_t cost,
		const RenderInfo& render_info,
		const std::optional< int32_t > smac_source_id = std::nullopt
	);
	virtual ~FacilityDef() = default;

	const std::string m_id;
	const std::string m_name;
	const std::string m_description;
	const size_t m_cost;
	const RenderInfo m_render_info;
	// Positional identity in the frozen FACILITIES catalog, never a name lookup.
	// Does not imply build eligibility, project ownership or orbital counts.
	const std::optional< int32_t > m_smac_source_id;

	const std::string ToString( const std::string& prefix = "" ) const;

	static const types::Buffer Serialize( const FacilityDef* def );
	static FacilityDef* Deserialize( types::Buffer& buf );

};

}
}
}
