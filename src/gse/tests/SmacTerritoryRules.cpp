#include "SmacTerritoryRules.h"
#include "fixtures/SmacTerritoryRules.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

namespace gse::tests {
namespace {
namespace territory = game::backend::map::smac_territory;
namespace fixture = smac_territory_fixture;

territory::Input Input( const fixture::Case& row ) {
	territory::Input input{64,32,true,row.border,{},row.bases};
	// Supplied outputs of the actual region builder in these two controlled
	// worlds, not a replacement flood-fill or a live Map region assumption.
	for ( int32_t y = 0; y < 32; ++y ) { for ( int32_t x = y & 1; x < 64; x += 2 ) {
		const bool sea = row.coast && x > 32, polar = y == 0 || y == 31;
		input.tiles.push_back( {sea,static_cast<uint8_t>(sea ? (polar ? 127 : 65) : (polar ? 63 : 1))} );
	} }
	return input;
}
bool Rejects( const std::function<void()>& fn ) {
	try { fn(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}

std::string CheckTerritoryOriginalMaps() {
	size_t writes = 0;
	for ( const auto& row : fixture::cases ) {
		const auto input = Input(row); const auto actual = territory::Rebuild(input);
		GT_ASSERT( actual.size() == 1024 && std::string(row.claims).size() == 1024, "full-map extent differs" );
		for ( size_t i = 0; i < actual.size(); ++i ) {
			const auto expected = row.claims[i] == '.' ? std::nullopt : std::optional<int32_t>(row.claims[i]-'0');
			GT_ASSERT( actual[i] == expected, "original claim differs at " + std::string(row.name) + " tile " + std::to_string(i) );
			++writes;
		}
	}
	GT_ASSERT( writes == 14336, "literal original write coverage changed" ); GT_OK();
}

std::string CheckTerritoryBoundaries() {
	auto input = Input(fixture::cases[0]);
	const auto index = []( int x, int y ) { return y*32+x/2; };
	const auto claims = territory::Rebuild(input);
	GT_ASSERT( claims[index(26,16)] == 1 && !claims[index(26,18)], "integer metric or strict distance8 cutoff differs" );
	input.bases = {{{0,16},1}};
	GT_ASSERT( territory::Rebuild(input)[index(62,16)] == 1, "wrapped metric missed seam" );
	input.horizontal_wrap = false;
	GT_ASSERT( !territory::Rebuild(input)[index(62,16)], "nonwrapping metric crossed seam" );
	input = Input(fixture::cases[0]); input.tiles[index(18,16)].region = 2;
	GT_ASSERT( !territory::Rebuild(input)[index(18,16)], "region identity was inferred from adjacent terrain" );
	input = Input(fixture::cases[8]); input.border_distance = 1;
	const auto empty_sea = territory::Rebuild(input);
	GT_ASSERT( std::none_of(empty_sea.begin(),empty_sea.end(),[](const auto& value){return value.has_value();}), "sea half-cutoff zero admitted a claim" );
	input = Input(fixture::cases[0]); input.border_distance = std::numeric_limits<int32_t>::max();
	GT_ASSERT( territory::Rebuild(input)[index(62,16)] == 1, "positive signed32 border overflowed" );
	for ( int scenario = 0; scenario < 15; ++scenario ) {
		auto bad = Input(fixture::cases[0]);
		if(scenario==0){bad.width=0;} if(scenario==1){bad.width=513;} if(scenario==2){bad.width=63;}
		if(scenario==3){bad.height=257;} if(scenario==4){bad.border_distance=0;} if(scenario==5){bad.border_distance=-1;}
		if(scenario==6){bad.tiles.pop_back();} if(scenario==7){bad.tiles.back().region=64;} if(scenario==8){bad.tiles.back().sea=true;}
		if(scenario==9){bad.bases[0].owner=0;} if(scenario==10){bad.bases[0].owner=8;} if(scenario==11){bad.bases[0].coordinate={-2,16};}
		if(scenario==12){bad.bases[0].coordinate={16,17};} if(scenario==13){bad.bases[0].coordinate={64,16};} if(scenario==14){bad.bases.push_back(bad.bases[0]);}
		GT_ASSERT( Rejects([&]{territory::Rebuild(bad);}), "invalid territory input admitted at " + std::to_string(scenario) );
	}
	GT_OK();
}
}

void AddSmacTerritoryRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest("SMAC territory matches 14 original complete land and sea maps", GT(){return CheckTerritoryOriginalMaps();});
	task->AddTest("SMAC territory validates geometry region ownership and integer boundaries", GT(){return CheckTerritoryBoundaries();});
}
}
