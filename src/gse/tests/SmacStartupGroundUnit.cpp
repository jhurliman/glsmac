#include "SmacStartupGroundUnit.h"
#include "fixtures/SmacStartupGroundUnit.h"
#include "game/backend/unit/SmacUnitInitialization.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"

namespace gse::tests {
namespace {
template<class Record> std::string Bytes(const Record& r) {return std::string(reinterpret_cast<const char*>(r.data()),r.size());}
template<class Record> void Word(Record& r,size_t off,uint32_t v,unsigned size=4) {for(unsigned k=0;k<size;++k) {r[off+k]=uint8_t(v>>(8*k));}}
}
void AddSmacStartupGroundUnitTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC ordinary ground factory rejects unclosed sight producers before record creation",GT(){
        using namespace game::backend;using namespace game::backend::unit;
        std::vector<std::array<uint8_t,44>> tiles(16*24);for(auto& tile:tiles) {tile[0]=104;tile[2]=15;tile[3]=1;}
        faction::SmacStartupFaction actor;SmacGameplayRandomState random(0,1);
        faction::InitializeSmacFactionPrefix(actor,{1,3,2,255,0x1a0e,0,false,10,{-1,0,0,0,1,0},"Template"," "},random);
        auto foreign=InitializeSmacUnitRecord(0,2,2,0);foreign.x=18;foreign.y=12;
        std::vector<SmacNativeUnitRecord> units{foreign};Word(tiles[12*16+9],8,0x100002);tiles[12*16+9][2]=2;
        const auto before_tiles=tiles;const auto before_units=units;const auto before_actor=actor.record;
        SmacStartupUnitFactoryState state;state.dirty=16;state.cached_base=0;state.last_distance=1234;
        SmacStartupGroundUnitOptions options{{32,24,true},{{16,12}},1,2,3,0,1};
        for(const bool sensor:{false,true}) {options.sensor_sight=sensor;bool rejected=false;
            try {CreateSmacStartupGroundUnit(tiles,units,actor,{},state,options);}catch(const std::invalid_argument&) {rejected=true;}
            GT_ASSERT(rejected && tiles==before_tiles && units==before_units && actor.record==before_actor
                && state.dirty==16 && state.cached_base==0 && state.last_distance==1234,"Unclosed sight path changed staged factory state");
        }
        return {};
    });
    task->AddTest("SMAC ordinary ground factory matches three hundred six original pools sight and remote home journals",GT(){
        using namespace game::backend;using namespace game::backend::unit;size_t total=0,total_metadata=0;
        for(const auto& c:fixtures::startup_ground::Cases()) {
            const auto& o=c.options;std::vector<std::array<uint8_t,44>> tiles(16*24);
            for(int y=0;y<24;++y) {for(int x=y&1;x<32;x+=2) {auto& r=tiles[size_t(y)*16+x/2];r[0]=c.coast && x>16?32:104;r[1]=(r[0]>>5)<3?20:50;
                r[2]=15;r[3]=1;r[4]=c.known;Word(r,12+4*o.owner,0x12345678);}}
            for(const auto& p:c.history) {tiles[size_t(p[1])*16+p[0]/2][4]|=uint8_t(1u<<o.owner);}
            faction::SmacStartupFaction actor;SmacGameplayRandomState random(0,1);
            faction::InitializeSmacFactionPrefix(actor,{o.owner,3,2,255,0x1a0e,0,false,10,{-1,0,0,0,1,0},"Template"," "},random);
            if(c.reveal) {Word(actor.record,0,0x200);}Word(actor.record,0x95e1fc-0x95d1a8,uint32_t(c.existing));actor.record[0x95d9fc-0x95d1a8+o.design]=c.design_count;
            std::vector<SmacNativeUnitRecord> units;for(size_t i=0;i<c.existing;++i) {auto r=InitializeSmacUnitRecord(int32_t(i),o.design,o.owner,o.morale);
                r.x=int16_t(o.position[0]);r.y=int16_t(o.position[1]);r.stack_next=i+1<c.existing?int16_t(i+1):-1;r.stack_previous=i?int16_t(i-1):-1;units.push_back(r);}
            auto& center=tiles[size_t(o.position[1])*16+o.position[0]/2];if(c.existing) {Word(center,8,0x100002);center[2]=uint8_t(o.owner);}
            std::vector<base::SmacStartupBaseRecord> bases;
            if(c.base) {base::SmacStartupBaseRecord b{};Word(b,0,uint32_t(o.position[0]),2);Word(b,2,uint32_t(o.position[1]),2);b[4]=uint8_t(o.owner);b[6]=1;b[10]=uint8_t(1u<<o.owner);b[11+o.owner]=1;bases.push_back(b);
                center[8]|=1;center[2]=uint8_t(o.owner);Word(actor.record,0x95e200-0x95d1a8,1);actor.record[0x95e320-0x95d1a8+1]=1;}
            if(c.remote) {uint32_t own=0;
                for(size_t i=0;i<c.remote_bases.size();++i) {const auto& row=c.remote_bases[i];base::SmacStartupBaseRecord b;
                    for(size_t j=0;j<b.size();++j) {b[j]=uint8_t(j*7+i*11+3);}Word(b,0,uint32_t(row[0]),2);Word(b,2,uint32_t(row[1]),2);b[4]=uint8_t(row[2]);b[6]=1;b[10]=uint8_t(1u<<row[2]);b[11+row[2]]=1;bases.push_back(b);
                    auto& t=tiles[size_t(row[1])*16+row[0]/2];t[2]=uint8_t((t[2]&240)|row[2]);t[3]=uint8_t(row[3]);t[8]|=1;
                    if(row[2]==o.owner) {++own;++actor.record[0x95e320-0x95d1a8+row[3]];}}
                Word(actor.record,0x95e200-0x95d1a8,own);
                for(const auto& row:c.remote_units) {auto unit=InitializeSmacUnitRecord(int32_t(units.size()),2,row[2],0);unit.x=int16_t(row[0]);unit.y=int16_t(row[1]);units.push_back(unit);
                    auto& t=tiles[size_t(row[1])*16+row[0]/2];t[2]=uint8_t((t[2]&240)|row[2]);const auto old=uint32_t(t[8])|(uint32_t(t[9])<<8)|(uint32_t(t[10])<<16)|(uint32_t(t[11])<<24);Word(t,8,old|0x100002);}
            }
            const auto tileBytes=[&](){std::string s;for(const auto& r:tiles) {s+=Bytes(r);}return s;};
            const auto unitBytes=[&](){std::string s;for(const auto& r:units) {s+=Bytes(OriginalSmacUnitBytes(r));}return s;};
            const auto baseBytes=[&](){std::string s;for(const auto& r:bases) {s+=Bytes(r);}return s;};
            GT_ASSERT(util::SHA256(tileBytes())==c.input_tiles && util::SHA256(Bytes(actor.record))==c.input_actor
                && util::SHA256(unitBytes())==c.input_units && util::SHA256(baseBytes())==c.input_bases,"Original ground factory input recipe differs");
            SmacStartupUnitFactoryState state;state.dirty=16;state.cached_base=c.initial_cache;state.last_distance=c.initial_distance;std::string trace,metadata;size_t count=0,metadata_count=0;
            const auto pack=[&](uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {trace+=char(uint8_t(v>>(8*k)));}};
            const auto result=CreateSmacStartupGroundUnit(tiles,units,actor,bases,state,o,[&](const auto& s){pack(s.offset,4);pack(s.size,1);pack(s.value,4);pack(s.caller,4);++count;},[&](const auto& s){
                const auto append=[&](uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {metadata+=char(uint8_t(v>>(8*k)));}};
                append(uint8_t(s.space),1);append(s.offset,4);append(s.size,1);append(s.value,4);append(s.caller,4);++metadata_count;});
            GT_ASSERT(result==c.result && int(units.size())==c.count && state.cached_base==c.cached && state.last_distance==c.distance && state.dirty==c.dirty
                && util::SHA256(tileBytes())==c.tiles && util::SHA256(Bytes(actor.record))==c.actor && util::SHA256(unitBytes())==c.units
                && util::SHA256(baseBytes())==c.bases && count==c.writes && util::SHA256(trace)==c.stores,"Original ground factory full records or journal differ");total+=count;
            if(c.remote) {std::string all_actors;const std::string empty(8380,'\0');for(int f=0;f<8;++f) {all_actors+=f==o.owner?Bytes(actor.record):empty;}
                GT_ASSERT(util::SHA256(all_actors)==c.all_actors && metadata_count==c.metadata_writes && util::SHA256(metadata)==c.metadata,"Original remote home all-faction or metadata journal differs");total_metadata+=metadata_count;}
        }
        GT_ASSERT(total==6910 && total_metadata==432,"Original factory coverage changed");return {};
    });
}
}
