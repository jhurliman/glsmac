#pragma once
namespace gse::tests::workshop_data_fixture {
// Native DATA receipts; freeze92d5a04d299620a805bf710345bc6fdc6e50079dbf8e095c6e3888958dc47d07.
struct Movement {game::backend::unit::SmacWorkshopDraft draft;int speed,triad,mode,expected;};
inline const Movement Cases[]{
    {{2,0,0,0,0,1,{}},1,0,0,1},
    {{2,0,14,0,0,1,{}},1,0,8,1},
    {{2,5,0,0,32,2,{}},8,2,0,9},
    {{2,0,0,0,3,1,{}},1,0,0,1},
    {{2,0,0,0,3,1,{}},1,0,0,1},
    {{2,0,0,0,2,1,{}},1,0,0,1},
    {{2,0,12,9,0,2,{}},1,0,2,1},
    {{2,0,0,0,0,1,{}},1,0,0,1},
    {{2,0,0,0,0,1,{}},1,0,0,1},
    {{2,0,0,0,0,1,{}},1,0,0,1},
    {{2,0,0,0,0,1,{}},1,0,0,1},
};
}
