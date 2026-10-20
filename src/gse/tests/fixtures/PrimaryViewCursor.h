#pragma once
#include <array>
namespace gse::tests::fixtures::primary_cursor {
struct Case {const char* id;std::array<int,2> cursor;std::array<int,6> view;};
inline const Case Cases[]={
    {"zero_units_two_bases",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"zero_units_no_bases",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_ready",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_ready_selected1",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_ready_distant",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_exhausted",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_sentry",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"foreign_units_only",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"zero_units_invalid_cursor",{{-1,38}},{{0,1,1610682083,0,40,40}}},
    {"two_ready_invalid_cursor",{{80,38}},{{0,1,1610682083,0,40,40}}},
    {"zero_units_parity_cursor",{{23,38}},{{0,1,1610682083,0,23,38}}},
    {"no_control_zero_units",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_exhausted_selected1",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"two_sentry_selected1",{{22,38}},{{0,1,1610682083,0,22,38}}},
    {"cursor_already_on_unit",{{20,40}},{{0,1,1610682083,0,20,40}}},
    {"negative_saved_zoom",{{22,38}},{{0,1,1610682083,0,22,38}}},
};
}
