#include "SmacPlanetMap.h"

#include <algorithm>
#include <stdexcept>
#include <set>
#include "game/backend/base/SmacEcology.h"

namespace game::backend::map {
namespace {
uint32_t Read32(const uint8_t* value) {
    return uint32_t(value[0]) | (uint32_t(value[1]) << 8) | (uint32_t(value[2]) << 16) | (uint32_t(value[3]) << 24);
}
int32_t ReadSigned32(const uint8_t* bytes) {
    const auto value = Read32(bytes);
    return value <= INT32_MAX ? static_cast<int32_t>(value) : static_cast<int32_t>(int64_t(value) - (int64_t(1) << 32));
}
}

uint32_t SmacPlanetMap::Tile::Items() const { return Read32(bytes.data() + 8); }
uint32_t SmacPlanetMap::Tile::Landmarks() const { return Read32(bytes.data() + 12); }
int32_t SmacPlanetMap::ResourcePattern() const {
    const auto value = Read32(m_header.data() + 8);
    return value <= INT32_MAX ? static_cast<int32_t>(value) : static_cast<int32_t>(int64_t(value) - (int64_t(1) << 32));
}
int32_t SmacPlanetMap::OceanCoverageCategory() const {
    const auto value=Read32(m_header.data()+0xa34);
    return value<=INT32_MAX ? static_cast<int32_t>(value) : static_cast<int32_t>(int64_t(value)-(int64_t(1)<<32));
}
int32_t SmacPlanetMap::WorldSizeCategory() const { return static_cast<int32_t>(Read32(m_header.data() + 0xa30)); }
int32_t SmacPlanetMap::CloudCoverCategory() const { return ReadSigned32(m_header.data() + 0xa44); }
int32_t SmacPlanetMap::NativeLifeCategory() const { return ReadSigned32(m_header.data() + 0xa48); }
int32_t SmacPlanetMap::CumulativeSeaLevelOffset() const { return ReadSigned32(m_header.data() + 0x00c); }

std::string SmacPlanetMap::Encode() const {
    if (m_tiles.size() != TILE_COUNT) { throw std::logic_error("Standard Planet container is incomplete"); }
    std::string result("TERRANMAP\0\x1a\x05\0\0\0", PREFIX_BYTES);
    result.append(reinterpret_cast<const char*>(m_header.data()), m_header.size());
    for (const auto& tile : m_tiles) { result.append(reinterpret_cast<const char*>(tile.bytes.data()), tile.bytes.size()); }
    result.append(reinterpret_cast<const char*>(m_trailer.data()), m_trailer.size());
    return result;
}

SmacPlanetMap SmacPlanetMap::WithBloomTerrain(const base::SmacBloomTerrainPlan& plan) const {
    if (m_tiles.size() != TILE_COUNT) { throw std::logic_error("Standard Planet container is incomplete"); }
    auto result = *this;
    std::set<std::array<int32_t,2>> visited;
    constexpr uint32_t event_items = 0x00288070u; // fungus plus five removable improvements
    const auto write32 = [](uint8_t* bytes, uint32_t value) {
        for (size_t byte = 0; byte < 4; ++byte) { bytes[byte] = static_cast<uint8_t>(value >> (8 * byte)); }
    };
    for (const auto& delta : plan.Changes()) {
        const auto x = delta.coordinate[0], y = delta.coordinate[1];
        if (x < 0 || y < 0 || x >= static_cast<int32_t>(WIDTH) || y >= static_cast<int32_t>(HEIGHT)
            || ((x ^ y) & 1) || !visited.insert(delta.coordinate).second) {
            throw std::invalid_argument("Invalid or duplicate Planet bloom coordinate");
        }
        const auto& before = delta.before;
        const auto& after = delta.after;
        auto& tile = result.m_tiles.at(size_t(y) * (WIDTH / 2) + size_t(x) / 2);
        if ((tile.Climate() >> 5) != before.altitude_class || (tile.bytes[5] >> 6) != before.rockiness
            || tile.Landmarks() != before.landmarks) {
            throw std::invalid_argument("Planet bloom terrain authority is stale");
        }
        tile.bytes[0] = static_cast<uint8_t>((tile.bytes[0] & 0x1f) | (after.altitude_class << 5));
        tile.bytes[5] = static_cast<uint8_t>((tile.bytes[5] & 0x3f) | (after.rockiness << 6));
        // Source items are an import projection, not the live Base/Unit registry
        // or current Former state. Copy only the event-owned terrain bits.
        write32(tile.bytes.data() + 8, (tile.Items() & ~event_items) | (after.items & event_items));
        write32(tile.bytes.data() + 12, after.landmarks);
    }
    return result;
}

SmacPlanetMap SmacPlanetMap::Decode(const std::string& bytes) {
    // Check the exact bounded layout before reading any scalar or allocating
    // records. Other versions/maps are not silently accepted as standard Planet.
    if (bytes.size() != FILE_BYTES) { throw std::invalid_argument("Standard Planet map has an unsupported length"); }
    const auto* data = reinterpret_cast<const uint8_t*>(bytes.data());
    static constexpr uint8_t signature[] = {'T','E','R','R','A','N','M','A','P',0,0x1a};
    if (!std::equal(std::begin(signature), std::end(signature), data) || Read32(data + 11) != 5) {
        throw std::invalid_argument("Standard Planet map requires TERRANMAP version 5");
    }
    if (Read32(data + PREFIX_BYTES) != WIDTH || Read32(data + PREFIX_BYTES + 4) != HEIGHT) {
        throw std::invalid_argument("Standard Planet map requires 80 by 80 parity coordinates");
    }
    SmacPlanetMap result;
    std::copy_n(data + PREFIX_BYTES, HEADER_BYTES, result.m_header.begin());
    result.m_tiles.resize(TILE_COUNT);
    const auto* records = data + PREFIX_BYTES + HEADER_BYTES;
    for (size_t index = 0; index < TILE_COUNT; ++index) {
        std::copy_n(records + index * RECORD_BYTES, RECORD_BYTES, result.m_tiles[index].bytes.begin());
    }
    std::copy_n(records + TILE_COUNT * RECORD_BYTES, TRAILER_BYTES, result.m_trailer.begin());
    return result;
}

const SmacPlanetMap::Tile& SmacPlanetMap::At(uint32_t x, uint32_t y) const {
    if (x >= WIDTH || y >= HEIGHT || (x & 1) != (y & 1)) { throw std::out_of_range("Invalid standard Planet tile coordinate"); }
    return m_tiles.at(size_t(y) * (WIDTH / 2) + x / 2);
}

}
