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
	if ( ( id == "smac" ) != settings.GetSmacBaseSquareRules().has_value() ) {
		throw std::runtime_error( "World snapshot ruleset and numeric rules do not match" );
	}
}

}

void WriteHeader( types::Buffer& buffer, const settings::GlobalSettings& settings ) {
	RequireFrozenProfile( settings );
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
}

}
