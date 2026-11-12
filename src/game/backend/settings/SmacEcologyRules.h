#pragma once

#include "types/Buffer.h"
#include <cstdint>
#include <optional>
#include <array>

namespace game::backend::settings {
// Loaded #WORLDBUILDER authority. Absence in legacy settings is not a default 5.
struct SmacEcologyRules {
    static constexpr uint32_t MAX_WIRE_BYTES = 512;
    int32_t sea_level_rises;
    // Full source generation authority, when present. V1/short diagnostic
    // profiles retain absence and cannot silently use stock defaults.
    std::optional<std::array<int32_t,31>> world_builder;
    std::optional<std::array<int32_t,2>> warming_frequency;
    static std::optional<SmacEcologyRules> Read(const std::string& source);
    void Validate() const;
    bool operator==(const SmacEcologyRules& other) const { return sea_level_rises == other.sea_level_rises && world_builder==other.world_builder && warming_frequency==other.warming_frequency; }
    types::Buffer Serialize() const;
    static SmacEcologyRules Deserialize(types::Buffer buffer);
};
}
