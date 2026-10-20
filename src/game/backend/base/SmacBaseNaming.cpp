#include "SmacBaseNaming.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
std::string GenerateSmacBaseName(faction::SmacStartupFaction& actor,std::array<uint8_t,128>& output,
    const SmacBaseNameCatalog& catalog,int32_t owner,int32_t pattern,bool advance,bool sea,const SmacBaseNameObserver& observer) {
    if(owner<1 || owner>7 || catalog.land.empty() || catalog.land.size()>256 || catalog.sea.size()>256 || catalog.generic.size()>256) {
        throw std::invalid_argument("Automatic Base naming requires an ordinary installed faction catalog");
    }
    const auto next=[&](size_t off){uint32_t v=0;for(unsigned k=0;k<4;++k) {v|=uint32_t(actor.record[off+k])<<(8*k);}
        const auto cursor=SmacWrap32(v);if(cursor<0 || cursor==INT32_MAX) {throw std::invalid_argument("Automatic Base name cursor outside admitted range");}return cursor+1;};
    const auto result=[&](){const auto end=std::find(output.begin(),output.end(),uint8_t(0));return std::string(output.begin(),end);};
    const auto finish=[&](const std::string& label,uint32_t off,int32_t ordinal,uint32_t pc){
        if(label.find(char(0))!=std::string::npos) {throw std::invalid_argument("Automatic Base labels require ordinary parsed text");}
        std::fill_n(output.begin(),24,0);std::copy_n(label.begin(),std::min<size_t>(24,label.size()),output.begin());output[23]=0;
        if(advance) {for(unsigned k=0;k<4;++k) {actor.record[off+k]=uint8_t(uint32_t(ordinal)>>(8*k));}
            if(observer) {observer({uint32_t(owner*8380)+off,uint32_t(ordinal),pc});}}
        return result();
    };
    if(sea) {const auto k=next(16);if(size_t(k)<=catalog.sea.size()) {return finish(catalog.sea[k-1],16,k,0x4f9a16);}}
    const auto k=next(12);const auto fallback=catalog.noun+" "+std::to_string(k);
    if(catalog.noun.empty() || catalog.noun.find(char(0))!=std::string::npos || fallback.size()>=output.size()) {throw std::invalid_argument("Automatic Base fallback exceeds output buffer");}
    std::copy(fallback.begin(),fallback.end(),output.begin());output[fallback.size()]=0;
    auto ordinal=size_t(k);
    if(k>1 && ordinal<=catalog.land.size()) {
        uint32_t state=((uint32_t(owner)+uint32_t(pattern))&254u)|1u;size_t accepted=1;
        while(accepted!=size_t(k)) {if(state&1) {state^=0x170;}state>>=1;if(state<catalog.land.size()) {++accepted;}}
        ordinal=state+1;
    }
    if(ordinal<=catalog.land.size()) {return finish(catalog.land[ordinal-1],12,k,0x4f9c6e);}
    const auto generic=ordinal-catalog.land.size();if(generic<=catalog.generic.size()) {return finish(catalog.generic[generic-1],12,k,0x4f9c6e);}
    return result();
}
}
