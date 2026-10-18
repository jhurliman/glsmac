#pragma once
#include <vector>
namespace gse::tests::smac_perihelion_fixture {
struct Case {const char* id;int32_t start,turn;bool active,maintenance;std::vector<bool> states;std::vector<int> transitions;};
inline const std::vector<Case> cases{
{"PERI-001",2100,89,false,true,{false},{0}},
{"PERI-002",2100,90,false,true,{true},{1}},
{"PERI-003",2100,91,false,true,{false},{0}},
{"PERI-004",2100,109,true,true,{true},{0}},
{"PERI-005",2100,110,true,true,{false},{2}},
{"PERI-006",2100,111,true,true,{true},{0}},
{"PERI-007",2100,170,false,true,{true},{1}},
{"PERI-008",2100,190,true,true,{false},{2}},
{"PERI-009",2100,90,false,true,{true},{1}},
{"PERI-010",2100,110,true,true,{false},{2}},
{"PERI-011",2100,90,false,false,{false},{0}},
{"PERI-012",2100,110,true,false,{true},{0}},
{"PERI-013",2140,50,false,true,{false},{0}},
{"PERI-014",2139,51,false,true,{true},{1}},
{"PERI-015",2160,50,true,true,{false},{2}},
{"PERI-016",2100,90,true,true,{true,true},{1,1}},
{"PERI-017",2100,110,true,true,{false,false},{2,0}},
{"PERI-018",2100,90,false,true,{true},{1}},
{"PERI-019",2000,110,false,true,{true},{1}},
{"PERI-020",2100,110,false,true,{false},{0}},
};
}
