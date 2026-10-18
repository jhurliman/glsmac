#pragma once

#include "types/Buffer.h"
#include <cstdint>
#include <optional>

namespace game::backend::settings {
// Loaded #WORLDBUILDER authority. Absence in legacy settings is not a default 5.
struct SmacEcologyRules {
    static constexpr uint32_t MAX_WIRE_BYTES = 28;
    int32_t sea_level_rises;
    static std::optional<SmacEcologyRules> Read(const std::string& source);
    void Validate() const;
    bool operator==(const SmacEcologyRules& other) const { return sea_level_rises == other.sea_level_rises; }
    types::Buffer Serialize() const;
    static SmacEcologyRules Deserialize(types::Buffer buffer);
};
}
