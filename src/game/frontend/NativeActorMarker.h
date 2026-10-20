#pragma once
#include "game/NativeZoom.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace game::frontend::native_actor {
inline uint8_t MarkerBorderIndex(std::string_view identifier) {return identifier=="SPARTANS"?44:0;}
inline int PopulationFontSize(int zoom) {game::native_zoom::Unit(zoom);const int size=(50*(zoom+16)/16)/3;return size==10?11:size;}
inline bool UsesMarker(int zoom) {game::native_zoom::Unit(zoom);return zoom< -8;}
inline bool ShowsName(int zoom) {game::native_zoom::Unit(zoom);return zoom> -11;}
// Ordinary actor flags only: the forced/expanded marker modes are separate.
inline std::array<int,4> Marker(bool base,int zoom) {
    if(!UsesMarker(zoom)) {throw std::invalid_argument("Native marker requires zoom below -8");}
    const auto scale=[zoom](int n){return n*(zoom+16)/16;};
    const int margin=scale(base?4:8);
    int left=margin,top=margin,right=scale(100)-margin,bottom=scale(50)-margin;
    if(!base) {const int shift=(bottom-top)/4;top+=shift;bottom+=shift;}
    const int square=((right-left)-(bottom-top))/2;left+=square;right-=square;
    if(base) {const int shift=scale(6)-margin;top+=shift;bottom+=shift;}
    return {{left,top,right,bottom}};
}
}
