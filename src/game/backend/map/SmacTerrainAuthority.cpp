#include "SmacTerrainAuthority.h"
#include "SmacContinentGeneration.h"
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
    SmacTerrainAuthority result;result.m_origin=Origin::Generated;auto& meta=result.m_metadata;
    meta.width=uint32_t(g.width);meta.height=uint32_t(g.height);meta.horizontal_wrap=state.topology==0;
    meta.pattern=state.resource_pattern;meta.sea_offset=state.reset_words[0];meta.climate_extra=state.reset_words[1];
    for(size_t i=0;i<7;++i) {meta.categories[i]=state.categories[i];}
    meta.generation_primary_rules=state.primary_rules;
    result.m_generated.reserve(state.records.size());for(const auto& record:state.records) {result.m_generated.push_back({record});}
    return result;
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
