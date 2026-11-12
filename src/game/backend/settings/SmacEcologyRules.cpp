#include "SmacEcologyRules.h"
#include "loader/txt/TXTLoader.h"
#include "util/String.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::settings {
std::optional<SmacEcologyRules> SmacEcologyRules::Read(const std::string& source) {
    bool present = false;
    for (const auto& line : util::String::Split(source, '\n')) {
        if (util::String::TrimCopy(line) == "#WORLDBUILDER") { present = true; break; }
    }
    if (!present) { return {}; }
    const auto rows = loader::txt::TXTLoader::ReadUniqueSection(source, "WORLDBUILDER");
    if (rows.size() <= 14) { throw std::invalid_argument("WORLDBUILDER lacks Sea Level Rises row15"); }
    const auto integer=[](std::string row) {
        row=row.substr(0,row.find(';'));
        const auto comma=row.find(',');
        if(comma!=std::string::npos && !util::String::TrimCopy(row.substr(comma+1)).empty()) {
            throw std::invalid_argument("WORLDBUILDER requires one numeric field per row");
        }
        const auto token=util::String::TrimCopy(row.substr(0,comma));
        const auto digits=!token.empty() && (token[0]=='+' || token[0]=='-')?1u:0u;
        if(token.size()<=digits || token.find_first_not_of("0123456789",digits)!=std::string::npos) {
            throw std::invalid_argument("WORLDBUILDER field is not a decimal integer");
        }
        const auto value=std::stoll(token);
        if(value<INT32_MIN || value>INT32_MAX) {throw std::invalid_argument("WORLDBUILDER exceeds signed32");}
        return static_cast<int32_t>(value);
    };
    SmacEcologyRules result{std::clamp(integer(rows.at(14)),1,100),{}};
    if(rows.size()>=27) {
        result.world_builder.emplace();
        for(size_t i=0;i<25;++i) {result.world_builder->at(i)=integer(rows.at(i));}
        const auto ratio_row=rows.at(25).substr(0,rows.at(25).find(';'));
        const auto ratios=util::String::Split(ratio_row,',');
        if(ratios.size()!=5) {throw std::invalid_argument("WORLDBUILDER continent ratios require five fields");}
        for(size_t i=0;i<5;++i) {result.world_builder->at(25+i)=integer(ratios[i]);}
        result.world_builder->at(30)=integer(rows.at(26));
        result.world_builder->at(14)=result.sea_level_rises; // native loader clamp
    }
    return result;
}
void SmacEcologyRules::Validate() const {
    if (sea_level_rises < 1 || sea_level_rises > 100) { throw std::invalid_argument("Loaded Sea Level Rises must be1..100"); }
    if(world_builder && world_builder->at(14)!=sea_level_rises) {throw std::invalid_argument("Generation and ecology Sea Level Rises differ");}
}
types::Buffer SmacEcologyRules::Serialize() const {
    Validate(); types::Buffer buffer;buffer.WriteInt(world_builder?2:1);buffer.WriteInt(sea_level_rises);
    if(world_builder) {for(const auto value:*world_builder) {buffer.WriteInt(value);}}
    return buffer;
}
SmacEcologyRules SmacEcologyRules::Deserialize(types::Buffer buffer) {
    if(buffer.lenw>MAX_WIRE_BYTES) {throw std::runtime_error("Oversized ecology/generation profile");}
    const auto version=buffer.ReadInt();
    if(version!=1 && version!=2) {throw std::runtime_error("Unsupported ecology/generation profile");}
    const auto value=buffer.ReadInt();
    if(value<1 || value>100) {throw std::runtime_error("Invalid loaded ecology profile");}
    SmacEcologyRules result{static_cast<int32_t>(value),{}};
    if(version==2) {
        result.world_builder.emplace();
        for(auto& row:*result.world_builder) {const auto raw=buffer.ReadInt();
            if(raw<INT32_MIN || raw>INT32_MAX) {throw std::runtime_error("Generation rule exceeds signed32");}row=static_cast<int32_t>(raw);}
    }
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing ecology/generation profile");}
    result.Validate();return result;
}
}
