#pragma once

#include <set>
#include <string>
#include <functional>
#include <optional>

#include "types/Serializable.h"

#include "game/backend/rules/default/Default.h" // only default rules for now
#include "game/backend/Account.h"
#include "SmacResourceRules.h"
#include "SmacCampaignRules.h"
#include "SmacCouncilRules.h"
#include "SmacEcologyRules.h"
#include "SmacSpecialistRules.h"
#include "SmacTerraformingRules.h"
#include "SmacProductionRules.h"
#include "SmacTechnologyRules.h"
#include "SmacWorldSizeRules.h"
#include "SmacFactionStartupRules.h"
#include "SmacPredefinedUnitRules.h"
#include "SmacMovementRules.h"
#include "SmacCombatRules.h"
#include "SmacTerritoryRules.h"

#include "gse/Wrappable.h"

#include "Types.h"

namespace game {
namespace backend {
namespace settings {

// includes

CLASS2( MapSettings, types::Serializable, gse::Wrappable )

	enum type_t {
		MT_RANDOM,
		MT_CUSTOM,
		MT_MAPFILE,
		MT_INSTALLED_PLANET,
	};
	type_t type = MT_RANDOM;

	std::string filename = "";

	/*
	 	{ settings::MAP_CONFIG_TINY,     { 68,  34 } },
		{ settings::MAP_CONFIG_SMALL,    { 88,  44 } },
		{ settings::MAP_CONFIG_STANDARD, { 112, 56 } },
		{ settings::MAP_CONFIG_LARGE,    { 140, 70 } },
		{ settings::MAP_CONFIG_HUGE,     { 180, 90 } }
	 */
	int64_t size_x = 112;
	int64_t size_y = 56;
	const std::optional< SmacWorldSizeSelection >& GetSmacWorldSize() const;
	void SetSmacWorldSize( const SmacWorldSizeSelection& selection );
	void ClearSmacWorldSize();
	void LockWorldSize();
	bool IsOrdinaryCampaignMap() const { return type==MT_RANDOM || type==MT_CUSTOM || type==MT_INSTALLED_PLANET; }

	/*
	 	{ settings::MAP_CONFIG_OCEAN_LOW,    0.4f }, // '30-50% of surface'
		{ settings::MAP_CONFIG_OCEAN_MEDIUM, 0.6f }, // '50-70% of surface'
		{ settings::MAP_CONFIG_OCEAN_HIGH,   0.8f }  // '70-90% of surface'
	 */
	float ocean_coverage = 0.4f;
	// Exact displayed presets map to the original retained choice K=0..2.
	// An arbitrary continuous port setting has no established SMAC category.
	std::optional<int32_t> GetSmacOceanCoverage() const;

	/*
	 	{ settings::MAP_CONFIG_EROSIVE_STRONG,  0.5f }, // 'strong'
		{ settings::MAP_CONFIG_EROSIVE_AVERAGE, 0.75f }, // 'average'
		{ settings::MAP_CONFIG_EROSIVE_WEAK,    1.0f }, // 'weak'
	 */
	float erosive_forces = 0.75f;

	/*
 		{ settings::MAP_CONFIG_LIFEFORMS_RARE,     0.25f }, // 'rare'
		{ settings::MAP_CONFIG_LIFEFORMS_AVERAGE,  0.5f }, // 'average'
		{ settings::MAP_CONFIG_LIFEFORMS_ABUNDANT, 0.75f }, // 'abundant'
	 */
	float native_lifeforms = 0.5f;

	/*
	 	{ settings::MAP_CONFIG_CLOUDS_SPARSE,  0.25f }, // 'sparse'
		{ settings::MAP_CONFIG_CLOUDS_AVERAGE, 0.5f }, // 'average'
		{ settings::MAP_CONFIG_CLOUDS_DENSE,   0.75f }, // 'dense'
	 */
	float cloud_cover = 0.5f;

	WRAPDEFS_DYNAMIC( MapSettings );

	const types::Buffer Serialize() const override;
	void Deserialize( types::Buffer buf ) override;
private:
	friend class GlobalSettings;
	std::optional< SmacWorldSizeSelection > m_smac_world_size;
	bool m_world_size_locked = false;
};

// settings that are synced between players (host has authority)
CLASS2( GlobalSettings, types::Serializable, gse::Wrappable )

	void Initialize();
	// Gameplay identity, independent of the installed asset distribution.
	enum ruleset_t { RULESET_UNSELECTED, RULESET_SMAC, RULESET_SMAX };
	const std::string& GetRulesetId() const;
	void SelectRuleset( const std::string& id );
	using RulesSource = std::function< std::string( const std::string& filename ) >;
	void InitializeHostRules( const RulesSource& read_source );
	const std::optional< SmacResourceRules >& GetSmacResourceRules() const;
	const std::optional< SmacProductionRules >& GetSmacProductionRules() const;
	const std::optional< SmacTechnologyRules >& GetSmacTechnologyRules() const;
	const std::optional< SmacWorldSizeRules >& GetSmacWorldSizeRules() const;
	const std::optional< SmacFactionStartupRules >& GetSmacFactionStartupRules() const;
	const std::optional< SmacPredefinedUnitRules >& GetSmacPredefinedUnitRules() const;
	const std::optional< SmacMovementRules >& GetSmacMovementRules() const;
	const std::optional< SmacCombatRules >& GetSmacCombatRules() const;
	const std::optional< SmacSpecialistRules >& GetSmacSpecialistRules() const;
	const std::optional< SmacTerritoryRules >& GetSmacTerritoryRules() const;
	const std::optional< SmacTerraformingRules >& GetSmacTerraformingRules() const;
	void SelectSmacWorldSize( int64_t category );
	void SelectSmacCustomSize( int64_t width, int64_t height );
	void SelectInstalledPlanet();
	void SelectSmacCampaignRules( bool current );
	const std::optional< SmacCampaignRules >& GetSmacCampaignRules() const;
	const std::optional< SmacCouncilRules >& GetSmacCouncilRules() const {return m_smac_council_rules;}
	const std::optional< SmacEcologyRules >& GetSmacEcologyRules() const {return m_smac_ecology_rules;}
	bool IsOrdinarySmacCampaign() const;
	void RestoreSmacWorldSize( int64_t category, int64_t width, int64_t height );
	void ValidateWorldSize( bool require_selection = true ) const;
	void LockWorldSize();
	// Source facility slot 1..71. Missing profile/slot remains unavailable;
	// this query does not establish faction ownership or buildability.
	SmacProductionPrerequisite GetSmacFacilityPrerequisite( int32_t source_id ) const;
	void LockRuleset();
	bool IsRulesetLocked() const;
	void ResetRuleset();

	MapSettings map = {};

	rules::Default rules = {}; // TODO: custom rules
	int64_t difficulty_level = rules.GetDefaultDifficultyLevelV();

	std::string game_name = "";

	// TODO: custom rules struct

	WRAPDEFS_DYNAMIC( GlobalSettings );

	const types::Buffer Serialize() const override;
	void Deserialize( types::Buffer buf ) override;

private:
	std::optional< SmacCampaignRules > m_smac_campaign_rules;
	bool m_campaign_rules_locked = false;
	ruleset_t m_ruleset = RULESET_UNSELECTED;
	bool m_ruleset_locked = false; // lifecycle guard, not saved state
	std::optional< SmacResourceRules > m_smac_resource_rules;
	std::optional< SmacProductionRules > m_smac_production_rules;
	std::optional< SmacTechnologyRules > m_smac_technology_rules;
	std::optional< SmacCouncilRules > m_smac_council_rules;
	std::optional< SmacEcologyRules > m_smac_ecology_rules;
	std::optional< SmacWorldSizeRules > m_smac_world_size_rules;
	std::optional< SmacFactionStartupRules > m_smac_faction_startup_rules;
	std::optional< SmacPredefinedUnitRules > m_smac_predefined_unit_rules;
	std::optional< SmacMovementRules > m_smac_movement_rules;
	std::optional< SmacCombatRules > m_smac_combat_rules;
	std::optional< SmacSpecialistRules > m_smac_specialist_rules;
	std::optional< SmacTerritoryRules > m_smac_territory_rules;
	std::optional< SmacTerraformingRules > m_smac_terraforming_rules;
	static ruleset_t ParseRuleset( const std::string& id );
};

// settings that aren't synced between players
CLASS2( LocalSettings, types::Serializable, gse::Wrappable )
public:

	Account account;

	enum game_mode_t {
		GM_SINGLEPLAYER,
		GM_MULTIPLAYER,
		GM_SCENARIO,
	};
	game_mode_t game_mode = GM_SINGLEPLAYER;

	enum network_type_t {
		NT_NONE,
		NT_SIMPLE_TCPIP,
		NT_HOTSEAT,
	};
	network_type_t network_type = NT_NONE;

	enum network_role_t {
		NR_NONE,
		NR_SERVER,
		NR_CLIENT,
	};
	network_role_t network_role = NR_NONE;

	std::string player_name = "";
	std::string remote_address = "127.0.0.1";

	std::set< std::string > banned_addresses = {};

	WRAPDEFS_DYNAMIC( LocalSettings );

	const types::Buffer Serialize() const override;
	void Deserialize( types::Buffer buf ) override;
};

CLASS2( Settings, types::Serializable, gse::Wrappable )

	GlobalSettings global;
	LocalSettings local;
	// Shared by actual session initialization and asset-free source tests.
	void PrepareRuleset( const GlobalSettings::RulesSource& read_source );

	WRAPDEFS_PTR( Settings );

	const types::Buffer Serialize() const override;
	void Deserialize( types::Buffer buf ) override;
};

}
}
}
