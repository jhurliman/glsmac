#include "WorldSnapshotRules.h"

#include "settings/Settings.h"
#include "types/Buffer.h"

#include <limits>
#include <stdexcept>

namespace game::backend::world_snapshot {
namespace {

void RequireFrozenProfile( const settings::GlobalSettings& settings ) {
	const auto& id = settings.GetRulesetId();
	if ( !settings.IsRulesetLocked() || ( id != "smac" && id != "smax" ) ) {
		throw std::runtime_error( "World snapshot requires a frozen gameplay ruleset" );
	}
	if ( ( id == "smac" ) != settings.GetSmacBaseSquareRules().has_value()
		|| ( id == "smac" ) != settings.GetSmacProductionRules().has_value()
		|| ( id == "smac" ) != settings.GetSmacTechnologyRules().has_value() ) {
		throw std::runtime_error( "World snapshot ruleset and numeric rules do not match" );
	}
	if ( ( id == "smac" ) != settings.GetSmacWorldSizeRules().has_value() ) { throw std::runtime_error( "World snapshot world-size profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacFactionStartupRules().has_value() ) { throw std::runtime_error( "World snapshot faction startup profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacPredefinedUnitRules().has_value() ) { throw std::runtime_error( "World snapshot predefined unit profile does not match ruleset" ); }
	if ( ( id == "smac" ) != settings.GetSmacMovementRules().has_value() ) { throw std::runtime_error( "World snapshot movement profile does not match ruleset" ); }
	settings.ValidateWorldSize();
}

}

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings ) {
	RequireFrozenProfile( settings );
	const auto& production = settings.GetSmacProductionRules();
	const auto& technologies = settings.GetSmacTechnologyRules();
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
	buffer.WriteInt( rules_marker );
	buffer.WriteInt( rules_version );
	buffer.WriteString( settings.GetRulesetId() );
	const auto& rules = settings.GetSmacBaseSquareRules();
	buffer.WriteBool( rules.has_value() );
	if ( rules ) {
		for ( const auto& row : { rules->base_square, rules->bonus_square, rules->recycling } ) {
			for ( const auto value : row ) { buffer.WriteInt( value ); }
		}
	}
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
}

void ValidateHeader( types::Buffer& buffer, const settings::GlobalSettings& settings ) {
	RequireFrozenProfile( settings );
	if ( buffer.ReadInt() != rules_marker || buffer.ReadInt() != rules_version ) {
		throw std::runtime_error( "Unsupported world snapshot rules header" );
	}
	const auto id = buffer.ReadString();
	if ( ( id != "smac" && id != "smax" ) || id != settings.GetRulesetId() ) {
		throw std::runtime_error( "World snapshot gameplay ruleset differs from the session" );
	}
	const auto& rules = settings.GetSmacBaseSquareRules();
	if ( buffer.ReadBool() != rules.has_value() ) {
		throw std::runtime_error( "World snapshot numeric rules presence differs from the session" );
	}
	if ( rules ) {
		for ( const auto& row : { rules->base_square, rules->bonus_square, rules->recycling } ) {
			for ( const auto expected : row ) {
				const auto actual = buffer.ReadInt();
				if ( actual < std::numeric_limits< int32_t >::min() || actual > std::numeric_limits< int32_t >::max() ) {
					throw std::runtime_error( "World snapshot numeric rule is outside signed int32 range" );
				}
				if ( actual != expected ) {
					throw std::runtime_error( "World snapshot numeric rules differ from the session" );
				}
			}
		}
	}
	const auto& production = settings.GetSmacProductionRules();
	if ( buffer.ReadBool() != production.has_value() ) {
		throw std::runtime_error( "World snapshot production profile presence differs from the session" );
	}
	if ( production ) {
		const auto incoming = settings::SmacProductionRules::Deserialize( types::Buffer( buffer.ReadString() ) );
		if ( !( incoming == *production ) ) {
			throw std::runtime_error( "World snapshot production profile differs from the session" );
		}
	}
	const auto& technologies = settings.GetSmacTechnologyRules();
	if ( buffer.ReadBool() != technologies.has_value() ) {
		throw std::runtime_error( "World snapshot technology catalog presence differs from the session" );
	}
	if ( technologies ) {
		const auto incoming = settings::SmacTechnologyRules::Deserialize( types::Buffer( buffer.ReadString() ) );
		if ( !( incoming == *technologies ) ) {
			throw std::runtime_error( "World snapshot technology catalog differs from the session" );
		}
	}
	const auto& sizes = settings.GetSmacWorldSizeRules();
	if ( buffer.ReadBool() != sizes.has_value() ) { throw std::runtime_error( "World snapshot world-size profile presence differs from the session" ); }
	if ( sizes ) {
		const auto incoming = settings::SmacWorldSizeRules::Deserialize( types::Buffer( buffer.ReadString() ) );
		const auto selection = settings::SmacWorldSizeSelection::Deserialize( types::Buffer( buffer.ReadString() ) );
		incoming.ValidateSelection( selection );
		if ( !( incoming == *sizes ) || !( selection == *settings.map.GetSmacWorldSize() ) ) { throw std::runtime_error( "World snapshot world-size profile or selection differs from the session" ); }
	}
	const auto& factions = settings.GetSmacFactionStartupRules();
	if ( buffer.ReadBool() != factions.has_value() ) { throw std::runtime_error( "World snapshot faction startup presence differs from the session" ); }
	if ( factions ) {
		const auto incoming = settings::SmacFactionStartupRules::Deserialize( types::Buffer( buffer.ReadString() ), *technologies );
		if ( !( incoming == *factions ) ) { throw std::runtime_error( "World snapshot faction startup profile differs from the session" ); }
	}
	const auto& units = settings.GetSmacPredefinedUnitRules();
	if ( buffer.ReadBool() != units.has_value() ) { throw std::runtime_error( "World snapshot predefined unit presence differs from the session" ); }
	if ( units ) {
		const auto incoming = settings::SmacPredefinedUnitRules::Deserialize( buffer.ReadString(), *technologies );
		if ( !( incoming == *units ) ) { throw std::runtime_error( "World snapshot predefined unit profile differs from the session" ); }
	}
	const auto& movement = settings.GetSmacMovementRules();
	if ( buffer.ReadBool() != movement.has_value() ) { throw std::runtime_error( "World snapshot movement presence differs from the session" ); }
	if ( movement ) {
		const auto incoming = settings::SmacMovementRules::Deserialize( types::Buffer( buffer.ReadString() ) );
		if ( !( incoming == *movement ) ) { throw std::runtime_error( "World snapshot movement profile differs from the session" ); }
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
