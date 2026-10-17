#include "SmacStartupDesignState.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <utility>

namespace gse::tests {
namespace {
using namespace game::backend;
using State = unit::SmacStartupDesignState;
bool Rejects( const std::function< void() >& fn ) { try { fn(); } catch ( const std::exception& ) { return true; } return false; }
faction::SmacTechnologyState Technologies( const std::vector< faction::SmacSimulationAdmission >& roster ) {
	return faction::SmacTechnologyState::NewWorld( roster, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
}
State Saved() {
	auto result = State::NewWorld( Technologies( { { 7, "MORGANITES" }, { 2, "SPARTANS" } } ), smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
	auto& d = result.owners[ 0 ].designs[ 0 ]; d.active = false; d.prototyped = false; d.cost = 255; d.plan = 255; d.carry = 254; d.abilities = 0xffffff; d.faction_exclusion = UINT32_MAX;
	result.owners[ 0 ].starting_scout = d.id;
	result.owners[ 1 ].designs[ 0 ].cost = 0;
	return result;
}
// Deliberately bypass typed validation to distinguish every framed field.
std::string Wire( const State& value, size_t index = SIZE_MAX, int64_t replacement = 0 ) {
	types::Buffer b; size_t n = 0; const auto write = [ & ]( int64_t v ) { b.WriteInt( n++ == index ? replacement : v ); };
	write( 0x534d5344 ); write( 1 ); write( value.global_difficulty ); write( value.owners.size() );
	for ( const auto& owner : value.owners ) {
		write( owner.simulation_id ); write( owner.starting_scout ); write( owner.designs.size() );
		for ( const auto& d : owner.designs ) {
			write( d.id ); write( d.chassis ); write( d.weapon ); write( d.defense ); write( d.reactor ); write( d.cost ); write( d.plan ); write( d.carry );
			write( d.abilities ); write( d.faction_exclusion ); b.WriteBool( d.active ); b.WriteBool( d.prototyped );
		}
	}
	return b.ToString();
}
}

void AddSmacStartupDesignStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC startup designs separate generation from marking for explicit owners and difficulty", GT() {
		const auto units = smac_startup_fixture::Units(); const auto technologies = smac_rules_fixture::Technologies();
		const char* names[] = { "Citizen", "Specialist", "Talent", "Librarian", "Thinker", "Transcend" };
		for ( int32_t difficulty = 0; difficulty < 6; ++difficulty ) {
			GT_ASSERT( State::OriginalDifficulty( names[ difficulty ] ) == difficulty, "canonical difficulty mapping changed" );
			for ( int32_t id = 1; id <= 7; ++id ) {
				for ( const auto& binding : settings::SmacFactionStartupRules::Bindings() ) {
					const auto tech = Technologies( { { id, binding.port_id } } );
					auto owner = State::GenerateEmptyOwner( tech.factions[ 0 ], technologies, units.catalog, difficulty );
					const bool morgan = std::string( binding.port_id ) == "MORGANITES", sparta = std::string( binding.port_id ) == "SPARTANS";
					GT_ASSERT( owner.simulation_id == id && owner.starting_scout == 2 && owner.designs.size() == ( morgan || sparta ? 1u : 0u ), "generation inferred a slot or selected before marking" );
					if ( !owner.designs.empty() ) {
						const auto& d = owner.designs[ 0 ];
						GT_ASSERT( d.id == 64 * id && d.chassis == ( sparta ? 1u : 0u ) && d.weapon == 0 && d.defense == ( morgan ? 1u : 0u )
							&& d.reactor == 1 && !d.abilities && d.cost == 2 && d.plan == ( sparta ? 3 : 2 ) && !d.carry && d.active
							&& d.prototyped == ( difficulty <= 1 ) && !d.faction_exclusion, "generated component/cache or premark state differs" );
					}
					State::MarkAndSelect( owner, units.catalog );
					GT_ASSERT( owner.starting_scout == ( sparta ? 64 * id : 2 ) && ( owner.designs.empty() || owner.designs[ 0 ].prototyped ), "startup marking/selection differs" );
				}
			}
		}
		for ( const auto* name : { "Captain", "citizen", "", "7" } ) { GT_ASSERT( Rejects( [ & ]() { State::OriginalDifficulty( name ); } ), "unknown difficulty identity accepted" ); }
		for ( const auto difficulty : { INT32_MIN, -1, 6, INT32_MAX } ) { GT_ASSERT( Rejects( [ & ]() { State::NewWorld( Technologies( { { 1, "MORGANITES" } } ), technologies, units, difficulty ); } ), "invalid original difficulty accepted" ); }
		GT_OK();
	} );
	task->AddTest( "SMAC startup design admission rejects extra ownership and changed capabilities without cosmetic guesses", GT() {
		const auto technology = Technologies( { { 4, "MORGANITES" } } ).factions[ 0 ]; const auto tech = smac_rules_fixture::Technologies();
		const auto original = smac_startup_fixture::Units().catalog;
		for ( size_t scenario = 0; scenario < 13; ++scenario ) {
			auto catalog = original; auto state = technology;
			if ( scenario == 0 ) { state.owned[ 88 ] = true; }
			if ( scenario == 1 ) { state.owned[ 1 ] = false; }
			if ( scenario == 2 ) { catalog.chassis.chassis[ 2 ].prerequisite = "None"; }
			if ( scenario == 3 ) { catalog.weapons.weapons[ 1 ].prerequisite = "None"; }
			if ( scenario == 4 ) { catalog.defenses.defenses[ 1 ].prerequisite = "Unknown"; }
			if ( scenario == 5 ) { catalog.reactors.reactors[ 1 ].prerequisite = "Tech1"; }
			if ( scenario == 6 ) { catalog.abilities.abilities[ 0 ].prerequisite = "None"; }
			if ( scenario == 7 ) { catalog.chassis.chassis[ 0 ].speed = 2; }
			if ( scenario == 8 ) { catalog.defenses.defenses[ 1 ].rating = 3; }
			if ( scenario == 9 ) { catalog.weapons.weapons[ 0 ].offense = 2; }
			if ( scenario == 10 ) { catalog.units[ 2 ].plan = 0; }
			if ( scenario == 11 ) { state.admission.faction_id = "Other"; }
			if ( scenario == 12 ) { state.admission.simulation_id = 8; }
			GT_ASSERT( Rejects( [ & ]() { State::GenerateEmptyOwner( state, tech, catalog, 3 ); } ), "unsupported startup capability admitted " + std::to_string( scenario ) );
		}
		auto compatible = original;
		compatible.chassis.chassis[ 0 ].names[ 0 ].name = "Cosmetic"; compatible.defenses.defenses[ 1 ].name = "Another name";
		compatible.defenses.defenses[ 1 ].cost = 2000;
		const auto expected_cost = unit::SmacPredefinedUnitCatalog::CalculateCost( compatible.chassis.chassis[ 0 ], compatible.weapons.weapons[ 0 ], compatible.defenses.defenses[ 1 ], 1, 0, compatible.abilities );
		const auto generated = State::GenerateEmptyOwner( technology, tech, compatible, 3 );
		GT_ASSERT( expected_cost > 255 && generated.designs[ 0 ].cost == static_cast< uint8_t >( expected_cost ), "startup used a hardcoded cost or cosmetic classifier" );
		GT_OK();
	} );
	task->AddTest( "SMAC startup design codec owns cached and inactive records without replay", GT() {
		const auto units = smac_startup_fixture::Units(); const auto expected = Saved();
		auto value = State::Deserialize( expected.Serialize( units.catalog ).ToString(), units.catalog );
		GT_ASSERT( value == expected && value.Find( 7 )->designs[ 0 ].cost == 255 && !value.Find( 7 )->designs[ 0 ].active && !value.Find( 7 )->designs[ 0 ].prototyped, "restore regenerated cached or prototype state" );
		auto copied = value; auto moved = std::move( copied ); value.owners.clear();
		GT_ASSERT( moved == expected && State::Deserialize( Wire( moved ), units.catalog ) == expected, "design state borrowed prior containers or wire" );
		GT_OK();
	} );
	task->AddTest( "SMAC startup design codec rejects bounded malformed fields and foreign selection", GT() {
		const auto catalog = smac_startup_fixture::Units().catalog; const auto saved = Saved();
		for ( size_t index = 0; index < 17; ++index ) {
			for ( const auto value : { INT64_MIN, int64_t( -1 ), INT64_MAX } ) {
				GT_ASSERT( Rejects( [ & ]() { State::Deserialize( Wire( saved, index, value ), catalog ); } ), "out-of-range design field accepted" );
			}
		}
		for ( const auto& item : std::vector< std::pair< size_t, int64_t > >{ {1,2}, {2,6}, {3,8}, {4,0}, {5,64}, {6,65}, {7,447}, {8,9}, {9,21}, {10,10}, {11,0}, {11,5}, {12,256}, {13,256}, {14,256}, {15,0x1000000}, {16,int64_t(UINT32_MAX)+1} } ) {
			GT_ASSERT( Rejects( [ & ]() { State::Deserialize( Wire( saved, item.first, item.second ), catalog ); } ), "invalid bounded design field accepted" );
		}
		const auto wire = Wire( saved );
		for ( size_t i = 0; i < wire.size(); ++i ) { GT_ASSERT( Rejects( [ & ]() { State::Deserialize( wire.substr( 0, i ), catalog ); } ), "truncated design record accepted" ); }
		GT_ASSERT( Rejects( [ & ]() { State::Deserialize( wire + "x", catalog ); } ) && Rejects( [ & ]() { State::Deserialize( std::string( State::MAX_WIRE_BYTES + 1, 'x' ), catalog ); } ), "trailing or over-budget design payload accepted" );
		for ( size_t scenario = 0; scenario < 5; ++scenario ) {
			auto bad = saved;
			if ( scenario == 0 ) { bad.owners.push_back( bad.owners[ 0 ] ); }
			if ( scenario == 1 ) { bad.owners[ 0 ].designs.push_back( bad.owners[ 0 ].designs[ 0 ] ); }
			if ( scenario == 2 ) { bad.owners[ 0 ].starting_scout = 128; }
			if ( scenario == 3 ) { bad.owners[ 0 ].designs[ 0 ].id = 511; bad.owners[ 0 ].starting_scout = 448; }
			if ( scenario == 4 ) { bad.owners.clear(); }
			GT_ASSERT( Rejects( [ & ]() { bad.Serialize( catalog ); } ), "invalid design ownership serialized" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC startup marking keeps ascending active Recon selection and skips inactive records", GT() {
		const auto catalog = smac_startup_fixture::Units().catalog;
		auto owner = Saved().owners[ 1 ]; auto second = owner.designs[ 0 ]; second.id += 3; owner.designs.push_back( second );
		auto inactive = second; inactive.id += 1; inactive.active = false; inactive.prototyped = false; owner.designs.push_back( inactive );
		State::MarkAndSelect( owner, catalog );
		GT_ASSERT( owner.starting_scout == second.id && owner.designs[ 0 ].prototyped && owner.designs[ 1 ].prototyped && !owner.designs[ 2 ].prototyped, "marking changed ordered/active selection semantics" );
		GT_OK();
	} );
}
}
