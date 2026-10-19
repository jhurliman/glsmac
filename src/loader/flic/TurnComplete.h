#pragma once
#include "FlicReader.h"
#include <stdexcept>

namespace loader::flic {
// Explicitly bounded turn.flc profile. Menu playback keeps its declared-frame
// semantics; this caller alone admits the measured empty physical ring frame.
inline std::array<Frame,19> DecodeTurnComplete(const std::vector<uint8_t>& bytes) {
    std::array<Frame,19> frames;
    for(size_t i=0;i<frames.size();++i) {
        frames[i]=FlicReader::ReadFrame(bytes,i,233*23);
        if(frames[i].width!=233 || frames[i].height!=23 || frames[i].frame_count!=19) {
            throw std::invalid_argument("Turn Complete requires the native233x23,19-frame profile");
        }
    }
    // ReadFrame already validates every physical envelope. Require exactly
    // nineteen declared frames followed by a16-byte, zero-chunk ring frame.
    const auto dword=[&](size_t at) {return uint32_t(bytes.at(at)) | uint32_t(bytes.at(at+1))<<8 |
        uint32_t(bytes.at(at+2))<<16 | uint32_t(bytes.at(at+3))<<24;};
    size_t at=dword(80);if(!at) {at=128;}
    for(size_t i=0;i<19;++i) {at+=dword(at);}
    if(at>bytes.size() || bytes.size()-at!=16 || dword(at)!=16 || bytes.at(at+6)!=0 || bytes.at(at+7)!=0) {
        throw std::invalid_argument("Turn Complete requires the measured empty ring frame");
    }
    return frames;
}
}
