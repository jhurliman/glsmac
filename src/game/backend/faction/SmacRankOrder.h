#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace game::backend::faction {

// Retained inverse rank positions, not a current survivor list or a score
// provider. Only an explicit ranking update replaces these identities.
struct SmacRankOrder {
	std::array<int32_t,8> faction_at_rank{};
	bool operator==(const SmacRankOrder& other) const {return faction_at_rank==other.faction_at_rank;}
	int32_t Leader() const {return faction_at_rank[7];}
	int32_t RunnerUp() const {return faction_at_rank[6];}
	int32_t DispositionReference(int32_t eliminated) const {
		if(eliminated<1 || eliminated>7) {throw std::invalid_argument("Ending reference requires a playable eliminated faction");}
		return Leader()==eliminated ? RunnerUp() : Leader();
	}
	static SmacRankOrder Update(const std::array<int32_t,7>& scores,uint8_t active) {
		if(active & 1u) {throw std::invalid_argument("Native faction cannot occupy a playable ranking slot");}
		auto remaining=scores;
		for(std::size_t i=0;i<remaining.size();++i) {
			if(!(active & (1u<<(i+1)))) {remaining[i]=0;}
			else if(remaining[i]<0) {throw std::invalid_argument("Ordinary ranking requires nonnegative active scores");}
		}
		SmacRankOrder result;
		for(int32_t rank=7;rank>=1;--rank) {
			std::size_t selected=0;int32_t best=-1;
			for(std::size_t i=0;i<remaining.size();++i) {
				if(remaining[i]>best) {selected=i;best=remaining[i];}
			}
			remaining[selected]=-1;
			if(active & (1u<<(selected+1))) {result.faction_at_rank[rank]=static_cast<int32_t>(selected+1);}
		}
		return result;
	}
};

}
