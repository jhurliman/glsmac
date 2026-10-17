// Literal RE-SMAC-RESEARCH-SUBMIT-001 enabled wrapper projection (disabled caller bypass excluded).
// Source SHA256 ba718757b0480314892e30fbc6988b86f2a5f1a3ed6b251778214e58083ec9c1
#pragma once
#include <array>
namespace gse::tests::economy_fixture {
struct Submission { const char* name; int rating,turn,difficulty,labs,fraction; bool halved; std::array<int,5> increments,fractions; };
inline const Submission submissions[] = {
{"neutral",0,3,3,1,0,false,{1,1,1,1,1},{0,0,0,0,0}},
{"positive_fraction",2,3,3,1,0,false,{1,1,1,1,2},{20,40,60,80,0}},
{"negative_fraction",-2,10,3,1,0,false,{0,1,1,1,1},{80,60,40,20,0}},
{"positive_clamp",6,3,3,1,0,false,{1,2,1,2,1},{50,0,50,0,50}},
{"negative_clamp_before_gate",-6,29,3,1,0,false,{0,0,0,0,0},{0,0,0,0,0}},
{"negative_clamp_after_gate",-6,30,3,1,0,false,{0,1,0,1,0},{50,0,50,0,50}},
{"negative_gate_before",-2,9,3,1,0,false,{0,0,0,0,0},{0,0,0,0,0}},
{"easy_early_suppression",0,4,1,1,0,false,{0,0,0,0,0},{0,0,0,0,0}},
{"easy_turn_five",0,5,1,1,0,false,{1,1,1,1,1},{0,0,0,0,0}},
{"existing_fraction",2,3,3,1,90,false,{2,1,1,1,1},{10,30,50,70,90}},
{"punishment_sphere_before_scaling",2,3,3,3,0,true,{1,1,1,1,2},{20,40,60,80,0}},
{"genejack_does_not_halve_submission",2,3,3,3,0,false,{3,4,3,4,4},{60,20,80,40,0}},
};
}
