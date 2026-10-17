#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace loader::txt {

enum class WeaponMode : int32_t {
	Projectile = 0, Energy = 1, Missile = 2,
	Transport = 7, Colonist = 8, Terraformer = 9, Convoy = 10,
	InfoWar = 11, Artifact = 12, Tectonic = 13, Fungal = 14
};

struct WeaponComponent {
	std::string name, short_name;
	int32_t offense;
	WeaponMode mode;
	int32_t cost, icon;
	std::string prerequisite;
};

struct WeaponTable {
	std::string ruleset_id;
	std::vector< WeaponComponent > weapons; // source order; index is local to this table
};

class UnitRulesTXTLoader {
public:
	// Pure #WEAPONS decoder. The caller supplies contents and an explicit
	// "smac"/"smax" identity; this does not select files or authenticate them.
	// Numeric metadata is signed int32; supported modes are 0..2, 7..12, plus
	// 13..14 for SMAX. No costs, technology eligibility, designs or art are built.
	// Missing/duplicate sections, invalid fields and duplicate exact short names
	// throw std::invalid_argument. No partial table escapes on rejection.
	static WeaponTable ParseWeapons( const std::string& source, const std::string& ruleset_id );
};

}
