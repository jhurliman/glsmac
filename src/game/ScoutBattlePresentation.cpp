#include "ScoutBattlePresentation.h"
#include "backend/unit/SmacScoutCombat.h"
#include <stdexcept>
#include <utility>
namespace game::scout_presentation {
Plan Prepare(const backend::unit::smac_scout::AttackResult& result) {
    using namespace backend::unit::smac_scout;
    const bool attacker=result.victory==Victory::Attacker;
    Plan plan{{attacker ? result.survivor.identity:result.deleted_identity,attacker ? result.deleted_identity:result.survivor.identity},{}};
    plan.steps.reserve(result.rounds.size());
    for(size_t i=0;i<result.rounds.size();++i) {
        const auto before=result.rounds[i].health_before;
        const auto after=i+1<result.rounds.size() ? result.rounds[i+1].health_before:result.health_after;
        const size_t target=after[0]<before[0] ? 0:1;
        if(after[target]!=before[target]-1 || after[1-target]!=before[1-target]
            || after[0]<0 || after[0]>10 || after[1]<0 || after[1]>10) {throw std::invalid_argument("Scout presentation requires the live Fission round domain");}
        const auto effect=after[target]==0 ? Effect::Death:(10-before[target])/3!=(10-after[target])/3 ? Effect::Hit:Effect::None;
        plan.steps.push_back({after,effect,target});
    }
    return plan;
}
Playback::Playback(Plan plan,bool fast):m_plan(std::move(plan)),m_fast(fast) {
    if(!m_plan.units[0] || !m_plan.units[1] || m_plan.units[0]==m_plan.units[1] || m_plan.steps.empty()) {throw std::invalid_argument("Scout presentation requires two stable identities and rounds");}
}
std::optional<Playback::Action> Playback::Next(uint64_t now) {
    if(m_phase==Phase::Finished) {return std::nullopt;}
    if(m_step==m_plan.steps.size()) {m_phase=Phase::Finished;return Action{Kind::Finished,{{0,0},Effect::None,0},0};}
    const auto step=m_plan.steps[m_step];
    if(m_phase==Phase::Health) {
        if(step.effect==Effect::None) {++m_step;}else {m_phase=Phase::Frames;m_frame=0;m_deadline=now;}
        return Action{Kind::Health,step,0};
    }
    if(now<m_deadline) {return std::nullopt;}
    if(m_phase==Phase::Tail) {++m_step;m_phase=Phase::Health;return Next(now);}
    if(m_frame==16) {
        m_phase=Phase::Tail;m_deadline=now+(m_fast ? 0:50);
        return Action{Kind::Clear,step,0};
    }
    m_deadline=now+(step.effect==Effect::Death ? 50:m_fast ? 0:25);
    return Action{Kind::Frame,step,m_frame++};
}
}
