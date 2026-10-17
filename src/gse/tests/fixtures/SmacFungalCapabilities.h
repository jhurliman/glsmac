#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

// Generated from pinned original outputs by tools/reference/project_fungal_cases.py.
// FUNGAL SHA256 b33de468151434c5de12af575ae92e3c52b298026cd30e7ab3d0bca3ac211a4e
// FIRST-WORKER SHA256 84564f265e33142655a924becf08853c0567c8240347fac7dea5658ffd76bbab
// FUNGAL-PARSER SHA256 7387c3656e1decedabf8a14c7e72d19f27289df404595d62bf7e46b6751b023a
// Excluded: FUNGAL-014 is native faction0.
namespace gse::tests::smac_fungal_fixture {
struct Technology { int32_t id; uint16_t flags; bool owned; std::array< int32_t, 2 > prerequisites; };
struct Modifier { int32_t resource, amount; };
struct Case { const char* id; int32_t simulation; std::vector< Technology > technologies; std::vector< Modifier > modifiers;
	int32_t planet, economy; bool increase, decrease, dust, solar; std::array< int32_t, 3 > capabilities, yields; };
inline const std::array< Case, 24 >& Cases() {
	static const std::array< Case, 24 > values = {{
		{ "FUNGAL-001", 1, {  }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-002", 1, { { 6, 256, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 0, 0 }, { 1, 0, 0 } },
		{ "FUNGAL-003", 1, { { 6, 256, true, { 0, 0 } } }, { { 0, 1 } }, 1, 0, false, false, false, false, { 2, 0, 0 }, { 2, 0, 0 } },
		{ "FUNGAL-004", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-005", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, { { 0, 1 } }, 1, 0, false, false, false, false, { 3, 3, 3 }, { 3, 3, 3 } },
		{ "FUNGAL-006", 1, { { 0, 256, false, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-007", 1, { { 0, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 1, 1 }, { 1, 1, 1 } },
		{ "FUNGAL-008", 1, { { 0, 256, true, { 0, 0 } }, { 1, 256, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 2, 0, 0 }, { 2, 0, 0 } },
		{ "FUNGAL-009", 1, {  }, { { 0, 2 }, { 0, -1 }, { 1, 3 } }, 0, 0, false, false, false, false, { 1, 3, 0 }, { 1, 3, 0 } },
		{ "FUNGAL-010", 1, { { 0, 256, true, { 0, 0 } } }, { { 0, -3 } }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-011", 1, {  }, { { 2, 2 } }, 0, 0, false, false, false, false, { 0, 0, 2 }, { 0, 0, 2 } },
		{ "FUNGAL-012", 1, {  }, { { 3, 9 } }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-013", 7, { { 0, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 1, 1, 1 }, { 1, 1, 1 } },
		{ "FUNGAL-015", 1, { { 88, 448, true, { 0, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-016", 1, { { 0, 256, true, { -2, 0 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-017", 1, { { 0, 256, true, { 0, -2 } } }, {  }, 0, 0, false, false, false, false, { 0, 0, 0 }, { 0, 0, 0 } },
		{ "FUNGAL-018", 1, { { 0, 256, true, { -1, -2 } } }, {  }, 0, 0, false, false, false, false, { 1, 0, 0 }, { 1, 0, 0 } },
		{ "FUNGAL-019", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 0, 5, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-020", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, -1, 0, false, false, false, false, { 2, 3, 3 }, { 1, 2, 2 } },
		{ "FUNGAL-021", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, -7, 0, false, false, false, false, { 2, 3, 3 }, { 0, 0, 0 } },
		{ "FUNGAL-022", 1, { { 6, 256, true, { 0, 0 } }, { 28, 64, true, { 0, 0 } }, { 47, 64, true, { 0, 0 } }, { 56, 128, true, { 0, 0 } }, { 57, 128, true, { 0, 0 } }, { 63, 256, true, { 0, 0 } }, { 64, 64, true, { 0, 0 } }, { 74, 128, true, { 0, 0 } } }, {  }, 5, 0, false, false, false, false, { 2, 3, 3 }, { 2, 3, 3 } },
		{ "FUNGAL-023", 1, {  }, { { 0, 105 }, { 1, 104 }, { 2, 103 } }, 0, 0, false, false, false, false, { 105, 104, 103 }, { 99, 99, 99 } },
		{ "FUNGAL-024", 1, {  }, { { 0, 105 }, { 1, 104 }, { 2, 103 } }, 0, 0, true, false, false, true, { 105, 104, 103 }, { 100, 100, 300 } },
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
struct Record { int32_t tag, parameter, amount; };
struct ParserCase { const char* id; std::vector< std::pair< const char*, const char* > > pairs; std::vector< Record > records; int32_t extra; std::array< int32_t, 3 > capabilities, yields; };
inline const std::array< ParserCase, 12 >& ParserCases() {
	static const std::array< ParserCase, 12 > values = {{
		{ "FUNGAL-PARSER-01", { { "FUNGNUTRIENT", "1" }, { "FUNGMINERALS", "2" }, { "FUNGENERGY", "3" } }, { { 5, 1, 0 }, { 14, 2, 0 }, { 15, 3, 0 } }, 0, { 1, 2, 3 }, { 1, 2, 3 } },
		{ "FUNGAL-PARSER-02", { { "FUNGENERGY", "3" }, { "FUNGMINERALS", "2" }, { "FUNGNUTRIENT", "1" } }, { { 15, 3, 0 }, { 14, 2, 0 }, { 5, 1, 0 } }, 0, { 1, 2, 3 }, { 1, 2, 3 } },
		{ "FUNGAL-PARSER-03", { { "FUNGMINERALS", "2" }, { "FUNGENERGY", "3" }, { "FUNGMINERALS", "4" }, { "FUNGENERGY", "5" } }, { { 14, 2, 0 }, { 15, 3, 0 }, { 14, 4, 0 }, { 15, 5, 0 } }, 0, { 0, 6, 8 }, { 0, 6, 8 } },
		{ "FUNGAL-PARSER-04", { { "FUNGMINERALS", "-4" }, { "FUNGMINERALS", "1" }, { "FUNGENERGY", "+4" }, { "FUNGNUTRIENT", "0" } }, { { 14, -4, 0 }, { 14, 1, 0 }, { 15, 4, 0 }, { 5, 0, 0 } }, 0, { 0, 0, 4 }, { 0, 0, 4 } },
		{ "FUNGAL-PARSER-05", { { "FUNGMINERALS", "1" }, { "FUNGMINERALS", "2" }, { "FUNGMINERALS", "3" }, { "FUNGMINERALS", "4" }, { "FUNGMINERALS", "5" }, { "FUNGMINERALS", "6" }, { "FUNGMINERALS", "7" }, { "FUNGMINERALS", "8" }, { "FUNGENERGY", "9" }, { "FUNGNUTRIENT", "9" } }, { { 14, 1, 0 }, { 14, 2, 0 }, { 14, 3, 0 }, { 14, 4, 0 }, { 14, 5, 0 }, { 14, 6, 0 }, { 14, 7, 0 }, { 14, 8, 0 } }, 0, { 0, 36, 0 }, { 0, 36, 0 } },
		{ "FUNGAL-PARSER-06", { { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGMINERALS", "9" } }, { { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 } }, 0, { 0, 0, 8 }, { 0, 0, 8 } },
		{ "FUNGAL-PARSER-07", { { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "SOCIAL", "+PLANET" }, { "FUNGENERGY", "4" }, { "FUNGMINERALS", "9" } }, { { 2, 7, 1 }, { 2, 7, 1 }, { 2, 7, 1 }, { 2, 7, 1 }, { 2, 7, 1 }, { 2, 7, 1 }, { 2, 7, 1 }, { 15, 4, 0 } }, 0, { 0, 0, 4 }, { 0, 0, 4 } },
		{ "FUNGAL-PARSER-08", { { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "TECH", "Ecology" }, { "FUNGMINERALS", "3" }, { "FUNGENERGY", "9" } }, { { 0, 6, 0 }, { 0, 6, 0 }, { 0, 6, 0 }, { 0, 6, 0 }, { 0, 6, 0 }, { 0, 6, 0 }, { 0, 6, 0 }, { 14, 3, 0 } }, 0, { 0, 3, 0 }, { 0, 3, 0 } },
		{ "FUNGAL-PARSER-09", { { "TECH", "3" }, { "FUNGMINERALS", "2" }, { "FUNGENERGY", "4" } }, { { 14, 2, 0 }, { 15, 4, 0 } }, 3, { 0, 2, 4 }, { 0, 2, 4 } },
		{ "FUNGAL-PARSER-10", { { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "FUNGENERGY", "1" }, { "TECH", "2" }, { "FUNGMINERALS", "9" }, { "TECH", "1" } }, { { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 }, { 15, 1, 0 } }, 1, { 0, 0, 8 }, { 0, 0, 8 } },
		{ "FUNGAL-PARSER-11", { { "TECH", "Ecology" }, { "FUNGMINERALS", "2" }, { "SOCIAL", "+PLANET" }, { "FUNGENERGY", "3" }, { "FUNGNUTRIENT", "1" }, { "TECH", "1" } }, { { 0, 6, 0 }, { 14, 2, 0 }, { 2, 7, 1 }, { 15, 3, 0 }, { 5, 1, 0 } }, 1, { 1, 2, 3 }, { 1, 2, 3 } },
		{ "FUNGAL-PARSER-12", { { "FUNGMINERALS", "2" }, { "FUNGENERGY", "3" }, { "SOCIAL", "+ECONOMY" }, { "SOCIAL", "+ECONOMY" }, { "SOCIAL", "+ECONOMY" }, { "SOCIAL", "+ECONOMY" }, { "SOCIAL", "+ECONOMY" }, { "SOCIAL", "+ECONOMY" }, { "TECH", "Ecology" }, { "TECH", "2" } }, { { 14, 2, 0 }, { 15, 3, 0 }, { 2, 0, 1 }, { 2, 0, 1 }, { 2, 0, 1 }, { 2, 0, 1 }, { 2, 0, 1 }, { 2, 0, 1 } }, 2, { 0, 2, 3 }, { 0, 2, 3 } },
	}}; return values;
}
}
