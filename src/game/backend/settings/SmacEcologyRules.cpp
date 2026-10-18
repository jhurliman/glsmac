#include "SmacEcologyRules.h"
#include "loader/txt/TXTLoader.h"
#include "util/String.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::settings {
std::optional<SmacEcologyRules> SmacEcologyRules::Read(const std::string& source) {
    bool present = false;
    for (const auto& line : util::String::Split(source, '\n')) {
        if (util::String::TrimCopy(line) == "#WORLDBUILDER") { present = true; break; }
    }
    if (!present) { return {}; }
    const auto rows = loader::txt::TXTLoader::ReadUniqueSection(source, "WORLDBUILDER");
    if (rows.size() <= 14) { throw std::invalid_argument("WORLDBUILDER lacks Sea Level Rises row15"); }
    const auto row = rows[14].substr(0, rows[14].find(';'));
    const auto comma = row.find(',');
    if (comma != std::string::npos && !util::String::TrimCopy(row.substr(comma + 1)).empty()) {
        throw std::invalid_argument("Sea Level Rises requires one numeric field");
    }
    const auto token = util::String::TrimCopy(row.substr(0, comma));
    // Deliberately admit signed32 decimal tokens, then apply the native loader
    // clamp. Reject malformed/overflowing source instead of atoi-style zeros.
    const auto digits = !token.empty() && (token[0] == '+' || token[0] == '-') ? 1u : 0u;
    if (token.size() <= digits || token.find_first_not_of("0123456789", digits) != std::string::npos) {
        throw std::invalid_argument("Sea Level Rises is not a decimal integer");
    }
    const auto value = std::stoll(token);
    if (value < INT32_MIN || value > INT32_MAX) { throw std::invalid_argument("Sea Level Rises exceeds signed32"); }
    return SmacEcologyRules{static_cast<int32_t>(std::clamp<int64_t>(value, 1, 100))};
}
void SmacEcologyRules::Validate() const {
    if (sea_level_rises < 1 || sea_level_rises > 100) { throw std::invalid_argument("Loaded Sea Level Rises must be1..100"); }
}
types::Buffer SmacEcologyRules::Serialize() const {
    Validate(); types::Buffer buffer; buffer.WriteInt(1); buffer.WriteInt(sea_level_rises); return buffer;
}
SmacEcologyRules SmacEcologyRules::Deserialize(types::Buffer buffer) {
    if (buffer.lenw > MAX_WIRE_BYTES || buffer.ReadInt() != 1) { throw std::runtime_error("Unsupported or oversized ecology profile"); }
    const auto value = buffer.ReadInt();
    if (value < 1 || value > 100 || buffer.lenr != buffer.lenw) { throw std::runtime_error("Invalid loaded ecology profile"); }
    return {static_cast<int32_t>(value)};
}
}
