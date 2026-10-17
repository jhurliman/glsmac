#include "SmacTerritoryRules.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::settings {

int32_t SmacTerritoryRules::BorderDistance() const { return std::clamp(m_raw_border,0,100); }
types::Buffer SmacTerritoryRules::Serialize() const {
	types::Buffer buffer; buffer.WriteInt(1); buffer.WriteInt(m_raw_border); return buffer;
}
SmacTerritoryRules SmacTerritoryRules::Deserialize( types::Buffer buffer ) {
	if (buffer.ReadInt() != 1) { throw std::runtime_error("Unsupported territory rule format"); }
	const auto raw = buffer.ReadInt();
	if (raw < std::numeric_limits<int32_t>::min() || raw > std::numeric_limits<int32_t>::max() || buffer.lenr != buffer.lenw) {
		throw std::runtime_error("Invalid territory rule value or trailing data");
	}
	return SmacTerritoryRules(static_cast<int32_t>(raw));
}

}
