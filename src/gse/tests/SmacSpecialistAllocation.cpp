#include "SmacSpecialistAllocation.h"
#include "game/backend/base/SmacSpecialistAllocation.h"
#include "game/backend/base/SmacSpecialists.h"
#include "fixtures/SmacSpecialistAllocation.h"
#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Allocation = SmacSpecialistAllocation;
namespace fixture = smac_specialist_allocation_fixture;
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
SmacSpecialistAllocationCatalog Stock() {
	return { 5, {{{3,0,0,true,false},{0,2,0,true,false},{0,0,3,false,false},{3,0,2,false,false},{2,2,0,false,false},{0,1,3,false,false},{2,2,4,false,false}}} };
}
SmacSpecialistBookkeeping State( int32_t retained = 0, int32_t provisional = 0, uint8_t padding = 3 ) {
	SmacSpecialistBookkeeping result{ {}, retained, provisional }; result.types.fill( padding ); return result;
}
}

void AddSmacSpecialistAllocationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC specialist allocation matches seventeen original cleanup projections", GT() {
		for ( const auto& row : fixture::cleanup ) {
			const auto split = Allocation::Split( row.input, row.catalog, row.managed, row.upkeep, row.overflow );
			GT_ASSERT( split.retained == row.split_r && split.provisional == row.split_q && split.types == row.input.types, "original split: " + std::string( row.id ) );
			const auto result = Allocation::Compact( split, row.catalog, row.managed, row.overflow );
			GT_ASSERT( result.retained == row.final_r && result.provisional == row.final_q, "original compaction counts: " + std::string( row.id ) );
			for ( size_t i = 0; i < row.prefix_count; ++i ) { GT_ASSERT( result.types[ i ] == row.prefix[ i ], "original compaction prefix: " + std::string( row.id ) ); }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation matches twenty eight original competition projections", GT() {
		for ( const auto& row : fixture::competition ) {
			const auto choice = Allocation::Compete( row.catalog, row.context );
			GT_ASSERT( ( choice.type ? int32_t( *choice.type ) : -1 ) == row.winner && choice.score == row.score, "original competition: " + std::string( row.id ) );
			const auto result = choice.type ? Allocation::Insert( row.input, *choice.type, row.workers, row.context.population, row.context.nutrients_per_citizen ) : SmacSpecialistInsertion{row.input};
			GT_ASSERT( result.inserted == row.inserted && result.rescan == row.rescan && result.specialists.provisional == row.final_q && result.specialists.retained == row.input.retained && result.specialists.types == row.expected, "original insertion: " + std::string( row.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation preserves seven observed competition and final fill histories", GT() {
		const auto catalog = Stock();
		for ( const auto& row : fixture::finalization ) {
			auto state = State( row.retained, 0, 1 );
			const SmacWorkerStepState workers{ 1, {row.nutrients,1,1}, row.population, false };
			int32_t insertions = 0;
			// These fixtures have already collected center and have two legal
			// zero-yield candidates. This composes specialist stages only.
			while ( workers.remaining_slots > state.retained + state.provisional ) {
				const auto choice = Allocation::Compete( catalog, { row.managed,row.population,row.turn,2,row.nutrients,1,state.provisional,0,0,0,0 } );
				if ( !choice.type ) { break; }
				const auto inserted = Allocation::Insert( state, *choice.type, workers, row.population, 2 );
				GT_ASSERT( inserted.inserted, "fixture composition did not progress" ); state = inserted.specialists; ++insertions;
				if ( !inserted.rescan ) { break; }
			}
			GT_ASSERT( insertions == row.insertions && state.retained == row.before_r && state.provisional == row.before_q, "original prefill: " + std::string( row.id ) );
			const auto final = Allocation::Fill( state, workers.remaining_slots, catalog, row.population );
			GT_ASSERT( final.total_count == row.final_count, "original final count: " + std::string( row.id ) );
			for ( size_t i = 0; i < row.prefix_count; ++i ) { GT_ASSERT( final.types[ i ] == row.prefix[ i ], "original final prefix: " + std::string( row.id ) ); }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation resolves owned catalog availability through existing helper", GT() {
		using namespace game::backend::settings;
		auto tech = smac_rules_fixture::Technologies();
		SmacSpecialistRules rules{5,{}};
		for ( int32_t id = 0; id < 7; ++id ) { rules.specialists[id] = {id,"Repeated","Repeated","None","Disable",3,2,1,"opaque"}; }
		std::array<bool,89> owned{}; owned[0] = owned[87] = owned[88] = true;
		rules.specialists[0].prerequisite = tech.technologies[0].key;
		rules.specialists[1].prerequisite = tech.technologies[87].key;
		rules.specialists[2].prerequisite = tech.technologies[88].key;
		rules.specialists[3].obsolete = tech.technologies[87].key;
		tech.technologies[0].prerequisites = {"Disable","None"};
		tech.technologies[87].prerequisites = {"None","Disable"};
		const auto resolved = ResolveSmacSpecialistAllocationCatalog( rules, tech, owned );
		GT_ASSERT( !resolved.rows[0].prerequisite_available && resolved.rows[1].prerequisite_available && !resolved.rows[2].prerequisite_available && resolved.rows[3].obsolete_available, "ordinary definition guard lost" );
		owned.fill(false); rules.specialists[1].economy = -77;
		const auto changed = ResolveSmacSpecialistAllocationCatalog( rules, tech, owned );
		GT_ASSERT( !changed.rows[1].prerequisite_available && !changed.rows[3].obsolete_available && resolved.rows[1].economy == 3, "resolved result borrowed or cached ownership" );
		rules.specialists[6].obsolete = "UnknownKey";
		GT_ASSERT( Rejects( [&] { ResolveSmacSpecialistAllocationCatalog(rules,tech,owned); } ), "unknown late catalog token admitted" );
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation retains explicit padding and bounded overflow ownership", GT() {
		const auto catalog = Stock();
		auto state = State( 16, 0, 1 ); state.types[15] = 0;
		GT_ASSERT( Rejects( [&] { Allocation::Compact(state,catalog,true,{}); } ), "one-past active overflow read silently defaulted" );
		const auto compacted = Allocation::Compact(state,catalog,true,1);
		GT_ASSERT( compacted.types[15] == 1 && compacted.retained == 16 && compacted.provisional == 0 && state.types[15] == 0, "padding import or owned input changed" );
		const auto skipped = Allocation::Compact(state,catalog,false,{});
		GT_ASSERT( skipped.types == state.types, "unmanaged compaction ran" );
		auto slots = SmacSpecialistSlots{state.types,127};
		GT_ASSERT( Rejects( [&] { Allocation::Split(slots,catalog,true,false,{}); } ), "overflow split silently defaulted" );
		const auto split = Allocation::Split(slots,catalog,true,false,0);
		GT_ASSERT( split.retained == 15 && split.provisional == 112, "127 count boundary" );
		const auto upkeep = Allocation::Split(slots,catalog,true,true,{});
		GT_ASSERT( upkeep.retained == 127 && upkeep.provisional == 0, "upkeep unexpectedly queried overflow" );
		auto full = State(0,127); const auto old = full.types;
		const auto filled = Allocation::Fill(full,127,catalog,127);
		GT_ASSERT( filled.total_count == 127 && filled.types[15] == 1 && full.types == old, "bounded final writes or detached ownership" );
		for ( auto bad : {State(-1),State(128),State(127,1)} ) { GT_ASSERT( Rejects([&] { Allocation::Compact(bad,catalog,true,1); }), "invalid count admitted" ); }
		state.types[15] = 7;
		GT_ASSERT( Rejects([&] { Allocation::Compact(state,catalog,false,{}); }), "malformed unused slot admitted" );
		GT_ASSERT( Rejects([&] { Allocation::Fill(State(),128,catalog,127); }), "128 final count admitted" );
		GT_ASSERT( Rejects([&] { Allocation::Insert(State(),7,{0,{},1,false},5,2); }), "invalid winner admitted" );
		GT_ASSERT( Rejects([&] { Allocation::Insert(State(),0,{0,{},1,false},5,0); }), "invalid consumption admitted" );
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation final fill has distinct grouped arithmetic and admission", GT() {
		auto catalog = Stock(); for ( auto& row : catalog.rows ) { row.prerequisite_available = false; }
		catalog.rows[0] = {INT32_MAX,1,-3,true,true};
		std::optional< SmacSpecialistSlots > grouped;
		try { grouped = Allocation::Fill(State(),1,catalog,5); } catch ( const std::exception& ) {}
		GT_ASSERT( grouped && grouped->types[0] == 0, "grouped economy plus labs rejected or obsolete filtered" );
		catalog.rows[0] = {INT32_MAX,-1,3,true,false};
		GT_ASSERT( Rejects([&] { Allocation::Fill(State(),1,catalog,5); }), "grouped intermediate overflow hidden by final sum" );
		catalog.rows[0] = {INT32_MAX,1,INT32_MAX,true,false};
		GT_ASSERT( Allocation::Fill(State(),1,catalog,4).types[0] == 0, "unused below-size output evaluated" );
		catalog.rows[0] = {0,0,0,true,false};
		GT_ASSERT( Rejects([&] { Allocation::Fill(State(),1,catalog,5); }), "unproved nonpositive final fill admitted" );
		catalog.rows[0].prerequisite_available = false;
		GT_ASSERT( Rejects([&] { Allocation::Fill(State(),1,catalog,5); }), "unproved unavailable final fallback admitted" );
		const auto no_fill = Allocation::Fill(State(1,0,4),0,catalog,5);
		GT_ASSERT( no_fill.total_count == 0 && no_fill.types[0] == 4, "no-fill path evaluated selector or erased padding" );
		catalog = Stock(); catalog.rows[1] = catalog.rows[0];
		GT_ASSERT( Allocation::Fill(State(),1,catalog,5).types[0] == 0, "later final-fill tie replaced earlier type" );
		GT_OK();
	} );
	task->AddTest( "SMAC specialist allocation checks evaluated competition instruction boundaries", GT() {
		auto catalog = Stock();
		SmacSpecialistCompetitionContext c{true,5,101,2,12,10,0,0,0,0,0};
		catalog.rows[0].psych = INT32_MAX;
		GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "psych shift overflow admitted" );
		c.managed = false;
		GT_ASSERT( !Allocation::Compete(catalog,c).type, "inactive competition evaluated unused outputs" );
		c.managed = true; catalog = Stock(); c.current_energy = INT32_MAX; c.provisional = 1;
		GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "denominator overflow clamped before addition" );
		c.current_energy = 10; c.provisional = 0; c.build_priority = INT32_MAX;
		GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "priority overflow hidden by later product" );
		c.build_priority = 0; c.maximum_candidate_nutrients = -1;
		GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "negative candidate maximum admitted" );
		// Preserve original demand -> candidate maximum -> turn order. These
		// extreme products test port rejection, not original overflow behavior.
		c = {true,127,100,INT32_MAX,255,10,0,0,0,0,37};
		for ( const auto turn : {100,101} ) {
			c.turn = turn;
			GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "turn gate hid evaluated demand overflow" );
		}
		c.turn = 100; c.nutrients_per_citizen = 2; catalog.rows[0].psych = INT32_MAX;
		const auto early = Allocation::Compete(catalog,c);
		GT_ASSERT( !early.type && early.score == 37, "early turn evaluated specialist scoring or changed tile choice" );
		c.turn = 101;
		GT_ASSERT( Rejects([&] { Allocation::Compete(catalog,c); }), "later turn skipped evaluated specialist scoring" );
		// A full specialist budget ends even with a deficit; insertion does not
		// evaluate its short-circuited demand when another vacancy forces rescan.
		GT_ASSERT( !Allocation::Insert(State(1),0,{0,{-999,0,0},1,false},127,INT32_MAX).rescan, "full budget rescanned" );
		GT_ASSERT( Allocation::Insert(State(),0,{0,{},2,false},127,INT32_MAX).rescan, "vacancy evaluated unused overflow demand" );
		GT_ASSERT( Rejects([&] { Allocation::Insert(State(),0,{0,{},1,false},127,INT32_MAX); }), "evaluated demand overflow admitted" );
		GT_OK();
	} );
}
}
