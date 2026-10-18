#include "SmacResearch.h"
#include "game/backend/faction/SmacRankOrder.h"
#include "game/backend/faction/SmacResearch.h"
#include "game/backend/settings/SmacCouncilRules.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacResearch.h"
#include "fixtures/SmacDominance.h"
#include "fixtures/SmacRegionalStrategy.h"
#include "fixtures/SmacRegionalInitial.h"
#include "game/backend/map/SmacTerritoryRules.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"
#include <sstream>
#include <limits>
#include <fstream>
#include <cstdlib>

namespace gse::tests {
namespace {
std::string ResearchCosts() {return research_fixture::Costs();}
std::string ProposalSource(const std::string& required="Tech41") {
	std::string text="#PROPOSALS\n";
	for(size_t i=0;i<11;++i) {text+="Repeated translated label, "+(i==1 ? required : "None")+", effect, unused suffix\n";}
	return text+"#END\n";
}
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
    task->AddTest("SMAC regional initial strategy matches32 literal native threshold stores",GT(){
        using namespace game::backend::faction;
        for(const auto& row:regional_initial_fixture::Cases()) {
            std::array<SmacRegionalStrategyRegion,128> regions{};regions[1]=row.region;
            const auto actual=RefreshSmacRegionalStrategy(row.context,regions,{},[](int32_t,uint8_t,int32_t)->bool {throw std::runtime_error("Initial pressure case unexpectedly routed");});
            GT_ASSERT(actual[1]==row.expected,"Native threshold store differs: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC regional strategy matches33 native policy controls with exact route dispatch",GT(){
        using namespace game::backend::faction;
        for(const auto& row:regional_strategy_fixture::Cases()) {
            std::array<SmacRegionalStrategyRegion,128> regions{};regions[1]=row.region;
            SmacRegionalStrategyRow previous{};previous[0]=99;previous[1]=row.old_code;size_t calls=0;
            const auto actual=RefreshSmacRegionalStrategy(row.context,regions,previous,[&](int32_t peer,uint8_t region,int32_t mode) {
                if(calls>=row.queries.size()) {throw std::runtime_error("Unexpected route call for "+std::string(row.id));}
                const auto query=row.queries[calls++];
                if(region!=1 || peer!=query.peer || mode!=query.mode) {throw std::runtime_error("Route order differs for "+std::string(row.id));}
                return query.result;
            });
            GT_ASSERT(actual[1]==row.expected && calls==row.queries.size() && actual[0]==99,"Native policy or route admission differs: "+std::string(row.id));
        }
        GT_OK();
    });
    task->AddTest("SMAC regional strategy matches14 native opening projections and region range canaries",GT(){
        using namespace game::backend::faction;
        for(const auto& row:regional_strategy_fixture::Openings()) {
            SmacRegionalStrategyContext context{1,1,1024,row.total_bases,1,0,false,true};
            std::array<SmacRegionalStrategyRegion,128> regions{};regions[1]=row.region;
            SmacRegionalStrategyRow previous{};previous[0]=99;previous[1]=row.old_code;
            const auto actual=RefreshSmacRegionalStrategy(context,regions,previous,[](int32_t,uint8_t,int32_t){return false;});
            GT_ASSERT(actual[1]==row.expected,"Native opening region1 differs: "+std::string(row.id));
            if(std::string(row.id)=="regional_reset_canaries") {
                GT_ASSERT(actual[0]==99 && actual[1]==7 && actual[2]==8 && actual[62]==8 && actual[63]==8
                    && actual[64]==8 && actual[65]==7 && actual[126]==7 && actual[127]==8,"Native all-region range canaries differ");
            }
        }
        GT_OK();
    });
    task->AddTest("SMAC regional strategy composes actual Base routing and retained path workspace",GT(){
        using namespace game::backend::faction;namespace t=game::backend::map::smac_territory;
        for(const auto& row:regional_strategy_fixture::Cases()) {
            const auto name=std::string(row.id);
            if(name!="war_live_adjacent_base_weak" && name!="war_live_adjacent_base_strong" && name!="war_stale_presence_no_live_base") {continue;}
            t::RegionalRouteMap map{64,32,true,{}, {},2};map.tiles.assign(1024,{false,1,1});map.tiles[16*32+10].territory=2;
            std::vector<t::Base> bases{{{{16,16}},1}};
            if(name!="war_stale_presence_no_live_base") {bases.push_back({{{20,16}},2});}
            t::RegionalLandWorkspace workspace(64,32);size_t calls=0;
            std::array<SmacRegionalStrategyRegion,128> regions{};regions[1]=row.region;SmacRegionalStrategyRow previous{};previous[1]=row.old_code;
            const auto actual=RefreshSmacRegionalStrategy(row.context,regions,previous,[&](int32_t peer,uint8_t region,int32_t mode) {
                ++calls;return t::RegionalBasesReachable(map,bases,1,peer,region,mode,row.region.peers[1].bases,row.region.peers[peer].bases,
                    workspace,[](int32_t){return 8;});
            });
            GT_ASSERT(actual[1]==row.expected && calls==row.queries.size(),"Composed actual route differs: "+name);
        }
        GT_OK();
    });
	if(const auto* asset=std::getenv("GLSMAC_TEST_ALPHA_ASSET")) {
		const std::string path=asset;
		task->AddTest("SMAC installed proposals match all11 native requirement records",[path](GSE*,context::Context*)->std::string {
			std::ifstream file(path,std::ios::binary);GT_ASSERT(file.good(),"Installed alpha.txt could not be opened");
			const std::string source((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
			const auto catalog=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
			const auto rules=game::backend::settings::SmacCouncilRules::Read(source,catalog);GT_ASSERT(rules.has_value(),"Installed proposals unavailable");
			const std::array<int32_t,11> expected{{-1,41,77,61,61,66,66,62,17,17,-2}};
			using Kind=game::backend::settings::SmacTechnologyReference::Kind;
			for(size_t i=0;i<11;++i) {const auto value=catalog.ResolveToken(rules->prerequisites[i]);
				const auto id=value.kind==Kind::NONE ? -1 : value.kind==Kind::DISABLED ? -2 : value.technology_id.value_or(-3);
				GT_ASSERT(id==expected[i],"Installed proposal requirement differs at row "+std::to_string(i+1));
			}
			GT_ASSERT(rules->SupremeLeader(catalog).technology_id==std::optional<int32_t>(41),"Installed Supreme Leader binding differs");GT_OK();
		});
	}
	task->AddTest("SMAC proposal prerequisites retain positional Supreme Leader authority and strict ingestion",GT(){
		using Council=game::backend::settings::SmacCouncilRules;using Kind=game::backend::settings::SmacTechnologyReference::Kind;
		const auto catalog=smac_rules_fixture::Technologies();const auto parsed=Council::Read(ProposalSource(),catalog);
		GT_ASSERT(parsed && parsed->SupremeLeader(catalog).technology_id==std::optional<int32_t>(41),"Proposal2 did not supply the loaded requirement");
		for(const auto& token:{"None","none","Disable","disable","tEcH41","  Tech41  "}) {
			const auto value=Council::Read(ProposalSource(token),catalog);const auto resolved=catalog.ResolveToken(value->prerequisites[1]);
			GT_ASSERT(value->SupremeLeader(catalog).kind==resolved.kind && value->SupremeLeader(catalog).technology_id==resolved.technology_id,"Proposal reader changed reference semantics");
		}
		GT_ASSERT(Council::Read(ProposalSource("None"),catalog)->SupremeLeader(catalog).kind==Kind::NONE
			&& Council::Read(ProposalSource("Disable"),catalog)->SupremeLeader(catalog).kind==Kind::DISABLED
			&& !Council::Read("#OTHER\n",catalog),"Missing proposal authority became a default requirement");
		const auto rejects=[&](const std::string& source){try{Council::Read(source,catalog);}catch(const std::exception&){return true;}return false;};
		GT_ASSERT(rejects(ProposalSource("Tech41X")) && rejects(ProposalSource("")) && rejects(ProposalSource(std::string(257,'x')))
			&& rejects(ProposalSource()+ProposalSource()) && rejects("#PROPOSALS\nRow,Tech41,effect\n#END\n"),"Malformed/unknown proposal profile was admitted");
		const auto wire=parsed->Serialize(catalog).ToString();
		GT_ASSERT(Council::Deserialize(types::Buffer(wire),catalog)==*parsed,"Proposal codec changed the frozen source");
		bool rejected=false;try{Council::Deserialize(types::Buffer(wire+"x"),catalog);}catch(const std::exception&){rejected=true;}
		GT_ASSERT(rejected,"Proposal codec dropped trailing data");GT_OK();
	});
	task->AddTest("SMAC proposal host binding restores freezes and preserves legacy unavailability",GT(){
		using game::backend::settings::GlobalSettings;
		const auto source=[](const std::string& filename){return smac_rules_fixture::Source(filename)+(filename=="alpha.txt" ? ProposalSource() : "");};
		GlobalSettings host;host.SelectRuleset("smac");host.InitializeHostRules(source);smac_rules_fixture::AcceptWorldSize(host);
		const auto before=host.Serialize().ToString();GlobalSettings restored;restored.Deserialize(types::Buffer(before));
		GT_ASSERT(restored.GetSmacCouncilRules() && *restored.GetSmacCouncilRules()==*host.GetSmacCouncilRules(),"Host/restored proposal authority differs");
		GlobalSettings changed;changed.SelectRuleset("smac");changed.InitializeHostRules([](const std::string& filename){return smac_rules_fixture::Source(filename)+(filename=="alpha.txt" ? ProposalSource("Tech39") : "");});smac_rules_fixture::AcceptWorldSize(changed);
		bool rejected=false;try{host.Deserialize(changed.Serialize());}catch(const std::exception&){rejected=true;}
		GT_ASSERT(rejected && host.Serialize().ToString()==before,"Frozen proposal mutation partially changed host settings");
		GlobalSettings legacy;legacy.Deserialize(smac_rules_fixture::Payload("smac",true));
		GT_ASSERT(!legacy.GetSmacCouncilRules(),"V17 restore invented proposal prerequisites");
		GlobalSettings minimal;minimal.SelectRuleset("smac");minimal.InitializeHostRules(smac_rules_fixture::Source);smac_rules_fixture::AcceptWorldSize(minimal);
		auto wire=minimal.Serialize();wire.ReadInt();types::Buffer prefix;prefix.WriteInt(18);types::Buffer marker;marker.WriteBool(false);
		const auto v18=prefix.ToString()+wire.ToString().substr(wire.lenr,wire.lenw-wire.lenr-2*marker.ToString().size());
		GlobalSettings migrated;migrated.Deserialize(types::Buffer(v18));GT_ASSERT(!migrated.GetSmacCouncilRules(),"V18 restore invented current installed requirements");
		GT_OK();
	});
    task->AddTest("SMAC dominance treats an empty retained leader rank as a non-match",GT(){
        namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
        const auto rank=f::SmacRankOrder::Update({},128);
        const f::SmacVictoryRequirements requirements{{Ref::Kind::UNKNOWN,{}},-1,-1};
        f::SmacVictoryContext context;context.turn=251;context.humans=128;
        const f::SmacDominanceQuery query{7,rank.Leader(),6,3,0,9999,0,0};
        GT_ASSERT(rank.Leader()==0 && !f::SmacLeadingHumanDominance(context,query,requirements,smac_rules_fixture::Technologies()),
            "A valid retained inactive rank gap aborted dominance instead of returning false");GT_OK();
    });
    task->AddTest("SMAC dominance accepts six native consumed-score sentinel comparisons",GT(){
        namespace f=game::backend::faction;using Ref=game::backend::settings::SmacTechnologyReference;
        const f::SmacVictoryRequirements requirements{{Ref::Kind::UNKNOWN,{}},-1,-1};
        f::SmacVictoryContext context;context.turn=251;context.humans=2;
        const auto catalog=smac_rules_fixture::Technologies();
        for(const uint32_t rules:{2u,0u,0x40u}) {for(const int32_t runner:{-1,0}) {
            context.rules=rules;const f::SmacDominanceQuery query{1,1,6,2,0,9999,-1,runner};
            GT_ASSERT(f::SmacLeadingHumanDominance(context,query,requirements,catalog)==(runner==-1),"Consumed native score sign was rejected or converted to zero");
        }}GT_OK();
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
