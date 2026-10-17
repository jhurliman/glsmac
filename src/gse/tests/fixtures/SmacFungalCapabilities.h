#pragma once

#include <array>
#include <cstdint>
#include <vector>

// Generated from pinned original outputs by tools/reference/project_fungal_cases.py.
// FUNGAL SHA256 b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e
// FIRST-WORKER SHA256 84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab
// Excluded: 009/011/023/024 require unadmitted mineral/energy faction tags; 014 is native faction0.
namespace gse::tests::smac_fungal_fixture {
struct Technology { int32_t id; uint16_t flags; bool owned; std::array< int32_t, 2 > prerequisites; };
struct Modifier { bool nutrient; int32_t amount; };
struct Case { const char* id; int32_t simulation; std::vector< Technology > technologies; std::vector< Modifier > modifiers;
	int32_t planet, economy; bool increase, decrease, dust, solar; std::array< int32_t, 3 > capabilities, yields; };
inline const std::array< Case, 20 >& Cases() {
	static const std::array< Case, 20 > values = {{
		{ "FUNGAL-001", 1, {  }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-002", 1, { { 6, 256, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 0, 0 }, { 1, 0, 0 } },
		{ "FUNGAL-003", 1, { { 6, 256, true, { 0, 0 } } }, { { true, 1 } }, 1, 0, false, false, false, false, { 2, 0, 0 }, { 2, 0, 0 } },
		{ "FUNGAL-004", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-005", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, { { true, 1 } }, 1, 0, false, false, false, false, { 3, 3, 3 }, { 3, 3, 3 } },
		{ "FUNGAL-006", 1, { { 0, 256, false, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-007", 1, { { 0, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 1, 1 }, { 1, 1, 1 } },
		{ "FUNGAL-008", 1, { { 0, 256, true, { 0, 0 } }, { 1, 256, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 2, 0, 0 }, { 2, 0, 0 } },
		{ "FUNGAL-010", 1, { { 0, 256, true, { 0, 0 } } }, { { true, -3 } }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-012", 1, {  }, { { false, 9 } }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-013", 7, { { 0, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 1, 1 }, { 1, 1, 1 } },
		{ "FUNGAL-015", 1, { { 88, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-016", 1, { { 0, 256, true, { -2, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-017", 1, { { 0, 256, true, { 0, -2 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-018", 1, { { 0, 256, true, { -1, -2 } } }, {  }, 0, 0, false, false, false, false, { 1, 0, 0 }, { 1, 0, 0 } },
		{ "FUNGAL-019", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 0, 5, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-020", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, -1, 0, false, false, false, false, { 2, 3, 3 }, { 1, 2, 2 } },
		{ "FUNGAL-021", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, -7, 0, false, false, false, false, { 2, 3, 3 }, { 0, 0, 0 } },
		{ "FUNGAL-022", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 5, 0, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-025", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 0, 0, true, true, true, true, { 2, 3, 3 }, { 2, 3, 6 } },
	}}; return values;
}
struct Worker { const char* faction; std::vector< int32_t > owned; std::array< int32_t, 3 > capabilities; };
inline const std::array< Worker, 7 >& Workers() {
	static const std::array< Worker, 7 > values = {{
		{ "GAIANS", { 6 }, { 2, 0, 0 } },
		{ "HIVE", { 43 }, { 0, 0, 0 } },
		{ "UNIVERSITY", { 0, 2 }, { 0, 0, 0 } },
		{ "MORGANITES", { 1 }, { 0, 0, 0 } },
		{ "SPARTANS", { 5 }, { 0, 0, 0 } },
		{ "BELIEVERS", { 4 }, { 0, 0, 0 } },
		{ "PEACEKEEPERS", { 0 }, { 0, 0, 0 } },
	}}; return values;
}
}
