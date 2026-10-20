#pragma once
#include <algorithm>
#include <stdexcept>
namespace game::native_zoom {
inline constexpr int minimum=-14,maximum=16;
inline int Unit(int zoom) {
    if(zoom<minimum || zoom>maximum) {throw std::invalid_argument("Unsupported native map zoom");}
    return std::max(1,(50*(zoom+16)/16+1)/4);
}
}
