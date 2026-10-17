#pragma once

#include "game/backend/settings/SmacTerraformingRules.h"

namespace gse::tests::terraforming_rules_fixture {

// Literal active times and resolved land/sea prerequisites from the original
// full loader(0) capture, former-command-loaded-inputs-20261017.json. Names and
// command text below are synthetic; no installed input rows are redistributed.
inline constexpr std::array< int32_t, 20 > times{4,8,8,4,4,1,3,5,10,4,6,6,12,12,24,18,12,12,8,8};
inline constexpr std::array< int32_t, 20 > land{-1,62,-1,-1,-1,-1,18,17,39,-1,-1,60,60,60,60,60,59,59,-1,-2};
inline constexpr std::array< int32_t, 20 > sea{-1,-2,-1,-1,-2,-2,-2,-2,-2,-2,-1,60,-2,-2,-2,-2,59,59,-2,-2};

inline std::string Token( int32_t id ) { return id == -1 ? "None" : id == -2 ? "Disable" : "Tech" + std::to_string( id ); }
inline game::backend::settings::SmacTerraformingRules Profile() {
	game::backend::settings::SmacTerraformingRules result;
	for ( size_t i = 0; i < result.actions.size(); ++i ) {
		const auto id = std::to_string( i );
		result.actions[ i ] = { static_cast< int32_t >( i ), "Land " + id, Token( land[ i ] ), "Sea " + id,
			Token( sea[ i ] ), times[ i ], "Perform " + id, "x", "Shift+X", i == 19 ? "opaque map annotation" : "" };
	}
	return result;
}
inline std::string Source() {
	std::string result = "#TERRAIN\n";
	for ( const auto& row : Profile().actions ) {
		result += row.land_name + "," + row.land_prerequisite + "," + row.sea_name + "," + row.sea_prerequisite + ","
			+ std::to_string( row.work_time ) + "," + row.order_text + "," + row.order_letter + "," + row.keystroke;
		if ( !row.trailing_annotation.empty() ) { result += "," + row.trailing_annotation; }
		result += "\n";
	}
	return result + "#END\n";
}
// Independent framing permits malformed source identity/text in decode tests.
inline types::Buffer Wire( const game::backend::settings::SmacTerraformingRules& profile ) {
	types::Buffer out; out.WriteInt( 1 ); out.WriteInt( 20 );
	for ( const auto& row : profile.actions ) {
		out.WriteInt( row.source_id ); out.WriteString( row.land_name ); out.WriteString( row.land_prerequisite );
		out.WriteString( row.sea_name ); out.WriteString( row.sea_prerequisite ); out.WriteInt( row.work_time );
		out.WriteString( row.order_text ); out.WriteString( row.order_letter ); out.WriteString( row.keystroke ); out.WriteString( row.trailing_annotation );
	}
	return out;
}

}
