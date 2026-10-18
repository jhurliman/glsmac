#include "SmacSiteSignal.h"
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {
int64_t Floor4(int64_t value) {return value/4-(value%4<0);}
int64_t Residue(int64_t value,int64_t modulus) {const auto result=value%modulus;return result<0?result+modulus:result;}
}
uint8_t ResolveSiteSignal(const SiteSignalInputs& input) {
    if(input.x<0 || input.x>=512 || input.y<0 || input.y>=256 || ((input.x^input.y)&1)
        || static_cast<uint8_t>(input.altitude_band)>7) {
        throw std::invalid_argument("Site signal requires bounded parity coordinates and decoded altitude");
    }
    if(input.tile_flags&0x00102000u) {return 0;}
    if(input.automatic_land_resources) {return (input.tile_flags&0x0c000000u)?2:0;}
    if(input.tile_flags&0x10000000u) {return 1;}
    if(input.pattern_offset==0) {return 0;}
    const int64_t u=(input.x+input.y)/2,v=input.x-u;
    const auto phase=Residue(u,4)+4*Residue(v,4);
    if(input.altitude_band>=AltitudeBand::Land3
        && phase==Residue(-5*Floor4(u)-3*Floor4(v)+input.pattern_offset,16)) {return 2;}
    // Unlike the primary pattern's floor shifts, these divisions truncate.
    // Only low5 bits survive native signed32 products/addition; signed64
    // evaluation preserves those bits without overflowing in this map domain.
    const auto secondary=int64_t(input.pattern_offset)+int64_t(input.coefficient_u)*(u/4)
        +int64_t(input.coefficient_v)*(v/4)+8;
    return phase==Residue(secondary,32)?1:0;
}
}
