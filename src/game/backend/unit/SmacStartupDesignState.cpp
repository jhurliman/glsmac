#include "SmacStartupDesignState.h"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace game::backend::unit {
namespace {
constexpr int64_t MAGIC = 0x534d5344; // SMSD
constexpr int64_t VERSION = 6;

void RequireDifficulty( int32_t difficulty ) {
	if ( difficulty < 0 || difficulty > 5 ) { throw std::runtime_error( "SMAC startup requires original difficulty 0..5" ); }
}

void ValidateOwner( const SmacDesignOwner& owner, const SmacPredefinedUnitCatalog& catalog ) {
	if ( owner.simulation_id < 1 || owner.simulation_id > 7 || owner.designs.size() > 64 || catalog.units.size() <= 2 ) {
		throw std::runtime_error( "Invalid SMAC custom design owner or catalog" );
	}
	const int32_t first = 64 * owner.simulation_id;
	int32_t previous = first - 1;
	bool selected = owner.starting_scout == 2;
	for ( const auto& design : owner.designs ) {
		if ( design.id < first || design.id >= first + 64 || design.id <= previous
			|| design.chassis >= catalog.chassis.chassis.size() || design.weapon >= catalog.weapons.weapons.size()
			|| design.defense >= catalog.defenses.defenses.size() || design.reactor < 1 || design.reactor > catalog.reactors.reactors.size()
			|| design.abilities > 0xffffff ) { throw std::runtime_error( "Invalid SMAC custom design record" ); }
        if((design.auxiliary_flags&7) || (design.explicitly_named && (!design.name || design.name->empty()))
            || (design.name && (design.name->size()>31 || design.name->find('\0')!=std::string::npos))
            || (design.riot && (design.riot->source_design<64 || design.riot->source_design>511 || design.riot->source_design==design.id))) {
            throw std::runtime_error("Invalid retained prototype metadata");
        }
		previous = design.id;
		selected = selected || design.id == owner.starting_scout;
	}
	if ( !selected ) { throw std::runtime_error( "SMAC starting Scout does not reference an owned or fallback design" ); }
}

bool Available( const std::string& key, const faction::SmacFactionTechnologyState& state, const settings::SmacTechnologyRules& technologies ) {
	const auto reference = technologies.ResolveToken( key );
	using Kind = settings::SmacTechnologyReference::Kind;
	return reference.kind == Kind::NONE || ( reference.kind == Kind::TECHNOLOGY_ID && state.owned[ *reference.technology_id ] );
}

template< typename T > void RequireAvailable( const std::vector< T >& rows, const std::vector< size_t >& expected,
	const faction::SmacFactionTechnologyState& state, const settings::SmacTechnologyRules& technologies ) {
	std::vector< size_t > actual;
	for ( size_t i = 0; i < rows.size(); ++i ) { if ( Available( rows[ i ].prerequisite, state, technologies ) ) { actual.push_back( i ); } }
	if ( actual != expected ) { throw std::runtime_error( "Unsupported SMAC startup component capability set" ); }
}

void RequireCapabilities( const faction::SmacFactionTechnologyState& state, const settings::SmacTechnologyRules& technologies,
	const SmacPredefinedUnitCatalog& catalog ) {
	technologies.Validate();
	if ( catalog.chassis.chassis.size() != 9 || catalog.weapons.weapons.size() != 21 || catalog.defenses.defenses.size() != 10
		|| catalog.reactors.reactors.size() != 4 || catalog.abilities.abilities.size() != 24 || catalog.units.size() != 14
		|| catalog.source.ruleset_id != "smac" || catalog.chassis.ruleset_id != "smac" || catalog.weapons.ruleset_id != "smac"
		|| catalog.defenses.ruleset_id != "smac" || catalog.reactors.ruleset_id != "smac" || catalog.abilities.ruleset_id != "smac" ) {
		throw std::runtime_error( "Unsupported SMAC startup component catalog" );
	}
	static const std::array< std::pair< const char*, size_t >, 7 > grants{ {
		{ "GAIANS", 6 }, { "HIVE", 43 }, { "UNIVERSITY", 2 }, { "MORGANITES", 1 }, { "SPARTANS", 5 }, { "BELIEVERS", 4 }, { "PEACEKEEPERS", 0 }
	} };
	std::optional< size_t > grant;
	for ( const auto& entry : grants ) { if ( state.admission.faction_id == entry.first ) { grant = entry.second; break; } }
	if ( !grant ) { throw std::runtime_error( "Unsupported SMAC startup faction profile" ); }
	for ( size_t i = 0; i < state.owned.size(); ++i ) {
		if ( state.owned[ i ] != ( i == *grant ) ) { throw std::runtime_error( "Unsupported SMAC startup technology ownership" ); }
	}
	const bool morgan = state.admission.faction_id == "MORGANITES", spartans = state.admission.faction_id == "SPARTANS";
	RequireAvailable( catalog.chassis.chassis, spartans ? std::vector< size_t >{ 0, 1 } : std::vector< size_t >{ 0 }, state, technologies );
	RequireAvailable( catalog.weapons.weapons, state.admission.faction_id == "GAIANS" ? std::vector< size_t >{ 0, 14, 15 } : std::vector< size_t >{ 0, 14 }, state, technologies );
	RequireAvailable( catalog.defenses.defenses, morgan ? std::vector< size_t >{ 0, 1 } : std::vector< size_t >{ 0 }, state, technologies );
	RequireAvailable( catalog.reactors.reactors, { 0 }, state, technologies );
	RequireAvailable( catalog.abilities.abilities, {}, state, technologies );
	using namespace loader::txt;
	for ( const size_t i : { size_t( 0 ), size_t( 1 ) } ) {
		const auto& c = catalog.chassis.chassis[ i ];
		if ( c.speed != static_cast< int32_t >( i + 1 ) || c.triad != ChassisTriad::Land || c.range != 0 || c.missile || c.cargo != 1 ) {
			throw std::runtime_error( "Unsupported SMAC startup chassis semantics" );
		}
		const auto& d = catalog.defenses.defenses[ i ];
		if ( d.rating != static_cast< int32_t >( i + 1 ) || d.mode != DefenseMode::Projectile ) { throw std::runtime_error( "Unsupported SMAC startup defense semantics" ); }
	}
	const auto& gun = catalog.weapons.weapons[ 0 ]; const auto& colony = catalog.weapons.weapons[ 14 ]; const auto& former = catalog.weapons.weapons[ 15 ];
	if ( gun.offense != 1 || gun.mode != WeaponMode::Projectile || colony.offense != 0 || colony.mode != WeaponMode::Colonist
		|| former.offense != 0 || former.mode != WeaponMode::Terraformer || catalog.reactors.reactors[ 0 ].power != 1 ) {
		throw std::runtime_error( "Unsupported SMAC startup weapon or reactor semantics" );
	}
	const auto& fallback = catalog.units[ 2 ];
	if ( fallback.source_id != 2 || fallback.chassis_id != 0 || fallback.weapon_id != 0 || fallback.defense_id != 0 || fallback.reactor != 1
		|| fallback.abilities != 0 || fallback.plan != 3 || !fallback.active || fallback.faction_exclusion != 0 ) {
		throw std::runtime_error( "Unsupported SMAC startup fallback Scout" );
	}
}

uint32_t ReadUnsigned( types::Buffer& buffer, uint32_t maximum ) {
	const auto value = buffer.ReadInt();
	if ( value < 0 || static_cast< uint64_t >( value ) > maximum ) { throw std::runtime_error( "SMAC custom design integer outside field range" ); }
	return static_cast< uint32_t >( value );
}
}

SmacIntrinsicDesignState SmacStartupDesignState::IntrinsicState(const SmacPredefinedUnit& definition) const {
    if(definition.source_id>=64 || definition.faction_exclusion>255) {throw std::invalid_argument("Intrinsic state requires a native predefined identity and exclusion byte");}
    const auto& retained=intrinsic_overrides.at(definition.source_id);if(retained) {return *retained;}
    return {uint16_t((definition.active?1:0)|(definition.prototyped?4:0)),uint8_t(definition.faction_exclusion)};
}
SmacPredefinedUnit SmacStartupDesignState::EffectiveIntrinsic(const SmacPredefinedUnit& definition) const {
    auto result=definition;const auto retained=IntrinsicState(definition);
    result.active=(retained.flags&1)!=0;result.prototyped=(retained.flags&4)!=0;result.faction_exclusion=retained.exclusion;return result;
}

SmacRetirementTuple SmacDesignRetirementTuple(const SmacCustomDesign& design) {
    if(design.chassis>255 || design.weapon>255 || design.defense>255) {throw std::invalid_argument("Retirement tuple component exceeds native byte");}
    SmacRetirementTuple tuple{};for(unsigned i=0;i<4;++i) {tuple[i]=uint8_t(design.abilities>>(8*i));}
    tuple[4]=uint8_t(design.chassis);tuple[5]=uint8_t(design.weapon);tuple[6]=uint8_t(design.defense);tuple[7]=design.reactor;return tuple;
}
SmacRetirementHistory SmacRetirementHistory::NewWorld() {SmacRetirementHistory result;result.Clear();return result;}
void SmacRetirementHistory::Clear() noexcept {for(auto& slot:slots) {slot.fill(255);}}
void SmacRetirementHistory::Validate() const {if(cursor<0 || cursor>=64) {throw std::invalid_argument("Retirement history cursor outside native ring");}}
void SmacRetirementHistory::Record(const SmacRetirementTuple& tuple) {Validate();slots.at(cursor)=tuple;cursor=(cursor+1)%64;}
bool SmacRetirementHistory::Contains(const SmacRetirementTuple& tuple,bool human) const {
    return human && std::find(slots.begin(),slots.end(),tuple)!=slots.end();
}

bool SmacCustomDesign::operator==( const SmacCustomDesign& other ) const {
	return id == other.id && chassis == other.chassis && weapon == other.weapon && defense == other.defense && reactor == other.reactor
		&& cost == other.cost && plan == other.plan && carry == other.carry && abilities == other.abilities && faction_exclusion == other.faction_exclusion
		&& active == other.active && prototyped == other.prototyped && name==other.name && explicitly_named==other.explicitly_named && riot==other.riot && auxiliary_flags==other.auxiliary_flags;
}
bool SmacDesignOwner::operator==( const SmacDesignOwner& other ) const { return simulation_id == other.simulation_id && designs == other.designs && starting_scout == other.starting_scout; }
bool SmacStartupDesignState::operator==( const SmacStartupDesignState& other ) const { return global_difficulty == other.global_difficulty && owners == other.owners && retirement_history==other.retirement_history && intrinsic_overrides==other.intrinsic_overrides; }

int32_t SmacStartupDesignState::OriginalDifficulty( const std::string& canonical_name ) {
	static const std::array< const char*, 6 > names{ "Citizen", "Specialist", "Talent", "Librarian", "Thinker", "Transcend" };
	for ( size_t i = 0; i < names.size(); ++i ) { if ( canonical_name == names[ i ] ) { return static_cast< int32_t >( i ); } }
	throw std::runtime_error( "Unknown SMAC canonical difficulty identity" );
}

SmacDesignOwner SmacStartupDesignState::GenerateEmptyOwner( const faction::SmacFactionTechnologyState& technology,
	const settings::SmacTechnologyRules& technologies, const SmacPredefinedUnitCatalog& catalog, int32_t difficulty ) {
	RequireDifficulty( difficulty ); RequireCapabilities( technology, technologies, catalog );
	SmacDesignOwner owner; owner.simulation_id = technology.admission.simulation_id;
	ValidateOwner( owner, catalog );
	const bool morgan = technology.admission.faction_id == "MORGANITES", spartans = technology.admission.faction_id == "SPARTANS";
	if ( morgan || spartans ) {
		SmacCustomDesign design;
		design.id = 64 * owner.simulation_id; design.chassis = spartans ? 1 : 0; design.defense = morgan ? 1 : 0;
		design.plan = spartans ? 3 : 2; design.prototyped = difficulty <= 1;
		design.cost = static_cast< uint8_t >( SmacPredefinedUnitCatalog::CalculateCost( catalog.chassis.chassis[ design.chassis ],
			catalog.weapons.weapons[ 0 ], catalog.defenses.defenses[ design.defense ], 1, 0, catalog.abilities ) );
		owner.designs.push_back( design );
	}
	return owner;
}

void SmacStartupDesignState::MarkAndSelect( SmacDesignOwner& owner, const SmacPredefinedUnitCatalog& catalog ) {
	ValidateOwner( owner, catalog );
	owner.starting_scout = 2;
	for ( auto& design : owner.designs ) {
		if ( design.active ) { design.prototyped = true; if ( design.plan == 3 ) { owner.starting_scout = design.id; } }
	}
}

SmacStartupDesignState SmacStartupDesignState::NewWorld( const faction::SmacTechnologyState& technology,
	const settings::SmacTechnologyRules& technologies, const settings::SmacPredefinedUnitRules& units, int32_t difficulty,
	const std::function<void(const faction::SmacFactionTechnologyState&)>& before_generate ) {
	units.Validate( technologies );
	SmacStartupDesignState result; result.global_difficulty = difficulty;
	for ( const auto& faction : technology.factions ) {
		if(before_generate) {before_generate(faction);}
        result.retirement_history.at(faction.admission.simulation_id)=SmacRetirementHistory::NewWorld();
		auto owner = GenerateEmptyOwner( faction, technologies, units.catalog, difficulty );
		MarkAndSelect( owner, units.catalog ); result.owners.push_back( std::move( owner ) );
	}
	result.Validate( units.catalog );
	return result;
}

void SmacStartupDesignState::Validate( const SmacPredefinedUnitCatalog& catalog ) const {
	RequireDifficulty( global_difficulty );
    for(const auto& history:retirement_history) {if(history) {history->Validate();}}
	if ( owners.empty() || owners.size() > 7 ) { throw std::runtime_error( "SMAC custom design state requires 1..7 owners" ); }
	std::array< bool, 8 > seen{};
	for ( const auto& owner : owners ) {
		ValidateOwner( owner, catalog );
		if ( seen[ owner.simulation_id ] ) { throw std::runtime_error( "Duplicate SMAC custom design owner" ); }
        seen[ owner.simulation_id ] = true;
    }
    for(const auto& owner:owners) {for(const auto& design:owner.designs) {
        const auto* current=&design;std::array<bool,512> visited{};
        while(current->riot) {
            if(visited[current->id]) {throw std::runtime_error("Cyclic riot prototype source history");}visited[current->id]=true;
            const auto source=current->riot->source_design;const auto* block=Find(source/64);
            if(!block) {throw std::runtime_error("Missing riot prototype source block");}
            const auto found=std::find_if(block->designs.begin(),block->designs.end(),[&](const auto& row){return row.id==source;});
            if(found==block->designs.end()) {throw std::runtime_error("Missing riot prototype source definition");}current=&*found;
        }
    }}
}

int32_t SmacStartupDesignState::RequestRiotPrototype(int32_t source,int32_t recipient,bool morgan,const SmacPredefinedUnitCatalog& catalog,bool human) {
    Validate(catalog);
    if(source<=64 || source>511 || recipient<1 || recipient>7 || !Find(recipient)) {throw std::runtime_error("Riot prototype request has no admitted source or recipient");}
    const auto* source_owner=Find(source/64);
    if(!source_owner) {throw std::runtime_error("Riot prototype source block is unavailable");}
    const auto found=std::find_if(source_owner->designs.begin(),source_owner->designs.end(),[&](const auto& row){return row.id==source;});
    if(found==source_owner->designs.end()) {throw std::runtime_error("Riot prototype source definition is unavailable");}
    const auto original=*found;const auto* target=Find(recipient);
    if(human) {
        const auto& history=retirement_history.at(recipient);
        if(!history) {throw std::runtime_error("Human riot prototype requires retained retirement history");}
        if(history->Contains(SmacDesignRetirementTuple(original),true)) {return -1;}
    }
    if(original.chassis==0 && original.weapon==0 && original.defense==0 && original.reactor==1 && original.abilities==0) {return 2;}
    const bool rover=original.chassis==1 && original.defense==0 && original.plan==3;
    const bool armored=original.chassis==0 && original.defense==1 && original.plan==2;
    if((!rover && !armored) || original.weapon!=0 || original.reactor!=1 || original.abilities!=0
        || original.cost!=2 || original.carry!=0 || original.faction_exclusion || !original.active || !original.prototyped) {
        throw std::runtime_error("This riot prototype needs a different measured component producer");
    }
    for(const auto& row:target->designs) {
        if(row.active && row.chassis==original.chassis && row.weapon==original.weapon && row.defense==original.defense
            && row.reactor==original.reactor && row.abilities==original.abilities) {return human ? -1:row.id;}
    }
    if(!human) {throw std::runtime_error("AI riot prototype allocation requires its measured constructor path");}
    const bool empty=target->designs.empty();
    const auto is_morgan=[&] {
        if(!morgan || target->designs.size()!=1) {return false;}const auto& row=target->designs[0];
        return row.id==64*recipient && row.chassis==0 && row.weapon==0 && row.defense==1 && row.reactor==1 && !row.abilities
            && row.cost==2 && row.plan==2 && !row.carry && !row.faction_exclusion && row.active && row.prototyped;
    };
    const auto is_spartan=[&] {
        if(target->designs.size()!=1) {return false;}const auto& row=target->designs[0];
        return row.id==64*recipient && row.chassis==1 && row.weapon==0 && row.defense==0 && row.reactor==1 && !row.abilities
            && row.cost==2 && row.plan==3 && !row.carry && !row.faction_exclusion && row.active && row.prototyped;
    };
    if(!empty && !(rover ? is_morgan():is_spartan())) {throw std::runtime_error("This occupied riot prototype block needs its original allocation producer");}
    SmacCustomDesign allocated;allocated.id=64*recipient+(empty?0:1);allocated.chassis=original.chassis;allocated.weapon=0;allocated.defense=original.defense;
    allocated.reactor=1;allocated.cost=2;allocated.plan=original.plan;allocated.carry=0;allocated.abilities=0;allocated.faction_exclusion=0;allocated.active=true;allocated.prototyped=true;
    allocated.riot=SmacRiotPrototypeMetadata{source,197,0,{uint8_t(rover ? 10:2),0,2,255,0}};
    if(original.name) {
        if(empty) {allocated.name=original.name;}
        else if(target->designs[0].name) {
            if(*target->designs[0].name!=*original.name) {allocated.name=original.name;}
            else if(original.name->size()<31) {allocated.name=*original.name+"*";}
        }
    }
    auto next=*this;auto owner=std::find_if(next.owners.begin(),next.owners.end(),[&](const auto& row){return row.simulation_id==recipient;});
    owner->designs.push_back(allocated);next.Validate(catalog);owners.swap(next.owners);return allocated.id;
}

const SmacDesignOwner* SmacStartupDesignState::Find( int32_t id ) const {
	for ( const auto& owner : owners ) { if ( owner.simulation_id == id ) { return &owner; } }
	return nullptr;
}

types::Buffer SmacStartupDesignState::Serialize( const SmacPredefinedUnitCatalog& catalog ) const {
	Validate( catalog );
	types::Buffer buffer;
    bool metadata=false;for(const auto& owner:owners) {for(const auto& design:owner.designs) {metadata=metadata || design.name || design.riot;}}
    bool named=false;for(const auto& owner:owners) {for(const auto& design:owner.designs) {named=named || design.explicitly_named;}}
    bool auxiliary=false;for(const auto& owner:owners) {for(const auto& design:owner.designs) {auxiliary=auxiliary || design.auxiliary_flags!=0;}}
    const bool history=std::any_of(retirement_history.begin(),retirement_history.end(),[](const auto& row){return row.has_value();});
    const bool overrides=std::any_of(intrinsic_overrides.begin(),intrinsic_overrides.end(),[](const auto& row){return row.has_value();});
    const auto version=overrides ? VERSION:history ? 5:auxiliary ? 4:named ? 3:metadata ? 2:1;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( version ); buffer.WriteInt( global_difficulty ); buffer.WriteInt( owners.size() );
	for ( const auto& owner : owners ) {
		buffer.WriteInt( owner.simulation_id ); buffer.WriteInt( owner.starting_scout ); buffer.WriteInt( owner.designs.size() );
		for ( const auto& d : owner.designs ) {
			buffer.WriteInt( d.id ); buffer.WriteInt( d.chassis ); buffer.WriteInt( d.weapon ); buffer.WriteInt( d.defense ); buffer.WriteInt( d.reactor );
			buffer.WriteInt( d.cost ); buffer.WriteInt( d.plan ); buffer.WriteInt( d.carry ); buffer.WriteInt( d.abilities ); buffer.WriteInt( d.faction_exclusion );
			buffer.WriteBool( d.active ); buffer.WriteBool( d.prototyped );
            if(version>=2) {
                buffer.WriteBool(d.name.has_value());if(d.name) {buffer.WriteString(*d.name);}
                buffer.WriteBool(d.riot.has_value());if(d.riot) {
                    buffer.WriteInt(d.riot->source_design);buffer.WriteInt(d.riot->flags);buffer.WriteInt(d.riot->prerequisite);
                    for(const auto byte:d.riot->auxiliary) {buffer.WriteInt(byte);}
                }
            }
			if(version>=3) {buffer.WriteBool(d.explicitly_named);}
            if(version>=4) {buffer.WriteInt(d.auxiliary_flags);}
		}
	}
    if(version>=5) {for(const auto& row:retirement_history) {
        buffer.WriteBool(row.has_value());if(row) {
            buffer.WriteInt(row->cursor);std::string bytes;bytes.reserve(512);
            for(const auto& slot:row->slots) {for(const auto byte:slot) {bytes.push_back(static_cast<char>(byte));}}buffer.WriteString(bytes);
        }
    }}
    if(version>=6) {for(const auto& row:intrinsic_overrides) {
        buffer.WriteBool(row.has_value());if(row) {buffer.WriteInt(row->flags);buffer.WriteInt(row->exclusion);}
    }}
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC custom design state exceeds wire budget" ); }
	return buffer;
}

SmacStartupDesignState SmacStartupDesignState::Deserialize( const std::string& wire, const SmacPredefinedUnitCatalog& catalog ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC custom design state exceeds wire budget" ); }
	types::Buffer buffer( wire );
    if(buffer.ReadInt()!=MAGIC) {throw std::runtime_error("Unsupported SMAC custom design state format");}
    const auto version=buffer.ReadInt();if(version!=1 && version!=2 && version!=3 && version!=4 && version!=5 && version!=VERSION) {throw std::runtime_error("Unsupported SMAC custom design state format");}
	SmacStartupDesignState result;
	result.global_difficulty = static_cast< int32_t >( ReadUnsigned( buffer, 5 ) );
	const auto owners = ReadUnsigned( buffer, 7 );
	result.owners.reserve( owners );
	for ( uint32_t i = 0; i < owners; ++i ) {
		SmacDesignOwner owner;
		owner.simulation_id = static_cast< int32_t >( ReadUnsigned( buffer, 7 ) );
		owner.starting_scout = static_cast< int32_t >( ReadUnsigned( buffer, 511 ) );
		const auto count = ReadUnsigned( buffer, 64 ); owner.designs.reserve( count );
		for ( uint32_t j = 0; j < count; ++j ) {
			SmacCustomDesign d;
			d.id = static_cast< int32_t >( ReadUnsigned( buffer, 511 ) );
			d.chassis = ReadUnsigned( buffer, UINT32_MAX ); d.weapon = ReadUnsigned( buffer, UINT32_MAX ); d.defense = ReadUnsigned( buffer, UINT32_MAX );
			d.reactor = static_cast< uint8_t >( ReadUnsigned( buffer, 255 ) ); d.cost = static_cast< uint8_t >( ReadUnsigned( buffer, 255 ) );
			d.plan = static_cast< uint8_t >( ReadUnsigned( buffer, 255 ) ); d.carry = static_cast< uint8_t >( ReadUnsigned( buffer, 255 ) );
			d.abilities = ReadUnsigned( buffer, 0xffffff ); d.faction_exclusion = ReadUnsigned( buffer, UINT32_MAX );
			d.active = buffer.ReadBool(); d.prototyped = buffer.ReadBool();
            if(version>=2) {
                if(buffer.ReadBool()) {d.name=buffer.ReadString(31);}
                if(buffer.ReadBool()) {
                    SmacRiotPrototypeMetadata metadata{};metadata.source_design=static_cast<int32_t>(ReadUnsigned(buffer,511));
                    metadata.flags=static_cast<uint8_t>(ReadUnsigned(buffer,255));metadata.prerequisite=static_cast<uint8_t>(ReadUnsigned(buffer,255));
                    for(auto& byte:metadata.auxiliary) {byte=static_cast<uint8_t>(ReadUnsigned(buffer,255));}d.riot=metadata;
                }
            }
            if(version>=3) {d.explicitly_named=buffer.ReadBool();}
            if(version>=4) {d.auxiliary_flags=static_cast<uint16_t>(ReadUnsigned(buffer,UINT16_MAX));}
            owner.designs.push_back( d );
		}
		result.owners.push_back( std::move( owner ) );
	}
    if(version>=5) {for(auto& row:result.retirement_history) {if(buffer.ReadBool()) {
        SmacRetirementHistory history;history.cursor=static_cast<int32_t>(ReadUnsigned(buffer,63));
        const auto bytes=buffer.ReadString(512);if(bytes.size()!=512) {throw std::runtime_error("Retirement history requires 512 bytes");}
        for(size_t i=0;i<512;++i) {history.slots[i/8][i%8]=static_cast<uint8_t>(bytes[i]);}row=history;
    }}}
    if(version>=6) {for(auto& row:result.intrinsic_overrides) {if(buffer.ReadBool()) {
        const auto flags=static_cast<uint16_t>(ReadUnsigned(buffer,65535));const auto exclusion=static_cast<uint8_t>(ReadUnsigned(buffer,255));
        row=SmacIntrinsicDesignState{flags,exclusion};
    }}}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC custom design trailing fields" ); }
	result.Validate( catalog ); return result;
}

}
