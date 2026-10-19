#pragma once

#include "SmacDominanceHistory.h"
#include <algorithm>
#include <vector>

namespace game::backend::faction {

// Original graph-builder line requests, before the separate pixel rasterizer.
struct SmacDominanceGraph {
    using Rect=std::array<int32_t,4>;
    struct Line {int32_t x1,y1,x2,y2;uint8_t color;};
    Rect clip;
    int32_t horizontal_step,vertical_divisor;
    std::vector<Line> lines;

    static SmacDominanceGraph Build(size_t turn,Rect clip,
        const std::array<int32_t,8>& current,const SmacDominanceHistory& history,
        const std::array<std::string,8>& filenames,const std::array<uint8_t,8>& normal_colors,
        const std::array<uint8_t,8>& spartan_colors) {
        // Only the measured1024-pixel columns/font advance are admitted.
        if(turn>=SmacDominanceHistory::ROWS || (clip!=Rect{272,76,492,162} && clip!=Rect{501,76,721,162})) {
            throw std::invalid_argument("Dominance graph requires measured1024 columns and turn0..999");
        }
        const int32_t width=clip[2]-clip[0],height=clip[3]-clip[1]-13-4;
        int32_t step=1;
        while(turn/static_cast<size_t>(step)>static_cast<size_t>(width)) {++step;}
        int32_t maximum=1;
        for(size_t owner=1;owner<8;++owner) {maximum=std::max(maximum,current[owner]);}
        const auto divisor=static_cast<int32_t>((int64_t(maximum)+height-1)/height);
        SmacDominanceGraph result{clip,step,divisor,{}};
        for(size_t owner=1;owner<8;++owner) {
            const auto color=(filenames[owner]=="SPARTANS" ? spartan_colors:normal_colors)[owner];
            bool first=true;int32_t previous_x=0,previous_y=0,x=clip[0];
            for(size_t sample=1;sample<=turn;sample+=step,++x) {
                const int32_t y=clip[3]-1-int32_t(history.samples[sample][owner])/divisor;
                if(!first) {result.lines.push_back({previous_x,previous_y,x,y,color});}
                previous_x=x;previous_y=y;first=false;
            }
        }
        return result;
    }
};

}
