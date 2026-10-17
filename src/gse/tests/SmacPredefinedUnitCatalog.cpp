#include "SmacPredefinedUnitCatalog.h"
#include "game/backend/unit/SmacPredefinedUnitCatalog.h"
#include "loader/txt/UnitRulesTXTLoader.h"
#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <utility>

namespace gse::tests {
namespace {
using namespace loader::txt;
using Catalog = game::backend::unit::SmacPredefinedUnitCatalog;

struct Inputs {
	UnitTable units{ "smac", {} };
	ChassisTable chassis{ "smac", {} };
	WeaponTable weapons{ "smac", {} };
	DefenseTable defenses{ "smac", {} };
	ReactorTable reactors{ "smac", {} };
	AbilityTable abilities{ "smac", {} };
	game::backend::settings::SmacTechnologyRules technologies = smac_rules_fixture::Technologies();
	Inputs() {
		const int32_t speeds[] = { 1, 2, 3, 4, 6, 8, 8, 8, 12 };
		for ( size_t i = 0; i < 9; ++i ) {
			ChassisComponent c{};
			for ( auto& name : c.names ) { name = { "Body" + std::to_string( i ), "N1" }; }
			c.speed = speeds[ i ]; c.cost = speeds[ i ]; c.triad = i < 3 ? ChassisTriad::Land : i < 5 ? ChassisTriad::Sea : ChassisTriad::Air;
			c.cargo = i == 3 ? 2 : i == 4 ? 4 : i == 8 ? 0 : 1; c.missile = i == 8; c.prerequisite = "None";
			chassis.chassis.push_back( c );
		}
		for ( size_t i = 0; i < 21; ++i ) { weapons.weapons.push_back( { "Tool" + std::to_string( i ), "W" + std::to_string( i ), 1, WeaponMode::Projectile, 1, -1, "None" } ); }
		const int32_t costs[] = { 10, 6, 4, 8, 4, 36 };
		const WeaponMode modes[] = { WeaponMode::Colonist, WeaponMode::Terraformer, WeaponMode::Transport, WeaponMode::Convoy, WeaponMode::InfoWar, WeaponMode::Artifact };
		for ( size_t i = 0; i < 6; ++i ) { auto& w = weapons.weapons[ i + 14 ]; w.mode = modes[ i ]; w.offense = 0; w.cost = costs[ i ]; }
		weapons.weapons[ 12 ].offense = -1; weapons.weapons[ 12 ].mode = WeaponMode::Missile; weapons.weapons[ 12 ].cost = 10;
		for ( size_t i = 0; i < 10; ++i ) { defenses.defenses.push_back( { "Shield" + std::to_string( i ), "D" + std::to_string( i ), 1, DefenseMode::Projectile, 1, "None" } ); }
		defenses.defenses[ 9 ].rating = -1; defenses.defenses[ 9 ].mode = DefenseMode::Binary; defenses.defenses[ 9 ].cost = 6;
		for ( int32_t i = 0; i < 4; ++i ) { reactors.reactors.push_back( { "Core" + std::to_string( i ), "R" + std::to_string( i ), i + 1, "None" } ); }
		for ( size_t i = 0; i < 24; ++i ) { abilities.abilities.push_back( { "Ability" + std::to_string( i ), 0, "None", "", "00000000000", "Synthetic" } ); }
		Add( 0, 14, 0, 8 ); Add( 0, 15, 0, 9 ); Add( 0, 0, 0, 3 );
		units.units[ 1 ].prerequisite = "Tech2";
	}
	void Add( const size_t c, const size_t w, const size_t d, const int32_t plan, const uint32_t mask = 0 ) {
		std::string binary( 24, '0' );
		for ( size_t i = 0; i < 24; ++i ) { if ( mask & ( 1u << i ) ) { binary[ 23 - i ] = '1'; } }
		units.units.push_back( { "Synthetic design", chassis.chassis[ c ].names[ 0 ].name, weapons.weapons[ w ].short_name, defenses.defenses[ d ].short_name,
			static_cast< UnitPlan >( plan ), 0, 0, "None", -1, binary } );
	}
	Catalog Build() const { return Catalog::Build( units, chassis, weapons, defenses, reactors, abilities, technologies ); }
};

bool Rejects( const std::function< void() >& action ) { try { action(); } catch ( const std::invalid_argument& ) { return true; } return false; }
}

void AddSmacPredefinedUnitCatalogTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC predefined catalog owns positional components and produces actual basic costs", GT() {
		Catalog saved;
		{
			Inputs input; saved = input.Build();
			input.chassis.chassis[ 0 ].names[ 0 ].name = "changed"; input.units.units.clear();
		}
		GT_ASSERT( saved.units.size() == 3 && saved.source.units.size() == 3 && saved.chassis.chassis[ 0 ].names[ 0 ].name == "Body0", "catalog borrowed source storage" );
		const uint8_t costs[] = { 3, 2, 1 }, automatic[] = { 8, 9, 0 }, plans[] = { 8, 9, 3 };
		for ( size_t i = 0; i < 3; ++i ) {
			const auto& unit = saved.units[ i ];
			GT_ASSERT( unit.source_id == i && unit.chassis_id == 0 && unit.defense_id == 0 && unit.reactor == 1 && unit.abilities == 0, "source or component identity changed" );
			GT_ASSERT( unit.calculated_cost == costs[ i ] && unit.cost == costs[ i ] && unit.automatic_plan == automatic[ i ] && unit.plan == plans[ i ], "computed cost or override order changed" );
			GT_ASSERT( !unit.carry && !unit.calculated_carry && unit.icon == 255 && unit.active && !unit.prototyped && !unit.faction_exclusion, "builder initial state differs" );
		}
		using Kind = game::backend::settings::SmacTechnologyReference::Kind;
		GT_ASSERT( saved.units[ 0 ].prerequisite.kind == Kind::NONE && saved.units[ 1 ].prerequisite.kind == Kind::TECHNOLOGY_ID && saved.units[ 1 ].prerequisite.technology_id == 2, "named prerequisites were not resolved" );
		auto copy = saved; auto moved = std::move( copy ); saved.source.units[ 0 ].name = "changed";
		GT_ASSERT( moved.source.units[ 0 ].name == "Synthetic design" && moved.units[ 2 ].weapon_id == 0, "copy/move lost independent metadata" );
		GT_OK();
	} );
	task->AddTest( "SMAC predefined loader applies nonzero overrides after computed byte caches", GT() {
		Inputs input;
		auto& row = input.units.units[ 2 ]; row.plan = static_cast< UnitPlan >( 256 ); row.cost = 256; row.carry = 256; row.icon = 256;
		auto result = input.Build().units[ 2 ];
		GT_ASSERT( result.calculated_cost == 1 && result.automatic_plan == 0 && !result.plan && !result.cost && !result.carry && !result.icon, "nonzero overrides were confused with computed zero" );
		row.plan = UnitPlan::AutoCalculate; row.cost = -1; row.carry = -1; row.icon = -1;
		result = input.Build().units[ 2 ];
		GT_ASSERT( result.plan == 0 && result.cost == 255 && result.carry == 255 && result.icon == 255, "signed byte override differs" );
		row.cost = 0; row.carry = 0; input.weapons.weapons[ 0 ].cost = 2000;
		GT_ASSERT( Catalog::CalculateCost( input.chassis.chassis[ 0 ], input.weapons.weapons[ 0 ], input.defenses.defenses[ 0 ], 1, 0, input.abilities ) == 501
			&& input.Build().units[ 2 ].calculated_cost == 245 && input.Build().units[ 2 ].cost == 245, "computed cost did not cache low eight bits" );
		GT_ASSERT( Rejects( []() { UnitRulesTXTLoader::ParseUnits( "#UNITS\n1\nSynthetic,Body,W,D,256,0,0,None,-1,0\n#END\n", "smac" ); } ), "typed plan override test silently widened text parser policy" );
		GT_OK();
	} );
	task->AddTest( "SMAC predefined catalog retains all fourteen ordered synthetic designs", GT() {
		Inputs input;
		input.abilities.abilities[ 5 ].cost = 1;
		input.Add( 3, 16, 0, 7 ); input.Add( 3, 15, 0, 9 ); input.Add( 0, 17, 0, 10 );
		input.Add( 1, 18, 0, 11 ); input.Add( 0, 19, 0, 12 ); input.Add( 0, 12, 9, 1 );
		input.Add( 3, 12, 9, 7 ); input.Add( 7, 12, 9, 4, 1u << 5 ); input.Add( 1, 0, 0, 3 );
		input.Add( 6, 0, 0, 4, 1u << 5 ); input.Add( 3, 16, 0, 7, 1u << 23 );
		input.units.units[ 7 ].cost = 10; input.units.units[ 7 ].icon = 2;
		input.units.units[ 8 ].cost = 5; input.units.units[ 9 ].cost = 8; input.units.units[ 9 ].carry = 4;
		input.units.units[ 10 ].cost = 10;
		const auto result = input.Build();
		const uint8_t costs[] = { 3, 2, 1, 3, 4, 3, 3, 10, 11, 11, 18, 2, 3, 3 };
		const uint8_t automatic[] = { 8, 9, 0, 7, 9, 10, 11, 12, 0, 6, 4, 3, 4, 7 };
		GT_ASSERT( result.units.size() == 14, "duplicate display names collapsed source rows" );
		for ( size_t i = 0; i < result.units.size(); ++i ) {
			const auto& row = result.units[ i ];
			GT_ASSERT( row.source_id == i && row.calculated_cost == costs[ i ] && row.automatic_plan == automatic[ i ], "ordered design computation changed at " + std::to_string( i ) );
			GT_ASSERT( result.chassis.chassis[ row.chassis_id ].names[ 0 ].name == result.source.units[ i ].chassis
				&& result.weapons.weapons[ row.weapon_id ].short_name == result.source.units[ i ].weapon
				&& result.defenses.defenses[ row.defense_id ].short_name == result.source.units[ i ].defense, "owned component indices no longer identify the source references" );
		}
		GT_ASSERT( result.units[ 3 ].calculated_carry == 2 && result.units[ 13 ].calculated_carry == 1
			&& result.units[ 9 ].calculated_carry == 0 && result.units[ 9 ].carry == 4, "transport cache and explicit nontransport override were conflated" );
		GT_ASSERT( result.units[ 8 ].cost == 5 && result.units[ 9 ].cost == 8 && result.units[ 10 ].cost == 10
			&& result.units[ 7 ].icon == 2, "later row overrides changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC unit cost helpers preserve ordered abilities land flags and domain arithmetic", GT() {
		Inputs input; auto c = input.chassis.chassis[ 0 ]; auto w = input.weapons.weapons[ 0 ]; auto d = input.defenses.defenses[ 0 ];
		c.cost = 4; w.cost = 2; d.cost = 3;
		const int32_t expected[] = { 6, 8, 9, 11, 11, 12, 14 };
		for ( int32_t code = -1; code >= -7; --code ) {
			input.abilities.abilities[ 0 ].cost = code;
			GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 1, input.abilities ) == expected[ -code - 1 ], "negative ability cost code changed" );
		}
		c.cost = w.cost = d.cost = 1; input.abilities.abilities[ 0 ].cost = 0; input.abilities.abilities[ 1 ].cost = 1;
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 3, input.abilities ) == 1, "zero contribution first incorrectly triggers stacking" );
		std::swap( input.abilities.abilities[ 0 ].cost, input.abilities.abilities[ 1 ].cost );
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 3, input.abilities ) == 2, "table-order stacking was lost" );
		input.abilities.abilities[ 0 ].cost = 0; w.cost = d.cost = 2;
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 1, input.abilities ) == 3, "land control differs" );
		input.abilities.abilities[ 0 ].flags = "10000000000";
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 1, input.abilities ) == 4, "land rule flag was not quarter-cost input" );
		c.triad = ChassisTriad::Sea;
		const auto sea = Catalog::CalculateCost( c, w, d, 1, 1, input.abilities ); input.abilities.abilities[ 0 ].flags = "0";
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 1, input.abilities ) == sea, "land flag affected sea" );
		c.missile = true; w.offense = 99; w.cost = 32;
		GT_ASSERT( Catalog::CalculateCost( c, w, d, 1, 3, input.abilities ) == 32, "missile offense shortcut changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC transport capacity uses sea reactor then Slow before Heavy and preserves package gate", GT() {
		Inputs input; auto& c = input.chassis.chassis[ 3 ]; const uint32_t slow = 1u << 23, heavy = 1u << 19;
		GT_ASSERT( Catalog::CalculateCapacity( c, 3, slow | heavy ) == 5, "capacity operation order changed" );
		GT_ASSERT( Catalog::CalculateCapacity( input.chassis.chassis[ 0 ], 4, heavy ) == 2 && Catalog::CalculateCapacity( input.chassis.chassis[ 0 ], 4, slow | heavy ) == 0, "land capacity used reactor or wrong order" );
		input.Add( 3, 16, 0, 7, heavy ); c.cargo = 255;
		GT_ASSERT( Catalog::CalculateCapacity( c, 1, heavy ) == 383 && input.Build().units.back().calculated_carry == 127, "capacity byte cache changed" );
		input.units.units.pop_back(); input.Add( 3, 0, 0, 7, slow | heavy );
		const auto result = input.Build().units.back();
		GT_ASSERT( result.automatic_plan == 6 && !result.calculated_carry && !result.carry, "final plan or ability mask substituted for transport package" );
		GT_OK();
	} );
	task->AddTest( "SMAC predefined catalog rejects unsupported identities and retains Unknown prerequisites", GT() {
		using Kind = game::backend::settings::SmacTechnologyReference::Kind;
		Inputs input; input.units.units[ 0 ].prerequisite = "Unseen"; input.units.units[ 1 ].prerequisite = "Disable";
		const auto catalog = input.Build();
		GT_ASSERT( catalog.units[ 0 ].prerequisite.kind == Kind::UNKNOWN && catalog.units[ 1 ].prerequisite.kind == Kind::DISABLED, "Unknown was treated as Disable" );
		for ( int bad = 0; bad < 12; ++bad ) {
			Inputs changed;
			switch ( bad ) {
				case 0: changed.units.ruleset_id = "smax"; break;
				case 1: changed.abilities.ruleset_id = "smax"; break;
				case 2: changed.reactors.reactors.clear(); break;
				case 3: changed.units.units[ 0 ].weapon = "missing"; break;
				case 4: changed.weapons.weapons[ 1 ].short_name = changed.weapons.weapons[ 0 ].short_name; break;
				case 5: std::swap( changed.chassis.chassis[ 0 ], changed.chassis.chassis[ 1 ] ); break;
				case 6: changed.chassis.chassis[ 0 ].speed = 2; break;
				case 7: changed.units.units[ 2 ].ability_bits = "10"; break;
				case 8: changed.units.units[ 0 ].ability_bits = std::string( 25, '0' ); break;
				case 9: changed.abilities.abilities[ 0 ].cost = -8; break;
				case 10: changed.units.units.resize( 65, changed.units.units[ 0 ] ); break;
				case 11: changed.weapons.weapons[ 0 ].cost = 0; break;
			}
			GT_ASSERT( Rejects( [ & ]() { changed.Build(); } ), "unsupported catalog input accepted" );
		}
		GT_ASSERT( Rejects( [ & ]() { Catalog::CalculateCapacity( input.chassis.chassis[ 0 ], 0, 0 ); } ), "zero reactor accepted" );
		input.chassis.chassis[ 0 ].cost = INT32_MAX;
		GT_ASSERT( Rejects( [ & ]() { input.Build(); } ), "overflowing cost arithmetic accepted" );
		GT_OK();
	} );
}
}
