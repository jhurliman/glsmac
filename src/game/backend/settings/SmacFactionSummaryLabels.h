#pragma once
#include "types/Buffer.h"
#include <array>
#include <optional>
#include <string>

namespace game::backend::settings {
// Optional retained presentation catalog. Old saves never acquire current
// installed text on load; numeric gameplay authorities remain independent.
struct SmacFactionSummaryLabels {
    static constexpr size_t MAX_LABEL_BYTES=256, MAX_WIRE_BYTES=8192;
    std::array<std::string,8> repute;
    std::array<std::string,7> might; // source order: rank7 down to rank1
    static std::optional<SmacFactionSummaryLabels> Read(const std::string& source);
    void Validate() const;
    types::Buffer Serialize() const;
    static SmacFactionSummaryLabels Deserialize(types::Buffer buffer);
    const std::string& Integrity(uint32_t word) const;
    const std::string& Might(int32_t rank) const;
    bool operator==(const SmacFactionSummaryLabels& other) const {return repute==other.repute && might==other.might;}
};
}
