#include "SmacScoutCombat.h"
#include "game/ScoutBattlePresentation.h"
#include "game/FrontendRequest.h"

#include "game/backend/unit/SmacScoutCombat.h"
#include "fixtures/SmacScoutCombat.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

namespace gse::tests {
namespace {
namespace model = game::backend::unit::smac_scout;
namespace fixture = fixtures::smac_scout;
using game::backend::SmacGameplayRandomState;
constexpr model::UnitId ATTACKER = UINT64_C( 0xfedcba9876543210 );
constexpr model::UnitId DEFENDER = UINT64_C( 0x123456789abcdef0 );

model::Participant Participant( const fixture::Input& i, const size_t side, const model::UnitId id ) {
	return { id, i.reactors[ side ], i.health[ side ], i.morale[ side ], i.morale[ side ], side == 0 ? i.spent : 0 };
}
model::AttackResult Resolve( const fixture::Input& i ) {
	return model::ResolveNeutralFlatAttack( Participant( i, 0, ATTACKER ), Participant( i, 1, DEFENDER ), i.allowance, { i.shared, i.crt }, { ATTACKER, DEFENDER } );
}
bool Rejects( const std::function< void() >& f ) {
	try { f(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
std::optional< model::UnitId > SurvivingReference( const int original_index, const model::UnitId survivor ) {
	// The original compacted roster has exactly one survivor. Translate the
	// original reference through that roster, never through a stale index or
	// the separate continuation reference. Invalid fixture indices fail loudly.
	if ( original_index == -1 ) { return std::nullopt; }
	if ( original_index != 0 ) { throw std::logic_error( "invalid singleton fixture index" ); }
	return survivor;
}
bool Singleton( const model::Survivor& s ) { return !s.stack_links[ 0 ] && !s.stack_links[ 1 ]; }
bool StepMatches( const fixture::Step& e, const model::Survivor& s, const model::References& refs, const model::UnitId continuation, const SmacGameplayRandomState& rng, const std::array< bool, 2 >& present, const int original_return ) {
	return s.identity == ATTACKER && s.health == e.health && s.stored_morale == e.morale && s.spent == e.spent &&
		s.position == ( e.position == 0 ? model::Position::Source : model::Position::Destination ) && Singleton( s ) &&
		refs.selected == SurvivingReference( e.selected_index, ATTACKER ) && refs.secondary == SurvivingReference( e.secondary_index, ATTACKER ) &&
		continuation == *SurvivingReference( e.continuation_index, ATTACKER ) && rng.Shared() == e.shared && rng.Crt() == e.crt && present == e.present && original_return == e.wrapper_return;
}
}

void AddSmacScoutCombatTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC Scout playback matches original grouped effects and preserves late frames",GT(){
        namespace av=game::scout_presentation;
        const model::Participant a(ATTACKER,1,10,1,1,0),d(DEFENDER,1,10,1,1,0);
        const auto result=model::ResolveNeutralFlatAttack(a,d,3,{3,3},{ATTACKER,DEFENDER});
        const auto plan=av::Prepare(result);
        const std::array<std::array<int32_t,2>,19> damage{{{{0,1}},{{1,1}},{{1,2}},{{2,2}},{{2,3}},{{2,4}},{{3,4}},{{4,4}},{{5,4}},{{5,5}},{{5,6}},{{6,6}},{{7,6}},{{7,7}},{{7,8}},{{8,8}},{{9,8}},{{9,9}},{{9,10}}}};
        const std::array<size_t,7> effect_rounds{{5,7,11,12,17,18,19}},targets{{1,0,1,0,0,1,1}};
        GT_ASSERT(plan.steps.size()==19 && plan.units[0]==ATTACKER && plan.units[1]==DEFENDER,"Playback lost stable participants or original rounds");
        size_t effect=0;
        for(size_t i=0;i<damage.size();++i) {
            GT_ASSERT((plan.steps[i].health==std::array<int32_t,2>{{10-damage[i][0],10-damage[i][1]}}),"Playback health differs from original timeline");
            if(effect<effect_rounds.size() && i+1==effect_rounds[effect]) {
                GT_ASSERT(plan.steps[i].target==targets[effect] && plan.steps[i].effect==(effect==6 ? av::Effect::Death:av::Effect::Hit),"Playback grouped effect differs from original call");++effect;
            } else {GT_ASSERT(plan.steps[i].effect==av::Effect::None,"Playback invented an effect between original grouped hits");}
        }
        for(const bool fast:{false,true}) {for(const uint64_t tick:{uint64_t(25),uint64_t(100)}) {
            av::Playback playback(plan,fast);size_t health=0,frames=0,clears=0;bool finished=false;uint64_t ended=0;
            for(uint64_t now=0;now<20000 && !finished;now+=tick) {while(const auto action=playback.Next(now)) {
                if(action->kind==av::Playback::Kind::Health) {++health;}
                if(action->kind==av::Playback::Kind::Frame) {GT_ASSERT(action->frame==frames%16,"Late clock skipped or reordered an original frame");++frames;}
                if(action->kind==av::Playback::Kind::Clear) {++clears;}
                if(action->kind==av::Playback::Kind::Finished) {finished=true;ended=now;}
            }}
            GT_ASSERT(finished && health==19 && frames==112 && clears==7 && !playback.Next(30000),"Playback lost health, frame, tail or terminal actions");
            GT_ASSERT(ended==(fast ? (tick==25 ? 800u:1600u):(tick==25 ? 3550u:11900u)),"Playback changed original minimum waits or caught up late frames");
        }}
        game::FrontendRequest request(game::FrontendRequest::FR_SCOUT_BATTLE);request.data.scout_battle.plan=new av::Plan(plan);
        game::FrontendRequest copy(request);request.data.scout_battle.plan->steps[0].health[0]=0;
        GT_ASSERT(copy.data.scout_battle.plan!=request.data.scout_battle.plan && copy.data.scout_battle.plan->steps[0].health[0]==10,"Frontend transport aliased the owning battle timeline");
        GT_OK();
    });

	task->AddTest("SMAC Scout Base garrison matches original17 rounds and sequential bonuses",GT(){
        const model::Participant a(ATTACKER,1,10,1,1,0),d(DEFENDER,1,10,1,1,0);
        const auto result=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},false,{25,25,25});
        // Original execution receipt: damage before, two raw draws, their
        // modulo280 values, then CRT state after the pair.
        const std::array<std::array<uint32_t,7>,17> rounds{{
			{{0,0,51,17945,51,25,3323539102}},
			{{0,1,27159,386,279,106,2172805640}},
			{{0,2,17345,27504,265,64,1802518722}},
			{{0,3,20815,20576,95,136,1348511052}},
			{{1,3,10960,6020,40,140,394542630}},
			{{2,3,5261,28277,221,277,4000656848}},
			{{3,3,24162,930,82,90,2208475850}},
			{{4,3,11002,19737,82,137,1293524884}},
			{{5,3,17913,1682,273,2,110250670}},
			{{5,4,10319,10474,239,114,686481048}},
			{{5,5,1102,28279,262,279,1853301202}},
			{{6,5,16179,21197,219,197,1389170396}},
			{{6,6,11333,12790,133,190,838235190}},
			{{7,6,15811,2607,131,87,170890336}},
			{{7,7,16929,15514,129,114,1016776154}},
			{{7,8,14450,19601,170,1,3432059684}},
			{{7,9,23796,23159,276,199,3665287870}}
        }};
        GT_ASSERT(result.rounds.size()==17 && result.draws.size()==34 && (result.strengths==std::array<int32_t,2>{280,280}),"Base battle strength/draw count differs");
        for(size_t i=0;i<rounds.size();++i) {
            const auto& r=rounds[i];const auto& got=result.rounds[i];
            GT_ASSERT(got.health_before[0]==10-int(r[0]) && got.health_before[1]==10-int(r[1])
                && result.draws[2*i].raw==r[2] && result.draws[2*i+1].raw==r[3]
                && got.draws[0]==int(r[4]) && got.draws[1]==int(r[5]) && result.draws[2*i+1].state_after==r[6],"original Base round changed");
        }
        GT_ASSERT(result.victory==model::Victory::Attacker && result.survivor.health==3 && result.survivor.stored_morale==2
            && result.survivor.spent==3 && result.survivor.position==model::Position::Source && !result.references.secondary
            && result.promotion.winner_weight==84 && result.promotion.loser_weight==280 && result.random.Crt()==3665287870u
            && result.random.Shared()==3 && result.original_returns.wrapper_return==0,"Base aftermath or promotion differs");
        const auto fanatic=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},true,{25,25,25});
        GT_ASSERT((fanatic.strengths==std::array<int32_t,2>{350,280}),"FANATIC and Base stages were added instead of multiplied");
        const auto fractional=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},true,{17,19,23});
        GT_ASSERT((fractional.strengths==std::array<int32_t,2>{311,275}),"modded stages lost independent integer truncation");GT_OK();
    });
	task->AddTest( "SMAC Scout combat matches every original round draw and promotion", GT() {
		size_t round_total = 0, draw_total = 0, admission_draws = 0, parity_draws = 0;
		for ( const auto& c : fixture::CASES ) {
			const auto result = Resolve( c.input );
			GT_ASSERT( result.strengths == c.strengths && result.round_widths == c.widths, "strength/partial movement differs: " + std::string( c.id ) );
			GT_ASSERT( result.health_after == c.health_after, "final hit/health differs: " + std::string( c.id ) );
			GT_ASSERT( result.rounds.size() == c.round_count && result.draws.size() == c.draw_count, "draw suppression or lethal stop differs: " + std::string( c.id ) );
			for ( size_t i = 0; i < result.rounds.size(); ++i ) {
				const auto& actual = result.rounds[ i ]; const auto& expected = fixture::ROUNDS[ c.first_round + i ];
				GT_ASSERT( actual.health_before == expected.health_before && actual.draws == expected.draws, "round order/modulo/tie differs: " + std::string( c.id ) );
			}
			for ( size_t i = 0; i < result.draws.size(); ++i ) {
				const auto& actual = result.draws[ i ]; const auto& expected = fixture::DRAWS[ c.first_draw + i ];
				GT_ASSERT( static_cast< int >( actual.phase ) == expected.phase && actual.state_before == expected.before && actual.state_after == expected.after && actual.raw == expected.raw, "CRT phase/state/raw differs: " + std::string( c.id ) );
				admission_draws += actual.phase == model::DrawPhase::PromotionAdmission; parity_draws += actual.phase == model::DrawPhase::PromotionParity;
			}
			const auto& p = result.promotion;
			GT_ASSERT( p.winner_weight == c.weights[ 0 ] && p.loser_weight == c.weights[ 1 ] && p.admitted == c.admitted && p.stored_before == c.stored_before && p.stored_after == c.stored_after, "promotion differs: " + std::string( c.id ) );
			GT_ASSERT( result.random.Shared() == c.shared_after && result.random.Crt() == c.crt_after, "final random states differ: " + std::string( c.id ) );
			round_total += result.rounds.size(); draw_total += result.draws.size();
		}
		GT_ASSERT( round_total == 3073 && draw_total == 6194 && admission_draws == 67 && parity_draws == 31, "original transcript coverage changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC Scout normal returns preserve victory and stable reference meaning", GT() {
		for ( const auto& c : fixture::CASES ) {
			const auto result = Resolve( c.input );
			const auto expected_id = c.attacker_won ? ATTACKER : DEFENDER;
			const auto expected_dead = c.attacker_won ? DEFENDER : ATTACKER;
			const size_t side = c.attacker_won ? 0 : 1;
			GT_ASSERT( result.victory == ( c.attacker_won ? model::Victory::Attacker : model::Victory::Defender ) && result.survivor.identity == expected_id && result.deleted_identity == expected_dead, "winner identity changed after compaction: " + std::string( c.id ) );
			const auto& s = result.survivor;
			GT_ASSERT( s.reactor == c.input.reactors[ side ] && s.health == c.health_after[ side ] && s.stored_morale == c.stored_after && s.spent == c.survivor_spent, "survivor state lost at normal return: " + std::string( c.id ) );
			GT_ASSERT( s.position == ( c.attacker_won ? model::Position::Source : model::Position::Destination ) && Singleton( s ) && result.unit_present == c.present, "post-kill position/stack/occupancy differs: " + std::string( c.id ) );
			GT_ASSERT( result.references.selected == SurvivingReference( c.selected_index, expected_id ) && result.references.secondary == SurvivingReference( c.secondary_index, expected_id ), "selection did not follow participant identity: " + std::string( c.id ) );
			GT_ASSERT( result.continuation_identity == *SurvivingReference( c.continuation_index, expected_id ), "continuation did not name compacted survivor: " + std::string( c.id ) );
			GT_ASSERT( result.original_returns.core_return == c.core_return && result.original_returns.wrapper_return == c.wrapper_return, "original return evidence differs: " + std::string( c.id ) );
			GT_ASSERT( result.random.Shared() == c.shared_after && result.random.Crt() == c.crt_after, "cleanup added random draws: " + std::string( c.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout attack then empty-flat continuation matches all three histories", GT() {
		for ( const auto& c : fixture::SEQUENCES ) {
			const auto first = Resolve( c.input );
			GT_ASSERT( StepMatches( c.first, first.survivor, first.references, first.continuation_identity, first.random, first.unit_present, first.original_returns.wrapper_return ), "first normal-return state differs: " + std::string( c.id ) );
			const auto second = model::ContinueEmptyFlatWithoutLottery( first.survivor, c.second_allowance, first.random, first.references );
			GT_ASSERT( StepMatches( c.second, second.survivor, second.references, second.continuation_identity, second.random, second.unit_present, second.original_wrapper_return ), "second normal-return state differs: " + std::string( c.id ) );
			GT_ASSERT( second.random == first.random && second.outcome == ( c.second.wrapper_return == 1 ? model::EmptyMove::Moved : model::EmptyMove::Exhausted ), "empty-flat result or draw suppression differs" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout reference retirement never substitutes continuation selection", GT() {
		const model::Participant attacker( 0, 1, 1, 1, 1, 0 ), defender( UINT64_MAX, 1, 1, 1, 1, 255 );
		const SmacGameplayRandomState rng( 0xabcdef01u, 308 ); // Original tied first round kills attacker.
		for ( const auto selected : { std::optional< model::UnitId >{}, std::optional< model::UnitId >( 0 ), std::optional< model::UnitId >( UINT64_MAX ) } ) {
			for ( const auto secondary : { std::optional< model::UnitId >{}, std::optional< model::UnitId >( 0 ), std::optional< model::UnitId >( UINT64_MAX ) } ) {
				const auto r = model::ResolveNeutralFlatAttack( attacker, defender, 3, rng, { selected, secondary } );
				GT_ASSERT( r.victory == model::Victory::Defender && r.survivor.identity == UINT64_MAX && r.survivor.spent == 255, "stable identity/spending endpoint rejected" );
				GT_ASSERT( r.references.selected == ( selected == 0 ? std::nullopt : selected ) && r.references.secondary == ( secondary == 0 ? std::nullopt : secondary ), "dead reference retained or survivor reference lost" );
				GT_ASSERT( r.continuation_identity == UINT64_MAX && r.random.Shared() == rng.Shared(), "continuation changed selection or shared RNG" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout combat rejects unsupported resolved domains before mutation", GT() {
		const model::Participant valid( ATTACKER, 1, 10, 1, 1, 0 ), defender( DEFENDER, 1, 10, 1, 1, 0 );
		const SmacGameplayRandomState random( UINT32_MAX, UINT32_MAX );
		const auto attempt = [ & ]( const model::Participant& a, const model::Participant& d, const int allowance, const model::References& refs ) { return Rejects( [ & ]() { model::ResolveNeutralFlatAttack( a, d, allowance, random, refs ); } ); };
		for ( const int value : { std::numeric_limits< int32_t >::min(), -1, 0, 5, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.reactor = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unsupported reactor accepted" );
		}
		for ( const int value : { -1, 0, 11, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.health = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "invalid HP accepted" );
		}
		for ( const int value : { -1, 7, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.stored_morale = a.effective_morale = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unsupported morale accepted" );
		}
		auto changed = valid; changed.effective_morale = 2; GT_ASSERT( attempt( changed, defender, 3, { ATTACKER, DEFENDER } ), "modified-morale context accepted" );
		for ( const int value : { -1, 253, 255, 256, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.spent = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unverified spending/wrap accepted" );
		}
		for ( const int value : { -1, 0, 1, 4, 7, std::numeric_limits< int32_t >::max() } ) {
			GT_ASSERT( attempt( valid, defender, value, { ATTACKER, DEFENDER } ), "unsupported allowance accepted" );
		}
		GT_ASSERT( attempt( valid, valid, 3, { ATTACKER, ATTACKER } ) && attempt( valid, defender, 3, { 42, DEFENDER } ), "invalid identity/reference accepted" );
		GT_ASSERT( random.Shared() == UINT32_MAX && random.Crt() == UINT32_MAX && valid.spent == 0 && defender.health == 10, "rejection changed inputs" );
		GT_OK();
	} );
	task->AddTest( "SMAC Scout transcript bounds and explicit empty-flat limits hold", GT() {
		const SmacGameplayRandomState random( 0, UINT32_MAX );
		const auto result = model::ResolveNeutralFlatAttack( { ATTACKER, 4, 40, 6, 6, 252 }, { DEFENDER, 4, 40, 6, 6, 255 }, 6, random, { ATTACKER, DEFENDER } );
		GT_ASSERT( result.round_widths[ 0 ] == 0 && result.rounds.size() == 40 && result.draws.size() == 41 && result.victory == model::Victory::Defender, "exhausted diagnostic draw suppression/elite admission differs" );
		for ( const auto& d : result.draws ) { GT_ASSERT( d.phase != model::DrawPhase::RoundAttacker && d.phase != model::DrawPhase::PromotionParity, "suppressed draw consumed CRT" ); }
		const auto full = model::ResolveNeutralFlatAttack( { ATTACKER, 4, 40, 6, 6, 0 }, { DEFENDER, 4, 40, 6, 6, 255 }, 6, random, { std::nullopt, std::nullopt } );
		GT_ASSERT( full.rounds.size() <= 79 && full.draws.size() <= 160 && full.random.Shared() == random.Shared(), "bounded admitted transcript exceeded domain" );
		model::Survivor source{ ATTACKER, 1, 10, 1, 255, model::Position::Source, { std::nullopt, std::nullopt } };
		const auto exhausted = model::ContinueEmptyFlatWithoutLottery( source, 3, random, { std::nullopt, ATTACKER } );
		GT_ASSERT( exhausted.outcome == model::EmptyMove::Exhausted && exhausted.survivor.spent == 255 && exhausted.random == random, "exhausted continuation changed state" );
		for ( const int spent : { 1, 2 } ) {
			source.spent = spent; GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "partial lottery path silently approximated" );
		}
		source.spent = 0; source.stack_links[ 0 ] = DEFENDER;
		GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "multi-unit continuation admitted" );
		source.stack_links[ 0 ].reset(); source.position = model::Position::Destination;
		GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "wrong-source continuation admitted" );
		GT_OK();
	} );
}

}
