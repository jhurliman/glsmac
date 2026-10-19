#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

namespace game::backend::faction {

// Presentation only: source faction name and default leader, independently of
// the session controller's roster name. Old saves may have no such metadata.
struct DisplayIdentity {
	static constexpr uint32_t MAX_FIELD_BYTES = 256;
	std::string faction_name, leader_title, leader_name;
	void Validate() const {
		for (const auto* field : {&faction_name, &leader_title, &leader_name}) {
			if (field->empty() || field->size() > MAX_FIELD_BYTES || field->find_first_of(std::string("\0\r\n", 3)) != std::string::npos) {
				throw std::runtime_error("Invalid faction display identity field");
			}
		}
	}
	bool operator==(const DisplayIdentity& other) const {
		return faction_name == other.faction_name && leader_title == other.leader_title && leader_name == other.leader_name;
	}
};

}
