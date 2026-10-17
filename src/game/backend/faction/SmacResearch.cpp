#include "SmacResearch.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
int32_t Checked(int64_t value) {
	if(value<INT32_MIN || value>INT32_MAX) { throw std::overflow_error("SMAC research arithmetic exceeds signed32"); }
	return static_cast<int32_t>(value);
}

bool Available(const settings::SmacTechnologyReference& reference,
	const SmacFactionTechnologyState& owner,const settings::SmacTechnologyRules& technologies) {
	using Kind=settings::SmacTechnologyReference::Kind;
	if(reference.kind==Kind::NONE) { return true; }
	if(reference.kind==Kind::DISABLED) { return false; }
	if(reference.kind!=Kind::TECHNOLOGY_ID || !reference.technology_id) { throw std::runtime_error("Unknown research prerequisite"); }
	const auto id=*reference.technology_id;
	if(id<0 || id>=88) { return false; }
	const auto& row=technologies.technologies[id];
	const auto first=technologies.ResolveToken(row.prerequisites[0]).kind;
	const auto second=technologies.ResolveToken(row.prerequisites[1]).kind;
	if(first==Kind::UNKNOWN || second==Kind::UNKNOWN) { throw std::runtime_error("Unknown owned research definition"); }
	return first!=Kind::DISABLED && (second!=Kind::DISABLED || first==Kind::NONE) && owner.owned[id];
}
}

int32_t CalculateSmacResearchCost(const SmacResearchCostInputs& input) {
	if(input.cache && *input.cache>=0) { return *input.cache; }
	if(input.discovery_rate==0) { return 999999999; }
	if(input.discovery_rate<0 || input.technology_cost_percent<0 || input.map_width<1 || input.map_height<1
		|| input.owner_difficulty<0 || input.owner_difficulty>5 || input.global_difficulty<0 || input.global_difficulty>5) {
		throw std::invalid_argument("Research cost inputs outside the measured ordinary domain");
	}
	const auto progression=Checked(int64_t(Checked(int64_t(input.score)+Checked(int64_t(input.deferred)*2)))-input.discount);
	const auto n=std::clamp(progression,2,9999)/2;
	int32_t leader=std::max(0,Checked(int64_t(input.score)+Checked(int64_t(input.deferred)*2)));
	for(size_t i=0;i<7;++i) { leader=std::max(leader,Checked(int64_t(input.other_scores[i])+Checked(int64_t(input.other_deferred[i])*2))); }
	leader/=2;
	static constexpr int32_t human[]{12,16,20,20,24,28}, ai[]{26,23,20,20,17,14};
	int32_t b=std::clamp(input.human?human[input.owner_difficulty]:ai[input.global_difficulty],12-n,12+n);
	const auto early_limit=input.stagnation?Checked(int64_t(b)*3)/2:b;
	const auto early=std::clamp(Checked(int64_t(n)-input.turn/(input.stagnation?12:8)),0,early_limit);
	b=Checked(int64_t(b)+early);
	const auto c=input.human?3:input.global_difficulty;
	const auto catchup_limit=Checked(int64_t(Checked(int64_t(b)*c)/10)+1);
	const auto catchup=std::clamp(Checked(int64_t(Checked(int64_t(Checked(int64_t(leader)-c))-n))+7)/(8-c),0,catchup_limit);
	b=Checked(int64_t(b)-catchup);
	const auto n2=std::clamp(n+(input.intrinsic_research>0?-1:input.intrinsic_research<0?1:0),1,99999);
	auto result=Checked(int64_t(n2)*b);
	if(input.discovery_rate!=100) { result=Checked(int64_t(result)*100)/input.discovery_rate; }
	if(input.technology_cost_percent!=100) { result=Checked(int64_t(result)*input.technology_cost_percent)/100; }
	const auto tiles=Checked(int64_t(input.map_width/2)*input.map_height);
	// Integer root of the actual tile count, without category or float rounding.
	int32_t low=0,high=46341;
	while(low+1<high) { const auto middle=low+(high-low)/2; if(int64_t(middle)*middle<=tiles) {low=middle;} else {high=middle;} }
	result=Checked(int64_t(result)*low)/56;
	if(input.stagnation) { result=Checked(int64_t(result)+result/2); }
	return std::clamp(result,1,99999999);
}

std::vector<int32_t> SmacManualResearchPool(const SmacFactionTechnologyState& owner,
	const settings::SmacTechnologyRules& technologies) {
	technologies.Validate();
	if(owner.admission.simulation_id<1 || owner.admission.simulation_id>7) { throw std::invalid_argument("Research picker requires an admitted owner"); }
	std::vector<int32_t> result;
	using Kind=settings::SmacTechnologyReference::Kind;
	constexpr size_t axes[]{3,1,2,0};
	for(size_t category=0;category<4;++category) {
		for(int32_t id=0;id<89;++id) {
			const auto& row=technologies.technologies[id]; size_t best=0;
			for(size_t index=1;index<4;++index) { if(row.priorities[axes[index]]>row.priorities[axes[best]]) {best=index;} }
			if(best!=category) { continue; }
			const auto first=technologies.ResolveToken(row.prerequisites[0]);
			const auto second=technologies.ResolveToken(row.prerequisites[1]);
			if(first.kind==Kind::UNKNOWN || second.kind==Kind::UNKNOWN) { throw std::runtime_error("Unknown research candidate prerequisite"); }
			if(first.kind==Kind::DISABLED || second.kind==Kind::DISABLED || !Available(first,owner,technologies) || !Available(second,owner,technologies)
				|| Available({Kind::TECHNOLOGY_ID,id},owner,technologies)) {continue;}
			if(!result.empty() && (id>=7 || owner.score!=0)
				&& Checked(int64_t(Checked(int64_t(id)-owner.score))+owner.admission.simulation_id)%3==0) {continue;}
			result.push_back(id);
		}
	}
	return result;
}

}
