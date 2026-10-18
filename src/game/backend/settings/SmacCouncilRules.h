#pragma once

#include "SmacTechnologyRules.h"
#include <array>
#include <optional>

namespace game::backend::settings {
// Frozen common #PROPOSALS prerequisite columns, not election eligibility.
struct SmacCouncilRules {
    static constexpr size_t MAX_TOKEN_BYTES=256;
    static constexpr size_t MAX_WIRE_BYTES=14+11*(6+MAX_TOKEN_BYTES);
    std::array<std::string,11> prerequisites;
    static std::optional<SmacCouncilRules> Read(const std::string& source,const SmacTechnologyRules& technologies);
    void Validate(const SmacTechnologyRules& technologies) const;
    SmacTechnologyReference SupremeLeader(const SmacTechnologyRules& technologies) const;
    bool operator==(const SmacCouncilRules& other) const {return prerequisites==other.prerequisites;}
    types::Buffer Serialize(const SmacTechnologyRules& technologies) const;
    static SmacCouncilRules Deserialize(types::Buffer buffer,const SmacTechnologyRules& technologies);
};
}
