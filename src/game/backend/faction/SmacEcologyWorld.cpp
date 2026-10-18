#include "SmacEcologyWorld.h"
#include <stdexcept>

namespace game::backend::faction {
void SmacEcologyWorld::Validate() const {
    if(native_life<0 || native_life>2 || cloud_cover<0 || cloud_cover>2) {
        throw std::invalid_argument("Ecology world requires admitted native-life and cloud categories0..2");
    }
}
SmacPerihelionUpdate AdvanceSmacPerihelion(bool active,int32_t turn,int32_t mission_year,bool maintenance) {
    using Transition=SmacPerihelionTransition;
    if(turn<0) {throw std::invalid_argument("Perihelion requires a nonnegative turn");}
    if(!maintenance) {return {active,Transition::None};}
    const auto ending=int64_t(mission_year)-2210;
    const auto starting=int64_t(mission_year)-2190;
    if(ending<INT32_MIN || starting<INT32_MIN) {throw std::overflow_error("Perihelion calendar subtraction exceeds measured signed32");}
    if(active && ending%80==0) {return {false,Transition::Ends};}
    if(turn>50 && starting%80==0) {return {true,Transition::Starts};}
    return {active,Transition::None};
}
}
