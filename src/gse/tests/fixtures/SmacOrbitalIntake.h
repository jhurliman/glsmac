// Literal ORBIT-INTAKE-001 semantic outputs; SHA256 fc9445afaa71bbfe9b91b9ed74fb01d2e64efe0c16f88bf8eb7a425a973eb6b7
#pragma once
#include <array>
#include <cstdint>
namespace gse::tests::fixtures {
struct OrbitalIntakeCase { const char* id; int32_t population; std::array< int32_t, 3 > counts; bool aerospace, owned_elevator; std::array< int32_t, 3 > expected; };
inline const std::array< OrbitalIntakeCase, 16 > ORBITAL_INTAKE = {{
    { "ORBIT-INTAKE-001", 4, { 0, 0, 0 }, false, false, { 0, 0, 0 } },
    { "ORBIT-INTAKE-002", 4, { 1, 1, 1 }, false, false, { 1, 1, 1 } },
    { "ORBIT-INTAKE-003", 20, { 3, 5, 7 }, false, false, { 2, 3, 4 } },
    { "ORBIT-INTAKE-004", 20, { 2, 4, 6 }, false, false, { 1, 2, 3 } },
    { "ORBIT-INTAKE-005", 20, { 2, 4, 6 }, true, false, { 2, 4, 6 } },
    { "ORBIT-INTAKE-006", 20, { 2, 4, 6 }, false, true, { 2, 4, 6 } },
    { "ORBIT-INTAKE-007", 20, { 2, 4, 6 }, false, true, { 2, 4, 6 } },
    { "ORBIT-INTAKE-008", 20, { 2, 4, 6 }, false, false, { 1, 2, 3 } },
    { "ORBIT-INTAKE-009", 20, { 2, 4, 6 }, true, true, { 2, 4, 6 } },
    { "ORBIT-INTAKE-010", 4, { 7, 8, 9 }, false, false, { 4, 4, 4 } },
    { "ORBIT-INTAKE-011", 4, { 7, 8, 9 }, true, false, { 4, 4, 4 } },
    { "ORBIT-INTAKE-012", 0, { 7, 8, 9 }, false, false, { 0, 0, 0 } },
    { "ORBIT-INTAKE-013", 4, { 0, 1, 20 }, false, false, { 0, 1, 4 } },
    { "ORBIT-INTAKE-014", 4, { 5, 5, 5 }, true, false, { 4, 4, 4 } },
    { "ORBIT-INTAKE-015", 4, { -1, -2, -3 }, false, false, { 0, 0, -1 } },
    { "ORBIT-INTAKE-016", 4, { -1, -2, -3 }, true, false, { -1, -2, -3 } },
}};
}
