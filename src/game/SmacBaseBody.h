#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
namespace game::smac_base_body {
struct Selection {int column,row;};
inline Selection Select(int population,int complex_limit,int dome_limit,bool headquarters,bool sea,bool perimeter,bool tachyon,bool cdf) {
    if(population<0 || population>127 || complex_limit<0 || complex_limit>126 || dome_limit<0 || dome_limit>126) {throw std::invalid_argument("Unclosed Base body selector inputs");}
    int size=population<4?0:population<std::max(7,complex_limit+1)?1:population<std::max(10,dome_limit+1)?2:3;
    if(headquarters) {size=std::min(3,size+1);}
    return {size,3*int(sea)+(tachyon?2:(perimeter || cdf)?1:0)};
}
struct Access {bool owned=false,unit_here=false,owner_pact=false,viewer_infiltration=false,revealed=false,empath=false,governor=false,neighbor_unit=false;};
inline bool Filled(const Access& a) noexcept {return a.unit_here || !(a.owned || a.owner_pact || a.viewer_infiltration || a.revealed || a.empath || a.governor || a.neighbor_unit);}
struct Style {bool filled;uint8_t border,inner,text;};
inline Style Colors(bool filled,bool spartans,uint8_t primary,uint8_t contrast,uint8_t text) noexcept {return {filled,uint8_t(filled && spartans?contrast:0),primary,uint8_t(filled?(spartans?44:0):text)};}
struct Layout {int width,height,baseline;};
// Measure 8/88, not the actual population. Supplied metrics belong to the font service.
inline Layout PopulationLayout(int population,int eight_width,int logical_height) {
    if(population<0 || population>127 || eight_width<1 || eight_width>64 || logical_height<1 || logical_height>64) {throw std::invalid_argument("Invalid Base population metrics");}
    return {(population<10?1:2)*eight_width+5,logical_height+4,logical_height+2};
}
}
