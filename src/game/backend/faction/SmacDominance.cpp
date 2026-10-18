#include "SmacDominance.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::faction {
namespace {
void Require(bool condition,const char* message) {if(!condition) {throw std::invalid_argument(message);}}
bool Positive(int32_t owner) {return owner>=1 && owner<=7;}
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Council vote arithmetic exceeds the measured signed32 domain");}
    return static_cast<int32_t>(value);
}
bool Has(const SmacVictoryContext& context,int32_t owner,const settings::SmacTechnologyReference& reference) {
    if(!Positive(owner)) {return false;}
    using Kind=settings::SmacTechnologyReference::Kind;
    if(reference.kind==Kind::NONE) {return true;}
    if(reference.kind==Kind::DISABLED) {return false;}
    Require(reference.kind==Kind::TECHNOLOGY_ID && reference.technology_id && *reference.technology_id>=0 && *reference.technology_id<89,
        "Victory predicate requires a resolved technology reference");
    return context.technologies[owner][*reference.technology_id];
}
bool MajorityTechnology(const SmacVictoryContext& context,int32_t query,const SmacVictoryRequirements& requirements,
    const settings::SmacTechnologyRules& catalog) {
    if(!Positive(query)) {return false;}
    const auto& required=requirements.supreme_leader;
    if(Has(context,query,required)) {return true;}
    if(required.kind!=settings::SmacTechnologyReference::Kind::TECHNOLOGY_ID) {return false;}
    for(const auto& token:catalog.technologies.at(*required.technology_id).prerequisites) {
        if(Has(context,query,catalog.ResolveToken(token))) {return true;}
    }
    return false;
}
}

int32_t SmacCouncilVotes(int32_t owner,const std::vector<SmacCouncilBase>& bases,bool empath,bool clinical,const std::vector<int32_t>& modifiers) {
    Require(Positive(owner),"Council votes require a positive faction");
    int32_t votes=0;
    for(const auto& base:bases) {
        Require(base.owner>=0 && base.owner<=7,"Council Base owner is invalid");
        if(base.owner==owner) {votes=Checked(int64_t(votes)+base.population);}
    }
    if(empath) {votes=Checked(int64_t(votes)+votes/2);}
    if(clinical) {votes=Checked(int64_t(votes)*2);}
    for(const auto value:modifiers) {if(value>=0) {votes=Checked(int64_t(votes)*value);}}
    return votes;
}

int32_t SmacPotentialMajority(const SmacVictoryContext& context,int32_t query,int32_t exclude,
    const SmacVictoryRequirements& requirements,const settings::SmacTechnologyRules& catalog) {
    Require(query>=0 && query<=7 && exclude>=-1 && exclude<=7,"Unsupported majority query or exclusion");
    if(!(context.rules&8) || !MajorityTechnology(context,query,requirements,catalog)) {return 0;}
    int64_t total=0;for(size_t owner=1;owner<=7;++owner) {total+=context.votes[owner];}
    int32_t result=0;
    for(int32_t owner=1;owner<=7;++owner) {
        if(owner==exclude) {continue;}
        if(exclude && (context.rules&0x1000)) {
            const auto word=exclude==-1 ? static_cast<uint32_t>(context.last_base_expansion_or_advice_turn[owner]) : context.relations[owner][exclude];
            if(word&1) {continue;}
        }
        if(int64_t(context.votes[owner])*2>=total && (!result || owner==query)) {result=owner;}
    }
    return result;
}

bool SmacVictoryPressure(const SmacVictoryContext& context,const SmacVictoryRequirements& requirements) {
    Require(context.turn>=0,"Victory pressure requires a nonnegative turn");
    for(int32_t owner=1;owner<=7;++owner) {
        if((context.humans&(1u<<owner)) && context.economic_deadlines[owner]>context.turn) {return true;}
    }
    // The intermediate majority(query0,-1) always fails its actual positive-
    // owner technology gate. It cannot substitute the score leader's knowledge.
    if(context.voice_host==-1) {return false;}
    Require(requirements.psi_gate>=0 && requirements.psi_gate<89 && requirements.voice_of_planet>=0 && requirements.voice_of_planet<89,
        "Pressure requires the current Psi Gate and Voice catalog technology IDs");
    for(int32_t owner=1;owner<=7;++owner) {
        if((context.humans&(1u<<owner)) && (context.technologies[owner][requirements.psi_gate]
            || context.technologies[owner][requirements.voice_of_planet])) {return true;}
    }
    return false;
}

bool SmacLeadingHumanDominance(const SmacVictoryContext& context,const SmacDominanceQuery& query,
    const SmacVictoryRequirements& requirements,const settings::SmacTechnologyRules& catalog) {
    Require(Positive(query.owner) && query.leader>=0 && query.leader<=7 && context.turn>=0 && query.difficulty>=0 && query.difficulty<=5
        && query.retained_base_count>=0,"Dominance query exceeds the measured domain");
    if(!(context.humans&(1u<<query.owner)) || query.owner!=query.leader
        || query.retained_base_count<=std::max<int64_t>(4,(int64_t(context.turn)+25)/50)) {return false;}
    const bool intense=(context.rules&0x40)!=0;
    if(!query.mode && query.difficulty<2 && !intense) {return false;}
    if(context.turn<=(intense ? 100 : 350-query.difficulty*50)) {return false;}
    if(SmacVictoryPressure(context,requirements) && SmacPotentialMajority(context,query.owner,-1,requirements,catalog)==query.owner) {return true;}
    if(intense) {return int64_t(query.leader_score)*4>=int64_t(query.runner_up_score)*5;}
    if(query.difficulty<3 && !(context.rules&2) && query.objectives>1000) {return int64_t(query.leader_score)>=int64_t(query.runner_up_score)*2;}
    return int64_t(query.leader_score)*2>=int64_t(query.runner_up_score)*3;
}
}
