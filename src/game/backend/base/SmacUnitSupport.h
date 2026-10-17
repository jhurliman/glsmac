#pragma once

#include <cstdint>
#include <vector>

namespace game::backend::base {

enum class SmacSupportHome { None, OtherBase, EvaluatingBase };
enum class SmacSupportPackage { Combat, Colony, Former, Transport, Supply, Probe, Artifact };
enum class SmacSupportDesign { BuiltIn, PlayerDesigned };
enum class SmacSupportTerrain { Land, Shelf, DeepOcean, Trench };

// Detached semantic projections supplied by the caller. Package is the weapon
// mode, not the design's AI plan. BuiltIn alone does not mean a psi weapon.
struct SmacSupportUnit {
	uint64_t id;
	bool owned_by_base_faction;
	SmacSupportHome home;
	SmacSupportPackage package;
	bool clean_reactor;
	SmacSupportDesign design;
	bool psi_weapon, fungus;
	SmacSupportTerrain terrain;
};

enum class SmacSupportDisposition { Ignored, Exempt, Free, Paid };
struct SmacSupportUnitResult {
	uint64_t id;
	// Ignored means no flag write by this base. Every other disposition replaces
	// its earlier paid-support indicator; only Paid sets that indicator.
	SmacSupportDisposition disposition;
};
struct SmacUnitSupportResult {
	int32_t free_allowance, minerals_per_paid_unit;
	int32_t eligible_count, paid_count, mineral_support;
	std::vector< SmacSupportUnitResult > units;
};

class SmacUnitSupport {
public:
	// Explicit original unit-array order, never inferred from IDs/creation age.
	// Population 0..127; SUPPORT+4 and all counts/costs must fit signed32.
	// The input vector is already owned by the caller. The count ceiling below
	// bounds worst-case arithmetic, not the original game's unit capacity.
	static constexpr uint32_t MAX_ORDERED_UNITS = 0x7fffffffu / 2;
	static SmacUnitSupportResult Recalculate( int32_t support_rating, int32_t population, const std::vector< SmacSupportUnit >& ordered_units );
};

}
