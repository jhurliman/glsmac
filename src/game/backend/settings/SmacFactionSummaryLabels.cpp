#include "SmacFactionSummaryLabels.h"
#include "loader/txt/TXTLoader.h"
#include "util/String.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::settings {
std::optional<SmacFactionSummaryLabels> SmacFactionSummaryLabels::Read(const std::string& source) {
    if(source.find('\0')!=std::string::npos) {throw std::invalid_argument("NUL in faction summary labels source");}
    bool has_repute=false,has_might=false;
    for(const auto& raw:util::String::Split(source,'\n')) {
        const auto line=util::String::TrimCopy(raw);
        has_repute=has_repute || line=="#REPUTE";has_might=has_might || line=="#MIGHT";
    }
    if(!has_repute && !has_might) {return {};}
    const auto repute_rows=loader::txt::TXTLoader::ReadUniqueSection(source,"REPUTE");
    const auto might_rows=loader::txt::TXTLoader::ReadUniqueSection(source,"MIGHT");
    if(repute_rows.size()!=8 || might_rows.size()!=7) {throw std::invalid_argument("Faction summary labels require8 REPUTE and7 MIGHT rows");}
    SmacFactionSummaryLabels result;
    for(size_t i=0;i<8;++i) {result.repute[i]=util::String::TrimCopy(repute_rows[i].substr(0,repute_rows[i].find(';')));}
    for(size_t i=0;i<7;++i) {
        auto row=might_rows[i].substr(0,might_rows[i].find(';'));
        result.might[i]=util::String::TrimCopy(row.substr(0,row.find(',')));
    }
    result.Validate();return result;
}
void SmacFactionSummaryLabels::Validate() const {
    const auto check=[](const auto& labels) {for(const auto& label:labels) {
        if(label.empty() || label.size()>MAX_LABEL_BYTES || label.find_first_of("\r\n\t,")!=std::string::npos || label.find('\0')!=std::string::npos) {
            throw std::invalid_argument("Invalid retained faction summary label");
        }
    }};
    check(repute);check(might);
}
types::Buffer SmacFactionSummaryLabels::Serialize() const {
    Validate();types::Buffer buffer;buffer.WriteInt(1);
    for(const auto& label:repute) {buffer.WriteString(label);}for(const auto& label:might) {buffer.WriteString(label);}return buffer;
}
SmacFactionSummaryLabels SmacFactionSummaryLabels::Deserialize(types::Buffer buffer) {
    if(buffer.lenw>MAX_WIRE_BYTES || buffer.ReadInt()!=1) {throw std::runtime_error("Unsupported faction summary labels frame");}
    SmacFactionSummaryLabels result;for(auto& label:result.repute) {label=buffer.ReadString(MAX_LABEL_BYTES);}
    for(auto& label:result.might) {label=buffer.ReadString(MAX_LABEL_BYTES);}
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Unexpected faction summary label fields");}result.Validate();return result;
}
const std::string& SmacFactionSummaryLabels::Integrity(uint32_t word) const {
    // The native reader clamps a signed32 field, including restored high bits.
    const int64_t value=word<=INT32_MAX ? int64_t(word) : int64_t(word)-(int64_t(1)<<32);
    return repute.at(static_cast<size_t>(std::clamp<int64_t>(value,0,7)));
}
const std::string& SmacFactionSummaryLabels::Might(int32_t rank) const {return might.at(7-std::clamp(rank,1,7));}
}
