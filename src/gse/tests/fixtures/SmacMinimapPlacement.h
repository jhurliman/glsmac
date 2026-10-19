#pragma once
#include "game/frontend/widget/SmacMinimapGeometry.h"
namespace gse::tests::minimap_placement_fixture {
using Point=game::frontend::widget::smac_minimap::Point;
struct Case {Point tile; std::optional<Point> anchor; std::optional<std::array<int32_t,4>> bbox; int count;};
// Literal original-execution calls and isolated painted bounds/counts.
inline const Case cases[]={
    {{{1,1}},Point{{-4,-2}},std::array<int32_t,4>{{0,0,4,2}},6},
    {{{79,1}},Point{{308,-2}},std::array<int32_t,4>{{308,0,316,2}},12},
    {{{0,2}},Point{{312,0}},std::array<int32_t,4>{{312,1,319,4}},15},
    {{{2,2}},Point{{0,0}},std::array<int32_t,4>{{0,1,8,4}},16},
    {{{40,40}},Point{{152,76}},std::array<int32_t,4>{{152,77,160,80}},16},
    {{{39,77}},Point{{148,150}},std::nullopt,0},
    {{{40,76}},Point{{152,148}},std::array<int32_t,4>{{152,149,160,151}},12},
    {{{0,76}},Point{{312,148}},std::array<int32_t,4>{{312,149,319,151}},11},
    {{{1,77}},Point{{-4,150}},std::nullopt,0},
    {{{0,78}},Point{{312,152}},std::nullopt,0},
    {{{79,79}},Point{{-12,154}},std::nullopt,0},
    {{{0,0}},std::nullopt,std::nullopt,0},
    {{{78,0}},std::nullopt,std::nullopt,0},
};
}
