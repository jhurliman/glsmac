#pragma once

#include <array>
#include <cstdint>

namespace gse::tests::fixtures::smac_gameplay_random {

// Test-owned semantic records from RE-SMAC-RNG-SEAMS-001. No original code/assets.
// Source JSON SHA256: ea87ebf6d67d6936eadbd5abae8b448db8b923b352f845a8a8349e3a336556f6
// Original status: 28/28 original-x86 cases. Here 24 draw/replay cases are
// compared directly; the remaining four compare supplied setter effects only.
// The original mode is provenance: selection is a caller decision, not RNG state.
struct Replay {
	const char* id;
	bool shared;
	uint8_t original_mode;
	uint32_t supplied, prior, companion, width;
	std::array< uint16_t, 4 > draws;
	uint32_t final_state;
};
inline const std::array< Replay, 24 > REPLAYS{ {
	{ "RNG-SEAM-001", true, 0, 0u, 305419896u, 2271560481u, 9u, { 8, 1, 8, 2 }, 2868466484u },
	{ "RNG-SEAM-002", false, 0, 0u, 3735928559u, 305419896u, 0u, { 38, 7719, 21238, 2437 }, 159719620u },
	{ "RNG-SEAM-003", true, 0, 1u, 305419896u, 2271560481u, 9u, { 3, 4, 0, 2 }, 3027450565u },
	{ "RNG-SEAM-004", false, 0, 1u, 3735928559u, 305419896u, 0u, { 41, 18467, 6334, 26500 }, 3884216597u },
	{ "RNG-SEAM-005", true, 0, 3u, 305419896u, 2271560481u, 9u, { 1, 2, 0, 0 }, 3345418727u },
	{ "RNG-SEAM-006", false, 0, 3u, 3735928559u, 305419896u, 0u, { 48, 7196, 9294, 9091 }, 2743275959u },
	{ "RNG-SEAM-007", true, 0, 2147483647u, 305419896u, 2271560481u, 9u, { 4, 7, 8, 3 }, 561998755u },
	{ "RNG-SEAM-008", false, 0, 2147483647u, 3735928559u, 305419896u, 0u, { 35, 29739, 3374, 11141 }, 2877673587u },
	{ "RNG-SEAM-009", true, 0, 2147483648u, 305419896u, 2271560481u, 9u, { 8, 1, 8, 2 }, 720982836u },
	{ "RNG-SEAM-010", false, 0, 2147483648u, 3735928559u, 305419896u, 0u, { 38, 7719, 21238, 2437 }, 2307203268u },
	{ "RNG-SEAM-011", true, 0, 4294967295u, 305419896u, 2271560481u, 9u, { 4, 7, 8, 3 }, 2709482403u },
	{ "RNG-SEAM-012", false, 0, 4294967295u, 3735928559u, 305419896u, 0u, { 35, 29739, 3374, 11141 }, 730189939u },
	{ "RNG-SEAM-013", true, 1, 0u, 305419896u, 2271560481u, 9u, { 8, 1, 8, 2 }, 2868466484u },
	{ "RNG-SEAM-014", false, 1, 0u, 3735928559u, 305419896u, 0u, { 38, 7719, 21238, 2437 }, 159719620u },
	{ "RNG-SEAM-015", true, 1, 1u, 305419896u, 2271560481u, 9u, { 3, 4, 0, 2 }, 3027450565u },
	{ "RNG-SEAM-016", false, 1, 1u, 3735928559u, 305419896u, 0u, { 41, 18467, 6334, 26500 }, 3884216597u },
	{ "RNG-SEAM-017", true, 1, 3u, 305419896u, 2271560481u, 9u, { 1, 2, 0, 0 }, 3345418727u },
	{ "RNG-SEAM-018", false, 1, 3u, 3735928559u, 305419896u, 0u, { 48, 7196, 9294, 9091 }, 2743275959u },
	{ "RNG-SEAM-019", true, 1, 2147483647u, 305419896u, 2271560481u, 9u, { 4, 7, 8, 3 }, 561998755u },
	{ "RNG-SEAM-020", false, 1, 2147483647u, 3735928559u, 305419896u, 0u, { 35, 29739, 3374, 11141 }, 2877673587u },
	{ "RNG-SEAM-021", true, 1, 2147483648u, 305419896u, 2271560481u, 9u, { 8, 1, 8, 2 }, 720982836u },
	{ "RNG-SEAM-022", false, 1, 2147483648u, 3735928559u, 305419896u, 0u, { 38, 7719, 21238, 2437 }, 2307203268u },
	{ "RNG-SEAM-023", true, 1, 4294967295u, 305419896u, 2271560481u, 9u, { 4, 7, 8, 3 }, 2709482403u },
	{ "RNG-SEAM-024", false, 1, 4294967295u, 3735928559u, 305419896u, 0u, { 35, 29739, 3374, 11141 }, 730189939u },
} };

struct SuppliedSamples {
	const char* id;
	uint32_t previous_shared, previous_crt, first, second, expected_shared, expected_crt;
};
inline const std::array< SuppliedSamples, 3 > SAMPLES{ {
	{ "RNG-SEAM-026", 305419896u, 2271560481u, 100u, 101u, 101u, 100u },
	{ "RNG-SEAM-027", 305419896u, 2271560481u, 4294967295u, 2147483648u, 2147483648u, 4294967295u },
	{ "RNG-SEAM-028", 305419896u, 2271560481u, 0u, 0u, 0u, 0u },
} };

struct SuppliedReset {
	uint32_t prior_shared, crt, before_second_write, after_first, after_second, final_crt;
};
inline const SuppliedReset RESET{ 3735928559u, 305419896u, 2309737967u, 0u, 0u, 305419896u };

}
