#include "SmacStartupTerritory.h"
#include "fixtures/SmacStartupTerritory.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
namespace {
template<class Record> std::string Bytes(const Record& r) {return std::string(reinterpret_cast<const char*>(r.data()),r.size());}
template<class Record> void Word(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r[off+k]=uint8_t(value>>(8*k));}}
}
void AddSmacStartupTerritoryTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC startup territory matches twenty six original complete maps regions actors and ordered stores",GT(){
        using namespace game::backend;size_t total=0;
        for(const auto& c:fixtures::startup_territory::Cases()) {
            std::vector<std::array<uint8_t,44>> tiles(16*24);std::array<faction::SmacStartupFaction,8> actors{};
            map::smac_generation::Regions regions;for(size_t i=0;i<regions.records.size();++i) {regions.records[i]=uint8_t(i*13+7);}
            for(int y=0;y<24;++y) {for(int x=y&1;x<32;x+=2) {auto& r=tiles[size_t(y)*16+x/2];const bool sea=c.sea || (c.coast && x>=16);
                r[0]=sea?56:120;r[2]=0x5f;r[3]=sea?64:1;r[4]=c.explored;r[7]=7;Word(r,8,c.forced && (x+y)%8==0?0x10000000:0);
            }}
            for(size_t f=0;f<8;++f) {auto& raw=actors[f].record;
                for(size_t j=0;j<75;++j) {const auto off=0x95eba0-0x95d1a8+16*j;Word(raw,off,UINT32_MAX,2);Word(raw,off+12,123);}
                for(size_t j=0;j<25;++j) {const auto off=0x95f050-0x95d1a8+16*j;Word(raw,off,UINT32_MAX,2);Word(raw,off+12,123);}
                Word(raw,0,f==1?c.flags:0);
            }
            if(c.full) {auto& raw=actors[1].record;
                for(size_t j=0;j<75;++j) {const auto off=0x95eba0-0x95d1a8+16*j;Word(raw,off,5,2);Word(raw,off+2,1,2);Word(raw,off+12,456);}
                for(size_t j=0;j<25;++j) {const auto off=0x95f050-0x95d1a8+16*j;Word(raw,off,5,2);Word(raw,off+2,1,2);Word(raw,off+12,456);}
            }
            if(c.duplicate) {auto& raw=actors[1].record;const auto off=0x95f050-0x95d1a8;Word(raw,off,3,2);Word(raw,off+4,16);Word(raw,off+8,8);Word(raw,off+12,777);}
            if(c.mixed) {const std::array<uint8_t,4> rock{{0,0x40,0x80,0xc0}};const std::array<uint32_t,4> items{{0,0x20,0x1000,0x100000}};
                for(size_t i=0;i<tiles.size();++i) {auto& r=tiles[i];r[5]=rock[i%4];const auto old=uint32_t(r[8])|(uint32_t(r[9])<<8)|(uint32_t(r[10])<<16)|(uint32_t(r[11])<<24);Word(r,8,old|items[i%4]);}}
            std::vector<base::SmacStartupBaseRecord> bases;
            for(const auto& b:c.bases) {base::SmacStartupBaseRecord raw{};Word(raw,0,uint32_t(b[0]),2);Word(raw,2,uint32_t(b[1]),2);raw[4]=uint8_t(b[2]);raw[6]=1;raw[10]=uint8_t(1u<<b[2]);bases.push_back(raw);
                auto& tile=tiles[size_t(b[1])*16+b[0]/2];tile[8]|=1;actors[b[2]].record[0x95e320-0x95d1a8+tile[3]]=1;}
            const auto tileBytes=[&](){std::string result;for(const auto& t:tiles) {result+=Bytes(t);}return result;};
            const auto actorBytes=[&](){std::string result;for(const auto& a:actors) {result+=Bytes(a.record);}return result;};
            const auto baseBytes=[&](){std::string result;for(const auto& b:bases) {result+=Bytes(b);}return result;};
            GT_ASSERT(util::SHA256(tileBytes())==c.input_tiles && util::SHA256(actorBytes())==c.input_actors
                && util::SHA256(baseBytes())==c.input_bases && util::SHA256(Bytes(regions.records))==c.input_regions,"Native territory input recipe differs");
            std::string trace;size_t count=0;const auto pack=[&](uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {trace+=char(uint8_t(v>>(8*k)));}};
            const auto result=map::RebuildSmacStartupTerritory(tiles,actors,bases,regions,c.options,[&](const auto& s){pack(s.offset,4);pack(s.size,1);pack(s.value,4);pack(s.caller,4);++count;});
            GT_ASSERT(util::SHA256(tileBytes())==c.tiles && util::SHA256(actorBytes())==c.actors && util::SHA256(Bytes(regions.records))==c.regions
                && util::SHA256(baseBytes())==c.base_output && result.dirty==c.dirty && result.last_distance==c.last && result.uncached==0
                && count==c.count && util::SHA256(trace)==c.stores,"Complete original territory output differs");total+=count;
        }
        GT_ASSERT(total==19968,"Original territory coverage changed");return {};
    });
}
}
