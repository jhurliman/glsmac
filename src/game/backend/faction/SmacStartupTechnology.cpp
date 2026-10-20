#include "SmacStartupTechnology.h"
#include <stdexcept>

namespace game::backend::faction {
void GrantSmacStartupNamedTechnology(SmacStartupFaction& state,SmacStartupTechnologyOwnership& owned,
    int32_t faction,int32_t technology,int32_t source) {
    if(faction<0 || faction>7 || technology>=89) {throw std::invalid_argument("Invalid original turn-zero named technology grant");}
    if(technology<0) {return;}
    const auto read=[&](size_t off) {const auto& r=state.record;return uint32_t(r[off])|(uint32_t(r[off+1])<<8)|(uint32_t(r[off+2])<<16)|(uint32_t(r[off+3])<<24);};
    const auto write=[&](size_t off,uint32_t value) {for(unsigned k=0;k<4;++k) {state.record[off+k]=uint8_t(value>>(8*k));}};
    constexpr size_t target=0x95d574-0x95d1a8,counter=0x95f1f4-0x95d1a8;
    if(read(target)==uint32_t(technology)) {write(target,UINT32_MAX);}
    state.record[0x95d514-0x95d1a8+technology]=source>0?uint8_t(source):0;
    owned[technology]|=uint8_t(1u<<faction);write(counter,read(counter)+1u);
}
void GrantSmacStartupNamedTechnologies(SmacStartupFaction& state,SmacStartupTechnologyOwnership& owned,
    int32_t faction,const settings::SmacFactionStartupRule& profile) {
    const auto grants=profile.NamedTechnologyReferences();
    if(faction<0 || faction>7) {throw std::invalid_argument("Invalid named technology recipient");}
    for(const auto technology:grants) {if(technology>=89) {throw std::invalid_argument("Invalid named technology catalog reference");}}
    for(const auto technology:grants) {GrantSmacStartupNamedTechnology(state,owned,faction,technology);}
}
}
