#include "WorldSnapshotRules.h"

#include "settings/Settings.h"
#include "types/Buffer.h"
#include "network/Network.h"

#include <limits>
#include <stdexcept>

namespace game::backend::world_snapshot {
namespace {

// Every accepted frozen profile already fits the complete lobby packet. World
// downloads are chunked, so retain that existing bound at each outer frame.
constexpr uint32_t max_profile_bytes = network::Network::MAX_PACKET_BYTES;
constexpr uint32_t max_ruleset_bytes = 4;
// Version, category, width, height: four (type1 + length4 + int64 + checksum1).
constexpr uint32_t max_selection_bytes = 56;

void RequireFrameSize( const std::string& bytes, const uint32_t limit ) {
	if ( bytes.size() > limit ) { throw std::runtime_error( "World snapshot frozen frame exceeds its transport budget" ); }
}

void RequireFrozenProfile( const settings::GlobalSettings& settings ) {
	const auto& id = settings.GetRulesetId();
	if ( !settings.IsRulesetLocked() || ( id != "smac" && id != "smax" ) ) {
		throw std::runtime_error( "World snapshot requires a frozen gameplay ruleset" );
	}
	if ( ( id == "smac" ) != settings.GetSmacResourceRules().has_value()
		|| ( id == "smac" ) != settings.GetSmacProductionRules().has_value()
		|| ( id == "smac" ) != settings.GetSmacTechnologyRules().has_value() ) {
		throw std::runtime_error( "World snapshot ruleset and numeric rules do not match" );
	}
	if ( ( id == "smac" ) != settings.GetSmacWorldSizeRules().has_value() ) { throw std::runtime_error( "World snapshot world-size profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacFactionStartupRules().has_value() ) { throw std::runtime_error( "World snapshot faction startup profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacPredefinedUnitRules().has_value() ) { throw std::runtime_error( "World snapshot predefined unit profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacMovementRules().has_value() ) { throw std::runtime_error( "World snapshot movement profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacSpecialistRules().has_value() ) { throw std::runtime_error( "World snapshot specialist profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacTerritoryRules().has_value() ) { throw std::runtime_error( "World snapshot territory profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacTerraformingRules().has_value() ) { throw std::runtime_error( "World snapshot terraforming profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacCombatRules().has_value() ) { throw std::runtime_error( "World snapshot combat profile does not match ruleset" ); }
	settings.ValidateWorldSize();
}

}

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings ) {
	RequireFrozenProfile( settings );
	const auto& production = settings.GetSmacProductionRules();
	const auto& technologies = settings.GetSmacTechnologyRules();
	const auto& rules = settings.GetSmacResourceRules();
	const auto resource_bytes = rules ? rules->Serialize( *technologies ).ToString() : std::string{};
	// Validate and encode the entire owned profile before writing any header.
	const auto production_bytes = production ? production->Serialize().ToString() : std::string{};
	const auto technology_bytes = technologies ? technologies->Serialize().ToString() : std::string{};
	const auto& sizes = settings.GetSmacWorldSizeRules();
	const auto size_bytes = sizes ? sizes->Serialize().ToString() : std::string{};
	const auto selection_bytes = sizes ? settings.map.GetSmacWorldSize()->Serialize().ToString() : std::string{};
	const auto& factions = settings.GetSmacFactionStartupRules();
	const auto faction_bytes = factions ? factions->Serialize( *technologies ).ToString() : std::string{};
	const auto& units = settings.GetSmacPredefinedUnitRules();
	const auto unit_bytes = units ? units->Serialize( *technologies ).ToString() : std::string{};
	const auto& movement = settings.GetSmacMovementRules();
	const auto movement_bytes = movement ? movement->Serialize().ToString() : std::string{};
	const auto& specialists = settings.GetSmacSpecialistRules();
	const auto specialist_bytes = specialists ? specialists->Serialize().ToString() : std::string{};
	const auto& territory = settings.GetSmacTerritoryRules();
	const auto territory_bytes = territory ? territory->Serialize().ToString() : std::string{};
	const auto& terraforming = settings.GetSmacTerraformingRules();
	const auto terraforming_bytes = terraforming ? terraforming->Serialize().ToString() : std::string{};
	const auto& ecology = settings.GetSmacEcologyRules();
	const auto ecology_bytes = ecology ? ecology->Serialize().ToString() : std::string{};
	RequireFrameSize( settings.GetRulesetId(), max_ruleset_bytes );
	for ( const auto* bytes : { &production_bytes, &technology_bytes, &size_bytes, &faction_bytes } ) {
		RequireFrameSize( *bytes, max_profile_bytes );
	}
	RequireFrameSize( selection_bytes, max_selection_bytes );
	RequireFrameSize( unit_bytes, settings::SmacPredefinedUnitRules::MAX_WIRE_BYTES );
	RequireFrameSize( movement_bytes, settings::SmacMovementRules::MAX_WIRE_BYTES );
	RequireFrameSize( specialist_bytes, settings::SmacSpecialistRules::MAX_WIRE_BYTES );
	RequireFrameSize( resource_bytes, settings::SmacResourceRules::MAX_WIRE_BYTES );
	RequireFrameSize( territory_bytes, settings::SmacTerritoryRules::MAX_WIRE_BYTES );
	RequireFrameSize( terraforming_bytes, settings::SmacTerraformingRules::MAX_WIRE_BYTES );
	RequireFrameSize( ecology_bytes, settings::SmacEcologyRules::MAX_WIRE_BYTES );
	buffer.WriteInt( rules_marker );
	buffer.WriteInt( rules_version );
	buffer.WriteString( settings.GetRulesetId() );
	buffer.WriteBool( rules.has_value() );
	if ( rules ) { buffer.WriteString( resource_bytes ); }
	buffer.WriteBool( production.has_value() );
	if ( production ) { buffer.WriteString( production_bytes ); }
	buffer.WriteBool( technologies.has_value() );
	if ( technologies ) { buffer.WriteString( technology_bytes ); }
	buffer.WriteBool( sizes.has_value() );
	if ( sizes ) { buffer.WriteString( size_bytes ); buffer.WriteString( selection_bytes ); }
	buffer.WriteBool( factions.has_value() );
	if ( factions ) { buffer.WriteString( faction_bytes ); }
	buffer.WriteBool( units.has_value() );
	if ( units ) { buffer.WriteString( unit_bytes ); }
	buffer.WriteBool( movement.has_value() );
	if ( movement ) { buffer.WriteString( movement_bytes ); }
	buffer.WriteBool( specialists.has_value() );
	if ( specialists ) { buffer.WriteString( specialist_bytes ); }
	buffer.WriteBool( territory.has_value() );
	if ( territory ) { buffer.WriteString( territory_bytes ); }
	buffer.WriteBool( terraforming.has_value() );
	if ( terraforming ) { buffer.WriteString( terraforming_bytes ); }
	const auto& combat=settings.GetSmacCombatRules();
	buffer.WriteBool(combat.has_value());
	if(combat) {buffer.WriteString(combat->Serialize().ToString());}
    buffer.WriteInt(settings.map.type);
    const auto& campaign=settings.GetSmacCampaignRules();
    buffer.WriteBool(campaign.has_value());
    if(campaign) {buffer.WriteString(campaign->Serialize());}
    buffer.WriteBool(ecology.has_value());
    if(ecology) {buffer.WriteString(ecology_bytes);}
}

void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings ) {
	RequireFrozenProfile( settings );
	const auto marker=buffer.ReadInt(); const auto version=buffer.ReadInt();
	if (marker!=rules_marker || (version!=32 && version!=33 && version!=rules_version)) {
		throw std::runtime_error( "Unsupported world snapshot rules header" );
	}
	const auto id = buffer.ReadString( max_ruleset_bytes );
	if ( ( id != "smac" && id != "smax" ) || id != settings.GetRulesetId() ) {
		throw std::runtime_error( "World snapshot gameplay ruleset differs from the session" );
	}
	const auto& rules = settings.GetSmacResourceRules();
	if ( buffer.ReadBool() != rules.has_value() ) {
		throw std::runtime_error( "World snapshot numeric rules presence differs from the session" );
	}
	if ( rules ) {
		const auto incoming = settings::SmacResourceRules::Deserialize( types::Buffer( buffer.ReadString( settings::SmacResourceRules::MAX_WIRE_BYTES ) ), *settings.GetSmacTechnologyRules() );
		if ( !( incoming == *rules ) ) { throw std::runtime_error( "World snapshot resource profile differs from the session" ); }
	}
	const auto& production = settings.GetSmacProductionRules();
	if ( buffer.ReadBool() != production.has_value() ) {
		throw std::runtime_error( "World snapshot production profile presence differs from the session" );
	}
	if ( production ) {
		const auto incoming = settings::SmacProductionRules::Deserialize( types::Buffer( buffer.ReadString( max_profile_bytes ) ) );
		if ( !( incoming == *production ) ) {
			throw std::runtime_error( "World snapshot production profile differs from the session" );
		}
	}
	const auto& technologies = settings.GetSmacTechnologyRules();
	if ( buffer.ReadBool() != technologies.has_value() ) {
		throw std::runtime_error( "World snapshot technology catalog presence differs from the session" );
	}
	if ( technologies ) {
		const auto incoming = settings::SmacTechnologyRules::Deserialize( types::Buffer( buffer.ReadString( max_profile_bytes ) ) );
		if ( !( incoming == *technologies ) ) {
			throw std::runtime_error( "World snapshot technology catalog differs from the session" );
		}
	}
	const auto& sizes = settings.GetSmacWorldSizeRules();
	if ( buffer.ReadBool() != sizes.has_value() ) { throw std::runtime_error( "World snapshot world-size profile presence differs from the session" ); }
	if ( sizes ) {
		const auto incoming = settings::SmacWorldSizeRules::Deserialize( types::Buffer( buffer.ReadString( max_profile_bytes ) ) );
		const auto selection = settings::SmacWorldSizeSelection::Deserialize( types::Buffer( buffer.ReadString( max_selection_bytes ) ) );
		if(settings.map.type!=settings::MapSettings::MT_INSTALLED_PLANET) {incoming.ValidateSelection( selection );}
		else if(!(selection==settings::SmacWorldSizeSelection{4,80,80})) {throw std::runtime_error("World snapshot has invalid installed Planet geometry");}
		if ( !( incoming == *sizes ) || !( selection == *settings.map.GetSmacWorldSize() ) ) { throw std::runtime_error( "World snapshot world-size profile or selection differs from the session" ); }
	}
	const auto& factions = settings.GetSmacFactionStartupRules();
	if ( buffer.ReadBool() != factions.has_value() ) { throw std::runtime_error( "World snapshot faction startup presence differs from the session" ); }
	if ( factions ) {
		const auto incoming = settings::SmacFactionStartupRules::Deserialize( types::Buffer( buffer.ReadString( max_profile_bytes ) ), *technologies );
		if ( !( incoming == *factions ) ) { throw std::runtime_error( "World snapshot faction startup profile differs from the session" ); }
	}
	const auto& units = settings.GetSmacPredefinedUnitRules();
	if ( buffer.ReadBool() != units.has_value() ) { throw std::runtime_error( "World snapshot predefined unit presence differs from the session" ); }
	if ( units ) {
		const auto incoming = settings::SmacPredefinedUnitRules::Deserialize( buffer.ReadString( settings::SmacPredefinedUnitRules::MAX_WIRE_BYTES ), *technologies );
		if ( !( incoming == *units ) ) { throw std::runtime_error( "World snapshot predefined unit profile differs from the session" ); }
	}
	const auto& movement = settings.GetSmacMovementRules();
	if ( buffer.ReadBool() != movement.has_value() ) { throw std::runtime_error( "World snapshot movement presence differs from the session" ); }
	if ( movement ) {
		const auto incoming = settings::SmacMovementRules::Deserialize( types::Buffer( buffer.ReadString( settings::SmacMovementRules::MAX_WIRE_BYTES ) ) );
		if ( !( incoming == *movement ) ) { throw std::runtime_error( "World snapshot movement profile differs from the session" ); }
	}
	const auto& specialists = settings.GetSmacSpecialistRules();
	if ( buffer.ReadBool() != specialists.has_value() ) { throw std::runtime_error( "World snapshot specialist presence differs from the session" ); }
	if ( specialists ) {
		const auto incoming = settings::SmacSpecialistRules::Deserialize( types::Buffer( buffer.ReadString( settings::SmacSpecialistRules::MAX_WIRE_BYTES ) ) );
		if ( !( incoming == *specialists ) ) { throw std::runtime_error( "World snapshot specialist profile differs from the session" ); }
	}
	const auto& territory = settings.GetSmacTerritoryRules();
	if ( buffer.ReadBool() != territory.has_value() ) { throw std::runtime_error("World snapshot territory presence differs"); }
	if ( territory ) {
		const auto incoming = settings::SmacTerritoryRules::Deserialize( types::Buffer(buffer.ReadString(settings::SmacTerritoryRules::MAX_WIRE_BYTES)) );
		if (!(incoming == *territory)) { throw std::runtime_error("World snapshot territory profile differs"); }
	}
	const auto& terraforming = settings.GetSmacTerraformingRules();
	if ( buffer.ReadBool() != terraforming.has_value() ) { throw std::runtime_error( "World snapshot terraforming presence differs" ); }
	if ( terraforming ) {
		const auto incoming = settings::SmacTerraformingRules::Deserialize( types::Buffer( buffer.ReadString( settings::SmacTerraformingRules::MAX_WIRE_BYTES ) ) );
		if ( !( incoming == *terraforming ) ) { throw std::runtime_error( "World snapshot terraforming profile differs" ); }
	}
	const auto& combat=settings.GetSmacCombatRules();
	if(buffer.ReadBool()!=combat.has_value()) {throw std::runtime_error("World snapshot combat presence differs");}
	if(combat) {
		const auto incoming=settings::SmacCombatRules::Deserialize(types::Buffer(buffer.ReadString(settings::SmacCombatRules::MAX_WIRE_BYTES)));
		if(!(incoming==*combat)) {throw std::runtime_error("World snapshot combat profile differs");}
	}

    const auto& campaign=settings.GetSmacCampaignRules();
    if(version==32) {
        if(campaign || settings.map.type==settings::MapSettings::MT_INSTALLED_PLANET) {throw std::runtime_error("Legacy world cannot establish original campaign authority");}
    } else {
        if(buffer.ReadInt()!=settings.map.type) {throw std::runtime_error("World map provenance differs from the session");}
        if(buffer.ReadBool()!=campaign.has_value()) {throw std::runtime_error("World campaign options presence differs");}
        if(campaign && !(settings::SmacCampaignRules::Deserialize(buffer.ReadString(settings::SmacCampaignRules::MAX_WIRE_BYTES))==*campaign)) {throw std::runtime_error("World campaign options differ");}
    }
    const auto& ecology=settings.GetSmacEcologyRules();
    if(version<34) {
        if(ecology) {throw std::runtime_error("Legacy world cannot establish ecology authority");}
    } else {
        if(buffer.ReadBool()!=ecology.has_value()) {throw std::runtime_error("World ecology presence differs");}
        if(ecology && !(settings::SmacEcologyRules::Deserialize(types::Buffer(buffer.ReadString(settings::SmacEcologyRules::MAX_WIRE_BYTES)))==*ecology)) {
            throw std::runtime_error("World ecology profile differs");
        }
    }
}

void ValidateMapDimensions( const std::string& payload, const settings::GlobalSettings& settings ) {
	if ( settings.GetRulesetId() != "smac" ) { return; }
	settings.ValidateWorldSize();
	types::Buffer tiles( payload );
	const auto width = tiles.ReadInt(); const auto height = tiles.ReadInt();
	const auto& selection = *settings.map.GetSmacWorldSize();
	if ( width != selection.width || height != selection.height ) { throw std::runtime_error( "World map dimensions differ from the accepted SMAC world size" ); }
}

}
