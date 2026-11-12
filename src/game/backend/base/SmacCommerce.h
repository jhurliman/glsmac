#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace game::backend::base {
// RE-SMAC-COMMERCE-001: ordinary nonnegative metrics, original Base indices0..511.
// Relationship construction and raw metric publication belong to the caller.
struct SmacCommerceBase {int32_t owner,metric;};
struct SmacCommerceFaction {
    int32_t rating=0,base_count=0;
    uint8_t treaty_partners=0,pact_partners=0;
    bool blocked=false;
};
struct SmacCommerceInputs {
    std::vector<SmacCommerceBase> bases;
    size_t current=0;
    std::array<SmacCommerceFaction,8> factions{};
    int32_t enabled_technologies=0,governor=0;
    bool trade_doubled=false;
};
struct SmacCommerceCalculation {
    size_t rank=0;
    std::array<std::optional<size_t>,8> partners{};
    std::array<int32_t,8> incoming{},partner_quotes{};
    int32_t total=0;
};
inline int32_t SmacCommerceChecked(int64_t value) {
    if(value<0 || value>INT32_MAX) {throw std::overflow_error("SMAC commerce exceeds nonnegative signed32 domain");}
    return static_cast<int32_t>(value);
}
inline int32_t CalculateSmacCommerceRating(int32_t faction_bonus,int32_t economy,int32_t owned_enabled) {
    if(faction_bonus<0 || owned_enabled<0 || owned_enabled>88) {throw std::invalid_argument("Unsupported SMAC commerce rating inputs");}
    return SmacCommerceChecked(int64_t(faction_bonus)+owned_enabled+std::clamp(int64_t(economy)-2,int64_t(0),int64_t(3)));
}
inline SmacCommerceCalculation CalculateSmacCommerce(const SmacCommerceInputs& in) {
    if(in.bases.empty() || in.bases.size()>512 || in.current>=in.bases.size() || in.enabled_technologies<0 || in.enabled_technologies>89
        || in.governor<0 || in.governor>7) {throw std::invalid_argument("Invalid SMAC commerce world inputs");}
    std::array<std::vector<size_t>,8> order;
    for(size_t i=0;i<in.bases.size();++i) {
        const auto& base=in.bases[i];
        if(base.owner<1 || base.owner>7 || base.metric<0 || int64_t(base.metric)*512+i>INT32_MAX) {
            throw std::invalid_argument("Unsupported SMAC commerce Base rank key");
        }
        order[base.owner].push_back(i);
    }
    for(size_t owner=1;owner<8;++owner) {
        const auto& row=in.factions[owner];
        if(row.rating<0 || row.base_count<0 || row.base_count>512) {throw std::invalid_argument("Invalid SMAC commerce faction inputs");}
        std::sort(order[owner].begin(),order[owner].end(),[&](size_t a,size_t b) {
            return in.bases[a].metric!=in.bases[b].metric?in.bases[a].metric>in.bases[b].metric:a>b;
        });
    }
    const auto owner=in.bases[in.current].owner;
    const auto& own=in.factions[owner];
    SmacCommerceCalculation out;
    out.rank=size_t(std::find(order[owner].begin(),order[owner].end(),in.current)-order[owner].begin());
    const int32_t denominator=in.enabled_technologies+1;
    for(int32_t other=1;other<=7;++other) {
        const auto& peer=in.factions[other];
        if(other==owner || peer.base_count==0 || !((own.treaty_partners&(1u<<other)) || (peer.treaty_partners&(1u<<owner)))
            || out.rank>=order[other].size()) {continue;}
        const auto paired=order[other][out.rank];out.partners[other]=paired;
        int32_t amount=SmacCommerceChecked(int64_t(in.bases[in.current].metric)+in.bases[paired].metric+7)/8;
        if(in.trade_doubled) {amount=SmacCommerceChecked(int64_t(amount)*2);}
        const auto quote=[&](int32_t rating) {return SmacCommerceChecked(int64_t(amount)*(int64_t(rating)+1)+denominator/2)/denominator;};
        auto incoming=quote(own.rating),reverse=quote(peer.rating);
        if(!(own.pact_partners&(1u<<other))) {incoming/=2;reverse/=2;}
        if(in.governor==owner) {incoming=SmacCommerceChecked(int64_t(incoming)+1);}
        if(in.governor==other) {reverse=SmacCommerceChecked(int64_t(reverse)+1);}
        if(own.blocked || peer.blocked) {incoming=reverse=0;}
        out.incoming[other]=incoming;out.partner_quotes[other]=reverse;
        out.total=SmacCommerceChecked(int64_t(out.total)+incoming);
    }
    return out;
}
}
