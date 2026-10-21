#include "SmacPlanetMap.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "game/backend/map/SmacClimate.h"
#include "game/backend/map/SmacGlobalSpread.h"
#include "game/backend/map/SmacStartingBalance.h"
#include "game/backend/settings/Settings.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacBloomTerrain.h"
#include "game/backend/map/GlsmacResourceMapPolicy.h"
#include "game/backend/map/SmacTerrainAuthority.h"
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
    task->AddTest("SMAC generated terrain mutations preserve raw authority topology and ordered journals",GT(){
        using Authority=game::backend::map::SmacTerrainAuthority;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        namespace spread=game::backend::map::smac_spread;namespace balance=game::backend::map::smac_balance;
        Authority::Metadata meta;meta.width=80;meta.height=80;meta.horizontal_wrap=true;meta.pattern=11446;meta.generation_primary_rules=0x3a0e;
        for(size_t i=0;i<7;++i) {meta.categories[i]=1;}meta.categories[0]=2;
        std::string raw(3200*44,0);for(size_t i=0;i<3200;++i) {raw[i*44]=96;raw[i*44+1]=87;raw[i*44+6]=27;raw[i*44+7]=char(0xa5);}
        const auto off=[](int x,int y){return (size_t(y)*40+size_t(x/2))*44;};
        Put(raw,off(41,67)+8,0x200000);Put(raw,off(41,67)+16,0x200000);
        const auto policy=Policy::GeneratedTerrainV3(Authority::GeneratedRetained(meta,raw));const auto queued=policy.NativeTerrainSource();
        spread::Input before{{80,80,true},3200,1,{},{}};
        for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {before.tiles.push_back(queued->At(x,y));}}
        before.tiles[67*40+41/2].bytes[4]=2;
        before.tiles[66*40+42/2].bytes[8]=0x24;
        const auto plan=spread::Prepare(before,game::backend::SmacGameplayRandomState(123,1));
        GT_ASSERT(!plan.changes.empty(),"Generated spread test did not run the producer");
        const auto changed=policy.WithLiveGlobalSpread(before,plan);std::string expected;
        for(const auto& cell:plan.tiles) {expected.append(reinterpret_cast<const char*>(cell.bytes.data()),44);}
        GT_ASSERT(changed.NativeTerrainSource()->RecordBytes()==expected && queued->RecordBytes()==raw && !changed.OriginalPlanet()
            && Policy::Deserialize(changed.Serialize().ToString())==changed,"Generated live flora lost bytes, source or provenance");
        bool rejected=false;auto malformed=plan;malformed.changes.back().after.bytes[1]^=1;
        try {(void)policy.WithLiveGlobalSpread(before,malformed);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected && queued->RecordBytes()==raw,"Late generated flora journal failure changed its source");
        rejected=false;auto flat=before;flat.geometry.wrap=false;
        try {(void)policy.WithLiveGlobalSpread(flat,plan);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Generated flora accepted mismatched topology");
        balance::Result markers{game::backend::SmacGameplayRandomState(1,1),{},{},{},{{1,{{0,0}},0,0x04000000,0x04000000},{1,{{0,0}},0x04000000,0x08000000,0x0c000000}}};
        const auto marked=policy.WithStartingBalance(markers);auto marked_bytes=raw;Put(marked_bytes,8,0x0c000000);
        GT_ASSERT(marked.NativeTerrainSource()->RecordBytes()==marked_bytes && !marked.OriginalPlanet(),"Generated starting markers lost order or unowned fields");
        markers.writes.back().before=0;rejected=false;
        try {(void)policy.WithStartingBalance(markers);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected && queued->RecordBytes()==raw,"Stale late starting marker partly published");
        // Flat, non-Planet geometry and retained raw climate codes exercise the
        // common adapter independently of the wrapped installed container.
        meta.width=16;meta.height=8;meta.horizontal_wrap=false;meta.categories[0]=0;
        std::string flat_records(64*44,0);for(size_t i=0;i<64;++i) {flat_records[44*i]=char(255);flat_records[44*i+2]=char(0xaf);flat_records[44*i+3]=1;flat_records[44*i+7]=char(0xe7);}
        const auto initial=Policy::GeneratedTerrainV3(Authority::GeneratedRetained(meta,flat_records));auto climate=initial.NativeTerrainSource()->ExportClimateTerrain();
        GT_ASSERT(!climate.horizontal_wrap && climate.tiles.front().temperature==7 && climate.tiles.front().moisture==3,"Raw retained climate codes were normalized");
        climate.tiles[0]={6,7,3,255,129,0x81234567u,0xefa98765u,3,2,{{1,2,3,4,5,6,UINT32_MAX}}};
        const auto current=initial.WithClimateTerrain(climate,-11,7);const auto bytes=current.NativeTerrainSource()->RecordBytes();
        GT_ASSERT(uint8_t(bytes[0])==223 && uint8_t(bytes[2])==0xa3 && bytes[3]==1 && uint8_t(bytes[7])==0xe7
            && current.NativeTerrainSource()->Info().sea_offset==-11 && current.NativeTerrainSource()->Info().climate_extra==7
            && Policy::Deserialize(current.Serialize().ToString())==current && initial.NativeTerrainSource()->RecordBytes()==flat_records,
            "Generated climate changed unrelated fields, topology, metadata or queued source");
        rejected=false;climate.horizontal_wrap=true;
        try {(void)initial.WithClimateTerrain(climate,0,0);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Generated climate accepted another topology");GT_OK();
    });
    task->AddTest("SMAC Planet terrain authority preserves provenance opaque bytes and policy codecs",GT(){
        using Policy=game::backend::map::GlsmacResourceMapPolicy;using Authority=game::backend::map::SmacTerrainAuthority;
        auto wire=Synthetic();Put(wire,15+0xa30,4);Put(wire,15+0xa34,1);Put(wire,15+8,11446);
        Put(wire,15+0xa40,2);Put(wire,15+0xa44,0);Put(wire,15+0xa48,2);
        Put(wire,15+0x0c,uint32_t(-19));Put(wire,15+0x10,uint32_t(-3));
        Put(wire,15+0xa38,0xf1f2f3f4u);Put(wire,15+0xa3c,0x12345678u);
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*44]=96;}
        wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+6]=char(165);wire.back()=char(188);
        auto policy=Policy::InstalledPlanetV1(Planet::Decode(wire),true);const auto snapshot=policy.NativeTerrainSource();
        GT_ASSERT(snapshot && snapshot->SourceOrigin()==Authority::Origin::Installed && snapshot->InstalledSource()==policy.OriginalPlanetSource(),
            "Native terrain and installed source lost one immutable authority");
        const auto& meta=snapshot->Info();
        GT_ASSERT(meta.width==80 && meta.height==80 && meta.horizontal_wrap && meta.pattern==11446 && meta.sea_offset==-19 && meta.climate_extra==-3
            && meta.categories[0]==4 && meta.categories[1]==1 && meta.categories[4]==2 && meta.categories[5]==0 && meta.categories[6]==2
            && !meta.categories[2] && !meta.categories[3] && !meta.generation_primary_rules,"Installed metadata was guessed or changed");
        const auto raw=wire.substr(Planet::PREFIX_BYTES+Planet::HEADER_BYTES,Planet::TILE_COUNT*Planet::RECORD_BYTES);
        GT_ASSERT(snapshot->RecordBytes()==raw && snapshot->InstalledSource()->Encode()==wire,"Native terrain changed opaque container bytes");
        types::Buffer expected;expected.WriteInt(1);expected.WriteInt(2);expected.WriteBool(true);expected.WriteString(wire);
        GT_ASSERT(policy.Serialize().ToString()==expected.ToString(),"Shared terrain authority changed the saved policy wire");
        const auto restored=Policy::Deserialize(expected.ToString());
        GT_ASSERT(restored==policy && restored.NativeTerrainSource()->RecordBytes()==raw && restored.OriginalPlanet()->Encode()==wire,
            "Policy restore did not reconstruct the common terrain authority");
        policy=Policy::DisplayedTerrainV1();
        GT_ASSERT(!policy.NativeTerrainSource() && snapshot->RecordBytes()==raw,"Retired policy invalidated a queued terrain snapshot");GT_OK();
    });
    task->AddTest("SMAC Planet live flora publication preserves unowned bits and saves current observer memory",GT(){
        namespace spread=game::backend::map::smac_spread;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        auto wire=Synthetic();Put(wire,15+0xa30,4);
        const auto address=[](int x,int y) {return Planet::PREFIX_BYTES+Planet::HEADER_BYTES+(size_t(y)*40+x/2)*44;};
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {wire[Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*44]=96;}
        Put(wire,address(41,67)+8,0x200000);Put(wire,address(41,67)+16,0x200000);
        Put(wire,address(78,78)+8,0x10000000);Put(wire,address(78,78)+16,0x10000000);
        const auto source=Planet::Decode(wire);const auto policy=Policy::InstalledPlanetV1(source,true);
        spread::Input before{{80,80,true},3200,1,{},{}};
        for(uint32_t y=0;y<80;++y) {for(uint32_t x=y&1;x<80;x+=2) {before.tiles.push_back(source.At(x,y));}}
        const size_t source_index=67*40+41/2,target_index=66*40+42/2,changed=78*40+78/2;
        before.tiles[source_index].bytes[4]=2; // genuine current observer gate, absent in retained import
        auto put=[](Planet::Tile& tile,size_t offset,uint32_t value) {for(size_t k=0;k<4;++k) {tile.bytes[offset+k]=uint8_t(value>>(8*k));}};
        put(before.tiles[target_index],8,0x24); // current Former/material state differs from retained source
        before.tiles[changed].bytes[0]=104;before.tiles[changed].bytes[5]=64;
        put(before.tiles[changed],8,0x10000004);put(before.tiles[changed],16,0x10008000);
        const auto plan=spread::Prepare(before,game::backend::SmacGameplayRandomState(123,1));
        GT_ASSERT(!plan.changes.empty() && plan.changes.front().target==spread::Coordinate({42,66}),"Live projection did not execute the spread producer");
        const auto updated=policy.WithLiveGlobalSpread(before,plan);
        GT_ASSERT(updated.OriginalPlanet()->Header()==source.Header() && updated.OriginalPlanet()->Trailer()==source.Trailer()
            && policy.OriginalPlanet()->Encode()==wire && updated.AutomaticLandResources(),"Live publication changed source/header/trailer/policy");
        for(uint32_t y=0;y<80;++y) {for(uint32_t x=y&1;x<80;x+=2) {
            GT_ASSERT(updated.OriginalPlanet()->At(x,y).bytes==plan.tiles[size_t(y)*40+x/2].bytes,"Live publication differs from exact final projected plane");
            GT_ASSERT(updated.NativeTerrainSource()->At(x,y).bytes==updated.OriginalPlanet()->At(x,y).bytes,"Live publication left the shared terrain view stale");
        }}
        GT_ASSERT(updated.OriginalPlanet()->At(42,66).bytes[18]==32 && !(updated.OriginalPlanet()->At(42,66).Items()&0x20u)
            && updated.OriginalPlanet()->At(78,78).Items()==0x10000004u,"Live flora lost observer propagation or unrelated material bits");
        GT_ASSERT(Policy::Deserialize(updated.Serialize().ToString())==updated,"Current visibility/improvements failed retained-world codec");
        auto bad=before;bad.tiles.back().bytes[6]^=1;bool rejected=false;
        try {(void)policy.WithLiveGlobalSpread(bad,plan);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Live projection changed unowned terrain bytes");
        bad=before;bad.tiles.back().bytes[8]^=1;rejected=false;
        try {(void)policy.WithLiveGlobalSpread(bad,plan);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected,"Flora projection claimed live Base/Unit occupancy");
        auto broken=plan;broken.changes.back().after.bytes[6]^=1;rejected=false;
        try {(void)policy.WithLiveGlobalSpread(before,broken);}catch(const std::exception&) {rejected=true;}
        GT_ASSERT(rejected && policy.OriginalPlanet()->Encode()==wire,"Late flora journal failure partly published retained state");GT_OK();
    });

    task->AddTest("SMAC Planet global spread publication preserves opaque bytes and rejects stale or altered journals",GT(){
        namespace spread=game::backend::map::smac_spread;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        auto wire=Synthetic();Put(wire,15+0xa30,4);
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {
            const auto offset=Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES;
            wire[offset]=96;wire[offset+1]=87;wire[offset+6]=27;wire[offset+7]=static_cast<char>(0xa5);
        }
        for(size_t i=wire.size()-Planet::TRAILER_BYTES;i<wire.size();++i) {wire[i]=static_cast<char>(i%256);}
        const auto address=[](int x,int y) {return Planet::PREFIX_BYTES+Planet::HEADER_BYTES+(size_t(y)*40+x/2)*Planet::RECORD_BYTES;};
        Put(wire,address(41,67)+8,0x200000);Put(wire,address(41,67)+16,0x200000);
        Put(wire,address(42,66)+8,0x24);Put(wire,address(42,66)+16,0x24);
        const auto source=Planet::Decode(wire);const auto policy=Policy::InstalledPlanetV1(source,true);
        const auto prepare=[](const Planet& planet) {
            spread::Input input{{80,80,true},3200,1,{},{}};input.observer_status[1]=0x200;
            for(uint32_t y=0;y<80;++y) {for(uint32_t x=y&1;x<80;x+=2) {input.tiles.push_back(planet.At(x,y));}}
            return input;
        };
        const auto before=prepare(source);const auto plan=spread::Prepare(before,game::backend::SmacGameplayRandomState(123,1));
        GT_ASSERT(!plan.changes.empty() && plan.changes.front().target==spread::Coordinate({42,66}),"Synthetic publication did not execute actual spread producer");
        auto expected=wire;
        for(const auto& change:plan.changes) {for(size_t k=0;k<44;++k) {expected[address(change.target[0],change.target[1])+k]=static_cast<char>(change.after.bytes[k]);}}
        const auto updated=policy.WithGlobalSpread(before,plan);
        GT_ASSERT(updated.OriginalPlanet()->Encode()==expected && policy.OriginalPlanet()->Encode()==wire
            && updated.AutomaticLandResources() && Policy::Deserialize(updated.Serialize().ToString())==updated,"Spread publication changed opaque geometry/source or failed saved policy round trip");
        const auto rejects=[&](const spread::Input& input,const spread::Result& result) {try {(void)policy.WithGlobalSpread(input,result);}catch(const std::invalid_argument&) {return true;}return false;};
        auto stale=before;stale.tiles.back().bytes[1]^=1;
        GT_ASSERT(rejects(stale,plan),"Unrelated stale terrain accepted");
        auto malformed=plan;malformed.changes.back().after.bytes[1]^=1;
        GT_ASSERT(rejects(before,malformed) && policy.OriginalPlanet()->Encode()==wire,"Late journal error partly published terrain");
        malformed=plan;malformed.changes.pop_back();GT_ASSERT(rejects(before,malformed),"Unjournaled final mutation accepted");
        malformed=plan;malformed.tiles.back().bytes[6]^=1;GT_ASSERT(rejects(before,malformed),"Final terrain escaped its journal");
        auto flat=before;flat.geometry.wrap=false;GT_ASSERT(rejects(flat,plan),"Flat producer entered wrapped installed policy");
        bool rejected=false;try {(void)updated.WithGlobalSpread(before,plan);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Repeated spread publication replayed on changed terrain");
        auto world=source.ExportClimateTerrain(true);world.tiles[0].cached_occupant=0;world.tiles[0].items|=1;
        const auto retained=policy.WithClimateTerrain(world,0,0);const auto retained_before=prepare(*retained.OriginalPlanet());
        const auto retained_plan=spread::Prepare(retained_before,game::backend::SmacGameplayRandomState(123,1));
        const auto occupied=retained.WithGlobalSpread(retained_before,retained_plan);
        GT_ASSERT(occupied.OriginalPlanet()->At(0,0).Items()==1 && Policy::Deserialize(occupied.Serialize().ToString())==occupied,"Spread lost retained V2 occupied authority");GT_OK();
    });
    task->AddTest("SMAC Planet retained altitude band7 survives policy publication and altitude reconstruction",GT(){
        namespace c=game::backend::map::smac_climate;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        auto wire=Synthetic();Put(wire,15+0xa30,4);
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {
            const auto at=Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES;
            wire[at]=static_cast<char>((7u<<5)|(1u<<3)|2u);wire[at+1]=static_cast<char>(140);
            wire[at+2]=static_cast<char>(0xaf);wire[at+5]=static_cast<char>(0x80);
        }
        const auto original=Policy::InstalledPlanetV1(Planet::Decode(wire),true);
        auto world=original.OriginalPlanet()->ExportClimateTerrain(true);
        GT_ASSERT(world.tiles.front().elevation==7 && world.tiles.back().elevation==7,"Retained altitude band7 was lost on export");
        const auto retained=original.WithClimateTerrain(world,0,0);
        GT_ASSERT(retained.OriginalPlanet()->Encode()==wire && Policy::Deserialize(retained.Serialize().ToString())==retained,
            "Retained altitude band7 failed byte-exact publication or V2 save roundtrip");
        auto invalid=world;invalid.tiles.back().elevation=8;bool rejected=false;
        try {(void)original.WithClimateTerrain(invalid,0,0);}catch(const std::exception&){rejected=true;}
        GT_ASSERT(rejected && original.OriginalPlanet()->Encode()==wire,"Unencodable altitude partly changed original policy");
        game::backend::SmacGameplayRandomState state(123,456);c::Streams streams{state};
        c::RecalculateAltitudes(world,streams,{0,90,true,[](const auto&,int32_t,int32_t){return false;},{},{}});
        GT_ASSERT(streams.shared_draws==0 && streams.crt_draws==0,"Uniform retained band7 reconstruction consumed unexpected randomness");
        for(const auto& tile:world.tiles) {GT_ASSERT(tile.elevation==6 && tile.contour==140,"Altitude reconstruction did not classify retained band7 contour to6");}
        GT_OK();
    });

    task->AddTest("SMAC Planet climate publication preserves opaque bytes and retained policy history",GT(){
        namespace c=game::backend::map::smac_climate;using Policy=game::backend::map::GlsmacResourceMapPolicy;
        auto wire=Synthetic();Put(wire,15+0xa30,4);wire[15+0x30]=93;
        for(size_t i=0;i<Planet::TILE_COUNT;++i) {
            const auto at=Planet::PREFIX_BYTES+Planet::HEADER_BYTES+i*Planet::RECORD_BYTES;
            wire[at]=105;wire[at+1]=64;wire[at+2]=static_cast<char>(0xaf);wire[at+3]=static_cast<char>(i%128);
            wire[at+4]=0;wire[at+5]=static_cast<char>(0x6d);wire[at+6]=27;wire[at+7]=static_cast<char>(0xe7);
            Put(wire,at+8,0x1000u);Put(wire,at+12,0x12345678u);
            for(size_t owner=0;owner<7;++owner) {Put(wire,at+16+4*owner,0x80000000u+uint32_t(i*7+owner));}
        }
        for(size_t i=wire.size()-Planet::TRAILER_BYTES;i<wire.size();++i) {wire[i]=static_cast<char>(i%256);}
        const auto source=Planet::Decode(wire);auto world=source.ExportClimateTerrain(false);
        GT_ASSERT(!world.horizontal_wrap && world.tiles.size()==3200 && source.WithClimateTerrain(world,0,0).Encode()==wire,"Climate export or identity publication rewrote source bytes");
        const auto at=Planet::PREFIX_BYTES+Planet::HEADER_BYTES;
        auto& tile=world.tiles[0];tile={6,2,2,255,129,0x81234567u,0xefa98765u,3,2,{{1,2,3,4,5,6,UINT32_MAX}}};
        auto expected=wire;Put(expected,15+0x0c,static_cast<uint32_t>(-11));Put(expected,15+0x10,7);
        expected[at]=static_cast<char>(210);expected[at+1]=static_cast<char>(255);expected[at+2]=static_cast<char>(0xa3);expected[at+5]=static_cast<char>(0xad);expected[at+6]=static_cast<char>(129);
        Put(expected,at+8,0x81234567u);Put(expected,at+12,0xefa98765u);
        for(size_t i=0;i<7;++i) {Put(expected,at+16+4*i,i==6?UINT32_MAX:uint32_t(i+1));}
        const auto updated=source.WithClimateTerrain(world,-11,7);
        GT_ASSERT(updated.Encode()==expected && source.Encode()==wire,"Climate publication changed territory, region, site nibble, opaque header/trailer or source");
        world.horizontal_wrap=true;
        const auto policy=Policy::InstalledPlanetV1(source,true);const auto legacy=policy.Serialize().ToString();
        const auto current=policy.WithClimateTerrain(world,-11,7);const auto saved=current.Serialize().ToString();
        GT_ASSERT(current.NativeTerrainSource()->Info().sea_offset==-11 && current.NativeTerrainSource()->Info().climate_extra==7
            && current.NativeTerrainSource()->At(0,0).bytes==current.OriginalPlanet()->At(0,0).bytes
            && policy.NativeTerrainSource()->Info().sea_offset==source.CumulativeSeaLevelOffset(),"Climate publication left native metadata or queued source stale");
        GT_ASSERT(current.OriginalPlanet()->Encode()==expected && current.AutomaticLandResources() && Policy::Deserialize(saved)==current
            && Policy::Deserialize(legacy).Serialize().ToString()==legacy,"Retained V2 climate policy failed to retain occupied fields, automatic-resource setting or old V1 bytes");
        bool rejected=false;try {(void)Policy::InstalledPlanetV1(updated,true);}catch(const std::exception&){rejected=true;}
        GT_ASSERT(rejected,"Fresh import unexpectedly accepted retained occupied climate state");
        auto malformed=world;malformed.tiles.back().rockiness=3;rejected=false;
        try {(void)policy.WithClimateTerrain(malformed,0,0);}catch(const std::exception&){rejected=true;}
        GT_ASSERT(rejected && policy.Serialize().ToString()==legacy,"Late invalid tile partly published climate state");
        malformed=world;malformed.width=40;malformed.tiles.resize(1600);rejected=false;
        try {(void)source.WithClimateTerrain(malformed,0,0);}catch(const std::exception&){rejected=true;}
        GT_ASSERT(rejected,"Climate publication accepted different map geometry");
        for(const auto length:{size_t(0),size_t(6),saved.size()-1}) {rejected=false;try {(void)Policy::Deserialize(saved.substr(0,length));}catch(const std::exception&){rejected=true;}GT_ASSERT(rejected,"Truncated retained climate policy accepted");}
        auto flat=world;flat.horizontal_wrap=false;rejected=false;try {(void)policy.WithClimateTerrain(flat,0,0);}catch(const std::exception&){rejected=true;}GT_ASSERT(rejected,"Installed wrapped policy accepted a flat climate producer");
        auto invalid_header=expected;Put(invalid_header,15+0xa34,3);types::Buffer malformed_policy;malformed_policy.WriteInt(2);malformed_policy.WriteInt(2);malformed_policy.WriteBool(true);malformed_policy.WriteString(invalid_header);
        rejected=false;try {(void)Policy::Deserialize(malformed_policy.ToString());}catch(const std::exception&){rejected=true;}GT_ASSERT(rejected,"Retained climate policy accepted unsupported ocean category");
        auto suffix=saved+"x";rejected=false;try {(void)Policy::Deserialize(suffix);}catch(const std::exception&){rejected=true;}GT_ASSERT(rejected,"Retained climate policy accepted suffix bytes");GT_OK();
    });

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
            const auto retained=original.WithClimateTerrain(original.OriginalPlanet()->ExportClimateTerrain(true),0,0).WithBloomTerrain(plan);
            GT_ASSERT(retained.OriginalPlanet()->Encode()==prepared.OriginalPlanet()->Encode() && Policy::Deserialize(retained.Serialize().ToString())==retained,"Bloom publication lost retained climate policy mode or terrain");
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
            using Authority=game::backend::map::SmacTerrainAuthority;
            Authority::Metadata meta;meta.width=uint32_t(row.map.width);meta.height=uint32_t(row.map.height);meta.horizontal_wrap=row.map.horizontal_wrap;
            meta.pattern=11446;meta.generation_primary_rules=0x3a0e;for(auto& category:meta.categories) {category=1;}meta.categories[0]=0;
            std::string generated_input,generated_expected;
            for(int y=0;y<row.map.height;++y) {for(int x=y&1;x<row.map.width;x+=2) {
                const auto at=offset({{x,y}});auto input=wire.substr(at,44),output=expected.substr(at,44);input[3]=output[3]=1;
                generated_input+=input;generated_expected+=output;
            }}
            const auto generated=Policy::GeneratedTerrainV3(Authority::GeneratedRetained(meta,generated_input));
            const auto bloomed=generated.WithBloomTerrain(plan);
            GT_ASSERT(bloomed.NativeTerrainSource()->RecordBytes()==generated_expected && generated.NativeTerrainSource()->RecordBytes()==generated_input
                && !bloomed.OriginalPlanet() && Policy::Deserialize(bloomed.Serialize().ToString())==bloomed,"Native bloom receipt differs through generated authority");
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
    task->AddTest("SMAC Planet keeps temperature category and cached occupant distinct from tile codes and territory",GT(){
        for(const int32_t category:{-1,0,1,2,INT32_MAX}) {
            auto wire=Synthetic();Put(wire,Planet::PREFIX_BYTES+0xa40,static_cast<uint32_t>(category));
            Put(wire,Planet::PREFIX_BYTES+0x10,static_cast<uint32_t>(~category));
            const auto at=Planet::PREFIX_BYTES+Planet::HEADER_BYTES;
            wire[at]=static_cast<char>((3<<5)|(1<<3)|2);wire[at+2]=static_cast<char>(0xa6);wire[at+7]=1;
            const auto planet=Planet::Decode(wire);const auto& tile=planet.At(0,0);
            GT_ASSERT(planet.TemperatureCategory()==category && planet.ClimateExtra()==~category && (tile.Climate()&7)==2 && tile.CachedOccupant()==6
                && tile.SiteRating()==10 && tile.Owner()==1 && planet.Encode()==wire,"Independent climate/occupancy fields were conflated or normalized");
        }GT_OK();
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
            GT_ASSERT(map.WithClimateTerrain(map.ExportClimateTerrain(true),map.CumulativeSeaLevelOffset(),map.ClimateExtra()).Encode()==bytes,"Installed Planet climate export changed original bytes");
            GT_ASSERT(map.TemperatureCategory()==1 && map.ClimateExtra()==0 && map.CloudCoverCategory() == 1 && map.NativeLifeCategory() == 1 && map.CumulativeSeaLevelOffset() == 0,
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
