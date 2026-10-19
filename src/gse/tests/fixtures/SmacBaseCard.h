#pragma once
#include <array>
namespace gse::tests::base_card_fixture {
// Literal numeric observations from lower-console-base-card-cases.json.
struct Case {const char* name;bool roster;std::array<int,2> offset,size,anchor;std::array<int,4> destination;};
inline constexpr Case cases[] = {
{"selected_land_pop1",false,{{27,17}},{{41,38}},{{34,66}},{{54,78,84,106}}},
{"roster_land_pop1",true,{{27,17}},{{41,38}},{{156,197}},{{169,205,189,224}}},
{"selected_land_pop9",false,{{6,8}},{{87,58}},{{34,66}},{{38,72,103,115}}},
{"roster_land_pop9",true,{{6,8}},{{87,58}},{{156,197}},{{159,201,202,230}}},
{"selected_land_pop10",false,{{6,8}},{{87,58}},{{34,66}},{{38,72,103,115}}},
{"roster_land_pop10",true,{{6,8}},{{87,58}},{{156,197}},{{159,201,202,230}}},
{"selected_sea_pop1",false,{{18,17}},{{75,57}},{{34,66}},{{47,78,103,120}}},
{"roster_sea_pop1",true,{{18,17}},{{75,57}},{{156,197}},{{165,205,202,233}}},
{"selected_sea_pop9",false,{{0,8}},{{97,67}},{{34,66}},{{34,72,106,122}}},
{"roster_sea_pop9",true,{{0,8}},{{97,67}},{{156,197}},{{156,201,204,234}}},
{"selected_sea_pop10",false,{{0,8}},{{97,67}},{{34,66}},{{34,72,106,122}}},
{"roster_sea_pop10",true,{{0,8}},{{97,67}},{{156,197}},{{156,201,204,234}}},
{"selected_long_name_production",false,{{6,8}},{{87,58}},{{34,66}},{{38,72,103,115}}},
};
}
