#include "SmacPlanetMap.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/settings/Settings.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacBloomTerrain.h"
#include "game/backend/map/GlsmacResourceMapPolicy.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/WorldSnapshotRules.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <type_traits>
#include "game/backend/SmacGameplayRandomState.h"

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
    task->AddTest("SMAC Planet bloom authority preserves opaque geometry and survives saved resource policy", GT() {
        using Policy = game::backend::map::GlsmacResourceMapPolicy;
        using Coordinate = std::array<int32_t,2>;
        for (const auto& row : smac_bloom_terrain_fixture::cases) {
            auto wire = Synthetic(); Put(wire, 15 + 0xa30, 4);
            const auto offset = [](Coordinate c) {
                return Planet::PREFIX_BYTES + Planet::HEADER_BYTES + (size_t(c[1]) * 40 + size_t(c[0]) / 2) * Planet::RECORD_BYTES;
            };
            std::map<Coordinate,game::backend::base::SmacBloomTerrainCell> cells;
            for (const auto& cell : row.cells) { cells.emplace(cell.coordinate, cell.value); }
            for (int32_t y = 0; y < 80; ++y) { for (int32_t x = y & 1; x < 80; x += 2) {
                const Coordinate c{x,y}; const auto at = offset(c);
                const auto found = cells.find(c); const auto cell = found == cells.end() ? row.default_tile : found->second;
                for (size_t byte = 0; byte < Planet::RECORD_BYTES; ++byte) { wire[at + byte] = static_cast<char>((at + byte) % 256); }
                wire[at] = static_cast<char>((cell.altitude_class << 5) | 0x12);
                wire[at + 4] = 0; wire[at + 5] = static_cast<char>((cell.rockiness << 6) | 0x2d);
                wire[at + 7] = static_cast<char>(0xa0 | cell.affiliation);
                Put(wire, at + 8, cell.items & ~(3u | 0x800000u)); Put(wire, at + 12, cell.landmarks);
            } }
            namespace b = game::backend::base;
            const auto query = [&](int32_t x,int32_t y) {
                if (x<0 || x>=row.map.width || y<0 || y>=row.map.height || ((x+y)&1)) {
                    throw std::runtime_error("Invalid bloom producer query");
                }
                const auto found=cells.find({x,y});return found==cells.end() ? row.default_tile : found->second;
            };
            game::backend::SmacGameplayRandomState rng(1234,row.seed);size_t draws=0;
            const auto draw = [&]() -> int32_t {
                const auto raw=rng.NextCrtRaw15();
                if(draws>=row.draws.size() || raw!=row.draws[draws].raw || rng.Crt()!=row.draws[draws].after) {
                    throw std::runtime_error("Native saved-map CRT differs: "+std::string(row.id));
                }
                ++draws;return raw;
            };
            GT_ASSERT(b::AdmitSmacEcologyBloom(row.damage,row.perihelion,row.primary,row.secondary,draw), "Native bloom entry rejected");
            const auto winner=b::SelectSmacEcologyBloom(row.map,row.base,row.bases,[&](int32_t x,int32_t y) {
                const auto cell=query(x,y);uint32_t improvements=0;
                for(const auto bit:std::array<std::pair<uint32_t,uint32_t>,5>{{
                    {0x10,b::SmacEcologySquare::Mine},{0x40,b::SmacEcologySquare::Solar},{0x8000,b::SmacEcologySquare::Farm},
                    {0x80000,b::SmacEcologySquare::SoilEnricher},{0x200000,b::SmacEcologySquare::Forest}}}) {
                    if(cell.items&bit.first) {improvements|=bit.second;}
                }
                return b::SmacBloomTile{cell.altitude_class,bool(cell.items&0x20u),bool((cell.items&1u) && cell.affiliation<=7),improvements};
            },draw);
            GT_ASSERT(winner && winner->coordinate==row.winner, "Native saved-map winner differs");
            auto region=row.map;region.center=winner->coordinate;
            const auto plan=b::PrepareSmacBloomTerrain(region,row.history,row.difficulty,true,query,draw);
            GT_ASSERT(draws==row.draws.size() && rng.Shared()==1234, "Saved-map producer changed RNG consumption");
            const auto original = Policy::InstalledPlanetV1(Planet::Decode(wire), true);
            const auto prepared = original.WithBloomTerrain(plan);
            auto expected = wire;
            for (const auto& delta : row.changes) {
                const auto at = offset(delta.coordinate);
                expected[at] = static_cast<char>((delta.after.altitude_class << 5) | 0x12);
                expected[at + 5] = static_cast<char>((delta.after.rockiness << 6) | 0x2d);
                const auto old_items = original.OriginalPlanet()->At(delta.coordinate[0], delta.coordinate[1]).Items();
                Put(expected, at + 8, (old_items & ~0x00288070u) | (delta.after.items & 0x00288070u));
                Put(expected, at + 12, delta.after.landmarks);
            }
            GT_ASSERT(original.OriginalPlanet()->Encode() == wire && prepared.OriginalPlanet()->Encode() == expected,
                "Bloom changed source authority, contours, region or opaque bytes: " + std::string(row.id));
            const auto restored = Policy::Deserialize(prepared.Serialize().ToString());
            GT_ASSERT(restored == prepared && restored.AutomaticLandResources(), "Saved bloom resource authority changed");
            for (const auto& delta : row.changes) {
                game::backend::map::tile::Tile tile;
                tile.coord = {static_cast<size_t>(delta.coordinate[0]), static_cast<size_t>(delta.coordinate[1])};
                GT_ASSERT(static_cast<int32_t>(restored.Altitude(tile)) == delta.after.altitude_class,
                    "Reloaded resource calculation retained pre-bloom altitude");
            }
        }
        GT_OK();
    });
    task->AddTest("SMAC Planet bloom authority accepts only producer plans and rejects stale late peers atomically", GT() {
        using Policy = game::backend::map::GlsmacResourceMapPolicy;
        namespace b = game::backend::base;
        using Plan = b::SmacBloomTerrainPlan;
        static_assert(!std::is_default_constructible_v<Plan> && !std::is_aggregate_v<Plan>, "Only the producer constructs plans");
        static_assert(std::is_same_v<decltype(std::declval<Plan&>().Changes()),const std::vector<b::SmacBloomTerrainDelta>&>,
            "Consumers cannot mutate or concatenate producer deltas");
        const auto plan=b::PrepareSmacBloomTerrain({{8,6},16,12,true},3,2,true,
            [](int32_t,int32_t){return b::SmacBloomTerrainCell{0,0,0,0,15};},[]{return 1;});
        GT_ASSERT(plan.NonzeroTurn() && plan.Changes().size()>1, "Expected a multi-peer producer plan");
        auto wire = Synthetic(); Put(wire, 15 + 0xa30, 4);
        const auto original=Policy::InstalledPlanetV1(Planet::Decode(wire),false);
        const auto prepared=original.WithBloomTerrain(plan);
        GT_ASSERT(original.OriginalPlanet()->Encode()==wire && prepared.OriginalPlanet()->Encode()!=wire,
            "Detached producer plan failed to prepare changed authority");
        const auto& late=plan.Changes().back();
        const auto at=Planet::PREFIX_BYTES+Planet::HEADER_BYTES+(size_t(late.coordinate[1])*40+size_t(late.coordinate[0])/2)*Planet::RECORD_BYTES;
        wire[at]=static_cast<char>((late.before.altitude_class+1)<<5);
        const auto stale=Policy::InstalledPlanetV1(Planet::Decode(wire),false);
        bool rejected=false;
        try { (void)stale.WithBloomTerrain(plan); } catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected && stale.OriginalPlanet()->Encode()==wire, "Late stale peer partly published");
        rejected=false;
        try { (void)Policy::DisplayedTerrainV1().WithBloomTerrain(plan); } catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected, "Generated terrain invented original class authority");
        GT_OK();
    });
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
