#include "SmacFungalCapabilities.h"

#include "fixtures/SmacFactionStartupSource.h"
#include "fixtures/SmacFungalCapabilities.h"
#include "fixtures/WorldSnapshotRules.h"
#include "game/backend/faction/SmacTechnologyState.h"
#include "game/backend/resource/SmacTileNutrients.h"
#include "game/backend/resource/SmacTileMinerals.h"
#include "game/backend/resource/SmacTileEnergy.h"
#include "loader/txt/FactionTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend::faction;
using namespace game::backend::settings;
namespace source = smac_faction_startup_fixture;
namespace fixture = smac_fungal_fixture;

SmacTechnologyRules Catalog() {
	auto catalog = source::Technologies();
	for ( auto& row : catalog.technologies ) { row.flags = 0; row.prerequisites = { "None", "None" }; }
	return catalog;
}

SmacFactionStartupRules Profiles( const SmacTechnologyRules& catalog ) {
	return loader::txt::FactionTXTLoader::ParseStartupRules( source::Alpha(), source::Source, catalog );
}

std::array< int32_t, 3 > Values( const SmacFungalCapabilities& value ) { return { value.nutrients, value.minerals, value.energy }; }

bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}

bool Overflows( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::overflow_error& ) { return true; }
	return false;
}

std::array< int64_t, 3 > Yields( const SmacFungalCapabilities& caps, const fixture::Case& row ) {
	using namespace game::backend::resource::smac;
	const auto n = CalculateTileNutrients( TileNutrientRules( 0, 0, 0, 0, 0, 0, 0, 0 ), {
		NutrientTerrain( AltitudeBand::Land3, Moisture::Arid, Rockiness::Flat, ResourceBonus::None, false, false, true, false, false ),
		NutrientImprovements( false, false, false, false ), NutrientContext( false, false, false, caps.nutrients, row.planet, row.increase, row.decrease ) } );
	const auto m = CalculateTileMinerals( TileMineralRules( 0, 0, 0, 0, 0, 0, 0 ), {
		MineralTerrain( AltitudeBand::Land3, 0, ResourceBonus::None, false, false, true, false, false ),
		MineralImprovements( false, false ), MineralContext( false, false, caps.minerals, row.planet, row.increase, row.decrease ) } );
	const auto e = CalculateTileEnergy( TileEnergyRules( 0, 0, 0, 0, 0, 0 ), {
		EnergyTerrain( AltitudeBand::Land3, ResourceBonus::None, false, false, true, false, false, false ),
		EnergyImprovements( false, false, 0 ), EnergyContext( false, false, caps.energy, row.planet, row.economy, false, false, row.increase, row.decrease, row.dust, row.solar ) } );
	return { n.nutrients, m.minerals, e.energy };
}
}

void AddSmacFungalCapabilitiesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC fungal capabilities match twenty four admitted original builder and tile cases", GT() {
		for ( const auto& row : fixture::Cases() ) {
			auto catalog = Catalog(); auto profiles = Profiles( catalog ); profiles.factions[ 0 ].bonuses.clear();
			for ( const auto& t : row.technologies ) {
				auto& definition = catalog.technologies[ t.id ]; definition.flags = t.flags;
				for ( size_t i = 0; i < 2; ++i ) {
					definition.prerequisites[ i ] = t.prerequisites[ i ] == -1 ? "None" : t.prerequisites[ i ] == -2 ? "Disable" : catalog.technologies[ t.prerequisites[ i ] ].key;
				}
			}
			const std::array< const char*, 4 > kinds = { "FUNGNUTRIENT", "FUNGMINERALS", "FUNGENERGY", "VOTES" };
			for ( const auto& modifier : row.modifiers ) { profiles.factions[ 0 ].bonuses.push_back( SmacFactionStartupBonus::Parse( kinds[ modifier.resource ], std::to_string( modifier.amount ), catalog ) ); }
			auto state = SmacTechnologyState::NewWorld( { { row.simulation, "GAIANS" } }, catalog, profiles );
			for ( const auto& t : row.technologies ) { state.factions[ 0 ].owned[ t.id ] = t.owned; }
			const auto before = state.Serialize( catalog, profiles ).ToString();
			const auto actual = state.FungalCapabilities( row.simulation, catalog, profiles );
			GT_ASSERT( Values( actual ) == row.capabilities, "capability differs from original: " + std::string( row.id ) );
			const auto yields = Yields( actual, row );
			for ( size_t i = 0; i < 3; ++i ) { GT_ASSERT( yields[ i ] == row.yields[ i ], "produced capability/tile composition differs: " + std::string( row.id ) ); }
			GT_ASSERT( state.FungalCapabilities( row.simulation, catalog, profiles ) == actual && state.Serialize( catalog, profiles ).ToString() == before,
				"repeated derivation accumulates or mutates authoritative state" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC fungal capabilities consume all seven produced first worker ownership contexts", GT() {
		auto catalog = Catalog();
		// Loaded flag projections, not technology-name exceptions in production.
		for ( const auto& pair : std::array< std::pair< size_t, uint16_t >, 8 >{{ { 6, 256 }, { 28, 64 }, { 47, 64 }, { 56, 128 }, { 57, 128 }, { 63, 256 }, { 64, 64 }, { 74, 128 } }} ) { catalog.technologies[ pair.first ].flags = pair.second; }
		const auto profiles = Profiles( catalog );
		for ( const auto& row : fixture::Workers() ) {
			auto state = SmacTechnologyState::NewWorld( { { 1, row.faction } }, catalog, profiles );
			// The supplied original University choice0 is a later award, not a
			// new default chooser or a grant inferred from its catalog presence.
			state.factions[ 0 ].owned.fill( false );
			for ( const auto id : row.owned ) { state.factions[ 0 ].owned[ id ] = true; }
			const auto actual = state.FungalCapabilities( 1, catalog, profiles );
			GT_ASSERT( Values( actual ) == row.capabilities, "first-worker capability differs: " + std::string( row.faction ) );
			const auto wire = state.Serialize( catalog, profiles ).ToString();
			const auto restored = SmacTechnologyState::Deserialize( wire, catalog, profiles );
			GT_ASSERT( restored.FungalCapabilities( 1, catalog, profiles ) == actual && restored.Serialize( catalog, profiles ).ToString() == wire, "restore recomputation changed owned data" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC fungal capabilities recompute after ownership reset and exact restore without caches", GT() {
		auto catalog = Catalog(); catalog.technologies[ 6 ].flags = 0x1c0; const auto profiles = Profiles( catalog );
		auto state = SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "HIVE" } }, catalog, profiles );
		const SmacFungalCapabilities initial{ 2, 1, 1 }, cleared{ 1, 0, 0 }, zero{ 0, 0, 0 };
		GT_ASSERT( state.FungalCapabilities( 7, catalog, profiles ) == initial && state.FungalCapabilities( 2, catalog, profiles ) == zero, "owner isolation lost" );
		const auto wire = state.Serialize( catalog, profiles ).ToString();
		state.ClearOwnershipForNewWorld();
		GT_ASSERT( state.FungalCapabilities( 7, catalog, profiles ) == cleared, "clear retained cached capabilities" );
		state.InitializeFactionAtTurnZero( 7, catalog, profiles );
		GT_ASSERT( state.FungalCapabilities( 7, catalog, profiles ) == initial, "named grant not reflected" );
		state.factions[ 1 ].owned[ 6 ] = true;
		const SmacFungalCapabilities hive{ 1, 1, 1 };
		GT_ASSERT( state.FungalCapabilities( 2, catalog, profiles ) == hive, "explicit changed possession not reflected" );
		SmacFungalCapabilities detached{ 0, 0, 0 };
		{ auto local = SmacTechnologyState::Deserialize( wire, catalog, profiles ); detached = local.FungalCapabilities( 7, catalog, profiles ); }
		catalog.technologies[ 6 ].flags = 0;
		GT_ASSERT( detached == initial && state.FungalCapabilities( 7, catalog, profiles ) == cleared, "result borrows sources or old flag cache survives" );
		GT_OK();
	} );
	task->AddTest( "SMAC fungal capabilities respect record budget ordered floor and checked arithmetic", GT() {
		auto catalog = Catalog(); auto profiles = Profiles( catalog );
		auto& bonuses = profiles.factions[ 0 ].bonuses;
		const auto bonus = [ & ]( int32_t value ) { return SmacFactionStartupBonus::Parse( "FUNGNUTRIENT", std::to_string( value ), catalog ); };
		bonuses = { bonus( -3 ), bonus( 2 ) };
		auto state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, catalog, profiles );
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == 0, "floored each modifier rather than final sum" );
		bonuses = { bonus( 105 ) };
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == 105, "builder incorrectly capped at99" );
		bonuses.assign( 8, SmacFactionStartupBonus::Parse( "SOCIAL", "+PLANET", catalog ) ); bonuses.push_back( bonus( 105 ) );
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == 0, "ninth shared record or PLANET became capability" );
		bonuses[ 7 ] = bonus( 2 );
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == 2, "eighth shared record lost" );
		bonuses = { bonus( INT32_MAX ) };
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == INT32_MAX, "representable upper boundary rejected" );
		catalog.technologies[ 0 ].flags = 0x100; state.factions[ 0 ].owned[ 0 ] = true;
		GT_ASSERT( Overflows( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "technology plus modifier overflow accepted" );
		state.factions[ 0 ].owned[ 0 ] = false;
		bonuses = { bonus( INT32_MIN ), bonus( -1 ), bonus( INT32_MAX ) };
		GT_ASSERT( Overflows( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "intermediate underflow hidden by floor or later modifier" );
		bonuses = { bonus( INT32_MIN ) };
		GT_ASSERT( state.FungalCapabilities( 1, catalog, profiles ).nutrients == 0, "representable negative endpoint not floored" );
		GT_OK();
	} );
	task->AddTest( "SMAC fungal capabilities reject unknown inputs and preserve unsupported profile boundaries", GT() {
		auto catalog = Catalog(); auto profiles = Profiles( catalog );
		auto state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, catalog, profiles ); const auto before = state;
		for ( const auto id : { 0, -1, 2, 8, INT32_MIN, INT32_MAX } ) { GT_ASSERT( Rejects( [ & ] { state.FungalCapabilities( id, catalog, profiles ); } ), "native or absent simulation accepted" ); }
		catalog.technologies[ 6 ].flags = 0x100;
		for ( size_t field = 0; field < 2; ++field ) {
			catalog.technologies[ 6 ].prerequisites = { "None", "None" }; catalog.technologies[ 6 ].prerequisites[ field ] = "Unknown";
			GT_ASSERT( Rejects( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "unknown relevant prerequisite silently admitted" );
		}
		catalog.technologies[ 6 ].prerequisites = { "None", "None" };
		for ( const auto* keyword : { "FUNGUNKNOWN", "MORALE" } ) {
			GT_ASSERT( Rejects( [ & ] { SmacFactionStartupBonus::Parse( keyword, "1", catalog ); } ), "unproved faction keyword admitted" );
			std::string line; for ( size_t i = 0; i < 8; ++i ) { line += "SOCIAL,+PLANET,"; } line += std::string( keyword ) + ",1";
			GT_ASSERT( Rejects( [ & ] { loader::txt::FactionTXTLoader::ParseStartupBonuses( source::Faction( "GAIANS", line ), "GAIANS", catalog ); } ), "unsupported discarded keyword escaped validation" );
		}
		auto invalid = profiles; invalid.factions.back().bonuses.front().parameter = INT32_MAX;
		GT_ASSERT( Rejects( [ & ] { state.FungalCapabilities( 1, catalog, invalid ); } ) && state == before, "invalid frozen profile mutated state" );
		state.factions[ 0 ].target = 89;
		GT_ASSERT( Rejects( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "invalid owned technology state bypassed validation" );
		GT_OK();
	} );
	task->AddTest( "SMAC fungal parser matches twelve original shared record and capability compositions", GT() {
		for ( const auto& row : fixture::ParserCases() ) {
			auto catalog = Catalog(); catalog.technologies[ 6 ].flags = 0x1c0; auto profiles = Profiles( catalog );
			std::string line;
			for ( const auto& pair : row.pairs ) { if ( !line.empty() ) { line += ','; } line += std::string( pair.first ) + ',' + pair.second; }
			profiles.factions[ 0 ].bonuses = loader::txt::FactionTXTLoader::ParseStartupBonuses( source::Faction( "GAIANS", line ), "GAIANS", catalog );
			const auto& profile = profiles.factions[ 0 ]; const auto records = profile.Records();
			GT_ASSERT( records.size() == row.records.size() && profile.ExtraTechnologyCount() == row.extra, "original shared count or scalar TECH differs" );
			for ( size_t i = 0; i < records.size(); ++i ) {
				GT_ASSERT( static_cast< int32_t >( records[ i ].kind ) == row.records[ i ].tag && records[ i ].parameter == row.records[ i ].parameter && records[ i ].amount == row.records[ i ].amount,
					"original ordered record differs: " + std::string( row.id ) );
			}
			auto state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, catalog, profiles ); state.ClearOwnershipForNewWorld();
			const auto actual = state.FungalCapabilities( 1, catalog, profiles );
			GT_ASSERT( Values( actual ) == row.capabilities, "parsed capability differs: " + std::string( row.id ) );
			fixture::Case context{}; const auto yields = Yields( actual, context );
			for ( size_t i = 0; i < 3; ++i ) { GT_ASSERT( yields[ i ] == row.yields[ i ], "parsed capability tile composition differs" ); }
			const auto wire = profiles.Serialize( catalog ).ToString();
			const auto restored = SmacFactionStartupRules::Deserialize( types::Buffer( wire ), catalog );
			GT_ASSERT( restored == profiles && restored.Serialize( catalog ).ToString() == wire && state.FungalCapabilities( 1, catalog, restored ) == actual, "parser metadata or result changed after restore" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC fungal appended tags retain V2 numbers and checked mineral energy boundaries", GT() {
		using Kind = SmacFactionStartupBonus::Kind;
		const std::array< Kind, 16 > order = { Kind::NAMED_TECH, Kind::EXTRA_TECH, Kind::SOCIAL, Kind::FACILITY, Kind::IMMUNITY,
			Kind::FUNGNUTRIENT, Kind::VOTES, Kind::ENERGY, Kind::DRONE, Kind::TALENT, Kind::POPULATION, Kind::COMMERCE, Kind::FREEPROTO, Kind::FANATIC, Kind::FUNGMINERALS, Kind::FUNGENERGY };
		for ( size_t i = 0; i < order.size(); ++i ) { GT_ASSERT( static_cast< int32_t >( order[ i ] ) == static_cast< int32_t >( i ), "existing tag renumbered or append order differs" ); }
		const auto catalog = Catalog(); auto profiles = Profiles( catalog ); auto state = SmacTechnologyState::NewWorld( { { 1, "GAIANS" } }, catalog, profiles );
		for ( const auto* keyword : { "FUNGMINERALS", "FUNGENERGY" } ) {
			auto& bonuses = profiles.factions[ 0 ].bonuses; const auto bonus = [ & ]( int32_t n ) { return SmacFactionStartupBonus::Parse( keyword, std::to_string( n ), catalog ); };
			bonuses = { bonus( INT32_MAX ) }; auto wire = profiles.Serialize( catalog );
			GT_ASSERT( wire.ReadInt() == 2, "appended tags changed structural codec version" );
			SmacFactionStartupRules restored;
			GT_ASSERT( !Rejects( [ & ] { restored = SmacFactionStartupRules::Deserialize( profiles.Serialize( catalog ), catalog ); } ), "appended V2 tag rejected by reader" );
			GT_ASSERT( restored == profiles && state.FungalCapabilities( 1, catalog, restored ) == state.FungalCapabilities( 1, catalog, profiles ), "new tag failed exact V2 restoration" );
			bonuses.push_back( bonus( 1 ) );
			GT_ASSERT( Overflows( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "new resource overflow accepted" );
			bonuses = { bonus( INT32_MIN ), bonus( -1 ) };
			GT_ASSERT( Overflows( [ & ] { state.FungalCapabilities( 1, catalog, profiles ); } ), "new resource underflow hidden by floor" );
			bonuses = { bonus( INT32_MIN ) }; const auto caps = state.FungalCapabilities( 1, catalog, profiles );
			GT_ASSERT( caps.minerals == 0 && caps.energy == 0, "new resource signed endpoint floor differs" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC fungal appended tags survive settings world restore and remain exactly frozen", GT() {
		const auto catalog = smac_rules_fixture::Technologies(); auto profiles = smac_rules_fixture::Factions();
		profiles.factions.back().bonuses.push_back( SmacFactionStartupBonus::Parse( "FUNGMINERALS", "2", catalog ) );
		profiles.factions.back().bonuses.push_back( SmacFactionStartupBonus::Parse( "FUNGENERGY", "3", catalog ) );
		const auto payload = [ & ]( const SmacFactionStartupRules& value ) { return smac_rules_fixture::Payload( "smac", true,
			{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, value.Serialize( catalog ).ToString() ); };
		GlobalSettings settings; settings.Deserialize( payload( profiles ) ); smac_rules_fixture::AcceptWorldSize( settings );
		const auto before = settings.Serialize().ToString(); GlobalSettings restored; restored.Deserialize( types::Buffer( before ) );
		GT_ASSERT( *restored.GetSmacFactionStartupRules() == profiles && restored.Serialize().ToString() == before, "new tags lost across settings restore" );
		types::Buffer world; game::backend::world_snapshot::WriteHeader( world, settings ); const auto header = world.ToString(); world.WriteString( "body sentinel" );
		game::backend::world_snapshot::ValidateHeader( world, restored );
		GT_ASSERT( world.lenr == header.size() && world.ReadString() == "body sentinel", "header validation consumed body" );
		for ( size_t index = profiles.factions.back().bonuses.size() - 2; index < profiles.factions.back().bonuses.size(); ++index ) {
			auto changed = profiles; auto& bonus = changed.factions.back().bonuses[ index ]; bonus = SmacFactionStartupBonus::Parse( bonus.keyword, "4", catalog );
			GT_ASSERT( Rejects( [ & ] { settings.Deserialize( payload( changed ) ); } ) && settings.Serialize().ToString() == before, "frozen appended metadata changed" );
			GlobalSettings other; other.Deserialize( payload( changed ) ); smac_rules_fixture::AcceptWorldSize( other );
			types::Buffer different; game::backend::world_snapshot::WriteHeader( different, other ); const auto extent = different.lenw; different.WriteString( "body sentinel" );
			GT_ASSERT( Rejects( [ & ] { game::backend::world_snapshot::ValidateHeader( different, restored ); } ) && different.lenr <= extent, "world accepted changed appended metadata or read its body" );
		}
		GT_OK();
	} );
}
}
