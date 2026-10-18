#include "SmacWorldSizeRules.h"

#include "fixtures/SmacRulesSource.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "game/backend/WorldSnapshotRules.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <vector>
#include <cmath>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
namespace fixture = smac_rules_fixture;
bool Rejects( const std::function< void() >& f ) { try { f(); } catch ( const std::exception& ) { return true; } return false; }
const std::string source = "#WORLDSIZE\n5\nTiny|annotation,24,48\nSmall,32,64\nStandard,40,80\nLarge,44,90\nHuge,64,128\n#END\n";
}

void AddSmacWorldSizeRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest("SMAC ocean coverage retains preset categories without quantizing continuous port settings",GT(){
		MapSettings settings;
		const std::array<float,3> values{0.4f,0.6f,0.8f};
		for(size_t i=0;i<values.size();++i) {
			settings.ocean_coverage=values[i];MapSettings restored;restored.Deserialize(settings.Serialize());
			GT_ASSERT(settings.GetSmacOceanCoverage()==static_cast<int32_t>(i) && restored.GetSmacOceanCoverage()==settings.GetSmacOceanCoverage(),"saved selected preset lost its original category");
		}
		for(const float value:{0.2f,0.5f,0.55f,std::nextafter(0.4f,1.0f),NAN,INFINITY}) {
			settings.ocean_coverage=value;GT_ASSERT(!settings.GetSmacOceanCoverage(),"continuous or invalid coverage was rounded into a category");
		}
		settings.ocean_coverage=0.4f;MapSettings locked;locked.Deserialize(settings.Serialize());locked.LockWorldSize();
		const auto before=locked.Serialize().ToString();settings.ocean_coverage=0.6f;
		GT_ASSERT(Rejects([&]{locked.Deserialize(settings.Serialize());}) && locked.Serialize().ToString()==before,"host changed locked ocean coverage or rejected update partially published");
		settings.ocean_coverage=NAN;MapSettings raw;raw.Deserialize(settings.Serialize());raw.LockWorldSize();
		settings.erosive_forces=0.5f;raw.Deserialize(settings.Serialize());
		GT_ASSERT(!raw.GetSmacOceanCoverage() && raw.Serialize().ToString()==settings.Serialize().ToString(),"unchanged saved NaN coverage rejected an otherwise permitted update");
		const auto nan_before=raw.Serialize().ToString();settings.ocean_coverage=0.4f;
		GT_ASSERT(Rejects([&]{raw.Deserialize(settings.Serialize());}) && raw.Serialize().ToString()==nan_before,"locked NaN representation changed or rejection partially published");
		GT_OK();
	});
	task->AddTest( "SMAC world-size source preserves positional identity and doubles only source X", GT() {
		const auto rows = loader::txt::SmacRulesTXTLoader::ParseWorldSizeRules( source );
		const std::array< int32_t, 5 > widths{ 48, 64, 80, 88, 128 }, heights{ 48, 64, 80, 90, 128 };
		for ( size_t i = 0; i < 5; ++i ) {
			const auto selected = rows.SelectPreset( i );
			GT_ASSERT( selected.category == static_cast< int32_t >( i ) && selected.width == widths[ i ] && selected.height == heights[ i ], "wrong source/category interpretation" );
			const auto custom = SmacWorldSizeRules::SelectCustom( widths[ i ], heights[ i ] );
			GT_ASSERT( custom.category == 99 && !( custom == selected ), "custom geometry became preset identity" );
		}
		GT_ASSERT( rows.presets[ 0 ].name == "Tiny|annotation", "source name was normalized" );
		GT_ASSERT( SmacWorldSizeRules::Deserialize( rows.Serialize() ) == rows, "owned catalog codec lost fields" );
		GT_OK();
	} );
	task->AddTest( "SMAC world-size catalogs reject every unsupported preset before publication", GT() {
		for ( size_t index = 0; index < 5; ++index ) {
			auto rows = fixture::WorldSizes(); ++rows.presets[ index ].source_y;
			GT_ASSERT( Rejects( [ & ]() { rows.Validate(); } ) && Rejects( [ & ]() { rows.Serialize(); } ), "unselectable preset admitted to catalog" );
			// Build malformed wire/source directly; do not rely on Serialize rejecting it.
			types::Buffer wire; wire.WriteInt( 1 ); wire.WriteInt( 5 );
			std::string table = "#WORLDSIZE\n5\n";
			for ( const auto& row : rows.presets ) {
				wire.WriteString( row.name ); wire.WriteInt( row.source_x ); wire.WriteInt( row.source_y );
				table += row.name + ", " + std::to_string( row.source_x ) + ", " + std::to_string( row.source_y ) + "\n";
			}
			table += "#END\n";
			GT_ASSERT( Rejects( [ & ]() { SmacWorldSizeRules::Deserialize( wire ); } ), "wire catalog accepted an unselectable preset" );
			GT_ASSERT( Rejects( [ & ]() { loader::txt::SmacRulesTXTLoader::ParseWorldSizeRules( table ); } ), "source catalog accepted an unselectable preset" );
			auto source = fixture::Source( "alpha.txt" );
			const auto original = fixture::WorldSizeSource(); source.replace( source.find( original ), original.size(), table );
			GlobalSettings host; host.SelectRuleset( "smac" );
			GT_ASSERT( Rejects( [ & ]() { host.InitializeHostRules( [ & ]( const std::string& ) { return source; } ); } ), "host published an unselectable preset" );
			GT_ASSERT( !host.IsRulesetLocked() && !host.GetSmacResourceRules() && !host.GetSmacProductionRules() && !host.GetSmacTechnologyRules() && !host.GetSmacWorldSizeRules() && !host.map.GetSmacWorldSize(), "rejected preset partially published host state" );
			GlobalSettings peer;
			GT_ASSERT( Rejects( [ & ]() { peer.Deserialize( fixture::Payload( "smac", true, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, {}, {}, wire.ToString() ) ); } ), "peer published an unselectable preset" );
			GT_ASSERT( peer.GetRulesetId() == "unselected" && !peer.IsRulesetLocked() && !peer.GetSmacWorldSizeRules(), "rejected wire catalog partially published peer state" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC world-size parser and codecs reject malformed late fields without partial settings", GT() {
		std::vector< std::string > invalid{ "", "#WORLDSIZE\n0\n", source + source };
		for ( const auto& row : { "Huge,64", "Huge,64,128,", "Huge,64junk,128", "Huge,64,2147483648", "Huge,64,0", ",64,128" } ) {
			auto changed = source; changed.replace( changed.find( "Huge,64,128" ), std::string( "Huge,64,128" ).size(), row ); invalid.push_back( changed );
		}
		for ( const auto& text : invalid ) { GT_ASSERT( Rejects( [ & ]() { loader::txt::SmacRulesTXTLoader::ParseWorldSizeRules( text ); } ), "bad table accepted" ); }
		GlobalSettings settings; settings.SelectRuleset( "smac" );
		auto bad = fixture::Source( "alpha.txt" ); bad.replace( bad.find( "Fixture huge" ), std::string( "Fixture huge" ).size(), "" );
		GT_ASSERT( Rejects( [ & ]() { settings.InitializeHostRules( [ & ]( const std::string& ) { return bad; } ); } ), "bad late profile accepted" );
		GT_ASSERT( !settings.IsRulesetLocked() && !settings.GetSmacProductionRules() && !settings.GetSmacWorldSizeRules(), "bad source partially froze profiles" );
		settings.InitializeHostRules( fixture::Source ); const auto before = settings.Serialize().ToString();
		for ( size_t i = 0; i < 5; ++i ) {
			for ( const int field : { 0, 1, 2 } ) {
				auto rows = fixture::WorldSizes();
				if ( field == 0 ) { rows.presets[ i ].name += " changed"; }
				if ( field == 1 ) { ++rows.presets[ i ].source_x; }
				if ( field == 2 ) { rows.presets[ i ].source_y += 2; }
				GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, rows.Serialize().ToString() ) ); } ), "locked source row changed" );
				GT_ASSERT( settings.Serialize().ToString() == before, "rejected late field mutated settings" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC accepted world-size selection is explicit atomic locked and versioned", GT() {
		GlobalSettings settings; settings.SelectRuleset( "smac" ); settings.InitializeHostRules( fixture::Source );
		GT_ASSERT( !settings.map.GetSmacWorldSize() && Rejects( [ & ]() { settings.ValidateWorldSize(); } ), "missing choice silently became Standard" );
		settings.SelectSmacWorldSize( 1 ); const auto preset = settings.map.Serialize().ToString();
		settings.SelectSmacCustomSize( 48, 48 ); const auto custom = settings.map.Serialize().ToString();
		GT_ASSERT( preset != custom && settings.map.GetSmacWorldSize()->category == 99, "same dimensions lost category" );
		for ( const auto category : { INT64_MIN, int64_t( -1 ), int64_t( 5 ), int64_t( 99 ), INT64_MAX } ) { GT_ASSERT( Rejects( [ & ]() { settings.SelectSmacWorldSize( category ); } ), "invalid preset accepted" ); }
		for ( const auto xy : { std::pair< int64_t, int64_t >{ 31, 16 }, { 32, 15 }, { 513, 16 }, { 32, 257 }, { 33, 16 }, { 32, 25 }, { INT64_MAX, 16 } } ) {
			GT_ASSERT( Rejects( [ & ]() { settings.SelectSmacCustomSize( xy.first, xy.second ); } ) && settings.map.Serialize().ToString() == custom, "invalid custom partially changed accepted selection" );
		}
		GlobalSettings joined; joined.Deserialize( settings.Serialize() );
		GT_ASSERT( joined.map.GetSmacWorldSize() == settings.map.GetSmacWorldSize(), "global codec lost selection" );
		joined.LockWorldSize();
		GT_ASSERT( Rejects( [ & ]() { joined.SelectSmacWorldSize( 1 ); } ), "running accepted size changed" );
		settings.SelectSmacWorldSize( 1 );
		GT_ASSERT( Rejects( [ & ]() { joined.Deserialize( settings.Serialize() ); } ) && joined.map.GetSmacWorldSize()->category == 99, "locked rejoin changed category at equal geometry" );
		joined.ResetRuleset(); GT_ASSERT( !joined.map.GetSmacWorldSize(), "restart retained acceptance" );
		joined.SelectRuleset( "smax" ); joined.InitializeHostRules( fixture::Source );
		GT_ASSERT( !joined.GetSmacWorldSizeRules() && Rejects( [ & ]() { joined.SelectSmacCustomSize( 48, 48 ); } ), "SMAX inferred SMAC category" );
		GT_OK();
	} );
	task->AddTest( "SMAC world-size binding rejects selection and actual map prefix mismatches", GT() {
		using namespace game::backend::world_snapshot;
		GlobalSettings host; host.SelectRuleset( "smac" ); host.InitializeHostRules( fixture::Source ); host.SelectSmacWorldSize( 1 );
		types::Buffer header; WriteHeader( header, host );
		GlobalSettings peer; peer.Deserialize( host.Serialize() );
		peer.SelectSmacCustomSize( 48, 48 );
		GT_ASSERT( Rejects( [ & ]() { ValidateHeader( header, peer ); } ), "header accepted custom in place of same-size preset" );
		peer.SelectSmacWorldSize( 1 ); ValidateHeader( header = types::Buffer( header.ToString() ), peer );
		for ( const auto xy : { std::pair< int64_t, int64_t >{ 48, 48 }, { 64, 48 }, { 48, 47 }, { -1, 48 }, { INT64_MAX, 48 } } ) {
			types::Buffer map; map.WriteInt( xy.first ); map.WriteInt( xy.second );
			GT_ASSERT( Rejects( [ & ]() { ValidateMapDimensions( map.ToString(), host ); } ) == ( xy.first != 48 || xy.second != 48 ), "map prefix dimension admission changed" );
		}
		GT_OK();
	} );
}

}
