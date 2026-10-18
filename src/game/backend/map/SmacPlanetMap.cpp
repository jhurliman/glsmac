#include "SmacPlanetMap.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::map {
namespace {
uint32_t Read32(const uint8_t* value) {
    return uint32_t(value[0]) | (uint32_t(value[1]) << 8) | (uint32_t(value[2]) << 16) | (uint32_t(value[3]) << 24);
}
}

uint32_t SmacPlanetMap::Tile::Items() const { return Read32(bytes.data() + 8); }
uint32_t SmacPlanetMap::Tile::Landmarks() const { return Read32(bytes.data() + 12); }

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
