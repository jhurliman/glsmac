#pragma once
#include <cstdint>
#include <string_view>
namespace game {
inline uint8_t SmacOrderForegroundIndex(std::string_view original_filename) noexcept {return original_filename=="SPARTANS"?44:0;}
}
