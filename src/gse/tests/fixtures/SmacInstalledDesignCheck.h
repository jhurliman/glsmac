#pragma once
#include "SmacInstalledDesigns.h"
#include "SmacStartupCapability.h"
#include "SmacOpeningDesigns.h"
#include "util/SHA256.h"
#include <algorithm>
#include <stdexcept>

namespace gse::tests::fixtures::installed_designs {
struct CheckStats {size_t loader=0,localized=0,controller=0,mark=0;};
inline CheckStats Verify(const game::backend::unit::SmacDesignComponents& components,
    const std::vector<game::backend::unit::SmacPredefinedDesign>& rows,const game::backend::unit::SmacEarlyDesignNames& names,bool actual_text) {
    using namespace game::backend;using namespace game::backend::unit;CheckStats stats;
    const auto word=[](auto& r,size_t off,uint32_t v){for(unsigned k=0;k<4;++k) {r[off+k]=uint8_t(v>>(8*k));}};
    const auto bytes=[](const auto& pool){std::string s;for(const auto& r:pool) {s.append(reinterpret_cast<const char*>(r.data()),r.size());}return s;};
    const auto normalized=[&](SmacOriginalDesignPool pool,bool custom){
        for(size_t i=0;i<14;++i) {std::fill_n(pool[i].begin(),NAME_LENGTHS[i]+1,0);}
        if(custom) {for(size_t i=64;i<512;++i) {if(pool[i][48]&1) {std::fill_n(pool[i].begin(),32,0);}}}
        return util::SHA256(bytes(pool));
    };
    const auto append=[](std::string& s,uint32_t v,unsigned size){for(unsigned k=0;k<size;++k) {s+=char(uint8_t(v>>(8*k)));}};
    const auto trace=[&](std::string& s,const SmacDesignStore& v,bool space){if(space) {append(s,uint8_t(v.space),1);}append(s,v.offset,4);append(s,v.size,1);append(s,v.value,4);append(s,v.caller,4);};
    if(rows.size()!=14) {throw std::runtime_error("Installed design source count differs");}
    for(size_t i=0;i<14;++i) {if(rows[i].name.size()!=size_t(NAME_LENGTHS[i])) {throw std::runtime_error("Installed source name length differs");}}
    for(const auto& c:PREDEFINED) {
        SmacOriginalDesignPool pool;for(auto& r:pool) {r.fill(uint8_t(c.fill));}std::string stores;size_t count=0;
        LoadSmacPredefinedDesigns(pool,components,rows,[&](const auto& s){trace(stores,s,false);++count;});
        if(normalized(pool,false)!=c.normalized || (actual_text && util::SHA256(bytes(pool))!=c.full) || count!=c.count || util::SHA256(stores)!=c.trace) {throw std::runtime_error("Original predefined full pool or stores differ");}stats.loader+=count;
    }
    SmacOriginalDesignPool installed{};LoadSmacPredefinedDesigns(installed,components,rows);
    if(actual_text && util::SHA256(bytes(installed))!=GENERATED_FULL) {throw std::runtime_error("Original generated startup installed catalog differs");}
    for(const auto& c:LOCALIZED) {
        auto pool=installed;std::string stores;size_t count=0;int32_t first=-1;
        const auto label=NameSmacOpeningDesign(names,c.owner,c.chassis,c.armor);
        const auto expected_label=c.variant==2?names.scout+" "+names.rover:names.armor_short+" "+(c.variant?names.infantry_odd:names.infantry_even);
        if(label!=expected_label) {throw std::runtime_error("Original localized source-label selection differs");}
        const auto result=ConstructSmacOpeningDesign(pool,c.owner,c.chassis,c.armor,c.difficulty,label,first,false,[&](const auto& s){
            if(s.space==SmacDesignStore::Space::Designs) {trace(stores,s,false);++count;}},&components);
        if(result!=c.result || first!=c.result || normalized(pool,true)!=c.normalized || (actual_text && util::SHA256(bytes(pool))!=c.full)
            || count!=c.count || util::SHA256(stores)!=c.trace) {throw std::runtime_error("Original localized opening constructor differs");}
        const auto& record=pool[size_t(result)];const std::string actual(record.begin(),std::find(record.begin(),record.begin()+32,uint8_t(0)));
        if(actual!=label.substr(0,31)) {throw std::runtime_error("Localized opening label copy differs");}stats.localized+=count;
    }
    size_t index=0;
    for(const auto& c:EarlyCases()) {
        auto pool=installed;std::array<faction::SmacStartupFaction,8> actors{};faction::SmacStartupTechnologyOwnership ownership{};
        word(actors[c.owner].record,8,uint32_t(c.difficulty));const std::array<int32_t,5> personality{{-1,1,0,1,0}};
        for(size_t i=0;i<5;++i) {word(actors[c.owner].record,0x95d584-0x95d1a8+4*i,uint32_t(personality[i]));}
        for(const auto t:c.technologies) {ownership[t]|=uint8_t(1u<<c.owner);}
        constexpr std::array<uint32_t,5> addresses{{0x95d9bc,0x95d9c8,0x95d9cc,0x95d9c0,0x95d9c4}};
        const std::array<int32_t,5> values{{1,2,2,0,0}};
        for(int f=1;f<8;++f) {if(f!=c.owner) {for(size_t i=0;i<5;++i) {word(actors[f].record,addresses[i]-0x95d1a8,uint32_t(values[i]));}}}
        int32_t first=123;std::string stores;size_t count=0;
        GenerateSmacInstalledOpeningDesigns(actors,pool,ownership,opening_designs::Prerequisites(),capability::StockCatalog(),components,names,c.owner,c.difficulty,first,
            [&](const auto& s){trace(stores,s,true);++count;});
        std::string actor_bytes;for(const auto& a:actors) {actor_bytes.append(reinterpret_cast<const char*>(a.record.data()),a.record.size());}
        if(normalized(pool,true)!=c.normalized || (actual_text && util::SHA256(bytes(pool))!=c.full) || util::SHA256(actor_bytes)!=c.actors
            || first!=c.first || count!=c.count || util::SHA256(stores)!=c.trace) {throw std::runtime_error("Original installed early controller differs at "+std::to_string(index));}
        stores.clear();size_t marks=0;const auto scout=MarkAndSelectSmacStartingScout(pool,c.owner,[&](const auto& s){trace(stores,s,true);++marks;});
        if(scout!=c.scout || normalized(pool,true)!=c.marked_normalized || (actual_text && util::SHA256(bytes(pool))!=c.marked_full)
            || marks!=c.mark_count || util::SHA256(stores)!=c.mark_trace) {throw std::runtime_error("Original installed early caller marking differs");}
        stats.controller+=count;stats.mark+=marks;++index;
    }
    return stats;
}
}
