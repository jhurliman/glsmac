#include "HumanResearchValue.h"
#include "game/backend/faction/SmacResearchValue.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
namespace {
using game::backend::faction::SmacHumanResearchInputs;
using game::backend::faction::CalculateSmacHumanResearchValue;
// RE-SMAC-BLIND-RESEARCH-001, 107 original-code results, 17 retained contexts.
// Numeric source receipt SHA256: 11caf72dda2944f074a3051463aa57bbfacef4ac18058b08bbdc778b06acbf69
// Source receipt ratings are G/T/W/P; project them into catalog P/T/W/G.
SmacHumanResearchInputs Inputs( size_t context ) {
    SmacHumanResearchInputs v{};
    v.facility_prerequisites.fill( -2 );
    v.faction_id = 1;
    v.blind = true;
    v.technologies[0] = {{0,3,2,2},{-1,-1},32u,0u};
    v.technologies[1] = {{2,1,3,0},{-1,-1},0u,0u};
    v.technologies[2] = {{0,3,2,1},{-1,-1},0u,0u};
    v.technologies[3] = {{4,2,1,0},{-1,-1},0u,0u};
    v.technologies[4] = {{0,1,3,2},{-1,-1},0u,0u};
    v.technologies[5] = {{2,0,0,3},{-1,-1},0u,0u};
    v.technologies[6] = {{0,1,2,3},{-1,-1},256u,2u};
    v.technologies[7] = {{4,2,0,0},{72,1},0u,0u};
    v.technologies[8] = {{4,3,0,0},{3,2},0u,0u};
    v.technologies[9] = {{1,3,2,0},{7,12},0u,0u};
    v.technologies[10] = {{3,4,3,1},{30,7},0u,0u};
    v.technologies[11] = {{3,0,4,2},{12,46},0u,0u};
    v.technologies[12] = {{2,3,2,0},{13,22},0u,0u};
    v.technologies[13] = {{3,1,2,0},{1,3},0u,0u};
    v.technologies[14] = {{1,3,2,0},{20,73},0u,0u};
    v.technologies[15] = {{1,1,0,3},{29,35},0u,0u};
    v.technologies[16] = {{3,2,0,1},{8,53},0u,0u};
    v.technologies[17] = {{3,0,1,2},{36,72},0u,0u};
    v.technologies[18] = {{1,1,5,0},{16,11},0u,0u};
    v.technologies[19] = {{3,1,2,0},{15,42},0u,0u};
    v.technologies[20] = {{4,3,0,1},{18,9},0u,0u};
    v.technologies[21] = {{3,1,0,3},{69,41},0u,0u};
    v.technologies[22] = {{2,3,1,0},{1,2},2u,0u};
    v.technologies[23] = {{3,1,0,4},{21,33},0u,0u};
    v.technologies[24] = {{3,2,0,2},{-2,-2},0u,0u};
    v.technologies[25] = {{3,4,3,0},{14,61},0u,0u};
    v.technologies[26] = {{1,4,3,0},{65,34},0u,0u};
    v.technologies[27] = {{4,1,2,0},{26,23},0u,0u};
    v.technologies[28] = {{0,1,3,2},{54,57},64u,0u};
    v.technologies[29] = {{1,1,3,2},{40,30},0u,0u};
    v.technologies[30] = {{2,4,3,2},{17,53},2u,0u};
    v.technologies[31] = {{1,0,5,2},{19,66},0u,0u};
    v.technologies[32] = {{0,4,3,1},{2,-1},0u,0u};
    v.technologies[33] = {{0,4,3,2},{73,41},2u,0u};
    v.technologies[34] = {{0,4,3,3},{66,33},2u,0u};
    v.technologies[35] = {{2,0,0,4},{36,46},0u,0u};
    v.technologies[36] = {{2,0,1,4},{5,-1},0u,0u};
    v.technologies[37] = {{0,1,3,4},{44,43},0u,0u};
    v.technologies[38] = {{1,0,2,4},{13,49},0u,0u};
    v.technologies[39] = {{3,0,3,4},{38,36},0u,0u};
    v.technologies[40] = {{3,2,2,0},{9,11},0u,0u};
    v.technologies[41] = {{4,0,2,2},{39,52},2u,0u};
    v.technologies[42] = {{1,0,4,3},{18,68},0u,0u};
    v.technologies[43] = {{3,0,2,2},{5,4},0u,0u};
    v.technologies[44] = {{0,1,3,3},{4,-1},0u,0u};
    v.technologies[45] = {{0,0,5,2},{1,-1},4u,0u};
    v.technologies[46] = {{0,1,4,3},{45,32},4u,0u};
    v.technologies[47] = {{0,0,2,4},{60,58},64u,0u};
    v.technologies[48] = {{1,5,0,3},{4,0},1u,0u};
    v.technologies[49] = {{0,2,4,3},{0,44},32u,0u};
    v.technologies[50] = {{0,2,3,2},{49,52},32u,0u};
    v.technologies[51] = {{3,1,4,1},{41,76},32u,0u};
    v.technologies[52] = {{3,1,1,1},{48,46},0u,0u};
    v.technologies[53] = {{1,3,4,0},{32,37},0u,0u};
    v.technologies[54] = {{0,0,3,4},{75,55},0u,0u};
    v.technologies[55] = {{0,3,1,4},{67,63},0u,0u};
    v.technologies[56] = {{0,1,3,4},{65,28},128u,0u};
    v.technologies[57] = {{1,0,3,2},{71,64},128u,0u};
    v.technologies[58] = {{0,1,0,6},{48,6},0u,0u};
    v.technologies[59] = {{0,0,4,3},{45,60},4u,0u};
    v.technologies[60] = {{0,0,3,4},{6,49},0u,0u};
    v.technologies[61] = {{0,0,4,3},{59,37},4u,0u};
    v.technologies[62] = {{0,0,4,2},{10,59},0u,0u};
    v.technologies[63] = {{0,1,1,6},{74,62},256u,0u};
    v.technologies[64] = {{0,4,0,2},{63,75},73u,0u};
    v.technologies[65] = {{1,4,1,0},{20,55},1u,0u};
    v.technologies[66] = {{2,4,2,3},{77,68},0u,0u};
    v.technologies[67] = {{3,2,1,4},{51,35},0u,0u};
    v.technologies[68] = {{3,1,2,0},{10,38},0u,0u};
    v.technologies[69] = {{3,1,4,0},{25,15},0u,0u};
    v.technologies[70] = {{0,0,5,0},{-2,-2},0u,0u};
    v.technologies[71] = {{1,2,3,1},{34,31},32u,0u};
    v.technologies[72] = {{2,4,1,0},{3,22},0u,0u};
    v.technologies[73] = {{3,1,8,1},{42,46},4u,0u};
    v.technologies[74] = {{0,2,0,5},{47,76},128u,0u};
    v.technologies[75] = {{1,1,3,4},{61,33},4u,0u};
    v.technologies[76] = {{4,2,0,2},{50,17},48u,0u};
    v.technologies[77] = {{0,4,3,3},{39,30},0u,0u};
    v.technologies[78] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[79] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[80] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[81] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[82] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[83] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[84] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[85] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[86] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[87] = {{0,2,0,0},{-2,-2},0u,0u};
    v.technologies[88] = {{0,2,0,0},{56,27},0u,0u};
    v.facility_prerequisites[13] = 59;
    v.facility_prerequisites[14] = 61;
    v.facility_prerequisites[24] = 46;
    v.facility_prerequisites[25] = 31;
    v.facility_prerequisites[31] = 47;
    v.facility_prerequisites[32] = 64;
    v.facility_prerequisites[54] = 30;
    v.facility_prerequisites[58] = 55;
    v.facility_prerequisites[71] = 56;
    v.reactor_prerequisites = {10,25};
    v.former_prerequisite = 6;
    v.foil_prerequisite = 36;
    v.intrinsic_planet = 1;
    v.maximum_attack = 1;
    v.turn = 1;
    v.weapons[0] = {1,-1};
    v.weapons[1] = {2,3};
    v.weapons[2] = {4,8};
    v.weapons[3] = {5,7};
    v.weapons[4] = {6,38};
    v.weapons[5] = {8,16};
    v.weapons[6] = {10,68};
    v.weapons[7] = {12,20};
    v.weapons[8] = {13,66};
    v.weapons[9] = {16,69};
    v.weapons[10] = {20,23};
    v.weapons[11] = {24,27};
    v.weapons[12] = {-1,63};
    v.weapons[13] = {99,77};
    v.weapons[14] = {0,-1};
    v.weapons[15] = {0,6};
    v.weapons[16] = {0,36};
    v.weapons[17] = {0,46};
    v.weapons[18] = {0,32};
    v.weapons[19] = {0,-2};
    v.weapons[20] = {12,77};
    v.regional_known_tiles[1] = 21;
    v.regional_strategy[1] = 7;
    v.regional_strategy[2] = 8;
    v.regional_strategy[3] = 8;
    v.regional_strategy[4] = 8;
    v.regional_strategy[5] = 8;
    v.regional_strategy[6] = 8;
    v.regional_strategy[7] = 8;
    v.regional_strategy[8] = 8;
    v.regional_strategy[9] = 8;
    v.regional_strategy[10] = 8;
    v.regional_strategy[11] = 8;
    v.regional_strategy[12] = 8;
    v.regional_strategy[13] = 8;
    v.regional_strategy[14] = 8;
    v.regional_strategy[15] = 8;
    v.regional_strategy[16] = 8;
    v.regional_strategy[17] = 8;
    v.regional_strategy[18] = 8;
    v.regional_strategy[19] = 8;
    v.regional_strategy[20] = 8;
    v.regional_strategy[21] = 8;
    v.regional_strategy[22] = 8;
    v.regional_strategy[23] = 8;
    v.regional_strategy[24] = 8;
    v.regional_strategy[25] = 8;
    v.regional_strategy[26] = 8;
    v.regional_strategy[27] = 8;
    v.regional_strategy[28] = 8;
    v.regional_strategy[29] = 8;
    v.regional_strategy[30] = 8;
    v.regional_strategy[31] = 8;
    v.regional_strategy[32] = 8;
    v.regional_strategy[33] = 8;
    v.regional_strategy[34] = 8;
    v.regional_strategy[35] = 8;
    v.regional_strategy[36] = 8;
    v.regional_strategy[37] = 8;
    v.regional_strategy[38] = 8;
    v.regional_strategy[39] = 8;
    v.regional_strategy[40] = 8;
    v.regional_strategy[41] = 8;
    v.regional_strategy[42] = 8;
    v.regional_strategy[43] = 8;
    v.regional_strategy[44] = 8;
    v.regional_strategy[45] = 8;
    v.regional_strategy[46] = 8;
    v.regional_strategy[47] = 8;
    v.regional_strategy[48] = 8;
    v.regional_strategy[49] = 8;
    v.regional_strategy[50] = 8;
    v.regional_strategy[51] = 8;
    v.regional_strategy[52] = 8;
    v.regional_strategy[53] = 8;
    v.regional_strategy[54] = 8;
    v.regional_strategy[55] = 8;
    v.regional_strategy[56] = 8;
    v.regional_strategy[57] = 8;
    v.regional_strategy[58] = 8;
    v.regional_strategy[59] = 8;
    v.regional_strategy[60] = 8;
    v.regional_strategy[61] = 8;
    v.regional_strategy[62] = 8;
    v.regional_strategy[63] = 8;
    v.regional_strategy[64] = 8;
    v.regional_strategy[65] = 7;
    v.regional_strategy[66] = 7;
    v.regional_strategy[67] = 7;
    v.regional_strategy[68] = 7;
    v.regional_strategy[69] = 7;
    v.regional_strategy[70] = 7;
    v.regional_strategy[71] = 7;
    v.regional_strategy[72] = 7;
    v.regional_strategy[73] = 7;
    v.regional_strategy[74] = 7;
    v.regional_strategy[75] = 7;
    v.regional_strategy[76] = 7;
    v.regional_strategy[77] = 7;
    v.regional_strategy[78] = 7;
    v.regional_strategy[79] = 7;
    v.regional_strategy[80] = 7;
    v.regional_strategy[81] = 7;
    v.regional_strategy[82] = 7;
    v.regional_strategy[83] = 7;
    v.regional_strategy[84] = 7;
    v.regional_strategy[85] = 7;
    v.regional_strategy[86] = 7;
    v.regional_strategy[87] = 7;
    v.regional_strategy[88] = 7;
    v.regional_strategy[89] = 7;
    v.regional_strategy[90] = 7;
    v.regional_strategy[91] = 7;
    v.regional_strategy[92] = 7;
    v.regional_strategy[93] = 7;
    v.regional_strategy[94] = 7;
    v.regional_strategy[95] = 7;
    v.regional_strategy[96] = 7;
    v.regional_strategy[97] = 7;
    v.regional_strategy[98] = 7;
    v.regional_strategy[99] = 7;
    v.regional_strategy[100] = 7;
    v.regional_strategy[101] = 7;
    v.regional_strategy[102] = 7;
    v.regional_strategy[103] = 7;
    v.regional_strategy[104] = 7;
    v.regional_strategy[105] = 7;
    v.regional_strategy[106] = 7;
    v.regional_strategy[107] = 7;
    v.regional_strategy[108] = 7;
    v.regional_strategy[109] = 7;
    v.regional_strategy[110] = 7;
    v.regional_strategy[111] = 7;
    v.regional_strategy[112] = 7;
    v.regional_strategy[113] = 7;
    v.regional_strategy[114] = 7;
    v.regional_strategy[115] = 7;
    v.regional_strategy[116] = 7;
    v.regional_strategy[117] = 7;
    v.regional_strategy[118] = 7;
    v.regional_strategy[119] = 7;
    v.regional_strategy[120] = 7;
    v.regional_strategy[121] = 7;
    v.regional_strategy[122] = 7;
    v.regional_strategy[123] = 7;
    v.regional_strategy[124] = 7;
    v.regional_strategy[125] = 7;
    v.regional_strategy[126] = 7;
    v.regional_strategy[127] = 8;
    v.region_area[1] = 1024;
    v.region_area[63] = 64;
    v.regional_bases[1][1] = 1;
    switch ( context ) {
    case 0: break;
    case 1:
        v.regional_strategy[1] = 2;
        v.regional_bases[1][1] = 3;
        break;
    case 2:
        v.maximum_defense = 2;
        v.regional_strategy[1] = 0;
        break;
    case 3:
        v.maximum_attack = 2;
        v.maximum_defense = 2;
        v.regional_strategy[1] = 0;
        break;
    case 4:
        v.relations[2] = 8;
        v.regional_strategy[1] = 3;
        v.regional_bases[2][1] = 1;
        break;
    case 5:
        v.relations[2] = 9;
        v.regional_strategy[1] = 3;
        v.regional_bases[2][1] = 1;
        break;
    case 6:
        v.relations[2] = 41;
        v.regional_strategy[1] = 3;
        v.regional_bases[2][1] = 1;
        break;
    case 7:
        v.intrinsic_probe = 1;
        break;
    case 8:
        v.psi_bonus = 10;
        break;
    case 9:
        v.ecology_aggregate = 3;
        break;
    case 10:
        v.ecology_aggregate = 4;
        break;
    case 11:
        v.population_bonus = 3;
        break;
    case 12:
        v.population_bonus = 3;
        v.turn = 250;
        break;
    case 13:
        v.population_bonus = 3;
        v.turn = 251;
        break;
    case 14:
        v.regional_known_tiles[1] = 1023;
        break;
    case 15:
        v.regional_known_tiles[1] = 1024;
        break;
    case 16:
        v.regional_bases[2][1] = 1;
        v.regional_bases[3][1] = 1;
        v.regional_bases[4][1] = 1;
        v.regional_bases[5][1] = 1;
        v.regional_bases[6][1] = 1;
        v.regional_bases[7][1] = 1;
        break;
    default: throw std::invalid_argument( "Unknown original research context" );
    }
    return v;
}
struct Case { const char* id; size_t context; int32_t technology; std::array<int32_t,4> priorities; int32_t bases; bool endgame; int32_t expected; };
const Case cases[]{
    {"INSTALLED-VALUE-00",0,0,{1,1,1,1},1,false,1260},
    {"INSTALLED-VALUE-01",0,1,{1,1,1,1},1,false,363},
    {"INSTALLED-VALUE-02",0,2,{1,1,1,1},1,false,183},
    {"INSTALLED-VALUE-03",0,3,{1,1,1,1},1,false,852},
    {"INSTALLED-VALUE-04",0,4,{1,1,1,1},1,false,1083},
    {"INSTALLED-VALUE-05",0,5,{1,1,1,1},1,false,406},
    {"INSTALLED-VALUE-06",0,6,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-07",0,7,{1,1,1,1},1,false,124},
    {"INSTALLED-VALUE-08",0,8,{1,1,1,1},1,false,420},
    {"INSTALLED-VALUE-09",0,9,{1,1,1,1},1,false,90},
    {"INSTALLED-VALUE-10",0,10,{1,1,1,1},1,false,663},
    {"INSTALLED-VALUE-11",0,11,{1,1,1,1},1,false,45},
    {"INSTALLED-VALUE-12",0,12,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-13",0,13,{1,1,1,1},1,false,90},
    {"INSTALLED-VALUE-14",0,14,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-15",0,15,{1,1,1,1},1,false,75},
    {"INSTALLED-VALUE-16",0,16,{1,1,1,1},1,false,120},
    {"INSTALLED-VALUE-17",0,17,{1,1,1,1},1,false,61},
    {"INSTALLED-VALUE-18",0,18,{1,1,1,1},1,false,105},
    {"INSTALLED-VALUE-19",0,19,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-20",0,20,{1,1,1,1},1,false,480},
    {"INSTALLED-VALUE-21",0,21,{1,1,1,1},1,false,105},
    {"INSTALLED-VALUE-22",0,22,{1,1,1,1},1,false,61},
    {"INSTALLED-VALUE-23",0,23,{1,1,1,1},1,false,480},
    {"INSTALLED-VALUE-24",0,24,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-25",0,25,{1,1,1,1},1,false,300},
    {"INSTALLED-VALUE-26",0,26,{1,1,1,1},1,false,120},
    {"INSTALLED-VALUE-27",0,27,{1,1,1,1},1,false,140},
    {"INSTALLED-VALUE-28",0,28,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-29",0,29,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-30",0,30,{1,1,1,1},1,false,999},
    {"INSTALLED-VALUE-31",0,31,{1,1,1,1},1,false,40},
    {"INSTALLED-VALUE-32",0,32,{1,1,1,1},1,false,161},
    {"INSTALLED-VALUE-33",0,33,{1,1,1,1},1,false,270},
    {"INSTALLED-VALUE-34",0,34,{1,1,1,1},1,false,50},
    {"INSTALLED-VALUE-35",0,35,{1,1,1,1},1,false,60},
    {"INSTALLED-VALUE-36",0,36,{1,1,1,1},1,false,286},
    {"INSTALLED-VALUE-37",0,37,{1,1,1,1},1,false,321},
    {"INSTALLED-VALUE-38",0,38,{1,1,1,1},1,false,420},
    {"INSTALLED-VALUE-39",0,39,{1,1,1,1},1,false,450},
    {"INSTALLED-VALUE-40",0,40,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-41",0,41,{1,1,1,1},1,false,80},
    {"INSTALLED-VALUE-42",0,42,{1,1,1,1},1,false,40},
    {"INSTALLED-VALUE-43",0,43,{1,1,1,1},1,false,141},
    {"INSTALLED-VALUE-44",0,44,{1,1,1,1},1,false,841},
    {"INSTALLED-VALUE-45",0,45,{1,1,1,1},1,false,280},
    {"INSTALLED-VALUE-46",0,46,{1,1,1,1},1,false,80},
    {"INSTALLED-VALUE-47",0,47,{1,1,1,1},1,false,180},
    {"INSTALLED-VALUE-48",0,48,{1,1,1,1},1,false,405},
    {"INSTALLED-VALUE-49",0,49,{1,1,1,1},1,false,1620},
    {"INSTALLED-VALUE-50",0,50,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-51",0,51,{1,1,1,1},1,false,45},
    {"INSTALLED-VALUE-52",0,52,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-53",0,53,{1,1,1,1},1,false,243},
    {"INSTALLED-VALUE-54",0,54,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-55",0,55,{1,1,1,1},1,false,120},
    {"INSTALLED-VALUE-56",0,56,{1,1,1,1},1,false,40},
    {"INSTALLED-VALUE-57",0,57,{1,1,1,1},1,false,30},
    {"INSTALLED-VALUE-58",0,58,{1,1,1,1},1,false,105},
    {"INSTALLED-VALUE-59",0,59,{1,1,1,1},1,false,280},
    {"INSTALLED-VALUE-60",0,60,{1,1,1,1},1,false,420},
    {"INSTALLED-VALUE-61",0,61,{1,1,1,1},1,false,140},
    {"INSTALLED-VALUE-62",0,62,{1,1,1,1},1,false,180},
    {"INSTALLED-VALUE-63",0,63,{1,1,1,1},1,false,960},
    {"INSTALLED-VALUE-64",0,64,{1,1,1,1},1,false,180},
    {"INSTALLED-VALUE-65",0,65,{1,1,1,1},1,false,90},
    {"INSTALLED-VALUE-66",0,66,{1,1,1,1},1,false,220},
    {"INSTALLED-VALUE-67",0,67,{1,1,1,1},1,false,50},
    {"INSTALLED-VALUE-68",0,68,{1,1,1,1},1,false,360},
    {"INSTALLED-VALUE-69",0,69,{1,1,1,1},1,false,160},
    {"INSTALLED-VALUE-70",0,70,{1,1,1,1},1,false,25},
    {"INSTALLED-VALUE-71",0,71,{1,1,1,1},1,false,35},
    {"INSTALLED-VALUE-72",0,72,{1,1,1,1},1,false,213},
    {"INSTALLED-VALUE-73",0,73,{1,1,1,1},1,false,130},
    {"INSTALLED-VALUE-74",0,74,{1,1,1,1},1,false,210},
    {"INSTALLED-VALUE-75",0,75,{1,1,1,1},1,false,270},
    {"INSTALLED-VALUE-76",0,76,{1,1,1,1},1,false,80},
    {"INSTALLED-VALUE-77",0,77,{1,1,1,1},1,false,2400},
    {"INSTALLED-VALUE-78",0,78,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-79",0,79,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-80",0,80,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-81",0,81,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-82",0,82,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-83",0,83,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-84",0,84,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-85",0,85,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-86",0,86,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-87",0,87,{1,1,1,1},1,false,10},
    {"INSTALLED-VALUE-88",0,88,{1,1,1,1},1,false,10},
    {"regional_type2",1,3,{1,1,1,1},2,false,1140},
    {"regional_type0_attack_lower",2,3,{1,1,1,1},1,false,1044},
    {"regional_type0_attack_equal",3,3,{1,1,1,1},1,false,948},
    {"regional_other_relation",4,3,{1,1,1,1},1,false,900},
    {"regional_other_truce",5,3,{1,1,1,1},1,false,852},
    {"regional_other_truce_exception",6,3,{1,1,1,1},1,false,900},
    {"probe_positive",7,30,{1,1,1,1},1,false,504},
    {"psi_positive",8,55,{1,1,1,1},1,false,240},
    {"ecology_ratio_equal",9,59,{1,1,1,1},2,false,280},
    {"ecology_ratio_above",10,59,{1,1,1,1},2,false,282},
    {"population_hab_complex",11,46,{1,1,1,1},1,false,160},
    {"population_hab_dome_turn250",12,31,{1,1,1,1},1,false,40},
    {"population_hab_dome_turn251",13,31,{1,1,1,1},1,false,60},
    {"foil_explored_below_area",14,36,{1,1,1,1},1,false,286},
    {"foil_explored_equal_area",15,36,{1,1,1,1},1,false,1696},
    {"foil_all_factions_present",16,36,{1,1,1,1},1,false,141},
    {"endgame_ascent",0,56,{1,1,1,1},1,true,160},
    {"endgame_unrelated",0,0,{1,1,1,1},1,true,1260},
};
std::string CheckHumanResearchValues() {
    for ( const auto& c : cases ) {
        auto input = Inputs( c.context ); input.priorities = c.priorities; input.base_count = c.bases; input.endgame_pressure = c.endgame;
        const auto actual = CalculateSmacHumanResearchValue( input, c.technology );
        if ( actual != c.expected ) { return std::string(c.id) + ": " + std::to_string(actual) + " != " + std::to_string(c.expected); }
    }
    // Twelve additional full native queries isolate the two review questions.
    // See docs/behavior/human-research-review-discriminators-cases.json.
    SmacHumanResearchInputs controlled{};
    controlled.faction_id = 1;
    controlled.blind = true;
    controlled.cloud_cover = 1;
    controlled.facility_prerequisites.fill( -2 );
    controlled.reactor_prerequisites.fill( -2 );
    controlled.former_prerequisite = controlled.foil_prerequisite = -2;
    for ( auto& row : controlled.technologies ) { row.prerequisites = {-2,-2}; }
    controlled.technologies[0].prerequisites = {-1,-1};
    controlled.technologies[0].ratings = {8,0,0,0};
    for ( const uint32_t relation : {0u,8u,16u,24u} ) {
        controlled.relations[2] = relation;
        const auto expected = relation & 16u ? 8 : 4;
        const auto actual = CalculateSmacHumanResearchValue( controlled, 0 );
        if ( actual != expected ) { return "native weighted Vendetta discriminator: " + std::to_string(relation); }
    }
    controlled.relations[2] = 0;
    for ( int32_t id = 1; id <= 4; ++id ) { controlled.technologies[id].prerequisites = {id-1,-1}; }
    for ( const int32_t edges : {3,4} ) {
        controlled.facility_prerequisites[54] = edges;
        for ( const int32_t discover : {0,1,2,-1} ) {
            controlled.priorities[1] = discover;
            const auto expected = edges == 3 && discover != 0 ? 16 : 4;
            const auto actual = CalculateSmacHumanResearchValue( controlled, 0 );
            if ( actual != expected ) { return "native Hunter-Seeker depth discriminator: edges=" + std::to_string(edges) + " Discover=" + std::to_string(discover); }
        }
    }
    return {};
}
}
void AddHumanResearchValueTests( task::gsetests::GSETests* task ) {
    task->AddTest( "SMAC human research value matches 119 complete original queries", GT() {
        try { return CheckHumanResearchValues(); } catch ( const std::exception& error ) { GT_FAIL( error.what() ); }
    } );
}
}
