#pragma once
#include "SmacLandmarkTables.h"
#include <cstddef>

namespace game::backend::map::smac_terrain::landmark_tables {
struct PreparedHeightProfiles {
    std::array<std::array<uint8_t,113>,5> values=HEIGHT;
    std::array<bool,5> admitted{{true,true,true,true,true}};
};
// Shared original profile preparation, independent of rendering cache allocation.
template<class Getter> PreparedHeightProfiles PrepareHeightProfiles(size_t count,const Getter& word_at) {
    PreparedHeightProfiles result;
    for(size_t i=0;i<count;++i) {
        const auto word=word_at(i);const unsigned detail=word>>24;
        if(detail<128 || !(word&3u)) {continue;}const unsigned row=255-detail;
        for(unsigned p=0;p<2;++p) {
            if(!(word&(p==0?2u:1u))) {continue;}
            if(row>=49) {result.admitted[p]=false;continue;}
            for(unsigned v=0;v<5;++v) {result.values[p][MAPPING[row*5+v]]=0;}
        }
    }
    return result;
}
}
