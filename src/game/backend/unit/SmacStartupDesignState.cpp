#include "SmacStartupDesignState.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace game::backend::unit {
namespace {
constexpr int64_t MAGIC = 0x534d5344; // SMSD
constexpr int64_t VERSION = 1;

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

bool SmacCustomDesign::operator==( const SmacCustomDesign& other ) const {
	return id == other.id && chassis == other.chassis && weapon == other.weapon && defense == other.defense && reactor == other.reactor
		&& cost == other.cost && plan == other.plan && carry == other.carry && abilities == other.abilities && faction_exclusion == other.faction_exclusion
		&& active == other.active && prototyped == other.prototyped;
}
bool SmacDesignOwner::operator==( const SmacDesignOwner& other ) const { return simulation_id == other.simulation_id && designs == other.designs && starting_scout == other.starting_scout; }
bool SmacStartupDesignState::operator==( const SmacStartupDesignState& other ) const { return global_difficulty == other.global_difficulty && owners == other.owners; }

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
		auto owner = GenerateEmptyOwner( faction, technologies, units.catalog, difficulty );
		MarkAndSelect( owner, units.catalog ); result.owners.push_back( std::move( owner ) );
	}
	result.Validate( units.catalog );
	return result;
}

void SmacStartupDesignState::Validate( const SmacPredefinedUnitCatalog& catalog ) const {
	RequireDifficulty( global_difficulty );
	if ( owners.empty() || owners.size() > 7 ) { throw std::runtime_error( "SMAC custom design state requires 1..7 owners" ); }
	std::array< bool, 8 > seen{};
	for ( const auto& owner : owners ) {
		ValidateOwner( owner, catalog );
		if ( seen[ owner.simulation_id ] ) { throw std::runtime_error( "Duplicate SMAC custom design owner" ); }
		seen[ owner.simulation_id ] = true;
	}
}

const SmacDesignOwner* SmacStartupDesignState::Find( int32_t id ) const {
	for ( const auto& owner : owners ) { if ( owner.simulation_id == id ) { return &owner; } }
	return nullptr;
}

types::Buffer SmacStartupDesignState::Serialize( const SmacPredefinedUnitCatalog& catalog ) const {
	Validate( catalog );
	types::Buffer buffer;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( global_difficulty ); buffer.WriteInt( owners.size() );
	for ( const auto& owner : owners ) {
		buffer.WriteInt( owner.simulation_id ); buffer.WriteInt( owner.starting_scout ); buffer.WriteInt( owner.designs.size() );
		for ( const auto& d : owner.designs ) {
			buffer.WriteInt( d.id ); buffer.WriteInt( d.chassis ); buffer.WriteInt( d.weapon ); buffer.WriteInt( d.defense ); buffer.WriteInt( d.reactor );
			buffer.WriteInt( d.cost ); buffer.WriteInt( d.plan ); buffer.WriteInt( d.carry ); buffer.WriteInt( d.abilities ); buffer.WriteInt( d.faction_exclusion );
			buffer.WriteBool( d.active ); buffer.WriteBool( d.prototyped );
		}
	}
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC custom design state exceeds wire budget" ); }
	return buffer;
}

SmacStartupDesignState SmacStartupDesignState::Deserialize( const std::string& wire, const SmacPredefinedUnitCatalog& catalog ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC custom design state exceeds wire budget" ); }
	types::Buffer buffer( wire );
	if ( buffer.ReadInt() != MAGIC || buffer.ReadInt() != VERSION ) { throw std::runtime_error( "Unsupported SMAC custom design state format" ); }
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
			d.active = buffer.ReadBool(); d.prototyped = buffer.ReadBool(); owner.designs.push_back( d );
		}
		result.owners.push_back( std::move( owner ) );
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC custom design trailing fields" ); }
	result.Validate( catalog ); return result;
}

}
