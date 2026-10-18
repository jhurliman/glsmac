#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace game::backend::map {

// Bounded reader for the installed standard Planet v5 container. Keeps every
// original byte: decoding a record is distinct from admitting gameplay rules
// or projecting its contour onto GLSMAC's shared rendering vertices.
class SmacPlanetMap {
public:
    static constexpr uint32_t WIDTH = 80, HEIGHT = 80;
    static constexpr size_t HEADER_BYTES = 0xaa4, RECORD_BYTES = 44;
    static constexpr size_t TILE_COUNT = WIDTH / 2 * HEIGHT;
    static constexpr size_t PREFIX_BYTES = 15, TRAILER_BYTES = 128;
    static constexpr size_t FILE_BYTES = PREFIX_BYTES + HEADER_BYTES + TILE_COUNT * RECORD_BYTES + TRAILER_BYTES;
    struct Tile {
        std::array<uint8_t, RECORD_BYTES> bytes{};
        uint8_t Climate() const { return bytes[0]; }
        uint8_t Contour() const { return bytes[1]; }
        uint8_t Region() const { return bytes[3]; }
        uint8_t Owner() const { return bytes[7]; }
        uint32_t Items() const;
        uint32_t Landmarks() const;
    };
    static SmacPlanetMap Decode(const std::string& bytes);
    std::string Encode() const;
    int32_t ResourcePattern() const;
    int32_t WorldSizeCategory() const;
    int32_t OceanCoverageCategory() const;
    int32_t CloudCoverCategory() const;
    int32_t NativeLifeCategory() const;
    int32_t CumulativeSeaLevelOffset() const;
    const Tile& At(uint32_t x, uint32_t y) const;
    const std::array<uint8_t, HEADER_BYTES>& Header() const { return m_header; }
    const std::array<uint8_t, TRAILER_BYTES>& Trailer() const { return m_trailer; }
private:
    std::array<uint8_t, HEADER_BYTES> m_header{};
    std::array<uint8_t, TRAILER_BYTES> m_trailer{};
    std::vector<Tile> m_tiles;
};

}
