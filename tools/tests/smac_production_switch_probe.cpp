// Opt-in bounded evidence adapter. Original numeric IDs are confined to this tool.
#include "game/backend/base/SmacProductionSwitch.h"

#include <charconv>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace game::backend::base;

namespace {
ProductionItem Item( int32_t id ) {
	if ( id >= 0 ) { return { ProductionItemKind::Unit, std::to_string( id ) }; }
	if ( id == -38 ) { return { ProductionItemKind::Stockpile, "" }; }
	return { id <= -39 ? ProductionItemKind::Project : id <= -34 ? ProductionItemKind::Orbital : ProductionItemKind::Facility, std::to_string( -id ) };
}
int32_t OriginalId( const ProductionItem& item ) {
	if ( item.kind == ProductionItemKind::Stockpile ) { return -38; }
	const auto value = std::stoi( item.id );
	return item.kind == ProductionItemKind::Unit ? value : -value;
}
bool Bit( int32_t value ) { return value == 0 || value == 1; }
}

int main() {
	char line[4096];
	while ( std::cin.getline( line, sizeof( line ) ) ) {
		std::istringstream stream( line );
		std::string token;
		std::vector< int32_t > fields;
		while ( stream >> token ) {
			int32_t value;
			const auto parsed = std::from_chars( token.data(), token.data() + token.size(), value );
			if ( parsed.ec != std::errc() || parsed.ptr != token.data() + token.size() || fields.size() == 79 ) { return 2; }
			fields.push_back( value );
		}
		// mode human difficulty strictness percent exemption baseline balance
		// current balance keep target accept skunkworks facility-count facility-IDs...
		if ( fields.size() < 15 || fields[14] < 0 || fields[14] > 64 || fields.size() != 15 + size_t( fields[14] ) ||
			!Bit( fields[0] ) || !Bit( fields[1] ) || !Bit( fields[10] ) || !Bit( fields[12] ) || !Bit( fields[13] ) ||
			( fields[0] == 1 && fields[10] != 0 ) ) { return 2; }
		for ( const auto index : { 6, 8, 11 } ) { if ( fields[index] == std::numeric_limits< int32_t >::min() ) { return 2; } }
		SmacProductionSwitchContext context{ fields[3], fields[4], fields[5], fields[1] != 0, fields[2], fields[13] != 0, {} };
		for ( size_t i = 15; i < fields.size(); ++i ) {
			if ( fields[i] < 1 || fields[i] > 33 ) { return 2; }
			context.present_local_facilities.insert( std::to_string( fields[i] ) );
		}
		try {
			ProductionState state{ Item( fields[8] ), fields[9], Item( fields[6] ), fields[7] };
			bool refresh = false, prompt = false, cancelled = false;
			if ( fields[0] == 0 ) {
				const auto target = fields[10] ? std::nullopt : std::optional< ProductionItem >( Item( fields[11] ) );
				const auto result = SmacProductionSwitch::Automatic( state, target, context );
				state = result.state; refresh = result.refresh_requested;
			}
			else {
				const auto result = SmacProductionSwitch::Manual( state, Item( fields[11] ), fields[12] ? SmacPenaltyChoice::Accept : SmacPenaltyChoice::Cancel, context );
				state = result.state; prompt = result.penalty_prompt_required; cancelled = result.penalty_cancelled;
			}
			const auto loss = SmacProductionSwitch::BaselineLoss( state, context );
			std::cout << "ok " << OriginalId( state.current_item ) << ' ' << state.minerals << ' ' << OriginalId( state.baseline_item ) << ' ' << state.baseline_minerals
				<< ' ' << loss << ' ' << refresh << ' ' << prompt << ' ' << cancelled << '\n';
		}
		catch ( const std::invalid_argument& ) { std::cout << "invalid\n"; }
	}
	return std::cin.bad() || ( std::cin.fail() && !std::cin.eof() ) ? 2 : 0;
}
