#include "SmacFirstWorker.h"
#include "fixtures/SmacFirstWorker.h"
#include "fixtures/SmacOpeningAiWorker.h"
#include "task/gsetests/GSETests.h"
#include "util/SHA256.h"
#include <algorithm>

namespace gse::tests {
namespace {
template<class Record> void Word(Record& r,size_t off,uint32_t value,unsigned size=4) {for(unsigned k=0;k<size;++k) {r[off+k]=uint8_t(value>>(8*k));}}
template<class Record> uint32_t Read(const Record& r,size_t off) {uint32_t v=0;for(unsigned k=0;k<4;++k) {v|=uint32_t(r[off+k])<<(8*k);}return v;}
template<class Record> std::string Bytes(const Record& r) {return std::string(reinterpret_cast<const char*>(r.data()),r.size());}
void Append(std::string& s,uint32_t v,unsigned size) {for(unsigned k=0;k<size;++k) {s+=char(uint8_t(v>>(8*k)));}}
}
void AddSmacFirstWorkerTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC first worker derives original yields ranking intake and complete raw records",GT(){
        using namespace game::backend;using namespace game::backend::base;size_t stores=0,queries=0,case_index=0;
        auto cases=fixtures::first_worker::Cases();const auto ai=fixtures::first_worker::AiCases();
        cases.insert(cases.end(),ai.begin(),ai.end());
        for(const auto& c:cases) {
            std::vector<std::array<uint8_t,44>> tiles(384);
            for(auto& t:tiles) {t[0]=uint8_t(c.band*32+c.moisture*8);t[1]=50;t[2]=15;t[3]=1;t[4]=c.known?uint8_t(1u<<c.owner):0;t[5]=uint8_t(c.rockiness<<6);t[7]=uint8_t(c.owner);Word(t,8,c.items);}
            const SmacWorkerRadiusMap map{c.position,32,24,c.options.horizontal_wrap};
            const auto at=[&](auto p)->auto& {return tiles[size_t(p[1])*16+size_t(p[0]/2)];};
            for(const auto& p:c.patches) {
                const auto point=SmacWorkedSquareCollection::Resolve(map,p.index);if(!point) {continue;}auto& t=at(*point);
                if(p.items>=0) {Word(t,8,uint32_t(p.items));}if(p.landmarks>=0) {Word(t,12,uint32_t(p.landmarks));}
                if(p.known>=0) {t[4]=p.known?uint8_t(1u<<c.owner):0;}if(p.band>=0) {t[0]=uint8_t((t[0]&31)|(p.band<<5));}
                if(p.moisture>=0) {t[0]=uint8_t((t[0]&~24)|(p.moisture<<3));}if(p.rockiness>=0) {t[5]=uint8_t((t[5]&63)|(p.rockiness<<6));}
            }
            for(int i=0;i<21;++i) {const auto p=SmacWorkedSquareCollection::Resolve(map,i);if(p) {auto& t=at(*p);Word(t,8,Read(t,8)|0x1000);}}
            auto& center=at(c.position);center[2]=uint8_t((center[2]&0xf0)|c.owner);Word(center,8,Read(center,8)|1);
            faction::SmacStartupFaction actor;SmacGameplayRandomState random(1,1);
            const auto& p=c.personality;
            const auto human_mask=c.options.human_mask==255?uint8_t(1u<<c.owner):c.options.human_mask;
            faction::InitializeSmacFactionPrefix(actor,{c.owner,3,human_mask,255,c.options.primary_rules,0,false,10,{p[0],p[4],p[2],p[3],p[1],0},"Template"," "},random);
            Word(actor.record,0,c.reveal?0x200:0);Word(actor.record,0x95e200-0x95d1a8,1);
            for(size_t i=0;i<3;++i) {Word(actor.record,0x95d4bc-0x95d1a8+4*i,uint32_t(c.fungal[i]));}
            SmacStartupBaseRecord base;for(size_t i=0;i<304;++i) {base[i]=uint8_t(i*11+5);}
            Word(base,0,uint32_t(c.position[0]),2);Word(base,2,uint32_t(c.position[1]),2);base[4]=uint8_t(c.owner);base[6]=1;
            const std::array<size_t,9> offsets{{0x30,0x34,0x38,0x50,0x78,0x7c,0x80,0x84,0x88}};
            const std::array<uint32_t,9> values{{c.flags,0,0xdfbe41,1,c.initial_mask,0,91,0x12345670,0x76543210}};
            for(size_t i=0;i<9;++i) {Word(base,offsets[i],values[i]);}std::fill_n(base.begin()+0x8c,5,0);
            for(const auto id:c.facilities) {base[0x8c+id/8]|=uint8_t(1u<<(id&7));}
            const auto plane=[&](){std::string s;for(const auto& t:tiles) {s+=Bytes(t);}return s;};
            GT_ASSERT(util::SHA256(plane())==c.input_tiles && util::SHA256(Bytes(base))==c.input_base && util::SHA256(Bytes(actor.record))==c.input_actor,"First-worker declared input recipe differs at case "+std::to_string(case_index)+" tiles="+util::SHA256(plane())+" base="+util::SHA256(Bytes(base))+" actor="+util::SHA256(Bytes(actor.record)));
            faction::SmacStartupTechnologyOwnership ownership{};ownership[6]=uint8_t(1u<<c.owner);SmacFirstWorkerScratch scratch{0x24681357,0x24681357,0x24681357,{}};scratch.reasons.fill(0x24681357);
            std::string trace,query_trace,score_trace;size_t count=0,qcount=0,scount=0;
            const auto worker=CalculateSmacFirstWorker(tiles,base,actor,ownership,fixtures::first_worker::Rules(),c.options,scratch,
                [&](const auto& s){Append(trace,uint8_t(s.space),1);Append(trace,s.offset,4);Append(trace,s.size,1);Append(trace,s.value,4);Append(trace,s.caller,4);++count;},
                [&](const auto& q){Append(query_trace,q.dispatcher?1:0,1);for(const auto v:{q.resource,q.x,q.y,q.value}) {Append(query_trace,uint32_t(v),4);}++qcount;},
                [&](int32_t index,int32_t score){Append(score_trace,uint32_t(index),4);Append(score_trace,uint32_t(score),4);++scount;});
            std::string scratch_bytes;for(const auto v:{scratch.food_sufficient,scratch.remaining,scratch.unavailable}) {Append(scratch_bytes,uint32_t(v),4);}for(const auto v:scratch.reasons) {Append(scratch_bytes,uint32_t(v),4);}
            std::string actors;for(int f=0;f<8;++f) {actors+=f==c.owner?Bytes(actor.record):std::string(8380,0);}
            GT_ASSERT(util::SHA256(Bytes(base))==c.base && util::SHA256(actors)==c.actors && util::SHA256(plane())==c.tiles && worker==c.worker,"First-worker complete output differs");
            GT_ASSERT(count==c.stores && qcount==c.query_count && scount==c.score_count && util::SHA256(trace)==c.trace && util::SHA256(query_trace)==c.queries
                && util::SHA256(score_trace)==c.scores && util::SHA256(scratch_bytes)==c.scratch,"First-worker native journals or scratch differ");stores+=count;queries+=qcount;++case_index;
        }
        GT_ASSERT(stores==6922+8610 && queries==4119+5712,"First-worker native coverage changed");return {};
    });
}
}
