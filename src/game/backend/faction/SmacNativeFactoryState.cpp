#include "SmacNativeFactoryState.h"
#include <stdexcept>

namespace game::backend::faction {
SmacNativeFactoryState SmacNativeFactoryState::Initialize(int32_t morale,SmacGameplayRandomState& random) noexcept {
    // Original initialized-native control: eight CRT calls, no shared calls.
    // Localization/presentation is separate from this minimal factory context.
    for(int i=0;i<8;++i) {(void)random.NextCrtRaw15();}
    return {morale,{{-1,-1}},0,{}};
}
void SmacNativeFactoryState::Adjust(int32_t design,uint8_t plan,int32_t delta) {
    if(design<0 || design>=512 || (delta!=1 && delta!=-1)) {
        throw std::invalid_argument("Native count adjustment requires a catalog design and unit delta");
    }
    if(plan<=7) {active_plan_tally+=static_cast<uint32_t>(delta);}
    active_designs[design]=static_cast<uint8_t>(int32_t(active_designs[design])+delta);
}
bool SmacNativeFactoryState::operator==(const SmacNativeFactoryState& other) const noexcept {
    return definition_morale==other.definition_morale && assignment_reference==other.assignment_reference
        && active_plan_tally==other.active_plan_tally && active_designs==other.active_designs;
}
types::Buffer SmacNativeFactoryState::Serialize() const {
    types::Buffer wire;wire.WriteInt(1);wire.WriteInt(definition_morale);
    for(const auto value:assignment_reference) {wire.WriteInt(value);}
    wire.WriteInt(active_plan_tally);
    std::string counts;counts.reserve(active_designs.size());
    for(const auto value:active_designs) {counts+=static_cast<char>(value);}
    wire.WriteString(counts);return wire;
}
SmacNativeFactoryState SmacNativeFactoryState::Deserialize(const std::string& bytes) {
    if(bytes.size()>MAX_WIRE_BYTES) {throw std::runtime_error("Native factory state exceeds wire budget");}
    types::Buffer wire(bytes);
    if(wire.ReadInt()!=1) {throw std::runtime_error("Unsupported native factory state");}
    const auto signed32=[&] {
        const auto value=wire.ReadInt();
        if(value<INT32_MIN || value>INT32_MAX) {throw std::runtime_error("Invalid native factory signed word");}
        return static_cast<int32_t>(value);
    };
    const auto morale=signed32();const auto x=signed32();const auto y=signed32();
    const auto tally=wire.ReadInt();
    if(tally<0 || tally>UINT32_MAX) {throw std::runtime_error("Invalid native active-plan tally");}
    const auto counts=wire.ReadString(512);
    if(counts.size()!=512 || wire.lenr!=wire.lenw) {throw std::runtime_error("Invalid native active-design state");}
    SmacNativeFactoryState result{morale,{{x,y}},static_cast<uint32_t>(tally),{}};
    for(size_t i=0;i<counts.size();++i) {result.active_designs[i]=static_cast<uint8_t>(counts[i]);}
    return result;
}
}
