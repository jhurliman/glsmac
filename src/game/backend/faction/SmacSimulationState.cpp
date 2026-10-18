#include "SmacSimulationState.h"
#include "SmacRankingScore.h"

#include <set>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace game::backend::faction {

namespace {
int32_t RankingChecked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Ordinary ranking exceeds signed32 arithmetic");}
    return static_cast<int32_t>(value);
}
bool RankingTechnologyAvailable(const settings::SmacTechnologyReference& reference,
    const SmacFactionTechnologyState& owner,const settings::SmacTechnologyRules& technologies) {
    using Kind=settings::SmacTechnologyReference::Kind;
    if(reference.kind==Kind::NONE) {return true;}
    if(reference.kind==Kind::DISABLED) {return false;}
    if(reference.kind!=Kind::TECHNOLOGY_ID || !reference.technology_id) {throw std::runtime_error("Unknown ranking technology reference");}
    const auto id=*reference.technology_id;
    if(id<0 || id>=88 || !owner.owned[id]) {return false;}
    const auto& row=technologies.technologies[id];
    const auto first=technologies.ResolveToken(row.prerequisites[0]).kind;
    const auto second=technologies.ResolveToken(row.prerequisites[1]).kind;
    if(first==Kind::UNKNOWN || second==Kind::UNKNOWN) {throw std::runtime_error("Unknown ranking technology validity");}
    return first!=Kind::DISABLED && (second!=Kind::DISABLED || first==Kind::NONE);
}
}

void SmacCapabilityState::Refresh(int32_t owner,const SmacFactionTechnologyState& technology,
    const settings::SmacTechnologyRules& technologies,const unit::SmacPredefinedUnitCatalog& catalog,
    const settings::SmacCombatRules& combat,const std::array<uint32_t,8>& relations,
    int32_t psi_percent,bool dream_twister,bool neural_amplifier,bool no_colony_pods) {
    if(owner<1 || owner>7 || technology.admission.simulation_id!=owner || catalog.weapons.weapons.size()!=21 ||
        catalog.defenses.defenses.size()!=10 || catalog.chassis.chassis.size()!=9) {
        throw std::runtime_error("Capability refresh requires its positive owner and complete component tables");
    }
    const auto& previous=factions.at(owner-1).foreign;
    const auto signed_byte=[](int32_t raw) {const auto bits=static_cast<uint32_t>(raw)&255u;return bits<128?int32_t(bits):int32_t(bits)-256;};
    const auto available=[&](const auto& reference) {return RankingTechnologyAvailable(reference,technology,technologies);};
    const auto evaluate=[&](int32_t raw,bool weapon) {
        if(raw>=0) {return raw;}
        if(!combat.psi_ratios || combat.psi_ratios->at(0)[0]<=0 || combat.psi_ratios->at(0)[1]<=0) {
            throw std::runtime_error("PSI capability requires retained positive land combat ratios");
        }
        const auto offense=combat.psi_ratios->at(0)[0],defense=combat.psi_ratios->at(0)[1];
        auto value=weapon?RankingChecked(int64_t(previous[Armor])*offense)/defense:
            RankingChecked(int64_t(previous[Weapon])*defense)/offense;
        value=RankingChecked(int64_t(value)*RankingChecked(int64_t(100)+psi_percent))/100;
        if(weapon?dream_twister:neural_amplifier) {value=RankingChecked(int64_t(value)+value/2);}
        return value;
    };
    const auto native=std::find_if(catalog.units.begin(),catalog.units.end(),[](const auto& row){return row.source_id==8;});
    if(native==catalog.units.end()) {throw std::runtime_error("Capability refresh requires predefined design8");}
    const bool native_available=native->active && !(native->faction_exclusion&(1u<<owner)) && available(native->prerequisite)
        && !(native->plan==8 && no_colony_pods);
    Faction next;
    next.available={1,1,1,native_available?evaluate(signed_byte(catalog.weapons.weapons[12].offense),true):0,
        native_available?evaluate(signed_byte(catalog.defenses.defenses[9].rating),false):0};
    for(const auto& row:catalog.weapons.weapons) {
        const auto raw=signed_byte(row.offense);
        if(raw>=99 || !available(technologies.ResolveToken(row.prerequisite))) {continue;}
        const auto value=evaluate(raw,true);
        next.available[Weapon]=std::max(next.available[Weapon],value);
        if(raw<0) {next.available[PsiWeapon]=std::max(next.available[PsiWeapon],value);}
    }
    for(const auto& row:catalog.defenses.defenses) {
        if(!available(technologies.ResolveToken(row.prerequisite))) {continue;}
        const auto raw=signed_byte(row.rating),value=evaluate(raw,false);
        next.available[Armor]=std::max(next.available[Armor],value);
        if(raw<0) {next.available[PsiArmor]=std::max(next.available[PsiArmor],value);}
    }
    for(const auto& row:catalog.chassis.chassis) {
        if(row.triad==loader::txt::ChassisTriad::Land && available(technologies.ResolveToken(row.prerequisite))) {
            next.available[LandSpeed]=std::max(next.available[LandSpeed],int32_t(static_cast<uint8_t>(row.speed)));
        }
    }
    for(size_t tier=0;tier<4 && next.foreign[Weapon]==0;++tier) {
        for(size_t peer=1;peer<=7;++peer) {
            const auto flags=relations[peer];
            if(peer==static_cast<size_t>(owner) || (flags&1) || (tier<3 && !(flags&8)) ||
                (tier<2 && (flags&2)) || (tier==0 && !(flags&16))) {continue;}
            const auto& values=factions[peer-1].available;
            for(size_t i=0;i<values.size();++i) {next.foreign[i]=std::max(next.foreign[i],values[i]);}
        }
    }
    factions[owner-1]=next; // no partial cache publication on failed eligibility/arithmetic
}

void SmacSimulationState::RefreshCapabilities(int32_t owner,const settings::SmacTechnologyRules& technologies,
    const settings::SmacPredefinedUnitRules& units,const settings::SmacCombatRules& combat,bool no_colony_pods) {
    if(!capabilities) {return;} // preserve the explicit missing history of old port saves
    if(movement_capability!=MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Live capability refresh requires admitted project authority");
    }
    const auto* owned=technology.Find(owner);
    if(!owned) {throw std::runtime_error("Capability refresh has no owned technology record");}
    std::array<uint32_t,8> relations{};
    const auto& directions=hostility.owners.at(owner).directions;
    for(size_t i=0;i<relations.size();++i) {relations[i]=directions[i].primary;}
    // The current validated faction-keyword domain has no PSI scalar writer;
    // its definition constructor supplies0. Projects are explicitly excluded by
    // the campaign capability above, rather than inferred from missing Bases.
    capabilities->Refresh(owner,*owned,technologies,units.catalog,combat,relations,0,false,false,no_colony_pods);
}

int32_t CalculateSmacOrdinaryRankingScore(const SmacFactionTechnologyState& owner,
    int32_t population,const SmacDesignCounts& counts,const settings::SmacTechnologyRules& technologies,
    const unit::SmacPredefinedUnitCatalog& catalog,const unit::SmacStartupDesignState& designs) {
    if(population<0 || owner.future_count<0) {throw std::invalid_argument("Ordinary ranking requires nonnegative population and Future counter");}
    int32_t result=RankingChecked(int64_t(RankingChecked(int64_t(population)+owner.future_count))*4);
    for(int32_t id=0;id<88;++id) {
        if(!RankingTechnologyAvailable({settings::SmacTechnologyReference::Kind::TECHNOLOGY_ID,id},owner,technologies)) {continue;}
        for(const auto rating:technologies.technologies[id].priorities) {result=RankingChecked(int64_t(result)+rating);}
    }
    int32_t reference=1;
    for(const auto& faction:designs.owners) {
        for(const auto& design:faction.designs) {
            if(!design.active || !design.prototyped) {continue;}
            const auto power=catalog.weapons.weapons.at(design.weapon).offense;
            if(power>reference && power<99) {reference=power;}
        }
    }
    for(size_t id=0;id<counts.size();++id) {
        const auto count=counts[id];
        if(!count || count>250) {continue;}
        int32_t cost=0,power=0;
        if(id<64) {
            const auto row=std::find_if(catalog.units.begin(),catalog.units.end(),[&](const auto& row){return row.source_id==id;});
            if(row==catalog.units.end()) {throw std::runtime_error("Counted predefined ranking design is absent");}
            if(!RankingTechnologyAvailable(row->prerequisite,owner,technologies)) {continue;}
            cost=row->cost;power=catalog.weapons.weapons.at(row->weapon_id).offense;
        } else {
            const unit::SmacCustomDesign* found=nullptr;
            for(const auto& faction:designs.owners) {
                for(const auto& design:faction.designs) {if(design.id==static_cast<int32_t>(id)) {found=&design;}}
            }
            if(!found) {throw std::runtime_error("Counted custom ranking design is absent");}
            cost=found->cost;power=catalog.weapons.weapons.at(found->weapon).offense;
        }
        const auto weighted=RankingChecked(int64_t(count)*cost);
        const auto term=power==0 ? weighted/4 : power<0 ? weighted/2 : power>=99 ? weighted : RankingChecked(int64_t(power/reference)*weighted);
        result=RankingChecked(int64_t(result)+term);
    }
    return result;
}


bool SmacHostileDirection::operator==(const SmacHostileDirection& other) const {
	return primary==other.primary && secondary==other.secondary && transition_turn==other.transition_turn && paired_a==other.paired_a && paired_b==other.paired_b;
}
SmacHostileOwner SmacHostileOwner::NewWorld(const std::array<uint32_t,8>& retained_secondary) {
	SmacHostileOwner result;for(size_t i=0;i<8;++i) {result.directions[i].secondary=retained_secondary[i];}return result;
}
bool SmacHostileOwner::operator==(const SmacHostileOwner& other) const {
	return directions==other.directions && losses==other.losses && integrity==other.integrity && bookkeeping==other.bookkeeping;
}
bool SmacHostileState::operator==(const SmacHostileState& other) const {return owners==other.owners;}
void SmacHostileState::Reinitialize(int32_t owner) {
	if(owner<1 || owner>7) {throw std::runtime_error("Invalid reinitialized history owner");}
	auto& row=owners.at(owner);
	// Actual nonzero initializer revokes this mask mutually, retaining history.
	for(auto& direction:row.directions) {direction.primary&=0x6fff00b7u;}
	for(auto& entry:owners) {entry.second.directions.at(owner).primary&=0x6fff00b7u;}
}
void SmacHostileState::RequireOrdinaryHostility(int32_t actor,int32_t target) const {
	if(actor<1 || actor>7 || target<1 || target>7 || actor==target || !owners.count(actor) || !owners.count(target)) {throw std::runtime_error("Hostile action requires two admitted factions");}
	for(const auto& pair:{std::pair<int32_t,int32_t>{actor,target},{target,actor}}) {
		const auto& direction=owners.at(pair.first).directions[pair.second];
		if(direction.secondary || (direction.primary!=0 && direction.primary!=16 && direction.primary!=0x80008118u && direction.primary!=0x8000a118u)) {
			throw std::runtime_error("Hostile movement requires its ordinary no-peace or Vendetta diplomacy context");
		}
	}
}
void SmacHostileState::RecordScoutBattle(int32_t actor,int32_t target,int32_t winner,int32_t loser,int32_t lost_design) {
	RequireOrdinaryHostility(actor,target);
	if(!((winner==actor && loser==target)||(winner==target && loser==actor)) || lost_design<0 || lost_design>=512) {throw std::runtime_error("Invalid Scout battle history identity");}
	// Hostile movement uses mask32784 in the mutual setter, not war-break work.
	owners.at(actor).directions[target].primary|=0x80008118u;owners.at(target).directions[actor].primary|=0x80008118u;
	auto& direction=owners.at(winner).directions[loser];++direction.paired_a;++direction.paired_b;
	++owners.at(loser).losses[lost_design]; // actual unsigned DWORD/WORD stores
}
namespace {
constexpr int64_t MAGIC = 0x534d5353; // SMSS, mutable world state, not settings
constexpr int64_t VERSION = 17;

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC simulation integer outside signed32" ); }
	return static_cast< int32_t >( value );
}
uint32_t ReadUInt32(types::Buffer& buffer) {
	const auto value=buffer.ReadInt();
	if(value<0 || value>UINT32_MAX) {throw std::runtime_error("SMAC history integer outside unsigned32");}
	return static_cast<uint32_t>(value);
}

using StrategyRecords=std::optional<std::map<int32_t,SmacFactionStrategy>>;
void WriteStrategy(types::Buffer& buffer,const StrategyRecords& strategy) {
	buffer.WriteBool(strategy.has_value());
	if(strategy) {
		for(const auto& [owner,state]:*strategy) {
			buffer.WriteInt(owner);
			for(const auto priority:state.priorities) {buffer.WriteInt(priority);}
		}
	}
}
StrategyRecords ReadStrategy(types::Buffer& buffer,size_t count) {
	if(!buffer.ReadBool()) {return {};}
	StrategyRecords result{std::in_place};
	for(size_t i=0;i<count;++i) {
		const auto owner=ReadInt32(buffer);
		SmacFactionStrategy state{};
		for(auto& priority:state.priorities) {priority=ReadInt32(buffer);}
		if(owner<1 || owner>7 || !result->emplace(owner,state).second) {throw std::runtime_error("Invalid or duplicate SMAC strategy owner");}
	}
	return result;
}
void WriteCapabilities(types::Buffer& buffer,const std::optional<SmacCapabilityState>& capabilities) {
    buffer.WriteBool(capabilities.has_value());
    if(capabilities) {for(const auto& owner:capabilities->factions) {
        for(const auto& values:{owner.available,owner.foreign}) {for(const auto value:values) {buffer.WriteInt(value);}}
    }}
}
std::optional<SmacCapabilityState> ReadCapabilities(types::Buffer& buffer) {
    if(!buffer.ReadBool()) {return {};}
    std::optional<SmacCapabilityState> result{std::in_place};
    for(auto& owner:result->factions) {
        for(auto* values:{&owner.available,&owner.foreign}) {for(auto& value:*values) {value=ReadInt32(buffer);}}
    }
    return result;
}
}

std::array< int32_t, 3 > SmacSatelliteCounts::Intake( int32_t population, bool aerospace_complex, bool owns_space_elevator ) const {
	if ( population < 0 || population > 127 ) { throw std::invalid_argument( "Orbital intake population outside 0..127" ); }
	std::array< int32_t, 3 > result;
	for ( size_t i = 0; i < result.size(); ++i ) {
		auto count = resources[ i ];
		if ( !aerospace_complex && !owns_space_elevator ) {
			if ( count == INT32_MAX ) { throw std::invalid_argument( "Unmeasured orbital count increment overflow" ); }
			count = ( count + 1 ) / 2; // Signed truncation, including diagnostic negatives.
		}
		result[ i ] = std::min( count, population );
	}
	return result;
}

SmacGlobalResourceModifiers SmacGlobalResourceModifiers::NewWorld() noexcept { return { 0, false, false, {} }; }

void SmacGlobalResourceModifiers::Validate() const {
	// Preserve signed countdowns and both flags exactly; aging is a separate
	// turn-boundary operation. A council role is a faction ID, never a slot.
	if ( planetary_governor && ( *planetary_governor < 1 || *planetary_governor > 7 ) ) {
		throw std::runtime_error( "Invalid SMAC Planetary Governor simulation ID" );
	}
}

bool SmacGlobalResourceModifiers::operator==( const SmacGlobalResourceModifiers& other ) const noexcept {
	return atmospheric_dust_countdown == other.atmospheric_dust_countdown && solar_ordinary == other.solar_ordinary
		&& solar_upkeep == other.solar_upkeep && planetary_governor == other.planetary_governor;
}

bool SmacSimulationParticipant::operator==( const SmacSimulationParticipant& other ) const {
	return simulation_id == other.simulation_id && slot == other.slot && gsid == other.gsid && role == other.role && faction_id == other.faction_id
		&& active == other.active && restart_prohibited == other.restart_prohibited;
}

SmacFactionStrategy SmacFactionStrategy::NewWorld(const settings::SmacFactionStartupRule& profile) noexcept {
	const auto& p=profile.worker_priorities; // already frozen in semantic A/E/D/B/C order
	return {{p[1],p[2],p[3],p[4]}};
}

void SmacFactionStrategy::ApplyResearchPriorities(std::optional<uint8_t> accepted_mask) {
	if(!accepted_mask) {return;}
	if(*accepted_mask>15) {throw std::invalid_argument("Research priority mask exceeds four axes");}
	for(size_t axis=0;axis<priorities.size();++axis) {priorities[axis]=(*accepted_mask>>axis)&1u;}
}

void SmacSimulationState::Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const {
	if ( movement_capability != MovementCapability::Unavailable && movement_capability != MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Unknown SMAC supported-campaign movement capability" );
	}
	resource_modifiers.Validate();
	if(strategy) {
		if(strategy->size()!=roster.size()) {throw std::runtime_error("Incomplete SMAC faction strategy");}
		for(const auto& participant:roster) {
			if(!strategy->count(participant.simulation_id)) {throw std::runtime_error("Missing SMAC faction strategy owner");}
		}
	}
	technology.Validate( technologies, profiles );
	social.Validate( technologies, profiles );
	units.Validate( technologies ); designs.Validate( units.catalog );
    if(rank_order) {
        std::set<int32_t> identities;
        if(rank_order->faction_at_rank[0]) {throw std::runtime_error("Native faction cannot occupy retained rank zero");}
        for(const auto owner:rank_order->faction_at_rank) {
            if(!owner) {continue;}
            if(owner<1 || owner>7 || !identities.insert(owner).second
                || std::none_of(roster.begin(),roster.end(),[&](const auto& participant){return participant.simulation_id==owner;})) {
                throw std::runtime_error("Invalid retained ranking identity");
            }
        }
    }

	if ( roster.empty() || roster.size() > 7 || roster.size() != technology.factions.size() || roster.size() != designs.owners.size() || roster.size() != social.factions.size() || roster.size() != satellites.size() || roster.size() != economy.size() ) { throw std::runtime_error( "Incomplete SMAC simulation roster" ); }
	std::array< bool, 8 > ids{};
	std::array< bool, 7 > slots{};
	std::set< std::string > gsids;
	size_t text = 0;
	for ( const auto& participant : roster ) {
		if ( participant.simulation_id < 1 || participant.simulation_id > 7 || ids[ participant.simulation_id ]
			|| participant.slot < 0 || participant.slot > 6 || slots[ participant.slot ] ) { throw std::runtime_error( "Duplicate or invalid SMAC simulation identity" ); }
		ids[ participant.simulation_id ] = true; slots[ participant.slot ] = true;
		if ( !satellites.count( participant.simulation_id ) ) { throw std::runtime_error( "Missing SMAC faction satellite assets" ); }
		const auto finances = economy.find( participant.simulation_id );
		if ( finances == economy.end() ) { throw std::runtime_error( "Missing SMAC faction economy" ); }
		finances->second.Validate();
		if(!hostility.owners.count(participant.simulation_id)) {throw std::runtime_error("Missing SMAC faction hostile history");}
		if ( ( participant.slot == 0 && participant.role != roster::Role::Single && participant.role != roster::Role::Host )
			|| ( participant.slot != 0 && participant.role != roster::Role::Player ) ) { throw std::runtime_error( "Invalid SMAC simulation controller role" ); }
		for ( const auto* value : { &participant.gsid, &participant.faction_id } ) {
			if ( value->empty() || value->find( '\0' ) != std::string::npos || value->size() > MAX_TEXT_BYTES - text ) {
				throw std::runtime_error( "Invalid or oversized SMAC simulation controller text" );
			}
			text += value->size();
		}
		if ( !gsids.insert( participant.gsid ).second ) { throw std::runtime_error( "Duplicate SMAC simulation controller GSID" ); }
		const auto* state = technology.Find( participant.simulation_id );
		const auto* social_state = social.Find( participant.simulation_id );
		if ( !state || state->admission.faction_id != participant.faction_id || !designs.Find( participant.simulation_id )
			|| !social_state || social_state->admission.faction_id != participant.faction_id ) { throw std::runtime_error( "SMAC simulation roster and technology identities differ" ); }
	}
    uint8_t admitted=0;for(const auto& participant:roster) {admitted|=static_cast<uint8_t>(1u<<participant.simulation_id);}
    if(human_membership & static_cast<uint8_t>(~admitted)) {throw std::runtime_error("Human membership names an unadmitted faction");}
	if(hostility.owners.size()!=roster.size()) {throw std::runtime_error("Unexpected SMAC hostile-history owner");}
	if(human_defeat_request) {
		const auto owner=std::find_if(roster.begin(),roster.end(),[&](const auto& row){return row.simulation_id==*human_defeat_request;});
		if(owner==roster.end() || owner->active || !IsHuman(owner->simulation_id)) {throw std::runtime_error("Human defeat request requires an inactive saved participant");}
	}
}

bool SmacSimulationState::operator==( const SmacSimulationState& other ) const { return roster == other.roster && technology == other.technology && designs == other.designs && social == other.social && gameplay_random == other.gameplay_random && movement_capability == other.movement_capability && resource_modifiers == other.resource_modifiers && movement_history == other.movement_history && satellites == other.satellites && economy == other.economy && do_or_die == other.do_or_die && hostility==other.hostility && human_defeat_request==other.human_defeat_request && rank_order==other.rank_order && human_membership==other.human_membership && strategy==other.strategy && capabilities==other.capabilities; }

SmacSatelliteCounts SmacSimulationState::GetSatelliteCounts( int32_t simulation_id ) const {
	if ( roster.empty() || roster.size() > 7 || satellites.size() != roster.size() ) { throw std::runtime_error( "Incomplete SMAC faction satellite assets" ); }
	for ( const auto& owner : roster ) {
		if ( !satellites.count( owner.simulation_id ) ) { throw std::runtime_error( "Missing SMAC faction satellite assets" ); }
	}
	const auto found = satellites.find( simulation_id );
	if ( simulation_id < 1 || simulation_id > 7 || found == satellites.end() ) { throw std::runtime_error( "SMAC faction satellite assets are unavailable" ); }
	return found->second;
}

const SmacSimulationParticipant* SmacSimulationState::FindSlot( size_t slot ) const {
	for ( const auto& participant : roster ) { if ( static_cast< size_t >( participant.slot ) == slot ) { return &participant; } }
	return nullptr;
}

types::Buffer SmacSimulationState::Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const {
	Validate( technologies, profiles, units );
	const auto state = technology.Serialize( technologies, profiles );
	const auto design_state = designs.Serialize( units.catalog );
	const auto social_state = social.Serialize( technologies, profiles );
	types::Buffer buffer;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( roster.size() );
	for ( const auto& participant : roster ) {
		buffer.WriteInt( participant.simulation_id ); buffer.WriteInt( participant.slot ); buffer.WriteString( participant.gsid );
		buffer.WriteInt( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( state.ToString() );
	buffer.WriteString( design_state.ToString() );
	buffer.WriteString( social_state.ToString() );
	buffer.WriteString( gameplay_random.Serialize() );
	buffer.WriteInt( static_cast< int64_t >( movement_capability ) );
	buffer.WriteInt( resource_modifiers.atmospheric_dust_countdown );
	buffer.WriteBool( resource_modifiers.solar_ordinary ); buffer.WriteBool( resource_modifiers.solar_upkeep );
	buffer.WriteBool( resource_modifiers.planetary_governor.has_value() );
	if ( resource_modifiers.planetary_governor ) { buffer.WriteInt( *resource_modifiers.planetary_governor ); }
	buffer.WriteBool( movement_history.has_value() );
	if ( movement_history ) { buffer.WriteString( movement_history->Serialize() ); }
	buffer.WriteInt( satellites.size() );
	for ( const auto& entry : satellites ) {
		buffer.WriteInt( entry.first );
		for ( const auto count : entry.second.resources ) { buffer.WriteInt( count ); }
	}
	buffer.WriteInt( economy.size() );
	for ( const auto& entry : economy ) { buffer.WriteInt( entry.first ); buffer.WriteString( entry.second.Serialize().ToString() ); }
	buffer.WriteBool( do_or_die );
	buffer.WriteInt( roster.size() );
	for ( const auto& participant : roster ) {
		buffer.WriteInt( participant.simulation_id ); buffer.WriteBool( participant.active ); buffer.WriteBool( participant.restart_prohibited );
	}
	buffer.WriteInt(hostility.owners.size());
	for(const auto& entry:hostility.owners) {
		buffer.WriteInt(entry.first);const auto& owner=entry.second;
		for(const auto& direction:owner.directions) {
			buffer.WriteInt(direction.primary);buffer.WriteInt(direction.secondary);buffer.WriteInt(direction.transition_turn);
			buffer.WriteInt(direction.paired_a);buffer.WriteInt(direction.paired_b);
		}
		buffer.WriteInt(owner.integrity);buffer.WriteInt(owner.bookkeeping);
		buffer.WriteInt(std::count_if(owner.losses.begin(),owner.losses.end(),[](auto value){return value!=0;}));
		for(size_t i=0;i<owner.losses.size();++i) {if(owner.losses[i]) {buffer.WriteInt(i);buffer.WriteInt(owner.losses[i]);}}
	}
	buffer.WriteBool(human_defeat_request.has_value());if(human_defeat_request) {buffer.WriteInt(*human_defeat_request);}
    buffer.WriteBool(rank_order.has_value());if(rank_order) {for(auto owner:rank_order->faction_at_rank) {buffer.WriteInt(owner);}}
    buffer.WriteInt(human_membership);
	WriteStrategy(buffer,strategy);
	WriteCapabilities(buffer,capabilities);
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC simulation state exceeds wire budget" ); }
	return buffer;
}

SmacSimulationState SmacSimulationState::Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
	const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC simulation state exceeds wire budget" ); }
	try {
		types::Buffer buffer( wire );
		if(buffer.ReadInt()!=MAGIC) {throw std::runtime_error("Unsupported SMAC simulation state format");}
		const auto version=buffer.ReadInt();
		if(version!=VERSION && version!=16 && version!=15 && version!=14 && version!=13 && version!=12 && version!=11) {throw std::runtime_error("Unsupported SMAC simulation state format");}
		const auto count = buffer.ReadInt();
		if ( count < 1 || count > 7 ) { throw std::runtime_error( "SMAC simulation roster requires 1..7 participants" ); }
		std::vector< SmacSimulationParticipant > roster;
		roster.reserve( static_cast< size_t >( count ) );
		for ( int64_t i = 0; i < count; ++i ) {
			SmacSimulationParticipant participant;
			participant.simulation_id = ReadInt32( buffer ); participant.slot = ReadInt32( buffer ); participant.gsid = buffer.ReadString();
			const auto role = buffer.ReadInt();
			if ( role < 1 || role > 3 ) { throw std::runtime_error( "Invalid SMAC simulation role" ); }
			participant.role = static_cast< roster::Role >( role ); participant.faction_id = buffer.ReadString();
			roster.push_back( std::move( participant ) );
		}
		auto technology = SmacTechnologyState::Deserialize( buffer.ReadString( SmacTechnologyState::MAX_WIRE_BYTES ), technologies, profiles );
		auto designs = unit::SmacStartupDesignState::Deserialize( buffer.ReadString( unit::SmacStartupDesignState::MAX_WIRE_BYTES ), units.catalog );
		auto social = SmacSocialState::Deserialize( buffer.ReadString( SmacSocialState::MAX_WIRE_BYTES ), technologies, profiles );
		const auto random = SmacGameplayRandomState::Deserialize( buffer.ReadString( SmacGameplayRandomState::WIRE_SIZE ) );
		const auto capability = buffer.ReadInt();
		if ( capability != 0 && capability != 1 ) { throw std::runtime_error( "Unknown SMAC supported-campaign movement capability" ); }
		const auto dust = ReadInt32( buffer );
		const auto ordinary = buffer.ReadBool(); const auto upkeep = buffer.ReadBool();
		std::optional< int32_t > governor;
		if ( buffer.ReadBool() ) { governor = ReadInt32( buffer ); }
		SmacSimulationState result( random, static_cast< MovementCapability >( capability ), { dust, ordinary, upkeep, governor } );
		if ( buffer.ReadBool() ) { result.movement_history = SmacMovementHistory::Deserialize( buffer.ReadString( SmacMovementHistory::MAX_WIRE_BYTES ) ); }
		const auto satellite_owners = buffer.ReadInt();
		if ( satellite_owners < 1 || satellite_owners > 7 || satellite_owners != count ) { throw std::runtime_error( "Invalid SMAC satellite owner count" ); }
		for ( int64_t i = 0; i < satellite_owners; ++i ) {
			const auto id = ReadInt32( buffer );
			SmacSatelliteCounts assets;
			for ( auto& resource : assets.resources ) { resource = ReadInt32( buffer ); }
			if ( id < 1 || id > 7 || !result.satellites.emplace( id, assets ).second ) { throw std::runtime_error( "Invalid or duplicate SMAC satellite owner" ); }
		}
		const auto economy_owners = buffer.ReadInt();
		if ( economy_owners < 1 || economy_owners > 7 || economy_owners != count ) { throw std::runtime_error( "Invalid SMAC economy owner count" ); }
		for ( int64_t i = 0; i < economy_owners; ++i ) {
			const auto id = ReadInt32( buffer );
			auto finances = SmacEconomyState::Deserialize( buffer.ReadString( SmacEconomyState::MAX_WIRE_BYTES ) );
			if ( id < 1 || id > 7 || !result.economy.emplace( id, std::move( finances ) ).second ) { throw std::runtime_error( "Invalid or duplicate SMAC economy owner" ); }
		}
		result.do_or_die = buffer.ReadBool();
		if ( buffer.ReadInt() != count ) { throw std::runtime_error( "Invalid SMAC faction lifecycle count" ); }
		for ( auto& participant : roster ) {
			if ( buffer.ReadInt() != participant.simulation_id ) { throw std::runtime_error( "SMAC faction lifecycle identity differs from its roster" ); }
			participant.active = buffer.ReadBool(); participant.restart_prohibited = buffer.ReadBool();
		}
		if(version==11) {
			// V11 had no legal combat/diplomacy-history writer. Its native campaigns
			// therefore migrate to the same pristine history, without replaying play.
			for(const auto& participant:roster) {result.hostility.owners.emplace(participant.simulation_id,SmacHostileOwner::NewWorld({}));}
		} else {
			if(buffer.ReadInt()!=count) {throw std::runtime_error("Invalid SMAC hostile-history owner count");}
			for(int64_t i=0;i<count;++i) {
				const auto id=ReadInt32(buffer);SmacHostileOwner owner;
				for(auto& direction:owner.directions) {
					direction.primary=ReadUInt32(buffer);direction.secondary=ReadUInt32(buffer);direction.transition_turn=ReadInt32(buffer);
					direction.paired_a=ReadUInt32(buffer);direction.paired_b=ReadUInt32(buffer);
				}
				owner.integrity=ReadUInt32(buffer);owner.bookkeeping=ReadUInt32(buffer);
				const auto losses=buffer.ReadInt();if(losses<0 || losses>512) {throw std::runtime_error("Invalid SMAC loss-history count");}
				int32_t previous=-1;
				for(int64_t j=0;j<losses;++j) {
					const auto design=ReadInt32(buffer);const auto value=ReadUInt32(buffer);
					if(design<=previous || design>=512 || value==0 || value>UINT16_MAX) {throw std::runtime_error("Invalid SMAC sparse loss history");}
					owner.losses[design]=static_cast<uint16_t>(value);previous=design;
				}
				if(id<1 || id>7 || !result.hostility.owners.emplace(id,std::move(owner)).second) {throw std::runtime_error("Invalid SMAC hostile-history owner");}
			}
		}
		if(version>=13 && buffer.ReadBool()) {result.human_defeat_request=ReadInt32(buffer);}
        if(version>=14 && buffer.ReadBool()) {
            result.rank_order=SmacRankOrder{};for(auto& owner:result.rank_order->faction_at_rank) {owner=ReadInt32(buffer);}
        }
        if(version>=15) {
            const auto membership=ReadInt32(buffer);
            if(membership<0 || membership>UINT8_MAX) {throw std::runtime_error("Human membership exceeds an unsigned byte");}
            result.human_membership=static_cast<uint8_t>(membership);
        } else {
            // V11-V14 admitted only human Players. This is the old port-format
            // contract, not inference from current focus or transport roles.
            for(const auto& participant:roster) {result.human_membership|=static_cast<uint8_t>(1u<<participant.simulation_id);}
        }
		if(version>=16) {result.strategy=ReadStrategy(buffer,static_cast<size_t>(count));}
		if(version>=17) {result.capabilities=ReadCapabilities(buffer);}
		if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC simulation trailing fields" ); }
		result.roster = std::move( roster ); result.technology = std::move( technology ); result.designs = std::move( designs ); result.social = std::move( social );
		result.Validate( technologies, profiles, units );
		return result;
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

}
