#pragma once
namespace gse::tests::workshop_picker_fixture {
// Declared editable inputs and native results; saved pools remain private.
// Freeze SHA256:99b731728c802634308707bcd522e1705f4519e84c7c0e00452d99a23a53b16d.
using Editor=game::backend::unit::SmacWorkshopEditor;
struct Case {const char* name;Editor before,after;int kind,choice,slot;bool automatic,second,admitted;};
inline const Case Cases[]{
    {"same_domain_keeps_reverse_slots",{{64,0,1,2,262656,1,"Retained Draft"},{18,9}},{{64,1,1,2,262656,1,"Retained Draft"},{18,9}},0,1,0,true,true,true},
    {"air_from_hand_suggestion",{{64,0,0,0,0,1,"Retained Draft"},{-1,-1}},{{64,5,0,0,2,1,"Retained Draft"},{1,-1}},0,5,0,true,true,true},
    {"air_from_probe_before_reset",{{64,0,18,0,0,1,"Retained Draft"},{-1,-1}},{{64,5,0,0,2,1,"Retained Draft"},{1,-1}},0,5,0,true,true,true},
    {"air_to_land_removes_radar",{{64,5,1,0,2,1,"Retained Draft"},{1,-1}},{{64,0,1,0,0,1,"Retained Draft"},{-1,-1}},0,0,0,true,true,true},
    {"enter_missile_payload_and_clear",{{64,0,1,2,262144,1,"Retained Draft"},{18,-1}},{{64,8,20,0,0,1,"Retained Draft"},{-1,-1}},0,8,0,true,true,true},
    {"leave_missile_resets_payload",{{64,8,20,0,0,1,"Retained Draft"},{-1,-1}},{{64,1,0,0,0,1,"Retained Draft"},{-1,-1}},0,1,0,true,true,true},
    {"former_empty_mask_suggestions",{{64,0,1,0,0,1,"Retained Draft"},{-1,-1}},{{64,0,15,0,8193,1,"Retained Draft"},{0,13}},1,15,0,true,true,true},
    {"former_dirty_mask_skips_suggestions",{{64,0,1,0,512,1,"Retained Draft"},{9,-1}},{{64,0,15,0,512,1,"Retained Draft"},{9,-1}},1,15,0,true,true,true},
    {"air_probe_rejected",{{64,5,1,0,0,1,"Retained Draft"},{-1,-1}},{{64,5,1,0,0,1,"Retained Draft"},{-1,-1}},1,18,0,true,true,true},
    {"armor_same_value_suggests",{{64,0,0,1,0,1,"Retained Draft"},{-1,-1}},{{64,0,0,1,512,1,"Retained Draft"},{9,-1}},2,1,0,true,true,true},
    {"reactor_cleanup_missing_second_tech",{{64,0,0,0,262656,1,"Retained Draft"},{18,9}},{{64,0,0,0,262144,4,"Retained Draft"},{18,-1}},3,4,0,true,false,true},
    {"ability_none_preserves_second_slot",{{64,0,0,0,262656,1,"Retained Draft"},{9,18}},{{64,0,0,0,262144,1,"Retained Draft"},{-1,18}},4,24,0,true,true,true},
    {"ability_manual_reverse_order",{{64,0,0,0,512,1,"Retained Draft"},{-1,9}},{{64,0,0,0,262656,1,"Retained Draft"},{18,9}},4,18,0,true,true,true},
    {"former_incompatible_mask_cleared_without_suggestions",{{64,0,1,0,131072,1,"Retained Draft"},{17,-1}},{{64,0,15,0,0,1,"Retained Draft"},{-1,-1}},1,15,0,true,true,true},
    {"former_automatic_setting_off",{{64,0,1,0,0,1,"Retained Draft"},{-1,-1}},{{64,0,15,0,0,1,"Retained Draft"},{-1,-1}},1,15,0,false,true,true},
    {"second_without_technology",{{64,0,0,1,512,1,"Existing Guard"},{9,-1}},{{64,0,0,1,512,1,"Existing Guard"},{9,-1}},4,24,1,true,false,false},
    {"missile_ability_disabled",{{64,8,0,1,512,1,"Existing Guard"},{9,-1}},{{64,8,0,1,512,1,"Existing Guard"},{9,-1}},4,24,0,true,true,false},
};
}
