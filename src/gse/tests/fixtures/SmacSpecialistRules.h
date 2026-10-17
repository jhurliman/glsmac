#pragma once

#include "SmacRulesSource.h"
#include "game/backend/settings/SmacSpecialistRules.h"

namespace gse::tests::smac_specialist_fixture {

using smac_rules_fixture::Technologies;

inline game::backend::settings::SmacSpecialistRules Specialists() {
	game::backend::settings::SmacSpecialistRules result{ -3, {} };
	for ( size_t i = 0; i < result.specialists.size(); ++i ) {
		result.specialists[ i ] = { static_cast< int32_t >( i ), "Repeated singular", "Repeated plural",
			i == 0 ? "None" : i == 1 ? "Disable" : i == 2 ? "Unknown" : "tEcH" + std::to_string( i ),
			i == 6 ? "Disable" : "Tech" + std::to_string( i ),
			i == 0 ? INT32_MIN : static_cast< int32_t >( i ), -static_cast< int32_t >( i ), i == 6 ? INT32_MAX : 0, "opaque" + std::to_string( i ) };
	}
	return result;
}

inline std::string SpecialistSource() {
	std::string source = "#CITIZENS\n";
	for ( const auto& row : Specialists().specialists ) {
		source += row.singular + "," + row.plural + "," + row.prerequisite + "," + row.obsolete + ","
			+ std::to_string( row.economy ) + "," + std::to_string( row.psych ) + "," + std::to_string( row.labs ) + "," + row.trailing_token + "\n";
	}
	return source + "Ordinary A,Ordinary plural A,\nOrdinary B,Ordinary plural B,\nOrdinary C,Ordinary plural C,\n#END\n";
}

inline types::Buffer UncheckedSpecialists( const game::backend::settings::SmacSpecialistRules& profile ) {
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( profile.minimum_base_size ); result.WriteInt( profile.specialists.size() );
	for ( const auto& row : profile.specialists ) {
		result.WriteInt( row.source_id ); result.WriteString( row.singular ); result.WriteString( row.plural );
		result.WriteString( row.prerequisite ); result.WriteString( row.obsolete );
		result.WriteInt( row.economy ); result.WriteInt( row.psych ); result.WriteInt( row.labs ); result.WriteString( row.trailing_token );
	}
	return result;
}

// Only the independently supplied section shape and positional minimum are
// needed by this catalog parser; no host Settings profile is constructed.
inline std::string ProductionSource() {
    std::string source = "#RULES\n";
    for ( size_t row = 1; row <= 70; ++row ) {
        source += std::to_string( row == 11 ? Specialists().minimum_base_size : 0 ) + ", ; synthetic annotation\n";
    }
    return source + "#END\n";
}
}
