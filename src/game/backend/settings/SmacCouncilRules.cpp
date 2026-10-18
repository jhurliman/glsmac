#include "SmacCouncilRules.h"
#include "loader/txt/TXTLoader.h"
#include "util/String.h"
#include <stdexcept>

namespace game::backend::settings {
std::optional<SmacCouncilRules> SmacCouncilRules::Read(const std::string& source,const SmacTechnologyRules& technologies) {
    bool present=false;
    for(const auto& line:util::String::Split(source,'\n')) {if(util::String::TrimCopy(line)=="#PROPOSALS") {present=true;break;}}
    // Older port sources have no proposal authority. Keep that unavailable,
    // rather than inventing the installed game's Mind/Machine requirement.
    if(!present) {return {};}
    const auto rows=loader::txt::TXTLoader::ReadUniqueSection(source,"PROPOSALS");
    if(rows.size()!=11) {throw std::invalid_argument("SMAC proposals require eleven positional records");}
    SmacCouncilRules result;
    for(size_t i=0;i<rows.size();++i) {
        const auto first=rows[i].find(',');const auto second=first==std::string::npos ? first : rows[i].find(',',first+1);
        if(first==std::string::npos || second==std::string::npos) {throw std::invalid_argument("Proposal lacks its prerequisite column");}
        result.prerequisites[i]=util::String::TrimCopy(rows[i].substr(first+1,second-first-1));
    }
    result.Validate(technologies);return result;
}
void SmacCouncilRules::Validate(const SmacTechnologyRules& technologies) const {
    for(const auto& token:prerequisites) {
        if(token.empty() || token.size()>MAX_TOKEN_BYTES || token.find('\0')!=std::string::npos
            || technologies.ResolveToken(token).kind==SmacTechnologyReference::Kind::UNKNOWN) {
            throw std::invalid_argument("Proposal requires a bounded resolved prerequisite token");
        }
    }
}
SmacTechnologyReference SmacCouncilRules::SupremeLeader(const SmacTechnologyRules& technologies) const {
    Validate(technologies);return technologies.ResolveToken(prerequisites[1]);
}
types::Buffer SmacCouncilRules::Serialize(const SmacTechnologyRules& technologies) const {
    Validate(technologies);types::Buffer buffer;buffer.WriteInt(1);
    for(const auto& token:prerequisites) {buffer.WriteString(token);}return buffer;
}
SmacCouncilRules SmacCouncilRules::Deserialize(types::Buffer buffer,const SmacTechnologyRules& technologies) {
    if(buffer.lenw>MAX_WIRE_BYTES || buffer.ReadInt()!=1) {throw std::runtime_error("Unsupported or oversized proposal profile");}
    SmacCouncilRules result;for(auto& token:result.prerequisites) {token=buffer.ReadString(MAX_TOKEN_BYTES);}
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Unexpected proposal profile fields");}
    result.Validate(technologies);return result;
}
}
