#include "SmacCampaignRules.h"
#include <stdexcept>

namespace game::backend::settings {

void SmacCampaignRules::BeginOrdinary() {
    Validate();
    // Explicit GLSMAC fresh ordinary setup authority, separate from either
    // original rule-choice operation. Remembered custom preferences survive.
    primary&=0xffff;secondary&=RANDOMIZATION;objective_facility=0;
    ordinary_new_game=true;menu_choice=-1;
}
void SmacCampaignRules::SelectStandard() {
    Validate();primary=(primary&0xffff0000u)|STANDARD;menu_choice=0;
}
void SmacCampaignRules::SelectCurrent(bool citizen) {
    Validate();primary=(primary&0xffff0000u)|(remembered_primary&0xffff);
    if(citizen) {primary|=0x0200;}
    secondary=(secondary&~RANDOMIZATION)|remembered_secondary;menu_choice=1;
}
bool SmacCampaignRules::IsOrdinary() const {
    return ordinary_new_game && objective_facility==0 && !(primary&0x60400000u) && !(secondary&~RANDOMIZATION);
}
void SmacCampaignRules::Validate() const {
    if(menu_choice < -1 || menu_choice>1 || (remembered_secondary&~RANDOMIZATION)
        || objective_facility<0 || objective_facility>71) {throw std::runtime_error("Unsupported SMAC campaign preference/objective state");}
}
bool SmacCampaignRules::operator==(const SmacCampaignRules& other) const {
    return primary==other.primary && secondary==other.secondary && remembered_primary==other.remembered_primary
        && remembered_secondary==other.remembered_secondary && menu_choice==other.menu_choice
        && ordinary_new_game==other.ordinary_new_game && objective_facility==other.objective_facility;
}
std::string SmacCampaignRules::Serialize() const {
    Validate();types::Buffer wire;wire.WriteInt(1);wire.WriteInt(primary);wire.WriteInt(secondary);
    wire.WriteInt(remembered_primary);wire.WriteInt(remembered_secondary);wire.WriteInt(menu_choice);
    wire.WriteBool(ordinary_new_game);wire.WriteInt(objective_facility);return wire.ToString();
}
SmacCampaignRules SmacCampaignRules::Deserialize(const std::string& bytes) {
    if(bytes.size()>MAX_WIRE_BYTES) {throw std::runtime_error("SMAC campaign rules exceed wire budget");}
    types::Buffer wire(bytes);
    if(wire.ReadInt()!=1) {throw std::runtime_error("Unsupported SMAC campaign rules format");}
    const auto word=[&]() {const auto value=wire.ReadInt();if(value<0 || value>UINT32_MAX) {throw std::runtime_error("SMAC campaign word exceeds unsigned32");}return static_cast<uint32_t>(value);};
    SmacCampaignRules result;result.primary=word();result.secondary=word();result.remembered_primary=word();result.remembered_secondary=word();
    const auto menu=wire.ReadInt();result.ordinary_new_game=wire.ReadBool();const auto objective=wire.ReadInt();
    if(menu < -1 || menu>1 || objective<0 || objective>71) {throw std::runtime_error("SMAC campaign selection/objective exceeds supported range");}
    result.menu_choice=static_cast<int32_t>(menu);result.objective_facility=static_cast<int32_t>(objective);
    result.Validate();if(wire.lenr!=wire.lenw) {throw std::runtime_error("Unexpected SMAC campaign rules fields");}return result;
}
}
