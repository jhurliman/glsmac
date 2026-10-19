#include "SmacSocialState.h"

#include "game/backend/faction/SmacSocialState.h"
#include "fixtures/SmacFactionStartupSource.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <functional>
#include <yaml-cpp/yaml.h>

namespace gse::tests {
namespace {
using namespace game::backend::faction;
using namespace game::backend::settings;
using Phase = SmacFactionSocialState::Phase;
namespace fixture = smac_faction_startup_fixture;

SmacFactionStartupRules Profiles() { return loader::txt::FactionTXTLoader::ParseStartupRules( fixture::Alpha(), fixture::Source, fixture::Technologies() ); }

SmacSocialState State() {
	std::vector< SmacSimulationAdmission > admissions;
	for ( size_t i = 0; i < 7; ++i ) { admissions.push_back( { static_cast< int32_t >( 7 - i ), SmacFactionStartupRules::Bindings()[ i ].port_id } ); }
	return SmacSocialState::NewWorld( admissions, fixture::Technologies(), Profiles() );
}

SmacFactionStartupRules WithBonuses( const std::string& line ) {
	auto profiles = Profiles();
	profiles.factions[ 0 ].bonuses = loader::txt::FactionTXTLoader::ParseStartupBonuses( fixture::Faction( "GAIANS", line ), "GAIANS", fixture::Technologies() );
	return profiles;
}

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

std::string Wire( const SmacSocialState& value, size_t changed_number = SIZE_MAX, int64_t replacement = 0 ) {
	types::Buffer buffer; size_t number = 0;
	const auto write = [ & ]( int64_t item ) { buffer.WriteInt( number++ == changed_number ? replacement : item ); };
	write( 0x534d534f ); write( 1 ); write( value.factions.size() );
	for ( const auto& faction : value.factions ) {
		write( faction.admission.simulation_id ); buffer.WriteString( faction.admission.faction_id ); write( static_cast< int32_t >( faction.phase ) );
		for ( const auto* model : { &faction.selected, &faction.pending } ) { for ( const auto item : *model ) { write( item ); } }
		for ( const auto item : faction.current ) { write( item ); }
		for ( const auto* values : { &faction.intrinsic, &faction.retained } ) {
			buffer.WriteBool( values->has_value() ); if ( *values ) { for ( const auto item : **values ) { write( item ); } }
		}
	}
	return buffer.ToString();
}
}

void AddSmacSocialStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC social early reset preserves supplied intrinsic and retained values without inventing defaults", GT() {
		auto state = State(); const auto initial = state;
		state.ValidateTurn( 0 );
		GT_ASSERT( Rejects( [ & ]() { state.ValidateTurn( 1 ); } ), "unrefreshed state admitted at first command" );
		for ( size_t i = 0; i < state.factions.size(); ++i ) {
			auto& owner = state.factions[ i ];
			GT_ASSERT( !owner.intrinsic && !owner.retained && owner.current == SmacSocialRatings{} && owner.admission.simulation_id == static_cast< int32_t >( 7 - i ), "early state invented snapshot or rebound identity" );
			owner.selected.fill( 3 ); owner.pending.fill( -7 ); owner.current.fill( 99 );
			owner.intrinsic = SmacSocialRatings{}; owner.intrinsic->fill( INT32_MIN );
			owner.retained = SmacSocialRatings{}; owner.retained->fill( i % 2 ? -4 : 4 ); owner.phase = Phase::Refreshed;
			const auto before = owner;
			SmacSocialState::ResetForStartup( owner );
			GT_ASSERT( owner.selected == initial.factions[ i ].selected && owner.pending == initial.factions[ i ].pending && owner.current == SmacSocialRatings{}
				&& owner.phase == Phase::Early && owner.intrinsic == before.intrinsic && owner.retained == before.retained && owner.admission == before.admission, "reset cleared an untouched snapshot or missed early fields" );
		}
		state.Validate( fixture::Technologies(), Profiles() ); state.ValidateTurn( 0 ); GT_OK();
	} );
    task->AddTest("SMAC social owner refresh matches all seven original command boundaries and mixed social-record restore",GT(){
        const auto receipt=YAML::LoadFile("docs/behavior/seven-first-upkeep-authority-cases.json");
        const auto tech=fixture::Technologies();const auto profiles=Profiles();
        std::vector<SmacSimulationAdmission> admissions;
        for(int32_t id=7;id>=1;--id) {admissions.push_back({id,SmacFactionStartupRules::Bindings()[id-1].port_id});}
        auto state=SmacSocialState::NewWorld(admissions,tech,profiles);
        const auto ratings=[](const YAML::Node& n) {SmacSocialRatings out{};for(size_t a=0;a<11;++a) {out[a]=n[a].as<int32_t>();}return out;};
        for(int32_t id=1;id<=7;++id) {
            const auto before=state;
            state.BuildStartupIntrinsic(id,tech,profiles);
            GT_ASSERT(state.Find(id)->intrinsic==ratings(receipt["before"]["factions"][id-1]["social_intrinsic"])
                && state.Find(id)->current==SmacSocialRatings{} && state.Find(id)->phase==Phase::Early && !state.Find(id)->retained,"Late intrinsic publication refreshed current/retained ratings");
            for(const auto& other:before.factions) {if(other.admission.simulation_id!=id) {GT_ASSERT(*state.Find(other.admission.simulation_id)==other,"Late intrinsic publication changed a different owner");}}
        }
        state.ValidateTurn(0);
        for(int32_t scheduled=1;scheduled<=7;++scheduled) {
            const auto before=state;
            state.RefreshOwnerForFirstCommand(scheduled,tech,profiles);
            const auto expected=receipt["command_boundaries"][scheduled-1]["state"]["factions"];
            for(int32_t id=1;id<=7;++id) {
                const auto* owner=state.Find(id);const auto e=expected[id-1];
                GT_ASSERT(owner->current==ratings(e["social_current"]) && owner->intrinsic==ratings(e["social_intrinsic"]),"Owner refresh differs from native command-boundary social vectors");
                if(id<=scheduled) {GT_ASSERT(owner->phase==Phase::Refreshed && owner->retained==ratings(e["social_snapshot"]),"Processed owner lacks exact retained social snapshot");}
                else {GT_ASSERT(*owner==*before.Find(id),"Future owner refreshed before its own upkeep");}
            }
            const auto saved=state;
            state=SmacSocialState::Deserialize(state.Serialize(tech,profiles).ToString(),tech,profiles);
            GT_ASSERT(state==saved,"Mixed first-round state was altered during restoration");
            state.RefreshOwnerForFirstCommand(scheduled,tech,profiles);
            GT_ASSERT(state==saved,"Completed owner refresh replayed after restoration");
            if(scheduled<7) {GT_ASSERT(Rejects([&]{state.ValidateTurn(1);}),"Legacy world boundary silently admitted a partial native round without a controller cursor");}
        }
        state.ValidateTurn(1);GT_OK();
    });
    task->AddTest("SMAC social late intrinsic and owner refresh preserve authority and reject invalid transitions atomically",GT(){
        const auto tech=fixture::Technologies();const auto profiles=Profiles();auto state=State();
        auto& owner=state.factions.front();owner.intrinsic=SmacSocialRatings{};owner.intrinsic->fill(7);
        const auto early=state;
        for(const int32_t id:{0,8,-1}) {
            GT_ASSERT(Rejects([&]{state.BuildStartupIntrinsic(id,tech,profiles);}) && Rejects([&]{state.RefreshOwnerForFirstCommand(id,tech,profiles);}) && state==early,"Unknown social owner partially mutated state");
        }
        state.RefreshOwnerForFirstCommand(owner.admission.simulation_id,tech,profiles);
        GT_ASSERT(state.Find(owner.admission.simulation_id)->intrinsic==early.factions.front().intrinsic
            && state.Find(owner.admission.simulation_id)->current==*early.factions.front().intrinsic,"Refresh silently rebuilt an owned intrinsic snapshot");
        const auto ready=state;
        GT_ASSERT(Rejects([&]{state.BuildStartupIntrinsic(owner.admission.simulation_id,tech,profiles);}) && state==ready,"Late initializer replayed after upkeep");
        auto invalid=profiles;invalid.initial_social_models.back().modifiers="+MORALE";
        GT_ASSERT(Rejects([&]{state.RefreshOwnerForFirstCommand(1,tech,invalid);}) && state==ready,"Owner refresh context failure changed another owner");GT_OK();
    });
	task->AddTest( "SMAC social first refresh derives all seven profiles and preserves later exact state", GT() {
		// Reduced all-axis outputs from RE-SMAC-START-FACTION-001.
		const std::array< SmacSocialRatings, 7 > expected{{
			{ 0, 2, 0, 0, -1, -1, 0, 1, 0, 0, 0 }, { -2, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0 },
			{ 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 2 }, { 1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 2, 1, 0, 0, 0, -1, 0 }, { 0, 0, 2, 0, 0, 0, 0, -1, 1, 0, -2 }, { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
		}};
		const auto profiles = Profiles(); const auto tech = fixture::Technologies();
		auto state = State();
		for ( auto& owner : state.factions ) { owner.retained = SmacSocialRatings{}; owner.retained->fill( -4 ); }
		state.RefreshForFirstCommand( tech, profiles ); state.ValidateTurn( 1 );
		for ( size_t i = 0; i < state.factions.size(); ++i ) {
			const auto& owner = state.factions[ i ];
			GT_ASSERT( owner.phase == Phase::Refreshed && owner.intrinsic == expected[ i ] && owner.current == expected[ i ] && owner.retained == expected[ i ], "source profile social projection differs" );
		}
		GT_ASSERT( Rejects( [ & ]() { state.ValidateTurn( 0 ); } ), "late phase accepted at founding turn zero" );
		state.factions[ 0 ].current.fill( INT32_MIN ); state.factions[ 0 ].retained->fill( INT32_MAX );
		const auto saved = state; state.RefreshForFirstCommand( tech, profiles );
		GT_ASSERT( state == saved, "first refresh replayed after its phase completed" ); GT_OK();
	} );
	task->AddTest( "SMAC social shared record budget and immunity order preserve intrinsic separately", GT() {
		const auto tech = fixture::Technologies();
		for ( const auto* line : { "IMMUNITY,EFFIC,SOCIAL,---EFFIC,SOCIAL,-MORALE", "SOCIAL,---EFFIC,SOCIAL,-MORALE,IMMUNITY,EFFIC" } ) {
			const auto profiles = WithBonuses( line );
			const auto intrinsic = SmacSocialState::BuildIntrinsic( "GAIANS", tech, profiles );
			const auto current = SmacSocialState::RefreshNeutral( intrinsic, "GAIANS", tech, profiles );
			GT_ASSERT( intrinsic[ 1 ] == -3 && intrinsic[ 4 ] == -1 && current[ 1 ] == 0 && current[ 4 ] == -1, "immunity modified intrinsic or depended on record order" );
		}
		std::string line; for ( int i = 0; i < 8; ++i ) { line += "SOCIAL,-SUPPORT,"; }
		const auto profiles = WithBonuses( line + "IMMUNITY,SUPPORT,SOCIAL,+MORALE,ENERGY,99" );
		const auto intrinsic = SmacSocialState::BuildIntrinsic( "GAIANS", tech, profiles );
		GT_ASSERT( intrinsic[ 2 ] == -8 && intrinsic[ 4 ] == 0 && SmacSocialState::RefreshNeutral( intrinsic, "GAIANS", tech, profiles ) == intrinsic, "post-cap records acquired social effects" ); GT_OK();
	} );
	task->AddTest( "SMAC social immunity clamps only the complete named axis at both signed endpoints", GT() {
		const char* axes[] = { "ECONOMY", "EFFIC", "SUPPORT", "TALENT", "MORALE", "POLICE", "GROWTH", "PLANET", "PROBE", "INDUSTRY", "RESEARCH" };
		const auto tech = fixture::Technologies(); const auto plain = WithBonuses( "" );
		for ( size_t axis = 0; axis < 11; ++axis ) {
			const auto immune = WithBonuses( std::string( "IMMUNITY," ) + axes[ axis ] );
			for ( const int32_t rating : { INT32_MIN, -1000, -7, -4, -3, -2, -1, 0, 1, 2, 3, 4, 7, 999, 1000, INT32_MAX } ) {
				SmacSocialRatings intrinsic{}; intrinsic.fill( -3 ); intrinsic[ axis ] = rating;
				auto expected = intrinsic; expected[ axis ] = rating < 0 ? 0 : rating > 999 ? 999 : rating;
				GT_ASSERT( SmacSocialState::RefreshNeutral( intrinsic, "GAIANS", tech, plain ) == intrinsic, "unprotected axis was clamped" );
				GT_ASSERT( SmacSocialState::RefreshNeutral( intrinsic, "GAIANS", tech, immune ) == expected && intrinsic[ axis ] == rating, "immunity missed endpoint or changed another axis/input" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC social duplicate immunity retains shared slots and repeated final clamps", GT() {
		const char* axes[] = { "ECONOMY", "EFFIC", "SUPPORT", "TALENT", "MORALE", "POLICE", "GROWTH", "PLANET", "PROBE", "INDUSTRY", "RESEARCH" };
		const auto tech = fixture::Technologies();
		for ( size_t axis = 0; axis < 11; ++axis ) {
			for ( const int count : { 2, 7, 8 } ) {
				std::string immunity; for ( int i = 0; i < count; ++i ) { immunity += std::string( "IMMUNITY," ) + axes[ axis ] + ","; }
				for ( const bool social_first : { false, true } ) {
					for ( const int sign : { -1, 1 } ) {
						const auto social = std::string( "SOCIAL," ) + ( sign < 0 ? "---" : "+++" ) + axes[ axis ];
						const auto profiles = WithBonuses( social_first ? social + "," + immunity.substr( 0, immunity.size() - 1 ) : immunity + social );
						const auto intrinsic = SmacSocialState::BuildIntrinsic( "GAIANS", tech, profiles );
						SmacSocialRatings expected{}; if ( social_first || count < 8 ) { expected[ axis ] = sign * 3; }
						GT_ASSERT( intrinsic == expected && profiles.factions[ 0 ].Records().size() == static_cast< size_t >( count < 8 ? count + 1 : 8 ), "duplicates were deduplicated before shared admission" );
						expected[ axis ] = std::max( 0, expected[ axis ] );
						GT_ASSERT( SmacSocialState::RefreshNeutral( intrinsic, "GAIANS", tech, profiles ) == expected, "repeated immunity changed a final rating" );
						auto state = SmacSocialState::NewWorld( { { 7, "GAIANS" } }, tech, profiles ); state.RefreshForFirstCommand( tech, profiles );
						GT_ASSERT( state.factions[ 0 ].intrinsic == intrinsic && state.factions[ 0 ].current == expected && state.factions[ 0 ].retained == expected, "duplicate projection differs through owned transition" );
					}
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC social admission rejects unsupported context before replacing any owner", GT() {
		const auto tech = fixture::Technologies(); auto state = State(); const auto before = state;
		for ( const int scenario : { 0, 1, 2, 4 } ) {
			auto profile = Profiles();
			if ( scenario == 0 ) { profile.initial_social_models.back().modifiers = "+MORALE"; }
			if ( scenario == 1 ) { profile.factions.back().bonuses[ 2 ].parameter = 11; }
			if ( scenario == 2 ) { profile.factions.back().bonuses[ 2 ].amount = INT32_MAX; }
			if ( scenario == 4 ) { profile.factions.back().port_id = "Unknown"; }
			GT_ASSERT( Rejects( [ & ]() { state.RefreshForFirstCommand( tech, profile ); } ) && state == before, "late context error partially refreshed owners" );
		}
		for ( const auto admissions : { std::vector< SmacSimulationAdmission >{}, std::vector< SmacSimulationAdmission >{ { 0, "GAIANS" } },
			std::vector< SmacSimulationAdmission >{ { 8, "GAIANS" } }, std::vector< SmacSimulationAdmission >{ { 1, "GAIANS" }, { 1, "HIVE" } },
			std::vector< SmacSimulationAdmission >{ { 1, "gaia" } }, std::vector< SmacSimulationAdmission >( 8, { 1, "GAIANS" } ) } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacSocialState::NewWorld( admissions, tech, Profiles() ); } ), "invalid admission inferred a source or ID" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC social codec preserves owned exact snapshots without reset or refresh replay", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles(); auto state = State();
		state.RefreshForFirstCommand( tech, profiles );
		state.factions[ 0 ].current.fill( INT32_MIN ); state.factions[ 0 ].intrinsic->fill( INT32_MAX ); state.factions[ 0 ].retained->fill( -4 );
		SmacSocialState::ResetForStartup( state.factions[ 1 ] ); state.factions[ 1 ].retained.reset();
		const auto bytes = state.Serialize( tech, profiles ).ToString();
		GT_ASSERT( bytes == Wire( state ), "social wire order changed" );
		const auto saved = SmacSocialState::Deserialize( bytes, tech, profiles );
		GT_ASSERT( saved == state && saved.Find( 7 )->retained->at( 4 ) == -4 && !saved.Find( 6 )->retained, "restoration replayed or invented a snapshot" );
		state.factions.clear(); auto copy = saved; auto moved = std::move( copy );
		GT_ASSERT( moved.Serialize( tech, profiles ).ToString() == bytes, "social state borrowed source storage" );
		GT_ASSERT( Rejects( [ & ]() { moved.ValidateTurn( 0 ); } ) && Rejects( [ & ]() { moved.ValidateTurn( 1 ); } ), "mixed phase accepted at world boundary" ); GT_OK();
	} );
	task->AddTest( "SMAC social codec rejects malformed versions counts phase models ratings and tails atomically", GT() {
		const auto tech = fixture::Technologies(); const auto profiles = Profiles(); auto state = State(); state.factions.resize( 1 );
		state.RefreshForFirstCommand( tech, profiles ); const auto before = state;
		const auto reject = [ & ]( const std::string& bytes ) { return Rejects( [ & ]() { state = SmacSocialState::Deserialize( bytes, tech, profiles ); } ) && state == before; };
		const auto wire = Wire( before );
		for ( size_t size = 0; size < wire.size(); ++size ) { GT_ASSERT( reject( wire.substr( 0, size ) ), "truncated payload published" ); }
		GT_ASSERT( reject( wire + "x" ) && reject( std::string( SmacSocialState::MAX_WIRE_BYTES + 1, 'x' ) ), "extra/oversized payload accepted" );
		for ( size_t index = 0; index < 46; ++index ) {
			for ( const int64_t bad : { INT64_MIN, int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1, INT64_MAX } ) {
				GT_ASSERT( reject( Wire( before, index, bad ) ), "wide numeric field narrowed" );
			}
		}
		GT_ASSERT( reject( Wire( before, 1, 0 ) ) && reject( Wire( before, 4, 256 ) ), "legacy version or aliased phase accepted" );
		for ( size_t index = 5; index < 13; ++index ) { GT_ASSERT( reject( Wire( before, index, 1 ) ), "unproved model selection accepted" ); }
		for ( const int scenario : { 0, 1, 2, 3, 4 } ) {
			auto bad = before;
			if ( scenario == 0 ) { bad.factions[ 0 ].admission.faction_id = "Unknown"; }
			if ( scenario == 1 ) { bad.factions[ 0 ].retained.reset(); }
			if ( scenario == 2 ) { bad.factions[ 0 ].intrinsic.reset(); }
			if ( scenario == 3 ) { bad.factions[ 0 ].phase = Phase::Early; bad.factions[ 0 ].current[ 0 ] = 1; }
			if ( scenario == 4 ) { bad.factions.push_back( bad.factions.front() ); }
			GT_ASSERT( reject( Wire( bad ) ), "late structural error published state" );
		}
		GT_OK();
	} );
}

}
