#pragma once
#include <chrono>
namespace game::frontend::unit {
// RE-SMAC-BLINK-001: one shared phase toggle per admitted/coalesced delivery.
// Steady time schedules requests only; elapsed time never determines parity.
class SelectionBlink {
public:
    using Clock=std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds HALF_PERIOD{150};
    bool Poll(Clock::time_point now,bool admitted) {
        if(!m_armed) {m_next=now+HALF_PERIOD;m_armed=true;return false;}
        if(now<m_next) {return false;}
        m_next+=HALF_PERIOD*((now-m_next)/HALF_PERIOD+1); // retain cadence, coalesce to one delivery
        if(!admitted) {return false;}
        m_visible=!m_visible;return true;
    }
    bool Visible() const {return m_visible;}
    void EnsureVisible() {m_visible=true;} // no timer rearm
private:
    Clock::time_point m_next{};
    bool m_armed=false;
    bool m_visible=true;
};
}
