#include "SmacResearch.h"
#include "game/backend/faction/SmacResearch.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacResearch.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"
#include <sstream>

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
	task->AddTest("SMAC research threshold matches 37 original cost queries",GT(){return ResearchCosts();});
	task->AddTest("SMAC research pool matches 13 original ordinary manual choices",GT(){return ResearchPools();});
	task->AddTest("SMAC research discovery rate matches 24 original reader fragments",GT(){return ResearchRates();});
}
}
