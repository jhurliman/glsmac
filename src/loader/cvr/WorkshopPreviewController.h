#pragma once
#include <cstdint>

namespace loader::cvr {
// Original Workshop scheduling/yaw state. Shared model phase is owned by the
// graphics lifetime, separately; selection refresh does not reset this state.
class WorkshopPreviewController {
public:
    void Reset();
    void Start(uint64_t now_ms);
    void Stop();
    bool Due(uint64_t now_ms);
    uint32_t BeginPaint(bool cached);
    static uint32_t AdvanceYaw(uint32_t bits);
    uint32_t Yaw() const {return m_yaw;}
    uint32_t PaintCount() const {return m_paints;}
private:
    uint32_t m_yaw=0,m_paints=0;
    uint64_t m_last_notification=0;
    bool m_running=false;
};
}
