#include "SmacPlanetMap.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/settings/Settings.h"
#include "fixtures/SmacRulesSource.h"
#include "game/backend/WorldSnapshotRules.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <fstream>
#include <iterator>

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
    task->AddTest("SMAC Planet ecology header words retain independent signed32 authorities", GT() {
        for (const int32_t value : {0, 1, 2, -1, 16909060, INT32_MIN, INT32_MAX}) {
            auto wire = Synthetic();
            Put(wire, 15 + 0x00c, static_cast<uint32_t>(value));
            Put(wire, 15 + 0xa44, static_cast<uint32_t>(~value));
            Put(wire, 15 + 0xa48, static_cast<uint32_t>(value));
            const auto map = Planet::Decode(wire);
            GT_ASSERT(map.CumulativeSeaLevelOffset() == value && map.CloudCoverCategory() == ~value
                && map.NativeLifeCategory() == value && map.Encode() == wire, "Ecology fields aliased, lost sign or changed the map bytes");
        }
        GT_OK();
    });
    if (const char* asset = std::getenv("GLSMAC_TEST_PLANET_ASSET")) {
        const std::string path = asset;
        task->AddTest("SMAC installed Planet ecology header matches original loader1 1 0", GT(path) {
            std::ifstream input(path, std::ios::binary);
            GT_ASSERT(input.good(), "Installed Planet asset unavailable");
            const std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
            const auto map = Planet::Decode(bytes);
            GT_ASSERT(map.CloudCoverCategory() == 1 && map.NativeLifeCategory() == 1 && map.CumulativeSeaLevelOffset() == 0,
                "Installed map ecology authority differs from original loader");
            GT_OK();
        });
    }
    task->AddTest("SMAC standard Planet native loader stages terrain topology cancellation and failure atomically", GT() {
        return CheckInstalledPlanetBackend();
    });
    if(const char* asset=std::getenv("GLSMAC_TEST_PLANET_ASSET")) {
        const std::string path=asset;
        task->AddTest("SMAC installed Planet asset matches original all-tile classification and resource counts", GT(path) {
            return CheckInstalledPlanetBackend(path);
        });
    }
    task->AddTest("SMAC campaign option producers preserve distinct active remembered and objective words", GT() {
        using game::backend::settings::SmacCampaignRules;
        SmacCampaignRules rules;
        rules.primary=0x6040ffff;rules.secondary=0x05800010;rules.remembered_primary=0xabcd0020;
        rules.remembered_secondary=0x01000000;rules.objective_facility=31;
        rules.SelectStandard();
        GT_ASSERT(rules.primary==0x60401a0e && rules.secondary==0x05800010 && rules.remembered_primary==0xabcd0020
            && rules.objective_facility==31 && !rules.IsOrdinary(),"Standard reset unrelated original words or established ordinary authority");
        GT_ASSERT(rules.BlindResearch() && rules.CooperativeVictory() && !rules.AutomaticLandResources() && !rules.Stagnation(),"Standard bit meanings changed");
        rules.SelectCurrent(false);
        GT_ASSERT(rules.primary==0x60400020 && rules.secondary==0x05000010 && !rules.BlindResearch() && rules.Stagnation(),"Current did not restore remembered low rules and independent randomization bits");
        rules.SelectCurrent(true);GT_ASSERT(rules.primary==0x60400220,"Citizen did not force Blind Research");
        rules.BeginOrdinary();rules.SelectStandard();
        GT_ASSERT(rules.IsOrdinary() && rules.primary==0x1a0e && rules.secondary==0x01000000 && rules.objective_facility==0
            && rules.remembered_primary==0xabcd0020,"Explicit fresh ordinary setup did not retain preferences independently");
        GT_ASSERT(SmacCampaignRules::Deserialize(rules.Serialize())==rules,"Saved campaign options changed");
        bool rejected=false;try {SmacCampaignRules::Deserialize(rules.Serialize()+"x");} catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Trailing campaign option fields accepted");
        GT_OK();
    });
    task->AddTest("SMAC installed Planet settings retain explicit options across frozen settings and world headers", GT() {
        using namespace game::backend::settings;
        GlobalSettings host;host.Initialize();host.SelectRuleset("smac");host.InitializeHostRules(smac_rules_fixture::Source);
        for(const float previous : {0.4f,0.8f,-1.0f,77.0f}) {
            host.map.ocean_coverage=previous;host.SelectInstalledPlanet();
            GT_ASSERT(host.map.GetSmacOceanCoverage()==1,"Planet retained abandoned generator ocean authority");
        }
        host.SelectSmacCampaignRules(false);host.LockWorldSize();
        GT_ASSERT(host.GetSmacCampaignRules() && host.GetSmacCampaignRules()->primary==0x1a0e
            && host.IsOrdinarySmacCampaign() && !host.GetSmacCampaignRules()->AutomaticLandResources(),"Ordinary Standard Planet authority missing");
        GlobalSettings restored;restored.Deserialize(host.Serialize());
        GT_ASSERT(restored.GetSmacCampaignRules()==host.GetSmacCampaignRules() && restored.map.type==MapSettings::MT_INSTALLED_PLANET
            && *restored.map.GetSmacWorldSize()==SmacWorldSizeSelection({4,80,80}),"Settings restore lost source category or options");
        types::Buffer world;game::backend::world_snapshot::WriteHeader(world,host);
        game::backend::world_snapshot::ValidateHeader(world,restored);
        bool rejected=false;try {host.SelectSmacCampaignRules(true);} catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Campaign options changed after world freeze");
        GT_OK();
    });
    task->AddTest("SMAC Planet ocean field decodes signed little endian independently of geometry",GT(){
        for(const int32_t category : {0,1,2,-1,3,16909060}) {
            auto wire=Synthetic();Put(wire,15+0xa34,static_cast<uint32_t>(category));
            GT_ASSERT(Planet::Decode(wire).OceanCoverageCategory()==category,"Ocean header field changed signed byte interpretation");
        }
        GT_OK();
    });
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
