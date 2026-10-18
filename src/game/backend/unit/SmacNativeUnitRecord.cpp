#include "SmacNativeUnitRecord.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace game::backend::unit {
namespace {
template<typename T> T Read(types::Buffer& input) {
    const auto value=input.ReadInt();
    if (value<std::numeric_limits<T>::min() || value>std::numeric_limits<T>::max())
        throw std::runtime_error("Native Unit scalar exceeds its original field domain");
    return static_cast<T>(value);
}
}

SmacNativeUnitRecord SmacNativeUnitRecord::InitializeMindWorm(int32_t identity, int32_t native_direct_morale) {
    return InitializeWild(identity,8,native_direct_morale);
}

SmacNativeUnitRecord SmacNativeUnitRecord::InitializeWild(int32_t identity, int32_t design, int32_t native_direct_morale) {
    if (identity<0 || identity>=2048 || design<8 || design>10) throw std::runtime_error("Native Unit pool or wild design invalid");
    SmacNativeUnitRecord result; result.id=static_cast<uint16_t>(identity); result.design=static_cast<int16_t>(design);
    // Native direct scalar plus1, stored through the original byte domain.
    result.stored_morale=static_cast<uint8_t>(static_cast<uint32_t>(native_direct_morale)+1u);
    result.Validate(); return result;
}

SmacNativeUnitRecord SmacNativeUnitRecord::AfterUnlinkedIndexErasure(int32_t removed) const {
    Validate();
    if(removed<0 || removed>=2048 || id==removed || stack_next==removed || stack_previous==removed) {
        throw std::invalid_argument("Dense Unit erasure requires an already unlinked survivor");
    }
    auto result=*this;
    if(result.id>removed) {--result.id;}
    if(result.stack_next>removed) {--result.stack_next;}
    if(result.stack_previous>removed) {--result.stack_previous;}
    if(result.order==1) {
        if(result.waypoint_x[0]==removed) {result.order=0;}
        else if(result.waypoint_x[0]>removed) {--result.waypoint_x[0];}
    }
    return result;
}

void SmacNativeUnitRecord::Validate() const {
    const bool unplaced=x==-4 && y==-4;
    if (id>=2048 || design<0 || design>=512 || owner>7 || ecology_owner_tag>7 || waypoint_count>4
        || (!unplaced && (x<0 || y<0 || ((x^y)&1))) || home_base < -1
        || stack_next < -1 || stack_next>=2048 || stack_previous < -1 || stack_previous>=2048)
        throw std::runtime_error("Invalid native Unit identity, coordinate or reference");
}

bool SmacNativeUnitRecord::operator==(const SmacNativeUnitRecord& other) const noexcept {
    return std::tie(id,x,y,state,flags,design,aux_word,owner,aux_owner_control,damage,order,automation_control,waypoint_count,stored_morale,terraforming_progress,ecology_owner_tag,observation_mask,movement_spent,facing,movement_iteration_control,status_character,probe_action_control,probe_target_control,home_base,stack_next,stack_previous,waypoint_x,waypoint_y)
        == std::tie(other.id,other.x,other.y,other.state,other.flags,other.design,other.aux_word,other.owner,other.aux_owner_control,other.damage,other.order,other.automation_control,other.waypoint_count,other.stored_morale,other.terraforming_progress,other.ecology_owner_tag,other.observation_mask,other.movement_spent,other.facing,other.movement_iteration_control,other.status_character,other.probe_action_control,other.probe_target_control,other.home_base,other.stack_next,other.stack_previous,other.waypoint_x,other.waypoint_y);
}

types::Buffer SmacNativeUnitRecord::Serialize() const {
    Validate(); types::Buffer output; output.WriteInt(1);
    output.WriteInt(id);
    output.WriteInt(x);
    output.WriteInt(y);
    output.WriteInt(state);
    output.WriteInt(flags);
    output.WriteInt(design);
    output.WriteInt(aux_word);
    output.WriteInt(owner);
    output.WriteInt(aux_owner_control);
    output.WriteInt(damage);
    output.WriteInt(order);
    output.WriteInt(automation_control);
    output.WriteInt(waypoint_count);
    output.WriteInt(stored_morale);
    output.WriteInt(terraforming_progress);
    output.WriteInt(ecology_owner_tag);
    output.WriteInt(observation_mask);
    output.WriteInt(movement_spent);
    output.WriteInt(facing);
    output.WriteInt(movement_iteration_control);
    output.WriteInt(status_character);
    output.WriteInt(probe_action_control);
    output.WriteInt(probe_target_control);
    output.WriteInt(home_base);
    output.WriteInt(stack_next);
    output.WriteInt(stack_previous);
    for (const auto value:waypoint_x) output.WriteInt(value);
    for (const auto value:waypoint_y) output.WriteInt(value);
    return output;
}

SmacNativeUnitRecord SmacNativeUnitRecord::Deserialize(const std::string& wire) {
    if (wire.size()>MAX_WIRE_BYTES) throw std::runtime_error("Native Unit record exceeds wire budget");
    types::Buffer input(wire);
    if (input.ReadInt()!=1) throw std::runtime_error("Unsupported native Unit record version");
    SmacNativeUnitRecord result;
    result.id=Read<uint16_t>(input);
    result.x=Read<int16_t>(input);
    result.y=Read<int16_t>(input);
    result.state=Read<uint32_t>(input);
    result.flags=Read<uint16_t>(input);
    result.design=Read<int16_t>(input);
    result.aux_word=Read<uint16_t>(input);
    result.owner=Read<uint8_t>(input);
    result.aux_owner_control=Read<uint8_t>(input);
    result.damage=Read<uint8_t>(input);
    result.order=Read<uint8_t>(input);
    result.automation_control=Read<uint8_t>(input);
    result.waypoint_count=Read<uint8_t>(input);
    result.stored_morale=Read<uint8_t>(input);
    result.terraforming_progress=Read<uint8_t>(input);
    result.ecology_owner_tag=Read<uint8_t>(input);
    result.observation_mask=Read<uint8_t>(input);
    result.movement_spent=Read<uint8_t>(input);
    result.facing=Read<uint8_t>(input);
    result.movement_iteration_control=Read<uint8_t>(input);
    result.status_character=Read<uint8_t>(input);
    result.probe_action_control=Read<uint8_t>(input);
    result.probe_target_control=Read<uint8_t>(input);
    result.home_base=Read<int16_t>(input);
    result.stack_next=Read<int16_t>(input);
    result.stack_previous=Read<int16_t>(input);
    for (auto& value:result.waypoint_x) value=Read<int16_t>(input);
    for (auto& value:result.waypoint_y) value=Read<int16_t>(input);
    if (input.lenr!=input.lenw) throw std::runtime_error("Trailing native Unit record fields");
    result.Validate(); return result;
}

int32_t SmacWildMindWormLevel(const SmacNativeUnitRecord& record, int32_t global_turn,
    int32_t opposing_faction, int32_t opposing_major_atrocities) {
    if(record.design!=8) {throw std::runtime_error("Mind Worm level requires design8");}
    return SmacWildNativeLevel(record,global_turn,opposing_faction,opposing_major_atrocities);
}

int32_t SmacWildNativeLevel(const SmacNativeUnitRecord& record, int32_t global_turn,
    int32_t opposing_faction, int32_t opposing_major_atrocities) {
    record.Validate();
    if (record.owner!=0 || record.design<8 || record.design>10 || global_turn<0
        || opposing_faction<0 || opposing_faction>7)
        throw std::runtime_error("Native level requires measured wild native context");
    const int32_t base=global_turn<45 ? 0 : global_turn<90 ? 1 : global_turn<170 ? 2
        : global_turn<250 ? 3 : global_turn<330 ? 4 : 6;
    // Original515dd3..515df6: state bit8 and flags bits3..4 are additive;
    // unrelated carried controls (including sight flags0x440) do not select level.
    return std::min(base+int32_t(record.design==10)+int32_t(opposing_faction>0 && opposing_major_atrocities!=0)
        +int32_t((record.state&0x100u)!=0)+int32_t((record.flags>>3)&3u),6);
}

}
