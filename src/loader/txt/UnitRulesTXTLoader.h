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

enum class DefenseMode : int32_t { Projectile = 0, Energy = 1, Binary = 2 };

struct DefenseComponent {
	std::string name, short_name;
	int32_t rating;
	DefenseMode mode;
	int32_t cost;
	std::string prerequisite;
};

struct DefenseTable {
	std::string ruleset_id;
	std::vector< DefenseComponent > defenses;
};

struct ReactorComponent {
	std::string name, short_name;
	int32_t power;
	std::string prerequisite;
};

struct ReactorTable {
	std::string ruleset_id;
	std::vector< ReactorComponent > reactors;
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
	// Same ownership, identity, row-order and strict field policy as weapons.
	// Defense modes are 0..2. Signed rating/cost/power are metadata only:
	// these decoders do not select a reactor or validate a playable design.
	static DefenseTable ParseDefenses( const std::string& source, const std::string& ruleset_id );
	static ReactorTable ParseReactors( const std::string& source, const std::string& ruleset_id );
};

}
