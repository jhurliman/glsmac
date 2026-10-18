#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
namespace game::backend::base {
// Read-only original ordinary-item label. The caller supplies the current
// recalculated surplus, not a retained upkeep value or a future-turn guarantee.
inline std::optional<std::string> FormatSmacProductionForecast(int32_t cost,int64_t stored,int32_t surplus) {
    if(cost<0 || stored<INT32_MIN || stored>INT32_MAX) {throw std::invalid_argument("Production display requires its signed32 cost/store domain");}
    if(cost==0) {return std::nullopt;} // original skips the ordinary cost block
    if(surplus<=0) {return "No Minerals";} // precedes the already-funded check
    int32_t turns=0;
    if(stored<cost) {
        // Original signed32 numerator wraps. Perform its bit arithmetic unsigned
        // and decode explicitly, avoiding signed overflow/implementation-defined casts.
        const uint32_t missing=uint32_t(cost)-uint32_t(static_cast<int32_t>(stored));
        const uint32_t bits=uint32_t(surplus)+missing-1u;
        const int64_t numerator=bits<=uint32_t(INT32_MAX) ? int64_t(bits) : int64_t(bits)-INT64_C(4294967296);
        const auto denominator=std::clamp(surplus,1,99999);
        turns=static_cast<int32_t>(std::clamp<int64_t>(numerator/denominator,0,999999));
    }
    return "TURNS:  "+std::to_string(turns);
}
}
