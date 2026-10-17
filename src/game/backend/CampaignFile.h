#pragma once
#include <cstddef>
#include <string>
#include "RosterRecords.h"

namespace game::backend::campaign {
// GLSMAC-native campaign file. The existing settings and world codecs stay authoritative.
struct Record {
    std::string settings;
    roster::RosterRecord roster;
    size_t local_slot;
    std::string world;
};
inline constexpr size_t MAX_BYTES = 128 * 1024 * 1024;
std::string Encode(const Record& record);
Record Decode(const std::string& bytes);
Record Read(const std::string& path);
// Writes beside the destination and atomically replaces it after a successful close.
std::string Write(const std::string& path, const Record& record);
}
