#include "SmacSimulationState.h"
#include "fixtures/SmacRegionalMilitary.h"

#include "game/backend/faction/SmacSimulationState.h"
#include "game/backend/faction/SmacRankOrder.h"
#include "game/backend/faction/SmacRankingScore.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacOrbitalIntake.h"
#include "fixtures/SmacCapabilityCases.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend;
using faction::SmacSimulationState;

SmacSimulationState State() {
	SmacSimulationState value( SmacGameplayRandomState( 0, UINT32_MAX ), SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain, faction::SmacGlobalResourceModifiers::NewWorld() );
	value.roster = { { 7, 0, "controller-one", roster::Role::Host, "GAIANS" }, { 2, 6, "controller-two", roster::Role::Player, "UNIVERSITY" } };
    value.human_membership=(1u<<7)|(1u<<2);
	for ( const auto& owner : value.roster ) { value.satellites.emplace( owner.simulation_id, faction::SmacSatelliteCounts::NewWorld() ); }
	for ( const auto& owner : value.roster ) { value.economy.emplace( owner.simulation_id, faction::SmacEconomyState::NewWorld( { 5, 0 }, 100, 0 ) ); }
	for(const auto& owner:value.roster) {value.hostility.owners.emplace(owner.simulation_id,faction::SmacHostileOwner::NewWorld({}));}
	value.technology = faction::SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.designs = unit::SmacStartupDesignState::NewWorld( value.technology, smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
	value.social = faction::SmacSocialState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.technology.factions[ 0 ].owned.fill( true ); value.technology.factions[ 0 ].labs = INT32_MIN;
	value.technology.factions[ 1 ].owned.fill( false ); value.technology.factions[ 1 ].score = INT32_MAX;
	value.technology.factions[ 1 ].target = 88; value.technology.factions[ 1 ].cached_threshold = -7;
	return value;
}

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

// Independent V15 framing retained for migration and malformed-record cases.
std::string Wire( const SmacSimulationState& value, size_t index = SIZE_MAX, int64_t replacement = 0, const std::string* random = nullptr, const std::string* modifiers = nullptr, const std::string* history = nullptr, const std::string* satellites = nullptr, const std::string* lifecycle = nullptr, bool legacy = false, bool v12 = false ) {
	types::Buffer buffer; size_t number = 0;
	const auto write = [ & ]( int64_t v ) { buffer.WriteInt( number++ == index ? replacement : v ); };
	write( 0x534d5353 ); write( legacy ? 11 : v12 ? 12 : 15 ); write( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		write( participant.simulation_id ); write( participant.slot ); buffer.WriteString( participant.gsid );
		write( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( value.technology.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( value.designs.Serialize( smac_startup_fixture::Units().catalog ).ToString() );
	buffer.WriteString( value.social.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( random ? *random : value.gameplay_random.Serialize() );
	write( static_cast< int64_t >( value.movement_capability ) );
	if ( modifiers ) { types::Buffer tail; tail.WriteBool( false ); return buffer.ToString() + *modifiers + tail.ToString(); }
	buffer.WriteInt( value.resource_modifiers.atmospheric_dust_countdown );
	buffer.WriteBool( value.resource_modifiers.solar_ordinary ); buffer.WriteBool( value.resource_modifiers.solar_upkeep );
	buffer.WriteBool( value.resource_modifiers.planetary_governor.has_value() );
	if ( value.resource_modifiers.planetary_governor ) { buffer.WriteInt( *value.resource_modifiers.planetary_governor ); }
	buffer.WriteBool( history || value.movement_history.has_value() );
	if ( history || value.movement_history ) { buffer.WriteString( history ? *history : value.movement_history->Serialize() ); }
	if ( satellites ) { return buffer.ToString() + *satellites; }
	buffer.WriteInt( value.satellites.size() );
	for ( const auto& entry : value.satellites ) { buffer.WriteInt( entry.first ); for ( const auto count : entry.second.resources ) { buffer.WriteInt( count ); } }
	buffer.WriteInt( value.economy.size() );
	for ( const auto& entry : value.economy ) {
		buffer.WriteInt(entry.first);
		if(legacy) {
			types::Buffer economy;const auto& e=entry.second;
			for(const auto field:{int64_t(0x534d4543),int64_t(1),int64_t(e.rates.psych),int64_t(e.rates.labs),int64_t(e.credits),int64_t(e.pending_debit),int64_t(e.research_fraction),int64_t(e.treaty_partners)}) {economy.WriteInt(field);}
			buffer.WriteString(economy.ToString());
		} else {buffer.WriteString(entry.second.Serialize().ToString());}
	}
	if ( lifecycle ) { return buffer.ToString() + *lifecycle; }
	buffer.WriteBool( value.do_or_die ); buffer.WriteInt( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		buffer.WriteInt( participant.simulation_id ); buffer.WriteBool( participant.active ); buffer.WriteBool( participant.restart_prohibited );
	}
	if(!legacy) {
		buffer.WriteInt(value.hostility.owners.size());
		for(const auto& item:value.hostility.owners) {
			buffer.WriteInt(item.first);
			for(const auto& d:item.second.directions) {buffer.WriteInt(d.primary);buffer.WriteInt(d.secondary);buffer.WriteInt(d.transition_turn);buffer.WriteInt(d.paired_a);buffer.WriteInt(d.paired_b);}
			buffer.WriteInt(item.second.integrity);buffer.WriteInt(item.second.bookkeeping);
			size_t count=0;for(auto loss:item.second.losses) {if(loss) {++count;}}buffer.WriteInt(count);
			for(size_t design=0;design<512;++design) {if(item.second.losses[design]) {buffer.WriteInt(design);buffer.WriteInt(item.second.losses[design]);}}
		}
	}
	if(!legacy && !v12) {buffer.WriteBool(value.human_defeat_request.has_value());if(value.human_defeat_request) {buffer.WriteInt(*value.human_defeat_request);}}
	if(!legacy && !v12) {buffer.WriteBool(value.rank_order.has_value());if(value.rank_order) {for(auto owner:value.rank_order->faction_at_rank) {buffer.WriteInt(owner);}}}
    if(!legacy && !v12) {buffer.WriteInt(value.human_membership);}
	return buffer.ToString();
}
std::string CurrentWire(const SmacSimulationState& value,int64_t version=19,bool include_capabilities=true,bool include_military=true) {
    types::Buffer suffix;suffix.WriteBool(value.strategy.has_value());
    if(value.strategy) {
        for(const auto& [owner,state]:*value.strategy) {
            suffix.WriteInt(owner);for(const auto priority:state.priorities) {suffix.WriteInt(priority);}
        }
    }
    if(version>=17 && include_capabilities) {
        suffix.WriteBool(value.capabilities.has_value());
        if(value.capabilities) {for(const auto& owner:value.capabilities->factions) {
            for(const auto& fields:{owner.available,owner.foreign}) {for(const auto field:fields) {suffix.WriteInt(field);}}
        }}
    }
    if(version>=18 && include_military) {
        suffix.WriteBool(value.regional_military.has_value());
        if(value.regional_military) {
            std::string packed;
            for(const auto& owner:value.regional_military->factions) {for(const auto* fields:{&owner.units,&owner.attack}) {for(const auto field:*fields) {
                packed+=static_cast<char>(field&255);packed+=static_cast<char>(field>>8);
            }}}
            if(version>=19) {for(const auto& owner:value.regional_military->factions) {
                packed+=owner.unit_phase_flags ? char(1) : char(0);
                if(owner.unit_phase_flags) {for(const auto flags:*owner.unit_phase_flags) {
                    packed+=static_cast<char>(flags&255);packed+=static_cast<char>(flags>>8);
                }}
            }}
            suffix.WriteString(packed);
        }
    }
    return Wire(value,1,version)+suffix.ToString();
}

unit::SmacPredefinedUnitCatalog CapabilityCatalog() {
    auto catalog=smac_startup_fixture::Units().catalog;
    for(auto& row:catalog.weapons.weapons) {row.prerequisite="Disable";}
    for(auto& row:catalog.defenses.defenses) {row.prerequisite="Disable";}
    for(auto& row:catalog.chassis.chassis) {row.prerequisite="Disable";}
    catalog.weapons.weapons[12].offense=-1;catalog.defenses.defenses[9].rating=-1;
    catalog.chassis.chassis[0].triad=loader::txt::ChassisTriad::Land;
    auto& native=catalog.units[8];native.active=true;native.faction_exclusion=0;native.plan=1;native.chassis_id=0;
    native.prerequisite={settings::SmacTechnologyReference::Kind::TECHNOLOGY_ID,58};
    return catalog;
}
}

void AddSmacSimulationStateTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC retained capability scans match 31 original binding and width controls",GT(){
        for(const auto& test:smac_capability_fixture::Cases()) {
            auto catalog=CapabilityCatalog();auto technologies=smac_rules_fixture::Technologies();
            faction::SmacFactionTechnologyState owned{{1,"GAIANS"}};
            faction::SmacCapabilityState state;state.factions[0].foreign={9,8,0,0,0};
            settings::SmacCombatRules combat{25,25,25,settings::SmacCombatRules::PsiRatios{{{3,2},{1,1},{1,1}}}};
            const auto token=[](int32_t id){return id==-1?std::string("None"):id==-2?std::string("Disable"):"Tech"+std::to_string(id);};
            bool no_colony=false;
            for(const auto& [key,value]:test.overrides) {
                auto& native=catalog.units[8];
                if(key=="weapon_0_raw") {catalog.weapons.weapons[0].offense=value;}
                else if(key=="weapon_0_technology") {catalog.weapons.weapons[0].prerequisite=token(value);}
                else if(key=="armor_0_raw") {catalog.defenses.defenses[0].rating=value;}
                else if(key=="armor_0_technology") {catalog.defenses.defenses[0].prerequisite=token(value);}
                else if(key=="chassis_0_raw") {catalog.chassis.chassis[0].speed=value;}
                else if(key=="chassis_0_technology") {catalog.chassis.chassis[0].prerequisite=token(value);}
                else if(key=="chassis_0_domain") {catalog.chassis.chassis[0].triad=static_cast<loader::txt::ChassisTriad>(value);}
                else if(key=="mind_worm_active_flags") {native.active=(value&1)!=0;}
                else if(key=="mind_worm_faction_exclusion_mask") {native.faction_exclusion=value;}
                else if(key=="mind_worm_technology_prerequisite") {native.prerequisite=technologies.ResolveToken(token(value));}
                else if(key=="mind_worm_chassis") {native.chassis_id=value;}
                else if(key=="mind_worm_plan") {native.plan=value;}
                else if(key=="technology_58_owner_mask") {owned.owned[58]=(value&2)!=0;}
                else if(key=="technology_77_owner_mask") {owned.owned[77]=(value&2)!=0;}
                else if(key=="technology_88_owner_mask") {owned.owned[88]=(value&2)!=0;}
                else if(key=="technology_58_first_prerequisite") {technologies.technologies[58].prerequisites[0]=token(value);}
                else if(key=="technology_58_second_prerequisite") {technologies.technologies[58].prerequisites[1]=token(value);}
                else if(key=="primary_rules") {no_colony=(value&0x00400000)!=0;}
                else if(key=="prior_foreign_weapon") {state.factions[0].foreign[0]=value;}
                else if(key=="prior_foreign_armor") {state.factions[0].foreign[1]=value;}
                else if(key=="peer2_five_caches") {state.factions[1].available.fill(value);}
                else {GT_ASSERT(false,"unbound native capability input "+key);}
            }
            state.Refresh(1,owned,technologies,catalog,combat,{},0,false,false,no_colony);
            GT_ASSERT((test.foreign?state.factions[0].foreign:state.factions[0].available)==test.expected,"original capability mismatch: "+std::string(test.name));
        }
        GT_OK();
    });
    task->AddTest("SMAC retained PSI capabilities match 15 original separate truncation and project controls",GT(){
        for(const auto& test:smac_capability_fixture::PsiCases()) {
            auto catalog=CapabilityCatalog();const auto technologies=smac_rules_fixture::Technologies();
            // Bind native semantic technologies to synthetic component tokens;
            // the calculation uses resolved ownership, not these numeric IDs.
            catalog.weapons.weapons[12].prerequisite="Tech59";catalog.defenses.defenses[9].prerequisite="Tech60";
            faction::SmacFactionTechnologyState owned{{1,"GAIANS"}};
            owned.owned[58]=(test.owned&1)!=0;owned.owned[59]=(test.owned&2)!=0;owned.owned[60]=(test.owned&4)!=0;
            faction::SmacCapabilityState state;state.factions[0].foreign={test.foreign[0],test.foreign[1],0,0,0};
            settings::SmacCombatRules combat{25,25,25,settings::SmacCombatRules::PsiRatios{{test.ratio,{1,1},{1,1}}}};
            state.Refresh(1,owned,technologies,catalog,combat,{},test.percent,(test.projects&1)!=0,(test.projects&2)!=0,false);
            GT_ASSERT(state.factions[0].available==test.expected,"original PSI mismatch: "+std::string(test.name));
        }
        GT_OK();
    });
    task->AddTest("SMAC capability foreign tiers retain native relationship order and reject failed PSI atomically",GT(){
        const auto catalog=CapabilityCatalog();const auto technologies=smac_rules_fixture::Technologies();
        faction::SmacFactionTechnologyState owned{{1,"GAIANS"}};
        settings::SmacCombatRules combat{25,25,25,settings::SmacCombatRules::PsiRatios{{{3,2},{1,1},{1,1}}}};
        faction::SmacCapabilityState state;state.factions[1].available={2,1,1,0,0};state.factions[2].available={8,1,1,0,0};
        std::array<uint32_t,8> relations{};relations[2]=24;relations[3]=8;
        state.Refresh(1,owned,technologies,catalog,combat,relations,0,false,false,false);
        GT_ASSERT(state.factions[0].foreign[0]==2,"early Vendetta tier was replaced by global strongest weapon");
        relations[2]|=1;
        state.Refresh(1,owned,technologies,catalog,combat,relations,0,false,false,false);
        GT_ASSERT(state.factions[0].foreign[0]==8,"forward truce did not exclude the earlier peer");
        owned.owned[58]=true;state.factions[0].foreign[1]=INT32_MAX;
        const auto before=state;
        GT_ASSERT(Rejects([&]{state.Refresh(1,owned,technologies,catalog,combat,relations,0,false,false,false);}) && state==before,
            "unsupported PSI overflow partially published capabilities");
        GT_OK();
    });
    task->AddTest("SMAC regional military matches 130 native resolved unit-phase projections",GT(){
        for(const auto& row:regional_military_fixture::Cases()) {
            auto actual=row.before;
            for(const auto& unit:row.units) {game::backend::faction::SmacRegionalMilitaryState::Accumulate(actual,unit);}
            GT_ASSERT(actual==row.expected,"native regional unit or attack projection differs: "+std::string(row.id));
        }
        GT_OK();
    });
    task->AddTest("SMAC unit-phase flags V19 retain exact scoped bits and V18 absence",GT(){
        using Military=game::backend::faction::SmacRegionalMilitaryState;
        auto value=State();value.regional_military.emplace();
        const std::array<uint16_t,7> patterns{0,2,2050,4096,8192,10242,14338};
        for(size_t owner=0;owner<7;++owner) {
            auto& flags=value.regional_military->factions[owner].unit_phase_flags;flags.emplace();
            for(size_t region=0;region<128;++region) {flags->at(region)=patterns[(owner+region)%patterns.size()];}
        }
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        const auto wire=value.Serialize(technologies,profiles,units).ToString();
        GT_ASSERT(wire==CurrentWire(value) && SmacSimulationState::Deserialize(wire,technologies,profiles,units)==value,"V19 lost scoped flag history or unoccupied/polar slots");
        const auto old=SmacSimulationState::Deserialize(CurrentWire(value,18),technologies,profiles,units);
        for(const auto& owner:old.regional_military->factions) {GT_ASSERT(!owner.unit_phase_flags,"V18 acquired invented flag history");}
        auto stale=old.regional_military->factions.front();
        Military::Accumulate(stale,{1,8,0,true,0,1,0,0,false,false});
        GT_ASSERT(!stale.unit_phase_flags,"arithmetic query manufactured missing flag authority");
        for(int variant=0;variant<4;++variant) {
            std::string packed(Military::V18_WIRE_BYTES,0);
            if(variant==0) {packed+=char(2);packed.append(6,0);}
            if(variant==1) {packed+=char(1);packed.append(6,0);}
            if(variant==2) {packed+=char(1);packed+=char(1);packed.append(255+6,0);}
            if(variant==3) {packed.append(8,0);}
            types::Buffer invalid;invalid.WriteBool(true);invalid.WriteString(packed);
            GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(CurrentWire(value,19,true,false)+invalid.ToString(),technologies,profiles,units);}),"malformed scoped flags accepted");
        }
        value.regional_military->factions.front().unit_phase_flags->at(0)=1;
        GT_ASSERT(Rejects([&]{value.Serialize(technologies,profiles,units);}),"Base/peer/final flags leaked into unit-phase history");GT_OK();
    });
    task->AddTest("SMAC regional military V18 preserves all word counters and legacy absence",GT(){
        auto value=State();value.regional_military.emplace();
        for(size_t owner=0;owner<7;++owner) {for(size_t region=0;region<128;++region) {
            value.regional_military->factions[owner].units[region]=static_cast<uint16_t>(owner*9000+region);
            value.regional_military->factions[owner].attack[region]=static_cast<uint16_t>(65535-owner*9000-region);
        }}
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        const auto wire=value.Serialize(technologies,profiles,units).ToString();
        GT_ASSERT(wire==CurrentWire(value) && SmacSimulationState::Deserialize(wire,technologies,profiles,units)==value,
            "military restore rebuilt counters or omitted unoccupied faction or polar regions");
        const auto old=SmacSimulationState::Deserialize(CurrentWire(value,17),technologies,profiles,units);
        GT_ASSERT(!old.regional_military,"V17 acquired invented owner-summary history");
        for(const auto length:{size_t(0),size_t(3583),size_t(3585)}) {
            types::Buffer invalid;invalid.WriteBool(true);invalid.WriteString(std::string(length,'x'));
            GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(CurrentWire(value,18,true,false)+invalid.ToString(),technologies,profiles,units);}),
                "malformed regional military extent accepted");
        }
        GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(wire.substr(0,wire.size()-1),technologies,profiles,units);}),"truncated regional military state accepted");
        GT_OK();
    });
    task->AddTest("SMAC capability history survives V17 restore while V16 preserves absence",GT(){
        auto value=State();value.capabilities.emplace();
        for(size_t owner=0;owner<7;++owner) {
            value.capabilities->factions[owner].available={int32_t(owner),127,255,70000,INT32_MIN};
            value.capabilities->factions[owner].foreign={INT32_MAX,-7,0,int32_t(owner),60000};
        }
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        const auto wire=value.Serialize(technologies,profiles,units).ToString();
        GT_ASSERT(SmacSimulationState::Deserialize(wire,technologies,profiles,units)==value,"capability restore recalculated stale or unoccupied faction caches");
        auto old=SmacSimulationState::Deserialize(CurrentWire(value,16),technologies,profiles,units);
        old.RefreshCapabilities(7,technologies,units,smac_rules_fixture::Combat(),false);
        GT_ASSERT(!old.capabilities,"legacy owner refresh fabricated capability history");
        types::Buffer invalid;invalid.WriteBool(true);invalid.WriteInt(int64_t(INT32_MAX)+1);
        for(size_t i=1;i<70;++i) {invalid.WriteInt(0);}
        GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(CurrentWire(value,17,false)+invalid.ToString(),technologies,profiles,units);}),"oversized signed capability field accepted");
        GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(wire.substr(0,wire.size()-1),technologies,profiles,units);}),"truncated capability history accepted");
        GT_OK();
    });
    task->AddTest("SMAC faction strategy retains signed priorities on cancel and normalizes accepted masks",GT(){
        auto profile=smac_startup_fixture::Factions().factions[0];
        profile.worker_priorities={-3,2,-1,4,INT32_MAX};
        auto strategy=faction::SmacFactionStrategy::NewWorld(profile);
        const std::array<int32_t,4> original{2,-1,4,INT32_MAX};
        GT_ASSERT(strategy.priorities==original,"startup strategy lost semantic header order or signed values");
        strategy.ApplyResearchPriorities({});
        GT_ASSERT(strategy.priorities==original,"cancel normalized retained priorities");
        for(uint8_t mask=0;mask<16;++mask) {
            strategy.ApplyResearchPriorities(mask);
            for(size_t axis=0;axis<4;++axis) {GT_ASSERT(strategy.priorities[axis]==((mask>>axis)&1),"accepted checkbox mask changed axis");}
        }
        const auto before=strategy;
        GT_ASSERT(Rejects([&]{strategy.ApplyResearchPriorities(16);}) && strategy==before,"invalid priority answer changed state");
        GT_ASSERT(profile.worker_priorities[1]==2,"mutable strategy changed frozen personality");
        GT_OK();
    });
    task->AddTest("SMAC saved faction strategy restores full priorities without replaying startup",GT(){
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        auto value=State();value.strategy.emplace();
        value.strategy->emplace(7,faction::SmacFactionStrategy{{INT32_MIN,2,-1,INT32_MAX}});
        value.strategy->emplace(2,faction::SmacFactionStrategy{{0,0,0,0}});
        const auto wire=value.Serialize(technologies,profiles,units).ToString();
        const auto restored=SmacSimulationState::Deserialize(wire,technologies,profiles,units);
        GT_ASSERT(restored==value && restored.strategy->at(7).priorities[0]==INT32_MIN,"restore replayed personality or normalized signed priorities");
        const auto legacy=SmacSimulationState::Deserialize(Wire(value),technologies,profiles,units);
        GT_ASSERT(!legacy.strategy,"V15 migration invented lost priority authority");
        auto missing=value;missing.strategy->erase(7);
        GT_ASSERT(Rejects([&]{missing.Serialize(technologies,profiles,units);}),"incomplete strategy admitted");
        auto foreign=value;foreign.strategy->erase(7);foreign.strategy->emplace(1,faction::SmacFactionStrategy{{}});
        GT_ASSERT(Rejects([&]{foreign.Serialize(technologies,profiles,units);}),"foreign strategy replaced a roster owner");
        for(const bool duplicate:{false,true}) {
            auto malformed=Wire(value,1,16);types::Buffer tail;tail.WriteBool(true);
            tail.WriteInt(2);for(int axis=0;axis<4;++axis) {tail.WriteInt(0);}
            tail.WriteInt(duplicate?2:7);
            tail.WriteInt(duplicate?0:int64_t(INT32_MAX)+1);for(int axis=1;axis<4;++axis) {tail.WriteInt(0);}
            malformed+=tail.ToString();
            GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(malformed,technologies,profiles,units);}),"malformed strategy identity or signed width admitted");
        }
        GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(wire.substr(0,wire.size()-1),technologies,profiles,units);}),"truncated strategy restored");
        GT_OK();
    });
    task->AddTest("SMAC ordinary ranking matches original cached population technology and design score cases",GT(){
        auto technologies=smac_rules_fixture::Technologies();
        // Installed original Ecology/Biogenetics ratings, not fixture defaults.
        technologies.technologies[6].priorities={0,1,2,3};
        technologies.technologies[0].priorities={0,3,2,2};
        auto catalog=smac_startup_fixture::Units().catalog;
        catalog.units[0].cost=3;catalog.units[1].cost=2;catalog.units[2].cost=1;
        faction::SmacFactionTechnologyState owner{{1,"GAIANS"}};owner.owned[6]=true;
        unit::SmacStartupDesignState designs;
        faction::SmacDesignCounts counts{};counts[2]=1;
        const auto score=[&](int32_t population){return faction::CalculateSmacOrdinaryRankingScore(owner,population,counts,technologies,catalog,designs);};
        GT_ASSERT(score(0)==7 && score(1)==11 && score(3)==19,"original retained population timing differs");
        owner.score=999;GT_ASSERT(score(1)==11,"historical acquisition score entered ranking");
        owner.future_count=2;GT_ASSERT(score(1)==19,"Future repeat counter missing");owner.future_count=0;
        owner.owned[0]=true;GT_ASSERT(score(1)==18,"owned Biogenetics rating sum differs");owner.owned[0]=false;
        counts[0]=2;counts[1]=2;GT_ASSERT(score(1)==13,"support designs rounded per unit instead of per design");
        owner.owned[6]=false;GT_ASSERT(score(1)==6,"Former prerequisite availability ignored");owner.owned[6]=true;
        counts={};counts[2]=5;
        catalog.weapons.weapons[1].offense=4;
        unit::SmacDesignOwner foreign;foreign.simulation_id=2;
        unit::SmacCustomDesign stronger;stronger.id=128;stronger.weapon=1;
        foreign.designs.push_back(stronger);designs.owners.push_back(foreign);
        GT_ASSERT(score(1)==15,"unprototyped design raised global reference");
        designs.owners[0].designs[0].prototyped=true;
        GT_ASSERT(score(1)==10,"weapon reference did not divide before count and cost multiplication");
        designs.owners[0].designs[0].active=false;GT_ASSERT(score(1)==15,"inactive design raised global reference");
        counts[2]=251;GT_ASSERT(score(1)==10,"above-250 count was clamped instead of skipped");
        GT_ASSERT(Rejects([&]{score(-1);}) && Rejects([&]{score(INT32_MAX);}),"invalid population or overflow accepted");
        GT_OK();
    });

    task->AddTest("SMAC saved design byte counts and retained ranks restore independently of current entities",GT(){
        auto value=State();auto& counts=*value.economy.at(7).design_counts;
        counts[0]=255;counts[2]=251;counts[128]=201;counts[511]=17;
        value.rank_order=faction::SmacRankOrder{};value.rank_order->faction_at_rank[7]=2;value.rank_order->faction_at_rank[4]=7;
        value.roster[1].active=false; // retained leaders can identify an eliminated faction
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        const auto restored=SmacSimulationState::Deserialize(value.Serialize(technologies,profiles,units).ToString(),technologies,profiles,units);
        GT_ASSERT(restored==value && restored.rank_order->DispositionReference(2)==0,"restore rebuilt saved counts or compacted retained rank gaps");
        auto duplicate=value;duplicate.rank_order->faction_at_rank[6]=2;
        GT_ASSERT(Rejects([&]{duplicate.Serialize(technologies,profiles,units);}),"duplicate retained rank identity accepted");
        auto old=State();auto raw=Wire(old,1,13);types::Buffer suffix;suffix.WriteBool(false);suffix.WriteInt(0);raw.resize(raw.size()-suffix.lenw);
        const auto migrated=SmacSimulationState::Deserialize(raw,technologies,profiles,units);
        GT_ASSERT(!migrated.rank_order,"V13 migration fabricated a retained ranking");
        GT_OK();
    });

    task->AddTest("SMAC human membership is saved independently of transport activity and ending requests",GT(){
        const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
        for(const uint8_t membership:{0,4,128,132}) {
            auto value=State();value.human_membership=membership;value.roster[0].active=false;
            const auto restored=SmacSimulationState::Deserialize(value.Serialize(technologies,profiles,units).ToString(),technologies,profiles,units);
            GT_ASSERT(restored==value && restored.IsHuman(7)==bool(membership&128) && restored.IsHuman(2)==bool(membership&4)
                && !restored.IsHuman(0) && !restored.IsHuman(8) && !restored.human_defeat_request,"membership followed activity/transport or invented a loss");
        }
        for(const uint8_t membership:{1,2,255}) {
            auto invalid=State();invalid.human_membership=membership;
            GT_ASSERT(Rejects([&]{invalid.Serialize(technologies,profiles,units);}),"unknown/native human identity accepted");
        }
        auto invalid=State();invalid.roster[0].active=false;invalid.human_membership=4;invalid.human_defeat_request=7;
        GT_ASSERT(Rejects([&]{invalid.Serialize(technologies,profiles,units);}),"nonhuman loss request accepted");
        auto value=State();auto old=Wire(value,1,14);types::Buffer frame;frame.WriteInt(0);old.resize(old.size()-frame.lenw);
        const auto legacy=SmacSimulationState::Deserialize(old,technologies,profiles,units);
        GT_ASSERT(legacy.human_membership==132,"known all-human V14 port campaign migration changed accepted controllers");
        const auto current=Wire(value);
        for(const int64_t membership:{int64_t(-1),int64_t(1),int64_t(2),int64_t(256),INT64_MAX}) {
            types::Buffer malformed;malformed.WriteInt(membership);
            GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(current.substr(0,current.size()-frame.lenw)+malformed.ToString(),technologies,profiles,units);}),"malformed saved membership accepted");
        }
        GT_OK();
    });
	task->AddTest("SMAC retained ranking keeps inactive gaps ties and disposition references",GT(){
		using faction::SmacRankOrder;
		// Literal first-command producer: stale references are replaced only by
		// a full update; scores are supplied here, not reconstructed from focus.
		const auto first=SmacRankOrder::Update({1,0,0,0,0,0,0},2);
		const std::array<int32_t,8> original{0,0,0,0,0,0,0,1};
		GT_ASSERT(first.faction_at_rank==original && first.Leader()==1 && first.RunnerUp()==0,"native first-command inverse order differs");
		const auto ties=SmacRankOrder::Update({10,10,10,10,10,10,10},254);
		const std::array<int32_t,8> lower_first{0,7,6,5,4,3,2,1};
		GT_ASSERT(ties.faction_at_rank==lower_first,"equal scores did not prefer lower faction slots");
		const auto gaps=SmacRankOrder::Update({99,99,99,99,99,99,0},128);
		GT_ASSERT(gaps.Leader()==0 && gaps.RunnerUp()==0 && gaps.faction_at_rank[1]==7,"inactive slots were removed from sorting or inverse gaps compacted");
		const auto retained=SmacRankOrder::Update({10,20,0,0,0,0,0},6);
		GT_ASSERT(retained.DispositionReference(2)==1 && retained.DispositionReference(1)==2 && retained.Leader()==2,
			"disposition rebuilt the saved references from surviving factions");
		GT_ASSERT(Rejects([]{SmacRankOrder::Update({-1,0,0,0,0,0,0},2);})
			&& Rejects([]{SmacRankOrder::Update({0,0,0,0,0,0,0},1);}),"unproved negative score or native faction admitted");GT_OK();
	});
	task->AddTest("SMAC human defeat request retains its owner independently of inactive status and migrates V12",GT(){
		const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
		auto value=State();value.roster[1].active=false;value.human_defeat_request=2;
		const auto wire=value.Serialize(technologies,profiles,units).ToString();
		GT_ASSERT(wire==CurrentWire(value) && SmacSimulationState::Deserialize(wire,technologies,profiles,units)==value,"ending request was omitted or changed on restore");
		auto legacy=value;legacy.human_defeat_request.reset();
		const auto migrated=SmacSimulationState::Deserialize(Wire(value,SIZE_MAX,0,nullptr,nullptr,nullptr,nullptr,nullptr,false,true),technologies,profiles,units);
		GT_ASSERT(migrated==legacy && !migrated.human_defeat_request,"V12 migration invented a request from inactive roster state");
		for(const int owner:{0,1,7,8,INT32_MAX}) {auto invalid=value;invalid.human_defeat_request=owner;
			GT_ASSERT(Rejects([&]{invalid.Serialize(technologies,profiles,units);}) && Rejects([&]{SmacSimulationState::Deserialize(Wire(invalid),technologies,profiles,units);}),"unknown or active request owner accepted");
		}
		GT_OK();
	});
	task->AddTest("SMAC simulation battle history survives restore and migrates pristine V11 campaigns",GT(){
		const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
		auto value=State();
		const auto migrated=SmacSimulationState::Deserialize(Wire(value,SIZE_MAX,0,nullptr,nullptr,nullptr,nullptr,nullptr,true),technologies,profiles,units);
		auto expected=value;for(auto& entry:expected.economy) {entry.second.population_aggregate.reset();entry.second.regional_base_counts.reset();entry.second.design_counts.reset();}
		GT_ASSERT(migrated==expected,"V11 migration changed existing campaign state or invented missing history");
		for(auto& entry:value.hostility.owners) {
			for(size_t i=0;i<8;++i) {entry.second.directions[i]={UINT32_MAX-static_cast<uint32_t>(i),static_cast<uint32_t>(i+1),INT32_MIN+static_cast<int32_t>(i),UINT32_MAX,static_cast<uint32_t>(i)};}
			entry.second.losses.fill(UINT16_MAX);entry.second.integrity=UINT32_MAX;entry.second.bookkeeping=0x80000000u;
		}
		const auto wire=value.Serialize(technologies,profiles,units).ToString();
		auto restored=SmacSimulationState::Deserialize(wire,technologies,profiles,units);
		GT_ASSERT(restored==value && wire==CurrentWire(value) && restored.Serialize(technologies,profiles,units).ToString()==wire,"restore lost nonderived relation words or any of the 512 design-loss counters");
		restored.hostility.owners.at(2).losses[511]=0;
		GT_ASSERT(!(restored==value) && value.hostility.owners.at(2).losses[511]==UINT16_MAX,"battle-history copy aliases input or equality omits loss history");GT_OK();
	});
	task->AddTest("SMAC simulation rejects malformed battle history before restoring a campaign",GT(){
		const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();const auto value=State();
		const auto prefix=Wire(value,1,12,nullptr,nullptr,nullptr,nullptr,nullptr,true);
		for(int scenario=0;scenario<14;++scenario) {
			types::Buffer suffix;suffix.WriteInt(scenario==0 ? 1 : 2);
			for(int owner=0;owner<2;++owner) {
				suffix.WriteInt(scenario==1 && owner==1 ? 2 : scenario==2 && owner==1 ? 3 : owner==0 ? 2 : 7);
				for(int i=0;i<8;++i) {suffix.WriteInt(scenario==3 && i==0 ? -1 : scenario==4 && i==0 ? int64_t(UINT32_MAX)+1 : 0);suffix.WriteInt(0);suffix.WriteInt(-1);suffix.WriteInt(0);suffix.WriteInt(0);}
				suffix.WriteInt(0);suffix.WriteInt(0);suffix.WriteInt(scenario==5 ? -1 : scenario==6 ? 513 : 2);
				suffix.WriteInt(scenario==7 ? -1 : scenario==8 ? 512 : 2);suffix.WriteInt(scenario==9 ? 0 : scenario==10 ? 65536 : 1);
				suffix.WriteInt(scenario==11 ? 2 : 511);suffix.WriteInt(65535);
			}
			auto raw=prefix+suffix.ToString();if(scenario==12) {raw.pop_back();}if(scenario==13) {raw+='x';}
			GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(raw,technologies,profiles,units);}),"malformed hostile history accepted, case "+std::to_string(scenario));
		}
		GT_OK();
	});
	task->AddTest("SMAC simulation retains independent activity restart prohibition and Do or Die",GT(){
		const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
		const auto initial=State();
		GT_ASSERT(!initial.do_or_die && initial.roster[0].active && !initial.roster[0].restart_prohibited,"standard new-campaign lifecycle differs");
		for(int scenario=0;scenario<4;++scenario) {
			auto value=initial;value.do_or_die=scenario&1;value.roster[0].active=!(scenario&2);value.roster[1].restart_prohibited=true;
			const auto wire=value.Serialize(technologies,profiles,units).ToString();
			auto restored=SmacSimulationState::Deserialize(wire,technologies,profiles,units);
			GT_ASSERT(restored==value && wire==CurrentWire(value) && restored.Serialize(technologies,profiles,units).ToString()==wire,
				"lifecycle restore inferred activity/restart count, reset technologies or changed identities");
			restored.roster[1].restart_prohibited=false;
			GT_ASSERT(value.roster[1].restart_prohibited && !(restored==value),"lifecycle aliases input or equality omits prohibition");
		}
		for(int scenario=0;scenario<7;++scenario) {
			types::Buffer tail;
			if(scenario==0){tail.WriteInt(0);}else{tail.WriteBool(false);}
			tail.WriteInt(scenario==1 ? 1 : 2);
			tail.WriteInt(scenario==2 ? 2 : 7);
			if(scenario==3){tail.WriteInt(1);}else{tail.WriteBool(true);}
			tail.WriteBool(false);tail.WriteInt(2);tail.WriteBool(false);
			if(scenario==4){tail.WriteInt(0);}else{tail.WriteBool(true);}
			auto raw=tail.ToString();if(scenario==5){raw.pop_back();}if(scenario==6){raw+='x';}
			GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(Wire(initial,SIZE_MAX,0,nullptr,nullptr,nullptr,nullptr,&raw),technologies,profiles,units);}),
				"malformed lifecycle count/identity/bool or framing accepted");
		}
		GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(Wire(initial,1,10),technologies,profiles,units);}),"pre-lifecycle simulation accepted");GT_OK();
	});
	task->AddTest( "SMAC simulation economy preserves owner rates and balances on restore", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		value.economy.at( 7 ) = { { 1, 7 }, INT32_MIN, INT32_MAX, 99, 0xa5 };
		const auto wire = value.Serialize( technologies, profiles, units ).ToString();
		const auto restored = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
		GT_ASSERT( restored == value && wire == CurrentWire( value ) && restored.Serialize( technologies, profiles, units ).ToString() == wire, "economy restore reset or lost supplied owner state" );
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value, 1, 8 ), technologies, profiles, units ); } ), "old simulation without economy accepted" );
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			auto bad = value; bad.economy.erase( 2 );
			if ( scenario == 1 ) { bad.economy.emplace( 3, value.economy.at( 2 ) ); }
			if ( scenario == 2 ) { bad.economy = value.economy; bad.economy.emplace( 3, value.economy.at( 2 ) ); }
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ), "missing or foreign financial owner admitted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC orbital intake matches sixteen original independent resource prefixes", GT() {
		for ( const auto& c : fixtures::ORBITAL_INTAKE ) {
			const faction::SmacSatelliteCounts counts{ c.counts };
			GT_ASSERT( counts.Intake( c.population, c.aerospace, c.owned_elevator ) == c.expected, "original orbital prefix differs: " + std::string( c.id ) );
			GT_ASSERT( counts.resources == c.counts && counts.Intake( c.population, c.aerospace, c.owned_elevator ) == c.expected, "per-Base query consumed the faction assets" );
		}
		const faction::SmacSatelliteCounts maximum{ { INT32_MAX, INT32_MIN, 1 } };
		GT_ASSERT( Rejects( [ & ] { maximum.Intake( 4, false, false ); } ), "unmeasured increment overflow accepted" );
		GT_ASSERT( maximum.Intake( 4, true, false ) == ( std::array< int32_t, 3 >{ 4, INT32_MIN, 1 } ), "unused halving overflow incorrectly rejected full output" );
		GT_ASSERT( Rejects( [ & ] { maximum.Intake( -1, true, false ); } ) && Rejects( [ & ] { maximum.Intake( 128, true, false ); } ), "unsupported population accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation satellite assets restore non-dense owners and exact signed counters", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		GT_ASSERT( value.GetSatelliteCounts( 7 ).resources == ( std::array< int32_t, 3 >{ 0, 0, 0 } ), "explicit new-campaign initialization differs" );
		value.satellites.at( 7 ) = { { 3, 7, 19 } }; value.satellites.at( 2 ) = { { INT32_MIN, -1, INT32_MAX } };
		const auto wire = value.Serialize( technologies, profiles, units ).ToString();
		auto restored = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
		GT_ASSERT( restored == value && wire == CurrentWire( value ) && restored.Serialize( technologies, profiles, units ).ToString() == wire, "restore reset/reordered satellite assets or changed framing" );
		auto copy = restored.GetSatelliteCounts( 7 ); copy.resources[ 0 ] = 100;
		restored.satellites.at( 7 ).resources[ 1 ] = 99;
		GT_ASSERT( value.GetSatelliteCounts( 7 ).resources == ( std::array< int32_t, 3 >{ 3, 7, 19 } ) && !( restored == value ), "counter copy aliases owner or equality ignores assets" );
		GT_ASSERT( Rejects( [ & ] { value.GetSatelliteCounts( 1 ); } ), "missing owner silently became zero" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation satellite suffix rejects missing duplicate and oversized records", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const auto value = State(); const auto decode = [ & ]( const std::string& wire ) { SmacSimulationState::Deserialize( wire, technologies, profiles, units ); };
		GT_ASSERT( Rejects( [ & ] { decode( Wire( value, 1, 7 ) ); } ), "pre-satellite SimulationV7 accepted" );
		for ( int scenario = 0; scenario < 12; ++scenario ) {
			types::Buffer suffix;
			suffix.WriteInt( scenario == 0 ? -1 : scenario == 1 ? 0 : scenario == 2 ? 8 : scenario == 3 ? 1 : 2 );
			suffix.WriteInt( scenario == 4 ? 0 : scenario == 5 ? 8 : scenario == 6 ? 3 : 2 );
			suffix.WriteInt( scenario == 8 ? int64_t( INT32_MAX ) + 1 : scenario == 9 ? int64_t( INT32_MIN ) - 1 : 3 );
			suffix.WriteInt( 4 ); suffix.WriteInt( 5 ); suffix.WriteInt( scenario == 7 ? 2 : 7 );
			suffix.WriteInt( 6 ); suffix.WriteInt( 7 ); suffix.WriteInt( 8 );
			auto raw = suffix.ToString(); if ( scenario == 10 ) { raw.clear(); } if ( scenario == 11 ) { raw.pop_back(); }
			GT_ASSERT( Rejects( [ & ] { decode( Wire( value, SIZE_MAX, 0, nullptr, nullptr, nullptr, &raw ) ); } ), "invalid satellite suffix accepted: " + std::to_string( scenario ) );
		}
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			auto bad = value; bad.satellites.erase( 2 );
			if ( scenario == 1 ) { bad.satellites.emplace( 3, faction::SmacSatelliteCounts::NewWorld() ); }
			if ( scenario == 2 ) { bad.satellites = value.satellites; bad.satellites.emplace( 3, faction::SmacSatelliteCounts::NewWorld() ); }
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ) && Rejects( [ & ] { bad.GetSatelliteCounts( 7 ); } ), "incomplete/extra satellite ownership accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "movement history simulation optional maximum bitmap survives exact codec copy", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		for ( const bool present : { false, true } ) {
			if ( present ) { value.movement_history = SmacMovementHistory::NewGeneratedWorld( 512, 256 ); value.movement_history->RetainNearest( 9999 ); }
			const auto wire = value.Serialize( technologies, profiles, units ).ToString();
			auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
			GT_ASSERT( wire == CurrentWire( value ) && wire.size() <= SmacSimulationState::MAX_WIRE_BYTES && decoded == value, "independent simulation framing or history restore differs" );
			if ( present ) { decoded.movement_history->RetainNearest( {} ); GT_ASSERT( value.movement_history->NearestDistance() == 9999 && !( decoded == value ), "history copy aliases nearest authority" ); }
		}
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value, 1, 6 ), technologies, profiles, units ); } ), "pre-history V6 accepted" );
		std::string large( SmacMovementHistory::MAX_WIRE_BYTES + 1, 'x' ), message;
		try { SmacSimulationState::Deserialize( Wire( value, SIZE_MAX, 0, nullptr, nullptr, &large ), technologies, profiles, units ); }
		catch ( const std::runtime_error& error ) { message = error.what(); }
		GT_ASSERT( message.find( "payload limit" ) != std::string::npos, "history oversized frame reached nested allocation instead of outer admission" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation resource modifiers retain independent phases and council identity", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const faction::SmacGlobalResourceModifiers reset{ 0, false, false, {} };
		GT_ASSERT( faction::SmacGlobalResourceModifiers::NewWorld() == reset, "new-world resource modifiers differ from original reset" );
		for ( const auto dust : { INT32_MIN, -1, 0, 1, INT32_MAX } ) {
			for ( const bool ordinary : { false, true } ) { for ( const bool upkeep : { false, true } ) {
				for ( const std::optional< int32_t > governor : { std::optional< int32_t >{}, { 1 }, { 2 }, { 7 } } ) {
					auto value = State(); value.resource_modifiers = { dust, ordinary, upkeep, governor };
					const auto wire = value.Serialize( technologies, profiles, units ).ToString();
					auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
					GT_ASSERT( decoded == value && decoded.Serialize( technologies, profiles, units ).ToString() == wire, "restore aged events collapsed phases or rebound council ID" );
					decoded.resource_modifiers = reset;
					GT_ASSERT( value.resource_modifiers == ( faction::SmacGlobalResourceModifiers{ dust, ordinary, upkeep, governor } ), "copied modifier state aliases original" );
				}
			} }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC simulation resource modifiers reject missing malformed and oversized late fields", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const auto decode = [ & ]( const std::string& wire ) { SmacSimulationState::Deserialize( wire, technologies, profiles, units ); };
		for ( int scenario = 0; scenario < 13; ++scenario ) {
			types::Buffer suffix;
			suffix.WriteInt( scenario == 1 ? int64_t( INT32_MAX ) + 1 : scenario == 2 ? int64_t( INT32_MIN ) - 1 : 10 );
			if ( scenario == 3 ) { suffix.WriteInt( 1 ); } else { suffix.WriteBool( true ); }
			if ( scenario == 4 ) { suffix.WriteInt( 0 ); } else { suffix.WriteBool( false ); }
			if ( scenario == 5 ) { suffix.WriteInt( 1 ); } else { suffix.WriteBool( true ); }
			suffix.WriteInt( scenario == 6 ? 0 : scenario == 7 ? 8 : scenario == 8 ? -1 : scenario == 9 ? INT64_MAX : 7 );
			auto raw = suffix.ToString();
			if ( scenario == 0 ) { raw.clear(); }
			if ( scenario == 10 ) { raw.pop_back(); }
			if ( scenario == 11 ) { raw += "x"; }
			if ( scenario == 12 ) { raw[ 19 ] = 2; raw[ 20 ] = 2; } // invalid first Bool payload with matching checksum
			GT_ASSERT( Rejects( [ & ] { decode( Wire( State(), SIZE_MAX, 0, nullptr, &raw ) ); } ), "invalid resource suffix accepted" );
		}
		for ( const auto id : { INT32_MIN, -1, 0, 8, INT32_MAX } ) {
			auto bad = State(); bad.resource_modifiers.planetary_governor = id;
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ), "invalid council identity serialized" );
		}
		GT_ASSERT( Rejects( [ & ] { decode( Wire( State(), 1, 5 ) ); } ), "pre-modifier V5 accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation movement capability is explicit bounded and preserved on restore", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		for ( const auto capability : { SmacSimulationState::MovementCapability::Unavailable, SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain } ) {
			auto value = State(); value.movement_capability = capability;
			const auto decoded = SmacSimulationState::Deserialize( value.Serialize( technologies, profiles, units ).ToString(), technologies, profiles, units );
			GT_ASSERT( decoded == value && decoded.movement_capability == capability, "restoration inferred supported campaign from absent project data" );
		}
		for ( const int64_t invalid : { int64_t( -1 ), int64_t( 2 ), INT64_MIN, INT64_MAX } ) {
			auto value = State(); value.movement_capability = static_cast< SmacSimulationState::MovementCapability >( invalid );
			GT_ASSERT( Rejects( [ & ] { value.Serialize( technologies, profiles, units ); } )
				&& Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value ), technologies, profiles, units ); } ), "unknown capability accepted" );
		}
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( State(), 1, 4 ), technologies, profiles, units ); } ), "pre-capability V4 accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation gameplay RNG pair is required bounded and exactly owned", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		for ( const uint32_t shared : { uint32_t( 0 ), UINT32_MAX } ) {
			for ( const uint32_t crt : { uint32_t( 0 ), UINT32_MAX } ) {
				value.gameplay_random = SmacGameplayRandomState( shared, crt );
				const auto wire = value.Serialize( technologies, profiles, units ).ToString();
				auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
				GT_ASSERT( decoded == value && value.gameplay_random.Shared() == shared && value.gameplay_random.Crt() == crt, "writer/reader changed zero or endpoint words" );
				const auto saved = value.gameplay_random; auto copy = decoded;
				for ( size_t i = 0; i < 32; ++i ) {
					GT_ASSERT( copy.gameplay_random.NextSharedLow16() == decoded.gameplay_random.NextSharedLow16() && copy.gameplay_random.NextCrtRaw15() == decoded.gameplay_random.NextCrtRaw15(), "owned pair continuation changed after copy" );
				}
				GT_ASSERT( value.gameplay_random == saved && copy == decoded, "copy borrowed original pair state" );
			}
		}
		for ( size_t mode = 0; mode < 6; ++mode ) {
			auto random = value.gameplay_random.Serialize();
			if ( mode == 0 ) { random[ 0 ] = 'X'; }
			if ( mode == 1 ) { random[ 4 ] = 2; }
			if ( mode == 2 ) { random.pop_back(); }
			if ( mode == 3 ) { random.push_back( 'x' ); }
			if ( mode == 4 ) { random.assign( 4096, 'x' ); }
			if ( mode == 5 ) { random.clear(); }
			std::string message;
			try { SmacSimulationState::Deserialize( Wire( value, SIZE_MAX, 0, &random ), technologies, profiles, units ); } catch ( const std::runtime_error& error ) { message = error.what(); }
			GT_ASSERT( !message.empty() && ( ( message.find( "payload limit" ) != std::string::npos ) == ( mode == 3 || mode == 4 ) ), "pair rejection confused outer limit with nested codec validation" );
		}
		GT_ASSERT( Rejects( [ & ]() { SmacSimulationState::Deserialize( Wire( value, 1, 3 ), technologies, profiles, units ); } ), "old simulation V3 accepted without a gameplay continuation contract" );
		types::Buffer large; large.WriteString( std::string( SmacGameplayRandomState::WIRE_SIZE + 1, 'x' ) ); const auto* cursor = large.dr;
		GT_ASSERT( Rejects( [ & ]() { large.ReadString( SmacGameplayRandomState::WIRE_SIZE ); } ) && large.dr == cursor && large.lenr == 0, "over-budget pair was copied/consumed before admission" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation codec owns explicit non-dense identities and exact technology continuation", GT() {
		const auto expected = State();
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions();
		auto decoded = SmacSimulationState::Deserialize( expected.Serialize( technologies, profiles, smac_startup_fixture::Units() ).ToString(), technologies, profiles, smac_startup_fixture::Units() );
		GT_ASSERT( decoded == expected && decoded.FindSlot( 6 )->simulation_id == 2 && !decoded.FindSlot( 1 ), "restoration reallocated simulation IDs or replayed named grants" );
		auto copied = decoded; decoded.roster.clear(); decoded.technology.factions.clear();
		GT_ASSERT( copied == expected && SmacSimulationState::Deserialize( Wire( copied ), technologies, profiles, smac_startup_fixture::Units() ) == expected, "runtime state borrowed input or copy storage" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation codec rejects malformed identity and late research records", GT() {
		const auto expected = State();
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions();
		const auto decode = [ & ]( const std::string& bytes ) { SmacSimulationState::Deserialize( bytes, technologies, profiles, smac_startup_fixture::Units() ); };
		for ( const auto index : { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u } ) {
			for ( const int64_t value : { INT64_MIN, int64_t( -1 ), INT64_MAX, int64_t( 256 ) } ) { GT_ASSERT( Rejects( [ & ]() { decode( Wire( expected, index, value ) ); } ), "invalid identity integer accepted" ); }
		}
		const auto wire = Wire( expected );
		for ( const auto size : { size_t( 0 ), size_t( 1 ), wire.size() - 1 } ) { GT_ASSERT( Rejects( [ & ]() { decode( wire.substr( 0, size ) ); } ), "truncated state accepted" ); }
		GT_ASSERT( Rejects( [ & ]() { decode( wire + "x" ); } ) && Rejects( [ & ]() { decode( std::string( SmacSimulationState::MAX_WIRE_BYTES + 1, 'x' ) ); } ), "trailing or oversized runtime state accepted" );
		for ( size_t scenario = 0; scenario < 13; ++scenario ) {
			auto bad = expected;
			if ( scenario == 0 ) { bad.roster[ 1 ].simulation_id = 7; }
			if ( scenario == 1 ) { bad.roster[ 1 ].slot = 0; }
			if ( scenario == 2 ) { bad.roster[ 1 ].gsid = bad.roster[ 0 ].gsid; }
			if ( scenario == 3 ) { bad.roster[ 1 ].gsid = std::string( "a\0b", 3 ); }
			if ( scenario == 4 ) { bad.roster[ 1 ].faction_id = "GAIANS"; }
			if ( scenario == 5 ) { bad.roster.pop_back(); }
			if ( scenario == 6 ) { bad.technology.factions[ 1 ].target = 89; }
			if ( scenario == 7 ) { bad.technology.factions[ 1 ].cached_threshold = -1; }
			if ( scenario == 8 ) { bad.roster[ 1 ].gsid.assign( SmacSimulationState::MAX_TEXT_BYTES, 'a' ); }
			if ( scenario == 9 ) { bad.social.factions.pop_back(); }
			if ( scenario == 10 ) { bad.social.factions[ 1 ].admission.simulation_id = 5; }
			if ( scenario == 11 ) { bad.social.factions[ 1 ].admission.faction_id = "HIVE"; }
			if ( scenario == 12 ) { bad.social.factions.push_back( { { 4, "HIVE" } } ); }
			GT_ASSERT( Rejects( [ & ]() { bad.Serialize( technologies, profiles, smac_startup_fixture::Units() ); } ), "invalid complete runtime state serialized" );
			if ( scenario < 6 || scenario >= 8 ) { GT_ASSERT( Rejects( [ & ]() { decode( Wire( bad ) ); } ), "invalid complete runtime state decoded" ); }
		}
		GT_OK();
	} );
	task->AddTest( "bounded Buffer string read rejects complete over-budget frames before cursor mutation", GT() {
		constexpr uint32_t cap = SmacSimulationState::MAX_WIRE_BYTES;
		types::Buffer exact; exact.WriteString( std::string( cap, 'x' ) ); exact.WriteInt( 91 );
		GT_ASSERT( exact.ReadString( cap ).size() == cap && exact.ReadInt() == 91, "exact-cap frame or following field changed" );
		types::Buffer oversized; oversized.WriteString( std::string( cap + 1, 'x' ) );
		const auto* cursor = oversized.dr;
		std::string error;
		try { oversized.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "payload limit" ) != std::string::npos && oversized.lenr == 0 && oversized.dr == cursor, "complete cap+1 frame was accepted or advanced cursor" );
		GT_ASSERT( oversized.ReadString().size() == cap + 1, "default string reader changed its admitted size" );
		types::Buffer wrong_type; wrong_type.WriteInt( cap + 1 ); error.clear();
		try { wrong_type.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "unexpected type" ) != std::string::npos && wrong_type.lenr == 0, "type error was confused with length admission" );
		types::Buffer truncated( exact.ToString().substr( 0, 8 ) ); error.clear();
		try { truncated.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "prematurely" ) != std::string::npos && truncated.lenr == 0, "truncated frame bypassed extent validation" );
		GT_OK();
	} );
}

}
