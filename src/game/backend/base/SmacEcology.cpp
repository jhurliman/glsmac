#include "SmacEcology.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("SMAC ecology exceeds measured signed32 arithmetic");}
    return static_cast<int32_t>(value);
}
int32_t FloorEighth(int32_t value) {
    const auto quotient=value/8;
    return quotient-(value<0 && value%8!=0 ? 1 : 0);
}
}

SmacTerraformingEcology CalculateSmacTerraformingEcology(const SmacWorkerRadiusMap& map,
    bool tree_farm,bool hybrid_forest,const std::function<SmacEcologySquare(int32_t,int32_t)>& query) {
    if(map.width<8 || !query) {throw std::invalid_argument("Ecology requires a nonaliasing radius and square query");}
    int32_t total=0;
    for(int32_t i=0;i<21;++i) {
        const auto coordinate=SmacWorkedSquareCollection::Resolve(map,i);
        if(!coordinate) {continue;}
        const auto tile=query((*coordinate)[0],(*coordinate)[1]);
        auto counted=tile.improvements&511u;int32_t points=0;
        while(counted) {counted&=counted-1;++points;}
        if(tile.worked) {points*=2;}
        if(tile.improvements&SmacEcologySquare::Borehole) {points+=8;}
        if(tile.improvements&SmacEcologySquare::EchelonMirror) {points+=6;}
        if(tile.improvements&SmacEcologySquare::Condenser) {points+=4;}
        if(tile.improvements&SmacEcologySquare::Forest) {--points;}
        if(tree_farm && hybrid_forest) {points=0;}
        else if(tree_farm || hybrid_forest) {points/=2;}
        total+=points;
    }
    return {total,FloorEighth(total)};
}

SmacEcologyBurden CalculateSmacEcologyBurden(const SmacEcologyBurdenInput& in) {
    if(in.history<0) {throw std::invalid_argument("Ecology requires nonnegative clean-mineral history");}
    const auto scaled=FloorEighth(in.raw_terraforming_points);
    auto residual=scaled;auto clean=Checked(int64_t(16)+in.history);
    if(residual>0) {const auto used=std::min(residual,clean);residual-=used;clean-=used;}
    const int32_t divisor=1+(in.nanoreplicator || in.singularity_inductor)+in.centauri_preserve+in.temple_of_planet+in.pholus_mutagen;
    const auto satellites=std::max(0,std::min(in.mining_satellites,in.population));
    const auto mineral_term=Checked(int64_t(in.processed_minerals)-satellites-clean)/divisor;
    return {scaled,residual,clean,divisor,satellites,Checked(int64_t(residual)+mineral_term)};
}

SmacEcologyDamage CalculateSmacEcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& in) {
    if(burden.divisor<1 || burden.divisor>5 || in.human_difficulty<0 || in.human_difficulty>5
        || in.global_difficulty<0 || in.global_difficulty>5 || in.native_life<0 || in.native_life>2 || in.major_atrocities<0) {
        throw std::invalid_argument("Ecology scaling exceeds the measured input domain");
    }
    auto after_human=burden.burden;
    if(in.human) {
        const auto denominator=std::clamp(in.sea_level_offset,0,100)/std::clamp(in.sea_level_rises_rule,1,100)+burden.divisor;
        after_human=Checked(int64_t(after_human)+Checked(int64_t(5)*in.major_atrocities)/denominator);
    }
    auto doubled=std::max(0,after_human);
    if(in.voice_of_planet_exists && in.transcendence_victory) {doubled=Checked(int64_t(doubled)*2);}
    if(in.perihelion) {doubled=Checked(int64_t(doubled)*2);}
    const auto difficulty=in.human ? (in.human_difficulty==0 ? 2 : in.human_difficulty<4 ? 3 : 5) : std::clamp(6-in.global_difficulty,1,3);
    auto product=Checked(int64_t(in.technology_score)-in.future_technology_counter);
    product=Checked(int64_t(product)*(3-std::clamp(in.planet,-3,2)));
    product=Checked(int64_t(product)*(in.native_life+1));
    product=Checked(int64_t(product)*doubled);
    product=Checked(int64_t(product)*difficulty);
    const auto sixth=product/6;
    return {after_human,doubled,difficulty,sixth,Checked(int64_t(sixth)+50)/100};
}

int32_t SmacEcologyResearchContribution(int32_t damage,int32_t current_planet) {
    return Checked(int64_t(4)*damage)/static_cast<int32_t>(std::clamp(int64_t(4)-current_planet,int64_t(1),int64_t(99)));
}
}
