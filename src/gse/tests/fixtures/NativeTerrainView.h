#pragma once
namespace gse::tests::native_view_fixture {
struct Case {int x,y;bool wrap;size_t count;uint64_t hash;};
inline constexpr Case cases[]={
{40,40,true,288,17619678200068966259ULL}, // interior
{0,40,true,288,698538110693649952ULL}, // wrap_west
{78,40,true,288,7930600060297347911ULL}, // wrap_east
{40,0,true,265,12019092978303032112ULL}, // north_pole
{40,78,true,230,15107757782264432130ULL}, // south_pole
{0,40,false,238,6965018465417074832ULL}, // flat_west
};
}
