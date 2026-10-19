#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace game::backend::unit::smac_scout {struct AttackResult;}
namespace game::scout_presentation {
enum class Effect {None,Hit,Death};
struct Step {std::array<int32_t,2> health;Effect effect;size_t target;};
struct Plan {std::array<uint64_t,2> units;std::vector<Step> steps;};
Plan Prepare(const backend::unit::smac_scout::AttackResult& result);
// A presentation clock only: no Unit pointers, gameplay RNG or backend callbacks.
// Late frames reset the next deadline to actual now; they never catch up/skip.
class Playback {
public:
    enum class Kind {Health,Frame,Clear,Finished};
    struct Action {Kind kind;Step step;size_t frame=0;};
    explicit Playback(Plan plan,bool fast=false);
    const Plan& GetPlan() const {return m_plan;}
    std::optional<Action> Next(uint64_t now);
private:
    Plan m_plan;bool m_fast;size_t m_step=0,m_frame=0;
    enum class Phase {Health,Frames,Tail,Finished};
    Phase m_phase=Phase::Health;uint64_t m_deadline=0;
};
}
