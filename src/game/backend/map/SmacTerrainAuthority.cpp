#include "SmacTerrainAuthority.h"
#include "SmacContinentGeneration.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace game::backend::map {
SmacTerrainAuthority SmacTerrainAuthority::Installed(SmacPlanetMap source) {
    (void)source.At(SmacPlanetMap::WIDTH-1,SmacPlanetMap::HEIGHT-1);
    SmacTerrainAuthority result;result.m_installed=std::make_shared<const SmacPlanetMap>(std::move(source));const auto& map=*result.m_installed;
    auto& meta=result.m_metadata;meta.width=SmacPlanetMap::WIDTH;meta.height=SmacPlanetMap::HEIGHT;
    const auto& header=map.Header();meta.horizontal_wrap=!(header[0x1c] || header[0x1d] || header[0x1e] || header[0x1f]);
    meta.pattern=map.ResourcePattern();meta.sea_offset=map.CumulativeSeaLevelOffset();meta.climate_extra=map.ClimateExtra();
    meta.categories[0]=map.WorldSizeCategory();meta.categories[1]=map.OceanCoverageCategory();
    meta.categories[4]=map.TemperatureCategory();meta.categories[5]=map.CloudCoverCategory();meta.categories[6]=map.NativeLifeCategory();
    return result;
}
SmacTerrainAuthority SmacTerrainAuthority::GeneratedPrestartup(const smac_generation::State& state,smac_generation::Geometry g) {
    if(state.stage!=smac_generation::Stage::Prestartup || state.generation_flags!=std::array<int32_t,2>{{0,0}}
        || g.width<16 || g.width>512 || (g.width&1) || g.height<8 || g.height>256 || (g.height&1)
        || uint64_t(g.width/2)*uint64_t(g.height)!=state.records.size()) {
        throw std::invalid_argument("Generated authority requires completed admitted pre-startup terrain");
    }
    if(state.topology<0 || state.topology>1) {throw std::invalid_argument("Generated authority requires explicit native topology");}
    Metadata meta;
    meta.width=uint32_t(g.width);meta.height=uint32_t(g.height);meta.horizontal_wrap=state.topology==0;
    meta.pattern=state.resource_pattern;meta.sea_offset=state.reset_words[0];meta.climate_extra=state.reset_words[1];
    for(size_t i=0;i<7;++i) {meta.categories[i]=state.categories[i];}
    meta.generation_primary_rules=state.primary_rules;
    std::string records;records.reserve(state.records.size()*44);
    for(const auto& record:state.records) {records.append(reinterpret_cast<const char*>(record.data()),record.size());}
    return GeneratedRetained(std::move(meta),records);
}
SmacTerrainAuthority SmacTerrainAuthority::GeneratedRetained(Metadata meta,const std::string& records) {
    if(meta.width<16 || meta.width>512 || (meta.width&1) || meta.height<8 || meta.height>256 || (meta.height&1)
        || uint64_t(meta.width/2)*meta.height*44!=records.size() || !meta.generation_primary_rules
        || meta.pattern<1 || meta.pattern>32767) {throw std::invalid_argument("Invalid retained generated terrain metadata or record length");}
    for(size_t i=0;i<7;++i) {if(!meta.categories[i] || *meta.categories[i]<0 || *meta.categories[i]>(i==0?4:2)) {
        throw std::invalid_argument("Retained generated terrain requires resolved source categories");}}
    SmacTerrainAuthority result;result.m_origin=Origin::Generated;result.m_metadata=std::move(meta);
    result.m_generated.resize(records.size()/44);
    for(size_t i=0;i<result.m_generated.size();++i) {auto& record=result.m_generated[i].bytes;
        std::copy_n(reinterpret_cast<const uint8_t*>(records.data())+44*i,44,record.begin());
        if((record[0]&7)>2 || ((record[0]>>3)&3)>2 || (record[5]>>6)>2 || record[3]>=128) {
            throw std::invalid_argument("Retained generated terrain contains unsupported climate or region fields");
        }
    }
    return result;
}
bool SmacTerrainAuthority::operator==(const SmacTerrainAuthority& other) const {
    const auto& a=m_metadata;const auto& b=other.m_metadata;
    if(m_origin!=other.m_origin || a.width!=b.width || a.height!=b.height || a.horizontal_wrap!=b.horizontal_wrap || a.pattern!=b.pattern
        || a.sea_offset!=b.sea_offset || a.climate_extra!=b.climate_extra || a.categories!=b.categories || a.generation_primary_rules!=b.generation_primary_rules) {return false;}
    if(m_installed || other.m_installed) {return m_installed && other.m_installed && m_installed->Encode()==other.m_installed->Encode();}
    return m_generated.size()==other.m_generated.size() && std::equal(m_generated.begin(),m_generated.end(),other.m_generated.begin(),
        [](const Cell& a,const Cell& b){return a.bytes==b.bytes;});
}
const SmacTerrainAuthority::Cell& SmacTerrainAuthority::At(uint32_t x,uint32_t y) const {
    if(x>=m_metadata.width || y>=m_metadata.height || (x&1)!=(y&1)) {throw std::out_of_range("Invalid native terrain coordinate");}
    if(m_installed) {return m_installed->At(x,y);}
    return m_generated.at(size_t(y)*size_t(m_metadata.width/2)+size_t(x/2));
}
std::string SmacTerrainAuthority::RecordBytes() const {
    std::string result;result.reserve(size_t(m_metadata.width/2)*size_t(m_metadata.height)*44);
    for(uint32_t y=0;y<m_metadata.height;++y) {for(uint32_t x=y&1;x<m_metadata.width;x+=2) {
        const auto& record=At(x,y).bytes;result.append(reinterpret_cast<const char*>(record.data()),record.size());
    }}return result;
}
}
