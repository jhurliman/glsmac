#include "WorkshopPreviewController.h"
#include "ScoutArithmetic.h"

namespace loader::cvr {
void WorkshopPreviewController::Reset() {
    m_yaw=0;m_paints=0;m_last_notification=0;m_running=false;
}
void WorkshopPreviewController::Start(const uint64_t now_ms) {
    if(!m_running) {m_running=true;m_last_notification=now_ms;}
}
void WorkshopPreviewController::Stop() {m_running=false;}
bool WorkshopPreviewController::Due(const uint64_t now_ms) {
    if(!m_running) {return false;}
    const auto elapsed=now_ms-m_last_notification;
    if(elapsed<40) {return false;}
    // Retain the periodic epoch, coalescing missed notifications into one
    // delivery. Rendering time does not move the next notification deadline.
    m_last_notification=now_ms-elapsed%40;
    return true;
}
uint32_t WorkshopPreviewController::AdvanceYaw(const uint32_t bits) {
    return scout::Store(scout::Add(scout::Decode(bits),scout::Decode(0x3d4ccccd)));
}
uint32_t WorkshopPreviewController::BeginPaint(const bool cached) {
    ++m_paints;const auto old=m_yaw;
    if(!cached) {m_yaw=AdvanceYaw(old);}
    return old;
}
}
