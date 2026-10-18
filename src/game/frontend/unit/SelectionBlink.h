#pragma once
#include <chrono>
namespace game::frontend::unit {
// Selection presentation only: no gameplay clock or simulation authority.
class SelectionBlink {
public:
    using Clock=std::chrono::steady_clock;
    // Provisional usability cadence until original animation timing is measured.
    static constexpr std::chrono::milliseconds HALF_PERIOD{500};
    bool Start(Clock::time_point now) {
        if(m_running) {return false;}
        m_epoch=now;m_running=true;return true;
    }
    void Stop() {m_running=false;}
    bool IsRunning() const {return m_running;}
    bool Visible(Clock::time_point now) const {
        if(!m_running || now<m_epoch) {return false;}
        const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-m_epoch);
        return (elapsed.count()/HALF_PERIOD.count())%2!=0;
    }
private:
    Clock::time_point m_epoch{};
    bool m_running=false;
};
}
