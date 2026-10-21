#pragma once
#include <cstdint>
namespace gse::tests::workshop_strip_fixture {
// Complete native planes and clipped targets; original buffers stay private.
// Freeze SHA256: 7fc13d4a01cad80d397a0e0ed446cdf7fcecc9e521e1933aa3d89621f21a10ff.
struct Frame {const char* name;int chassis,weapon,armor,reactor;uint64_t color,depth,shadow,target_27f,target_37f;};
inline constexpr Frame FRAMES[]{
    {"scout",0,0,0,1,UINT64_C(0xcedf160659c773ac),UINT64_C(0x9d8f148f7c5eedde),UINT64_C(0xea40d1530e785925),UINT64_C(0x286a1443cc0f5f0e),UINT64_C(0x1b799905cf0269f5)},
    {"rover",1,0,0,1,UINT64_C(0x8588b15336694e10),UINT64_C(0x6d9f675f029d1839),UINT64_C(0x4611c87ea61245a5),UINT64_C(0x69c1f0a99f80a0c6),UINT64_C(0x69c1f0a99f80a0c6)},
    {"foil",3,1,1,1,UINT64_C(0x93c6c2e2a3363292),UINT64_C(0x6c66de9749c8e59e),UINT64_C(0x75a97eab6d06d225),UINT64_C(0x2c9387aa767b75f5),UINT64_C(0x2c9387aa767b75f5)},
    {"needlejet",5,1,1,1,UINT64_C(0x8143405e15879550),UINT64_C(0xeff7fdc5b688ae65),UINT64_C(0x540caf3ec8f745a5),UINT64_C(0x13a619c1e06f40c2),UINT64_C(0x13a619c1e06f40c2)},
};
}
