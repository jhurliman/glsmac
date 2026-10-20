#pragma once
#include <cstdint>
namespace gse::tests::unit_admission_fixture {
// Actual original-executable results from live-object-name-stack-admission-cases.json.
// Fixed square22,38; ordinary view flags60010ee3. Editor, direct-viewer,
// special override/chassis and blink-phase controls retain separate contracts.
struct Case {
    const char* id;
    bool live,explored,reveal,unit,base,stack,selected,control,pending;
    uint8_t display,owner,seen;
    uint16_t flags;
    uint8_t order,tag,second_owner;
    int expected;
};
inline constexpr Case Cases[]={
    {"unit_unknown_owner3_seen0",true,false,false,true,false,false,false,true,true,3,3,0,0,0,15,3,-1},
    {"base_unknown_owner3_seen0",true,false,false,false,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"unit_unknown_owner3_seen8",true,false,false,true,false,false,false,true,true,3,3,8,0,0,15,3,-1},
    {"base_unknown_owner3_seen8",true,false,false,false,true,false,false,true,true,3,3,8,0,0,3,3,-1},
    {"unit_unknown_owner2_seen0",true,false,false,true,false,false,false,true,true,3,2,0,0,0,15,2,-1},
    {"base_unknown_owner2_seen0",true,false,false,false,true,false,false,true,true,3,2,0,0,0,2,2,-1},
    {"unit_unknown_owner2_seen8",true,false,false,true,false,false,false,true,true,3,2,8,0,0,15,2,-1},
    {"base_unknown_owner2_seen8",true,false,false,false,true,false,false,true,true,3,2,8,0,0,2,2,-1},
    {"unit_explored_owner3_seen0",true,true,false,true,false,false,false,true,true,3,3,0,0,0,15,3,0},
    {"base_explored_owner3_seen0",true,true,false,false,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"unit_explored_owner3_seen8",true,true,false,true,false,false,false,true,true,3,3,8,0,0,15,3,0},
    {"base_explored_owner3_seen8",true,true,false,false,true,false,false,true,true,3,3,8,0,0,3,3,-1},
    {"unit_explored_owner2_seen0",true,true,false,true,false,false,false,true,true,3,2,0,0,0,15,2,-1},
    {"base_explored_owner2_seen0",true,true,false,false,true,false,false,true,true,3,2,0,0,0,2,2,-1},
    {"unit_explored_owner2_seen8",true,true,false,true,false,false,false,true,true,3,2,8,0,0,15,2,0},
    {"base_explored_owner2_seen8",true,true,false,false,true,false,false,true,true,3,2,8,0,0,2,2,-1},
    {"unit_survey_owner3_seen0",true,false,false,true,false,false,false,true,true,3,3,0,0,0,15,3,-1},
    {"base_survey_owner3_seen0",true,false,false,false,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"unit_survey_owner3_seen8",true,false,false,true,false,false,false,true,true,3,3,8,0,0,15,3,-1},
    {"base_survey_owner3_seen8",true,false,false,false,true,false,false,true,true,3,3,8,0,0,3,3,-1},
    {"unit_survey_owner2_seen0",true,false,false,true,false,false,false,true,true,3,2,0,0,0,15,2,-1},
    {"base_survey_owner2_seen0",true,false,false,false,true,false,false,true,true,3,2,0,0,0,2,2,-1},
    {"unit_survey_owner2_seen8",true,false,false,true,false,false,false,true,true,3,2,8,0,0,15,2,-1},
    {"base_survey_owner2_seen8",true,false,false,false,true,false,false,true,true,3,2,8,0,0,2,2,-1},
    {"unit_reveal_owner3_seen0",true,false,true,true,false,false,false,true,true,3,3,0,0,0,15,3,0},
    {"base_reveal_owner3_seen0",true,false,true,false,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"unit_reveal_owner3_seen8",true,false,true,true,false,false,false,true,true,3,3,8,0,0,15,3,0},
    {"base_reveal_owner3_seen8",true,false,true,false,true,false,false,true,true,3,3,8,0,0,3,3,-1},
    {"unit_reveal_owner2_seen0",true,false,true,true,false,false,false,true,true,3,2,0,0,0,15,2,-1},
    {"base_reveal_owner2_seen0",true,false,true,false,true,false,false,true,true,3,2,0,0,0,2,2,-1},
    {"unit_reveal_owner2_seen8",true,false,true,true,false,false,false,true,true,3,2,8,0,0,15,2,0},
    {"base_reveal_owner2_seen8",true,false,true,false,true,false,false,true,true,3,2,8,0,0,2,2,-1},
    {"own_flags0x0_seen0",true,true,false,true,false,false,false,true,true,3,3,0,0,0,15,3,0},
    {"own_flags0x0_seen8",true,true,false,true,false,false,false,true,true,3,3,8,0,0,15,3,0},
    {"own_flags0x40_seen0",true,true,false,true,false,false,false,true,true,3,3,0,64,0,15,3,0},
    {"own_flags0x40_seen8",true,true,false,true,false,false,false,true,true,3,3,8,64,0,15,3,0},
    {"own_flags0x400_seen0",true,true,false,true,false,false,false,true,true,3,3,0,1024,0,15,3,0},
    {"own_flags0x400_seen8",true,true,false,true,false,false,false,true,true,3,3,8,1024,0,15,3,0},
    {"own_flags0x440_seen0",true,true,false,true,false,false,false,true,true,3,3,0,1088,0,15,3,-1},
    {"own_flags0x440_seen8",true,true,false,true,false,false,false,true,true,3,3,8,1088,0,15,3,0},
    {"own_flags0x441_seen0",true,true,false,true,false,false,false,true,true,3,3,0,1089,0,15,3,-1},
    {"own_flags0x441_seen8",true,true,false,true,false,false,false,true,true,3,3,8,1089,0,15,3,0},
    {"selected_visible",true,true,false,true,false,false,true,true,true,3,3,0,0,0,15,3,0},
    {"selected_on_base_order2",true,true,false,true,true,false,true,true,true,3,3,0,0,2,3,3,-1},
    {"unselected_on_base",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"base_invalid_tag",true,true,false,true,true,false,false,true,true,3,3,0,0,0,15,3,0},
    {"stack_domain_priority",true,true,false,true,false,true,false,true,true,3,3,0,0,0,15,3,1},
    {"stack_selected_head",true,true,false,true,false,true,true,true,true,3,3,0,0,0,15,3,0},
    {"stack_foreign_head_own_tail",true,true,false,true,false,true,false,true,true,3,2,8,0,0,15,3,1},
    {"stack_hidden_foreign_head",true,true,false,true,false,true,false,true,true,3,2,0,0,0,15,3,-1},
    {"startup_gate_0_0_False",false,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"startup_gate_0_1_False",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"startup_gate_0_0_True",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"startup_gate_1_0_False",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"first_base_roster_zero",true,true,false,false,false,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"first_base_name_pref_off",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"first_base_name_zoom_minus11",true,true,false,true,true,false,false,true,true,3,3,0,0,0,3,3,-1},
    {"startup_scout_gate_0_0",false,true,false,true,false,false,false,true,true,3,3,0,0,0,15,3,-1},
    {"startup_scout_gate_0_3",true,true,false,true,false,false,false,true,true,3,3,0,0,0,15,3,0},
    {"startup_scout_gate_1_0",true,true,false,true,false,false,false,true,true,3,3,0,0,0,15,3,0},
};
}
