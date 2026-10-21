#pragma once
namespace gse::tests::native_registry_fixture {
// Whole marked 512-record pool output digests from original installed startup.
// Freeze1193c2fcb18655d2c010233e63e05319c2bb5069b48eeb8d5abd4361b2fc200b.
struct Case {int owner,difficulty,scout;uint64_t pool;};
inline constexpr Case CASES[]{
{1,0,2,UINT64_C(0x857f7bc6c43b43a4)},
{1,1,2,UINT64_C(0x857f7bc6c43b43a4)},
{1,3,2,UINT64_C(0x857f7bc6c43b43a4)},
{1,5,2,UINT64_C(0x857f7bc6c43b43a4)},
{2,0,2,UINT64_C(0x857f7bc6c43b43a4)},
{2,1,2,UINT64_C(0x857f7bc6c43b43a4)},
{2,3,2,UINT64_C(0x857f7bc6c43b43a4)},
{2,5,2,UINT64_C(0x857f7bc6c43b43a4)},
{3,0,2,UINT64_C(0x857f7bc6c43b43a4)},
{3,1,2,UINT64_C(0x857f7bc6c43b43a4)},
{3,3,2,UINT64_C(0x857f7bc6c43b43a4)},
{3,5,2,UINT64_C(0x857f7bc6c43b43a4)},
{4,0,2,UINT64_C(0xcbd9df1386a0bb82)},
{4,1,2,UINT64_C(0xcbd9df1386a0bb82)},
{4,3,2,UINT64_C(0xcbd9df1386a0bb82)},
{4,5,2,UINT64_C(0xcbd9df1386a0bb82)},
{5,0,320,UINT64_C(0x10a0819135756967)},
{5,1,320,UINT64_C(0x10a0819135756967)},
{5,3,320,UINT64_C(0x10a0819135756967)},
{5,5,320,UINT64_C(0x10a0819135756967)},
{6,0,2,UINT64_C(0x857f7bc6c43b43a4)},
{6,1,2,UINT64_C(0x857f7bc6c43b43a4)},
{6,3,2,UINT64_C(0x857f7bc6c43b43a4)},
{6,5,2,UINT64_C(0x857f7bc6c43b43a4)},
{7,0,2,UINT64_C(0x857f7bc6c43b43a4)},
{7,1,2,UINT64_C(0x857f7bc6c43b43a4)},
{7,3,2,UINT64_C(0x857f7bc6c43b43a4)},
{7,5,2,UINT64_C(0x857f7bc6c43b43a4)},
};
}
