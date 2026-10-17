#pragma once

#include <cstdint>
#include <array>
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

enum class ChassisTriad : int32_t { Land = 0, Sea = 1, Air = 2 };

struct ChassisName {
	std::string name, grammar; // M1/M2/F1/F2/N1/N2, preserved as source metadata
};

struct ChassisComponent {
	// First four name/grammar pairs, then the two trailing large-name pairs.
	// names[0].name is the catalog key referenced by unit designs.
	std::array< ChassisName, 6 > names;
	int32_t speed;
	ChassisTriad triad;
	int32_t range;
	bool missile;
	int32_t cargo, cost;
	std::string prerequisite;
};

struct ChassisTable {
	std::string ruleset_id;
	std::vector< ChassisComponent > chassis;
};

enum class UnitPlan : int32_t {
	AutoCalculate = -1, Offensive = 0, Combat = 1, Defensive = 2,
	Reconnaissance = 3, AirSuperiority = 4, PlanetBuster = 5,
	NavalSuperiority = 6, NavalTransport = 7, Colonization = 8,
	Terraforming = 9, SupplyConvoy = 10, InfoWarfare = 11, AlienArtifact = 12
};

struct UnitDesignRecord {
	std::string name, chassis, weapon, defense; // unresolved exact source references
	UnitPlan plan;
	int32_t cost, carry;
	std::string prerequisite;
	int32_t icon;
	std::string ability_bits; // raw binary spelling, including width/leading zeros
};

struct UnitTable {
	std::string ruleset_id;
	std::vector< UnitDesignRecord > units; // source order; display names may repeat
};

struct AbilityRecord {
	std::string name;
	int32_t cost;
	std::string prerequisite, abbreviation, flags, description;
};

struct AbilityTable {
	std::string ruleset_id;
	std::vector< AbilityRecord > abilities; // source row identity; names/abbreviations may repeat
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
	// Nineteen columns; six name/grammar pairs, triad 0..2 and missile 0/1.
	// Signed speed/range/cargo/cost are metadata, not design eligibility.
	static ChassisTable ParseChassis( const std::string& source, const std::string& ruleset_id );
	// #UNITS starts with a nonnegative count followed by exactly that many
	// ten-column records. Plans are -1..12; cost/carry/icon remain signed int32.
	// References and ability bits are not resolved; no reactor is inferred.
	static UnitTable ParseUnits( const std::string& source, const std::string& ruleset_id );
	// Six columns; abbreviation may be empty and description keeps semicolons.
	// Cost is signed int32 metadata; flags retain nonempty binary spelling.
	// No ability index/bit mapping, prerequisite resolution or cost calculation.
	static AbilityTable ParseAbilities( const std::string& source, const std::string& ruleset_id );
};

}
