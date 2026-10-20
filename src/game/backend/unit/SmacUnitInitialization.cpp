#include "SmacUnitInitialization.h"
#include <stdexcept>

namespace game::backend::unit {
SmacNativeUnitRecord InitializeSmacUnitRecord(int32_t identity,int32_t design,int32_t owner,int32_t morale) {
    if(identity<0 || identity>=2048 || design<0 || design>=512 || owner<0 || owner>7) {throw std::runtime_error("Invalid native Unit initialization inputs");}
    SmacNativeUnitRecord result;result.id=uint16_t(identity);result.design=int16_t(design);result.owner=uint8_t(owner);
    result.stored_morale=uint8_t(uint32_t(morale)+1u);result.Validate();return result;
}
OriginalUnitBytes OriginalSmacUnitBytes(const SmacNativeUnitRecord& r) noexcept {
    OriginalUnitBytes bytes{};
    const auto word=[&](size_t off,uint32_t value,unsigned count=2){for(unsigned k=0;k<count;++k) {bytes[off+k]=uint8_t(value>>(8*k));}};
    word(0,uint16_t(r.x));word(2,uint16_t(r.y));word(4,r.state,4);word(8,r.flags);word(10,uint16_t(r.design));word(12,r.aux_word);
    bytes[14]=r.owner;bytes[15]=r.aux_owner_control;bytes[16]=r.damage;bytes[17]=r.order;bytes[18]=r.automation_control;bytes[19]=r.waypoint_count;
    for(size_t i=0;i<4;++i) {word(20+2*i,uint16_t(r.waypoint_x[i]));word(28+2*i,uint16_t(r.waypoint_y[i]));}
    bytes[36]=r.stored_morale;bytes[37]=r.terraforming_progress;bytes[38]=r.ecology_owner_tag;bytes[39]=r.observation_mask;
    bytes[40]=r.movement_spent;bytes[41]=r.facing;bytes[42]=r.movement_iteration_control;bytes[43]=r.status_character;
    bytes[44]=r.probe_action_control;bytes[45]=r.probe_target_control;
    word(46,uint16_t(r.home_base));word(48,uint16_t(r.stack_next));word(50,uint16_t(r.stack_previous));return bytes;
}
}
