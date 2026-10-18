#include "SmacPlanetMap.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {
using Planet = game::backend::map::SmacPlanetMap;
void Put(std::string& wire, size_t at, uint32_t value) {
    for (size_t byte = 0; byte < 4; ++byte) { wire.at(at + byte) = static_cast<char>(value >> (byte * 8)); }
}
std::string Synthetic() {
    std::string wire(Planet::FILE_BYTES, '\0');
    wire.replace(0, 11, "TERRANMAP\0\x1a", 11);
    Put(wire, 11, 5); Put(wire, 15, 80); Put(wire, 19, 80);
    return wire;
}
bool Rejects(const std::string& wire) {
    try { (void)Planet::Decode(wire); } catch (const std::invalid_argument&) { return true; }
    return false;
}
}
void AddSmacPlanetMapTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC standard Planet container preserves record ordering and opaque fields", GT() {
        auto wire = Synthetic();
        const auto begin = Planet::PREFIX_BYTES + Planet::HEADER_BYTES;
        for (size_t index = 0; index < Planet::TILE_COUNT; ++index) {
            Put(wire, begin + index * Planet::RECORD_BYTES + 8, static_cast<uint32_t>(index) | 0x80000000u);
            Put(wire, begin + index * Planet::RECORD_BYTES + 12, ~static_cast<uint32_t>(index));
            wire[begin + index * Planet::RECORD_BYTES + 43] = static_cast<char>(index % 256);
        }
        wire.back() = static_cast<char>(0xfe);
        const auto decoded = Planet::Decode(wire);
        for (uint32_t y = 0; y < 80; ++y) { for (uint32_t x = y & 1; x < 80; x += 2) {
            const auto index = y * 40 + x / 2;
            const auto& tile = decoded.At(x, y);
            GT_ASSERT(tile.Items() == (index | 0x80000000u) && tile.Landmarks() == ~index && tile.bytes[43] == index % 256, "Parity record ordering or unsigned/opaque bytes lost");
        } }
        GT_ASSERT(decoded.Trailer().back() == 0xfe && decoded.Header()[0] == 80, "Header/trailer bytes lost");
        for (const auto& point : {std::pair<uint32_t,uint32_t>{80,0}, {0,80}, {1,0}, {UINT32_MAX,UINT32_MAX}}) {
            bool rejected = false; try { decoded.At(point.first, point.second); } catch (const std::out_of_range&) { rejected = true; }
            GT_ASSERT(rejected, "Invalid coordinate accepted");
        }
        GT_OK();
    });
    task->AddTest("SMAC standard Planet container rejects malformed lengths signatures versions and geometry", GT() {
        const auto wire = Synthetic();
        for (const size_t length : {size_t(0), size_t(11), size_t(15), Planet::PREFIX_BYTES + Planet::HEADER_BYTES - 1, Planet::FILE_BYTES - 129, Planet::FILE_BYTES - 1}) {
            GT_ASSERT(Rejects(wire.substr(0,length)), "Truncated container accepted");
        }
        GT_ASSERT(Rejects(wire + "x"), "Trailing unknown bytes accepted");
        for (const auto at : {size_t(0), size_t(9), size_t(10), size_t(11), size_t(15), size_t(19)}) {
            auto malformed = wire; malformed[at] ^= 1;
            GT_ASSERT(Rejects(malformed), "Malformed signature/version/geometry accepted");
        }
        GT_OK();
    });
}
}
