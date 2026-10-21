#pragma once
namespace gse::tests::workshop_ability_fixture {
// Native chooser receipt; freeze SHA256:c8fcf42783b664a811caaa65adb320c7411a27ff37f45384b98454073e3e5474.
struct Row {unsigned slot;bool alltech;std::array<int32_t,2> slots;std::vector<uint32_t> values;};
inline const Row Rows[]{
    {0,true,{9,18},{24,1,2,3,4,5,8,9,10,11,12,14,15,16,17,20,22}},
    {1,true,{9,18},{24,1,2,3,4,5,8,10,11,12,14,15,16,17,18,20,22}},
    {0,false,{-1,-1},{24}},
};
struct Cost {int code;const char* text;};
inline const Cost Costs[]{
    {-7,"Cost: 2 (Defense+Speed-2)"},
    {-6,"Cost: 1 (Attack+Speed-2)"},
    {-5,"Cost: 3 (Attack+Defense-2)"},
    {-4,"Cost: 0 (Speed-1)"},
    {-3,"Cost: 2 (Defense-1)"},
    {-2,"Cost: 1 (Attack-1)"},
    {-1,"Cost: 0 (Attack/Defense)"},
    {0,"Cost: 1 (Land)"},
    {1,"Cost: 2 (Land)"},
};
}
