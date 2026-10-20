#pragma once
#include "SmacEcology.h"
namespace game::backend::base {
inline int32_t CalculateSmacTerraformingSquarePoints(SmacEcologySquare tile,bool tree_farm,bool hybrid_forest) {
    auto counted=tile.improvements&511u;int32_t points=0;
    while(counted) {counted&=counted-1;++points;}
    if(tile.worked) {points*=2;}
    if(tile.improvements&SmacEcologySquare::Borehole) {points+=8;}
    if(tile.improvements&SmacEcologySquare::EchelonMirror) {points+=6;}
    if(tile.improvements&SmacEcologySquare::Condenser) {points+=4;}
    if(tile.improvements&SmacEcologySquare::Forest) {--points;}
    if(tree_farm && hybrid_forest) {points=0;}
    else if(tree_farm || hybrid_forest) {points/=2;}
    return points;
}
}
