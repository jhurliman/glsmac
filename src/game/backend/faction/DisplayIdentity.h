#pragma once

#include <string>
#include <optional>
#include <cstdint>
#include <stdexcept>

namespace game::backend::faction {

// Saved faction identity, independently of
// the session controller's roster name. Old saves may have no such metadata.
struct DisplayIdentity {
	static constexpr uint32_t MAX_FIELD_BYTES = 256;
	std::string faction_name, leader_title, leader_name;
    struct SetupFields {
        std::string noun, adjective, description;
        bool female;
        bool operator==(const SetupFields& other) const {
            return noun==other.noun && adjective==other.adjective && description==other.description && female==other.female;
        }
    };
    // Old saves have no customization metadata. Do not infer it on restore.
    std::optional<SetupFields> setup;
    void ValidateUserSetup() const {
        Validate();
        if(!setup) {throw std::runtime_error("Faction setup fields are unavailable");}
        const auto check=[](const std::string& text,size_t limit) {
            if(text.size()>limit) {throw std::runtime_error("Faction setup field exceeds its original byte limit");}
            for(unsigned char c:text) {if(c<32 || c>126) {throw std::runtime_error("Faction setup currently supports printable ASCII only");}}
        };
        check(faction_name,39);check(leader_title,23);check(leader_name,23);
        check(setup->noun,23);check(setup->adjective,23);check(setup->description,23);
    }
	void Validate() const {
		for (const auto* field : {&faction_name, &leader_title, &leader_name}) {
			if (field->empty() || field->size() > MAX_FIELD_BYTES || field->find_first_of(std::string("\0\r\n", 3)) != std::string::npos) {
				throw std::runtime_error("Invalid faction display identity field");
			}
		}
        if(setup) {
            for(const auto* field:{&setup->noun,&setup->adjective,&setup->description}) {
                if(field->empty() || field->size()>MAX_FIELD_BYTES || field->find_first_of(std::string("\0\r\n",3))!=std::string::npos) {
                    throw std::runtime_error("Invalid faction setup identity field");
                }
            }
        }
	}
	bool operator==(const DisplayIdentity& other) const {
		return faction_name == other.faction_name && leader_title == other.leader_title && leader_name == other.leader_name && setup == other.setup;
	}
};

}
