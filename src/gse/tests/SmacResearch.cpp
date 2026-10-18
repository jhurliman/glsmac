#include "SmacResearch.h"
#include "game/backend/faction/SmacRankOrder.h"
#include "game/backend/faction/SmacResearch.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacResearch.h"
#include "fixtures/SmacDominance.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"
#include <sstream>
#include <limits>

namespace gse::tests {
namespace {
std::string ResearchCosts() {return research_fixture::Costs();}
std::string ResearchPools() {return research_fixture::Pools();}
std::string ResearchRates() {
	const auto catalog=smac_rules_fixture::Technologies();
	for(const auto& row:research_fixture::rates) {
		std::istringstream input(smac_rules_fixture::ProductionSource());std::string line,text;size_t index=0;
		bool rules=false;
		while(std::getline(input,line)) {
			if(!line.empty() && line[0]=='#') {rules=line=="#RULES";index=0;}
			else if(rules && ++index==8) {line=std::string(row.token)+", trailing";}
			text+=line+'\n';
		}
		const auto profile=loader::txt::SmacRulesTXTLoader::ParseResourceRules(text+"#RESOURCEINFO\nOcean,0,0,0\nBase,2,1,1\nBonus,0,0,0\nForest,0,0,0\nRecycling,0,0,0\nSea,0,0,0\nMonolith,0,0,0\nBorehole,0,0,0\nUnused,0,0,0\n#END\n",catalog);
		if(profile.discovery_rate!=row.expected) {return std::string("original discovery token: ")+row.token;}
		const auto restored=game::backend::settings::SmacResourceRules::Deserialize(profile.Serialize(catalog),catalog);
		if(restored.discovery_rate!=row.expected) {return "rate codec changed stored percentage";}
	}
	return {};
}
}
void AddSmacResearchTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC dominance treats an empty retained leader rank as a non-match",GT(){
        namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
        const auto rank=f::SmacRankOrder::Update({},128);
        const f::SmacVictoryRequirements requirements{{Ref::Kind::UNKNOWN,{}},-1,-1};
        f::SmacVictoryContext context;context.turn=251;context.humans=128;
        const f::SmacDominanceQuery query{7,rank.Leader(),6,3,0,9999,0,0};
        GT_ASSERT(rank.Leader()==0 && !f::SmacLeadingHumanDominance(context,query,requirements,smac_rules_fixture::Technologies()),
            "A valid retained inactive rank gap aborted dominance instead of returning false");GT_OK();
    });
	task->AddTest("SMAC dominance uses wide score comparisons and rejects unmeasured vote overflow",GT(){
		namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
		const f::SmacVictoryRequirements requirements{{Ref::Kind::TECHNOLOGY_ID,41},57,56};
		f::SmacVictoryContext context;context.turn=INT32_MAX;context.humans=2;context.rules=2;
		const f::SmacDominanceQuery query{1,1,INT32_MAX,5,0,9999,INT32_MAX,INT32_MAX};
		GT_ASSERT(!f::SmacLeadingHumanDominance(context,query,requirements,smac_rules_fixture::Technologies()),"Large equal scores overflowed into dominance");
		bool rejected=false;try{(void)f::SmacCouncilVotes(1,{{1,INT32_MAX}},false,true,{});}catch(const std::overflow_error&){rejected=true;}
		GT_ASSERT(rejected,"Unmeasured project multiplication overflow was accepted");
		rejected=false;try{(void)f::SmacCouncilVotes(1,{{1,INT32_MAX}},false,false,{2});}catch(const std::overflow_error&){rejected=true;}
		GT_ASSERT(rejected,"Unmeasured faction vote multiplication overflow was accepted");GT_OK();
	});
	task->AddTest("SMAC council votes match23 original population project and modifier cases",GT(){
		for(const auto& row:dominance_fixture::Votes()) {
			GT_ASSERT(game::backend::faction::SmacCouncilVotes(row.owner,row.bases,row.empath,row.clinical,row.modifiers)==row.expected,
				"Council votes differ: "+std::string(row.id));
		}GT_OK();
	});
	task->AddTest("SMAC potential majority matches26 original perspective and directed exclusion cases",GT(){
		namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
		auto catalog=smac_rules_fixture::Technologies();catalog.technologies[41].prerequisites={"Tech39","Tech52"};
		for(const auto& row:dominance_fixture::Majorities()) {
			f::SmacVictoryRequirements requirements{{Ref::Kind::TECHNOLOGY_ID,41},57,56};
			if(row.rule) {requirements.supreme_leader={row.rule==1 ? Ref::Kind::NONE : Ref::Kind::DISABLED,{}};}
			GT_ASSERT(f::SmacPotentialMajority(row.context,row.query,row.exclude,requirements,catalog)==row.expected,
				"Potential majority differs: "+std::string(row.id));
		}GT_OK();
	});
	task->AddTest("SMAC leading human dominance matches22 original strict admission and score cases",GT(){
		namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
		const f::SmacVictoryRequirements requirements{{Ref::Kind::TECHNOLOGY_ID,41},57,56};const auto catalog=smac_rules_fixture::Technologies();
		for(const auto& row:dominance_fixture::Scores()) {
			GT_ASSERT(f::SmacLeadingHumanDominance(row.context,row.query,requirements,catalog)==row.expected,
				"Leading human score path differs: "+std::string(row.id));
		}GT_OK();
	});
	task->AddTest("SMAC victory pressure matches48 original complete majority and dominance contexts",GT(){
		namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
		const f::SmacVictoryRequirements requirements{{Ref::Kind::TECHNOLOGY_ID,41},57,56};
		auto catalog=smac_rules_fixture::Technologies();catalog.technologies[41].prerequisites={"Tech39","Tech52"};
		for(const auto& row:dominance_fixture::Pressures()) {
			GT_ASSERT(f::SmacVictoryPressure(row.context,requirements)==row.pressure
				&& f::SmacPotentialMajority(row.context,1,-1,requirements,catalog)==row.majority
				&& f::SmacPotentialMajority(row.context,0,-1,requirements,catalog)==0
				&& f::SmacLeadingHumanDominance(row.context,row.query,requirements,catalog)==row.dominance,
				"Complete pressure/majority/dominance differs: "+std::string(row.id));
		}GT_OK();
	});
	task->AddTest("SMAC research threshold matches 37 original cost queries",GT(){return ResearchCosts();});
	task->AddTest("SMAC research pool matches 13 original ordinary manual choices",GT(){return ResearchPools();});
	task->AddTest("SMAC research discovery rate matches 24 original reader fragments",GT(){return ResearchRates();});
}
}
