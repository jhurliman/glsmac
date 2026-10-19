#pragma once
#include "game/frontend/widget/SmacMinimapGeometry.h"
namespace gse::tests::minimap_navigation_fixture {
using Point=game::frontend::widget::smac_minimap::Point;
struct Case {bool reverse,flat; Point window,input; std::optional<Point> expected;};
// Literal original-execution mapping results, independent of the port algorithm.
inline const Case cases[]={
    {false,false,{{239,119}},{{0,0}},Point{{0,0}}},
    {false,false,{{239,119}},{{1,0}},Point{{0,0}}},
    {false,false,{{239,119}},{{119,59}},Point{{39,39}}},
    {false,false,{{239,119}},{{238,118}},Point{{79,79}}},
    {false,false,{{239,119}},{{239,118}},std::nullopt},
    {false,false,{{239,119}},{{238,119}},std::nullopt},
    {false,false,{{239,119}},{{-1,0}},Point{{0,0}}},
    {false,false,{{239,119}},{{0,-1}},Point{{0,0}}},
    {false,false,{{239,119}},{{17,31}},Point{{5,21}}},
    {true,false,{{239,119}},{{0,0}},Point{{0,0}}},
    {true,false,{{239,119}},{{1,1}},Point{{2,1}}},
    {true,false,{{239,119}},{{40,40}},Point{{119,59}}},
    {true,false,{{239,119}},{{79,79}},Point{{236,117}}},
    {true,false,{{239,119}},{{-1,40}},Point{{236,59}}},
    {true,false,{{239,119}},{{80,40}},Point{{0,59}}},
    {true,false,{{239,119}},{{81,40}},Point{{2,59}}},
    {true,false,{{239,119}},{{-81,40}},std::nullopt},
    {true,false,{{239,119}},{{40,-1}},Point{{119,0}}},
    {true,false,{{239,119}},{{40,80}},Point{{119,117}}},
    {false,false,{{400,200}},{{39,24}},std::nullopt},
    {false,false,{{400,200}},{{40,24}},Point{{0,0}}},
    {false,false,{{400,200}},{{359,175}},Point{{79,79}}},
    {false,false,{{400,200}},{{360,175}},std::nullopt},
    {false,false,{{400,200}},{{359,176}},std::nullopt},
    {false,false,{{400,200}},{{40,23}},std::nullopt},
    {false,false,{{400,200}},{{200,100}},Point{{40,40}}},
    {true,false,{{400,200}},{{0,0}},Point{{40,24}}},
    {true,false,{{400,200}},{{40,40}},Point{{200,100}}},
    {true,false,{{400,200}},{{79,79}},Point{{356,174}}},
};
}
