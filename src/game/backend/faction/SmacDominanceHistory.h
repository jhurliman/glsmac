#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace game::backend::faction {

// RE-SMAC-DOMINANCE-GRAPH: turn-major signed-short samples. Slot0 and
// unsampled future rows are retained save data, not a reconstructed chart.
struct SmacDominanceHistory {
    static constexpr size_t ROWS=1000, SLOTS=8, BYTES=ROWS*SLOTS*2;
    std::array<std::array<int16_t,SLOTS>,ROWS> samples{};

    void Publish(size_t turn,const std::array<int32_t,SLOTS>& scores) {
        if(turn>=ROWS) {return;}
        for(size_t owner=1;owner<SLOTS;++owner) {
            const auto word=static_cast<uint16_t>(scores[owner]);
            samples[turn][owner]=static_cast<int16_t>(word<=INT16_MAX ? int32_t(word):int32_t(word)-65536);
        }
    }
    std::string Serialize() const {
        std::string bytes;bytes.reserve(BYTES);
        for(const auto& row:samples) {for(const auto value:row) {
            const auto word=static_cast<uint16_t>(value);
            bytes+=static_cast<char>(word&255);bytes+=static_cast<char>(word>>8);
        }}
        return bytes;
    }
    static SmacDominanceHistory Deserialize(const std::string& bytes) {
        if(bytes.size()!=BYTES) {throw std::invalid_argument("Dominance history requires16000 bytes");}
        SmacDominanceHistory result;size_t cursor=0;
        for(auto& row:result.samples) {for(auto& value:row) {
            const auto word=uint16_t(static_cast<uint8_t>(bytes[cursor]))
                | (uint16_t(static_cast<uint8_t>(bytes[cursor+1]))<<8);cursor+=2;
            value=static_cast<int16_t>(word<=INT16_MAX ? int32_t(word):int32_t(word)-65536);
        }}
        return result;
    }
    bool operator==(const SmacDominanceHistory& other) const {return samples==other.samples;}
};

}
