#include "OwnedNames.h"
#include "gse/tests/fixtures/SmacBaseNaming.h"
#include "util/SHA256.h"
#include <iostream>

int main() {
    using namespace game::backend;namespace fixture=gse::tests::fixtures::base_naming;
    try {
        const auto catalogs=Catalogs();size_t index=0,total=0;
        const auto append=[](std::string& s,uint32_t value,unsigned size){for(unsigned k=0;k<size;++k) {s+=char(uint8_t(value>>(8*k)));}};
        for(const auto& c:fixture::Cases()) {
            const auto& catalog=catalogs[c.owner-1];const auto counts=fixture::COUNTS[c.owner-1];
            if(catalog.land.size()!=size_t(counts.land) || catalog.sea.size()!=size_t(counts.sea) || catalog.generic.size()!=size_t(counts.generic)) {throw std::runtime_error("Owned parsed catalog counts differ");}
            std::array<faction::SmacStartupFaction,8> actors{};auto& actor=actors[c.owner];
            for(unsigned k=0;k<4;++k) {actor.record[12+k]=uint8_t(uint32_t(c.land)>>(8*k));actor.record[16+k]=uint8_t(uint32_t(c.sea_cursor)>>(8*k));}
            std::array<uint8_t,128> output;output.fill(165);std::string trace;size_t count=0;
            base::GenerateSmacBaseName(actor,output,catalog,c.owner,c.pattern,c.advance,c.sea,[&](const auto& v){append(trace,v.offset,4);append(trace,4,1);append(trace,v.value,4);append(trace,v.caller,4);++count;});
            std::string bytes;for(const auto& a:actors) {bytes.append(reinterpret_cast<const char*>(a.record.data()),a.record.size());}
            const std::string buffer(reinterpret_cast<const char*>(output.data()),output.size());
            if(util::SHA256(buffer)!=c.output || util::SHA256(bytes)!=c.actors || count!=c.count || util::SHA256(trace)!=c.stores) {
                throw std::runtime_error("Complete original naming output differs at control "+std::to_string(index));}
            ++index;total+=count;
        }
        if(index!=813 || total!=694) {throw std::runtime_error("Original naming coverage changed");}
        std::cout<<"PASS: 813 original complete naming buffers/eight-faction records and 694 cursor stores\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
