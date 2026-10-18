#include "Settings.h"
#include "network/Network.h"

#include "util/FS.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/FactionTXTLoader.h"

#include <limits>

#include "gse/value/Float.h"
#include "gse/value/Ptr.h"
#include "gse/value/Array.h"

namespace game {
namespace backend {
namespace settings {

WRAPMAP( type, MapSettings::type_t,
	{ MapSettings::MT_RANDOM, "random" },
	{ MapSettings::MT_CUSTOM, "custom" },
	{ MapSettings::MT_MAPFILE, "mapfile" },
)

WRAPIMPL_DYNAMIC_GETTERS( MapSettings )
			WRAPIMPL_GET_MAPPED( type )
			WRAPIMPL_GET_PTR( "filename", filename )
			WRAPIMPL_GET_PTR( "size_x", size_x )
			WRAPIMPL_GET_PTR( "size_y", size_y )
			WRAPIMPL_GET_PTR( "ocean_coverage", ocean_coverage )
			WRAPIMPL_GET_PTR( "erosive_forces", erosive_forces )
			WRAPIMPL_GET_PTR( "native_lifeforms", native_lifeforms )
			WRAPIMPL_GET_PTR( "cloud_cover", cloud_cover )
WRAPIMPL_DYNAMIC_SETTERS( MapSettings )
	if ( obj->m_world_size_locked && ( key == "size_x" || key == "size_y" || key == "type" || key == "filename" || key == "ocean_coverage" ) ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "World geometry and ocean coverage are locked for this session" );
	}
	WRAPIMPL_SET_MAPPED( type )
	WRAPIMPL_SET_PTR( "filename", String, filename )
	WRAPIMPL_SET_PTR( "size_x", Int, size_x )
	WRAPIMPL_SET_PTR( "size_y", Int, size_y )
	WRAPIMPL_SET_PTR( "ocean_coverage", Float, ocean_coverage )
	WRAPIMPL_SET_PTR( "erosive_forces", Float, erosive_forces )
	WRAPIMPL_SET_PTR( "native_lifeforms", Float, native_lifeforms )
	WRAPIMPL_SET_PTR( "cloud_cover", Float, cloud_cover )
WRAPIMPL_DYNAMIC_ON_SET( MapSettings )
	if ( property_name == "size_x" || property_name == "size_y" || ( property_name == "type" && type == MT_MAPFILE ) ) { m_smac_world_size.reset(); }
WRAPIMPL_DYNAMIC_END()

const std::optional< SmacWorldSizeSelection >& MapSettings::GetSmacWorldSize() const {
	if ( m_smac_world_size && ( m_smac_world_size->width != size_x || m_smac_world_size->height != size_y ) ) {
		THROW( "Map dimensions changed without an accepted SMAC world-size selection" );
	}
	return m_smac_world_size;
}

void MapSettings::SetSmacWorldSize( const SmacWorldSizeSelection& selection ) {
	if ( m_world_size_locked ) { THROW( "World size is locked for this session" ); }
	selection.Validate(); m_smac_world_size = selection; size_x = selection.width; size_y = selection.height;
}

void MapSettings::ClearSmacWorldSize() { m_smac_world_size.reset(); m_world_size_locked = false; }
void MapSettings::LockWorldSize() { m_world_size_locked = true; }

std::optional<int32_t> MapSettings::GetSmacOceanCoverage() const {
	if(ocean_coverage==0.4f) {return 0;}
	if(ocean_coverage==0.6f) {return 1;}
	if(ocean_coverage==0.8f) {return 2;}
	return {}; // never quantize a continuous setting or measure live terrain
}

const types::Buffer MapSettings::Serialize() const {
	types::Buffer buf;
	const auto& selection = GetSmacWorldSize();
	buf.WriteInt( 1 );
	buf.WriteBool( selection.has_value() );
	if ( selection ) { buf.WriteString( selection->Serialize().ToString() ); }
	buf.WriteInt( type );
	buf.WriteString( util::FS::GetBaseName( filename ) ); // don't send full path for security reasons, nobody needs it anyway
	buf.WriteInt( size_x );
	buf.WriteInt( size_y );
	buf.WriteFloat( ocean_coverage );
	buf.WriteFloat( erosive_forces );
	buf.WriteFloat( native_lifeforms );
	buf.WriteFloat( cloud_cover );

	return buf;
}

void MapSettings::Deserialize( types::Buffer buf ) {
	if ( buf.ReadInt() != 1 ) { THROW( "Unsupported map settings format" ); }
	MapSettings next;
	if ( buf.ReadBool() ) { next.m_smac_world_size = SmacWorldSizeSelection::Deserialize( types::Buffer( buf.ReadString() ) ); }
	const auto raw_type = buf.ReadInt();
	if ( raw_type < MT_RANDOM || raw_type > MT_MAPFILE ) { THROW( "Unknown map settings type" ); }
	next.type = static_cast< type_t >( raw_type );
	next.filename = buf.ReadString();
	next.size_x = buf.ReadInt();
	next.size_y = buf.ReadInt();
	next.ocean_coverage = buf.ReadFloat();
	next.erosive_forces = buf.ReadFloat();
	next.native_lifeforms = buf.ReadFloat();
	next.cloud_cover = buf.ReadFloat();
	if ( buf.lenr != buf.lenw ) { THROW( "Unexpected fields in map settings" ); }
	next.GetSmacWorldSize();
	if ( m_world_size_locked && ( !( next.m_smac_world_size == m_smac_world_size ) || next.size_x != size_x || next.size_y != size_y
		|| next.ocean_coverage != ocean_coverage ) ) { THROW( "Host changed locked world geometry or ocean coverage" ); }
	// Serializable's assignment calls Deserialize again; publish only values,
	// preserving this object's existing script wrappers and lifecycle lock.
	type = next.type;
	filename = std::move( next.filename );
	size_x = next.size_x;
	size_y = next.size_y;
	ocean_coverage = next.ocean_coverage;
	erosive_forces = next.erosive_forces;
	native_lifeforms = next.native_lifeforms;
	cloud_cover = next.cloud_cover;
	m_smac_world_size = next.m_smac_world_size;
}

void GlobalSettings::Initialize() {
	rules.Initialize();
	difficulty_level = rules.GetDefaultDifficultyLevelV();
}

GlobalSettings::ruleset_t GlobalSettings::ParseRuleset( const std::string& id ) {
	if ( id == "smac" ) { return RULESET_SMAC; }
	if ( id == "smax" ) { return RULESET_SMAX; }
	THROW( "Unknown gameplay ruleset: " + id + " (expected smac or smax)" );
}

const std::string& GlobalSettings::GetRulesetId() const {
	static const std::string ids[] = { "unselected", "smac", "smax" };
	return ids[ m_ruleset ];
}

void GlobalSettings::SelectRuleset( const std::string& id ) {
	const auto selected = ParseRuleset( id );
	if ( m_ruleset_locked ) {
		THROW( "Gameplay ruleset is locked; return to the main menu to start a new game" );
	}
	m_ruleset = selected;
	m_smac_resource_rules.reset();
	m_smac_production_rules.reset();
	m_smac_technology_rules.reset();
	m_smac_world_size_rules.reset();
	m_smac_faction_startup_rules.reset();
	m_smac_predefined_unit_rules.reset();
	m_smac_movement_rules.reset();
	m_smac_specialist_rules.reset();
	m_smac_territory_rules.reset();
	m_smac_terraforming_rules.reset();
	map.ClearSmacWorldSize();
}

void GlobalSettings::InitializeHostRules( const RulesSource& read_source ) {
	if ( m_ruleset_locked ) { return; } // Already frozen for this session.
	if ( m_ruleset == RULESET_UNSELECTED ) { THROW( "Select a gameplay ruleset before starting a game" ); }
	std::optional< SmacResourceRules > parsed;
	std::optional< SmacProductionRules > production;
	std::optional< SmacTechnologyRules > technologies;
	std::optional< SmacWorldSizeRules > world_sizes;
	std::optional< SmacFactionStartupRules > factions;
	std::optional< SmacPredefinedUnitRules > units;
	std::optional< SmacMovementRules > movement;
	std::optional< SmacSpecialistRules > specialists;
	std::optional< SmacTerritoryRules > territory;
	std::optional< SmacTerraformingRules > terraforming;
	if ( m_ruleset == RULESET_SMAC ) {
		const auto source = read_source( "alpha.txt" );
		movement = loader::txt::SmacRulesTXTLoader::ParseMovementRules( source );
		specialists = loader::txt::SmacRulesTXTLoader::ParseSpecialistRules( source );
		territory = loader::txt::SmacRulesTXTLoader::ParseTerritoryRules( source );
		terraforming = loader::txt::SmacRulesTXTLoader::ParseTerraformingRules( source );
		production = loader::txt::SmacRulesTXTLoader::ParseProductionRules( source );
		technologies = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( source );
		parsed = loader::txt::SmacRulesTXTLoader::ParseResourceRules( source, *technologies );
		world_sizes = loader::txt::SmacRulesTXTLoader::ParseWorldSizeRules( source );
		factions = loader::txt::FactionTXTLoader::ParseStartupRules( source, read_source, *technologies );
		units = SmacPredefinedUnitRules::Parse( source, *technologies );
	}
	// Stage the complete settings payload, including its existing mutable tail.
	// The later State admission also accounts for configured faction metadata.
	GlobalSettings staged;
	staged.m_ruleset = m_ruleset;
	staged.map.Deserialize( map.Serialize() );
	staged.rules.Deserialize( rules.Serialize() );
	staged.difficulty_level = difficulty_level;
	staged.game_name = game_name;
	staged.m_smac_resource_rules = parsed;
	staged.m_smac_production_rules = std::move( production );
	staged.m_smac_technology_rules = std::move( technologies );
	staged.m_smac_world_size_rules = std::move( world_sizes );
	staged.m_smac_faction_startup_rules = std::move( factions );
	staged.m_smac_predefined_unit_rules = std::move( units );
	staged.m_smac_movement_rules = std::move( movement );
	staged.m_smac_specialist_rules = std::move( specialists );
	staged.m_smac_territory_rules = std::move( territory );
	staged.m_smac_terraforming_rules = std::move( terraforming );
	staged.Serialize();
	m_smac_resource_rules = staged.m_smac_resource_rules;
	m_smac_production_rules = std::move( staged.m_smac_production_rules );
	m_smac_technology_rules = std::move( staged.m_smac_technology_rules );
	m_smac_world_size_rules = std::move( staged.m_smac_world_size_rules );
	m_smac_faction_startup_rules = std::move( staged.m_smac_faction_startup_rules );
	m_smac_predefined_unit_rules = std::move( staged.m_smac_predefined_unit_rules );
	m_smac_movement_rules = std::move( staged.m_smac_movement_rules );
	m_smac_specialist_rules = std::move( staged.m_smac_specialist_rules );
	m_smac_territory_rules = std::move( staged.m_smac_territory_rules );
	m_smac_terraforming_rules = std::move( staged.m_smac_terraforming_rules );
	LockRuleset();
}

const std::optional< SmacResourceRules >& GlobalSettings::GetSmacResourceRules() const {
	return m_smac_resource_rules;
}

const std::optional< SmacProductionRules >& GlobalSettings::GetSmacProductionRules() const {
	return m_smac_production_rules;
}

const std::optional< SmacTechnologyRules >& GlobalSettings::GetSmacTechnologyRules() const {
	return m_smac_technology_rules;
}

const std::optional< SmacWorldSizeRules >& GlobalSettings::GetSmacWorldSizeRules() const { return m_smac_world_size_rules; }

const std::optional< SmacFactionStartupRules >& GlobalSettings::GetSmacFactionStartupRules() const { return m_smac_faction_startup_rules; }
const std::optional< SmacPredefinedUnitRules >& GlobalSettings::GetSmacPredefinedUnitRules() const { return m_smac_predefined_unit_rules; }
const std::optional< SmacMovementRules >& GlobalSettings::GetSmacMovementRules() const { return m_smac_movement_rules; }
const std::optional< SmacSpecialistRules >& GlobalSettings::GetSmacSpecialistRules() const { return m_smac_specialist_rules; }
const std::optional< SmacTerritoryRules >& GlobalSettings::GetSmacTerritoryRules() const { return m_smac_territory_rules; }
const std::optional< SmacTerraformingRules >& GlobalSettings::GetSmacTerraformingRules() const { return m_smac_terraforming_rules; }

void GlobalSettings::SelectSmacWorldSize( const int64_t category ) {
	if ( m_ruleset != RULESET_SMAC || !m_smac_world_size_rules ) { THROW( "SMAC world-size rules are unavailable" ); }
	map.SetSmacWorldSize( m_smac_world_size_rules->SelectPreset( category ) );
}

void GlobalSettings::SelectSmacCustomSize( const int64_t width, const int64_t height ) {
	if ( m_ruleset != RULESET_SMAC || !m_smac_world_size_rules ) { THROW( "SMAC world-size rules are unavailable" ); }
	map.SetSmacWorldSize( SmacWorldSizeRules::SelectCustom( width, height ) );
}

void GlobalSettings::RestoreSmacWorldSize( const int64_t category, const int64_t width, const int64_t height ) {
	if ( category == 99 ) { SelectSmacCustomSize( width, height ); return; }
	if ( m_ruleset != RULESET_SMAC || !m_smac_world_size_rules ) { THROW( "SMAC world-size rules are unavailable" ); }
	if ( category == -1 ) {
		// Explicit unselected lobby rollback, never an accepted world category.
		if ( map.m_world_size_locked ) { THROW( "World size is locked for this session" ); }
		map.ClearSmacWorldSize(); map.size_x = width; map.size_y = height; return;
	}
	const auto next = m_smac_world_size_rules->SelectPreset( category );
	if ( next.width != width || next.height != height ) { THROW( "SMAC preset dimensions differ from the frozen source row" ); }
	map.SetSmacWorldSize( next );
}

void GlobalSettings::ValidateWorldSize( const bool require_selection ) const {
	const auto& selection = map.GetSmacWorldSize();
	if ( m_ruleset != RULESET_SMAC ) {
		if ( selection ) { THROW( "Non-SMAC settings contain a SMAC world-size selection" ); }
		return;
	}
	if ( !m_smac_world_size_rules ) { THROW( "SMAC world-size rules are unavailable" ); }
	if ( !selection ) { if ( require_selection ) { THROW( "Select a SMAC world size before starting a game" ); } return; }
	if ( map.type == MapSettings::MT_MAPFILE ) { THROW( "A map file does not establish a SMAC world-size category" ); }
	m_smac_world_size_rules->ValidateSelection( *selection );
}

void GlobalSettings::LockWorldSize() { ValidateWorldSize(); if ( m_ruleset == RULESET_SMAC ) { map.LockWorldSize(); } }

SmacProductionPrerequisite GlobalSettings::GetSmacFacilityPrerequisite( const int32_t source_id ) const {
	if ( !m_smac_production_rules || !m_smac_technology_rules || source_id < 1 || source_id > 71 ) {
		return { SmacProductionPrerequisite::Status::UNAVAILABLE, {} };
	}
	return m_smac_technology_rules->ResolveProductionPrerequisite( m_smac_production_rules->facilities[ source_id - 1 ].prerequisite );
}

void GlobalSettings::LockRuleset() {
	if ( m_ruleset == RULESET_UNSELECTED ) {
		THROW( "Select a gameplay ruleset before starting a game" );
	}
	if ( m_ruleset == RULESET_SMAC && ( !m_smac_resource_rules || !m_smac_production_rules || !m_smac_technology_rules || !m_smac_world_size_rules || !m_smac_faction_startup_rules || !m_smac_predefined_unit_rules || !m_smac_movement_rules || !m_smac_specialist_rules || !m_smac_terraforming_rules ) ) {
		THROW( "SMAC rules profiles have not been loaded" );
	}
	m_ruleset_locked = true;
}

bool GlobalSettings::IsRulesetLocked() const {
	return m_ruleset_locked;
}

void GlobalSettings::ResetRuleset() {
	m_ruleset = RULESET_UNSELECTED;
	m_ruleset_locked = false;
	m_smac_resource_rules.reset();
	m_smac_production_rules.reset();
	m_smac_technology_rules.reset();
	m_smac_world_size_rules.reset();
	m_smac_faction_startup_rules.reset();
	m_smac_predefined_unit_rules.reset();
	m_smac_movement_rules.reset();
	m_smac_specialist_rules.reset();
	m_smac_territory_rules.reset();
	m_smac_terraforming_rules.reset();
	map.ClearSmacWorldSize();
}

WRAPIMPL_DYNAMIC_GETTERS( GlobalSettings )
			{ "get_world_size", NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				try {
					const auto& selection = map.GetSmacWorldSize();
					return VALUE( gse::value::Array,, ( gse::value::array_elements_t{
						VALUE( gse::value::Int,, selection ? selection->category : -1 ), VALUE( gse::value::Int,, map.size_x ), VALUE( gse::value::Int,, map.size_y ) } ) );
				} catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
			} ) },
			{ "set_world_size", NATIVE_CALL( this ) {
				N_EXPECT_ARGS_MIN_MAX( 1, 3 ); N_GETVALUE( category, 0, Int );
				try {
					if ( arguments.size() == 1 ) { SelectSmacWorldSize( category ); }
					else { N_EXPECT_ARGS( 3 ); N_GETVALUE( width, 1, Int ); N_GETVALUE( height, 2, Int ); RestoreSmacWorldSize( category, width, height ); }
				} catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
				return VALUE( gse::value::Undefined );
			} ) },
			WRAPIMPL_GET_CUSTOM( "ruleset", String, GetRulesetId() )
			WRAPIMPL_GET_WRAPPED( map )
			WRAPIMPL_GET_WRAPPED( rules )
			WRAPIMPL_GET_MAPPED_CUSTOM( difficulty_level, rules.m_difficulty_levels )
			WRAPIMPL_GET_PTR( "game_name", game_name )
WRAPIMPL_DYNAMIC_SETTERS( GlobalSettings )
	WRAPIMPL_SET_PTR( "game_name", String, game_name )
	WRAPIMPL_SET_MAPPED_CUSTOM( difficulty_level, obj->rules.m_difficulty_levels )
WRAPIMPL_DYNAMIC_ON_SET( GlobalSettings )
WRAPIMPL_DYNAMIC_END()

const types::Buffer GlobalSettings::Serialize() const {
	types::Buffer buf;

	if ( m_ruleset == RULESET_UNSELECTED ) {
		THROW( "Cannot serialize global settings without a gameplay ruleset" );
	}
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_resource_rules.has_value()
		|| ( m_ruleset == RULESET_SMAC ) != m_smac_production_rules.has_value()
		|| ( m_ruleset == RULESET_SMAC ) != m_smac_technology_rules.has_value() ) {
		THROW( "Gameplay ruleset and SMAC resource rules do not match" );
	}
	// The integer prefix rejects the old string-first format in both directions.
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_world_size_rules.has_value() ) { THROW( "Gameplay ruleset and SMAC world-size rules do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_faction_startup_rules.has_value() ) { THROW( "Gameplay ruleset and SMAC faction startup rules do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_predefined_unit_rules.has_value() ) { THROW( "Gameplay ruleset and predefined unit profile do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_movement_rules.has_value() ) { THROW( "Gameplay ruleset and movement profile do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_specialist_rules.has_value() ) { THROW( "Gameplay ruleset and specialist profile do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_territory_rules.has_value() ) { THROW( "Gameplay ruleset and territory profile do not match" ); }
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_terraforming_rules.has_value() ) { THROW( "Gameplay ruleset and terraforming profile do not match" ); }
	ValidateWorldSize( false );
	buf.WriteInt( 16 );
	buf.WriteString( GetRulesetId() );
	buf.WriteBool( m_smac_resource_rules.has_value() );
	if ( m_smac_resource_rules ) {
		buf.WriteString( m_smac_resource_rules->Serialize( *m_smac_technology_rules ).ToString() );
		buf.WriteString( m_smac_production_rules->Serialize().ToString() );
		buf.WriteString( m_smac_technology_rules->Serialize().ToString() );
		buf.WriteString( m_smac_world_size_rules->Serialize().ToString() );
		buf.WriteString( m_smac_faction_startup_rules->Serialize( *m_smac_technology_rules ).ToString() );
	}
	buf.WriteBool( m_smac_predefined_unit_rules.has_value() );
	if ( m_smac_predefined_unit_rules ) { buf.WriteString( m_smac_predefined_unit_rules->Serialize( *m_smac_technology_rules ).ToString() ); }
	buf.WriteBool( m_smac_movement_rules.has_value() );
	if ( m_smac_movement_rules ) { buf.WriteString( m_smac_movement_rules->Serialize().ToString() ); }
	buf.WriteBool( m_smac_specialist_rules.has_value() );
	if ( m_smac_specialist_rules ) { buf.WriteString( m_smac_specialist_rules->Serialize().ToString() ); }
	buf.WriteBool( m_smac_territory_rules.has_value() );
	if ( m_smac_territory_rules ) { buf.WriteString( m_smac_territory_rules->Serialize().ToString() ); }
	buf.WriteBool( m_smac_terraforming_rules.has_value() );
	if ( m_smac_terraforming_rules ) { buf.WriteString( m_smac_terraforming_rules->Serialize().ToString() ); }
	buf.WriteString( map.Serialize().ToString() );
	buf.WriteString( rules.Serialize().ToString() );
	buf.WriteInt( difficulty_level );
	buf.WriteString( game_name );
	if ( buf.lenw > network::Network::MAX_PACKET_BYTES ) { THROW( "Global settings exceed the network packet budget" ); }

	return buf;
}

void GlobalSettings::Deserialize( types::Buffer buf ) {
	if ( buf.lenw > network::Network::MAX_PACKET_BYTES ) { THROW( "Global settings exceed the network packet budget" ); }
	if ( buf.ReadInt() != 16 ) {
		THROW( "Unsupported global settings format (expected version 16)" );
	}
	const auto ruleset = ParseRuleset( buf.ReadString() );
	const auto has_smac_rules = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_smac_rules ) {
		THROW( "Gameplay ruleset and SMAC resource rules do not match" );
	}
	std::optional< SmacResourceRules > parsed;
	std::optional< SmacProductionRules > production;
	std::optional< SmacTechnologyRules > technologies;
	std::optional< SmacWorldSizeRules > world_sizes;
	std::optional< SmacFactionStartupRules > factions;
	std::optional< SmacPredefinedUnitRules > units;
	std::optional< SmacMovementRules > movement;
	std::optional< SmacSpecialistRules > specialists;
	std::optional< SmacTerritoryRules > territory;
	std::optional< SmacTerraformingRules > terraforming;
	if ( has_smac_rules ) {
		// Bound the frame before copying; resolve tokens after the staged catalog.
		const auto resource_bytes = buf.ReadString( SmacResourceRules::MAX_WIRE_BYTES );
		production = SmacProductionRules::Deserialize( types::Buffer( buf.ReadString() ) );
		technologies = SmacTechnologyRules::Deserialize( types::Buffer( buf.ReadString() ) );
		parsed = SmacResourceRules::Deserialize( types::Buffer( resource_bytes ), *technologies );
		world_sizes = SmacWorldSizeRules::Deserialize( types::Buffer( buf.ReadString() ) );
		factions = SmacFactionStartupRules::Deserialize( types::Buffer( buf.ReadString() ), *technologies );
	}
	const auto has_units = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_units ) { THROW( "Gameplay ruleset and predefined unit profile do not match" ); }
	if ( has_units ) { units = SmacPredefinedUnitRules::Deserialize( buf.ReadString(), *technologies ); }
	const auto has_movement = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_movement ) { THROW( "Gameplay ruleset and movement profile do not match" ); }
	if ( has_movement ) { movement = SmacMovementRules::Deserialize( types::Buffer( buf.ReadString( SmacMovementRules::MAX_WIRE_BYTES ) ) ); }
	const auto has_specialists = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_specialists ) { THROW( "Gameplay ruleset and specialist profile do not match" ); }
	if ( has_specialists ) { specialists = SmacSpecialistRules::Deserialize( types::Buffer( buf.ReadString( SmacSpecialistRules::MAX_WIRE_BYTES ) ) ); }
	const auto has_territory = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_territory ) { THROW( "Gameplay ruleset and territory profile do not match" ); }
	if ( has_territory ) { territory = SmacTerritoryRules::Deserialize( types::Buffer( buf.ReadString( SmacTerritoryRules::MAX_WIRE_BYTES ) ) ); }
	const auto has_terraforming = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_terraforming ) { THROW( "Gameplay ruleset and terraforming profile do not match" ); }
	if ( has_terraforming ) { terraforming = SmacTerraformingRules::Deserialize( types::Buffer( buf.ReadString( SmacTerraformingRules::MAX_WIRE_BYTES ) ) ); }
	if ( m_ruleset_locked && ( terraforming.has_value() != m_smac_terraforming_rules.has_value()
		|| ( terraforming && !( *terraforming == *m_smac_terraforming_rules ) ) ) ) { THROW( "Host changed the frozen terraforming profile" ); }
	if ( m_ruleset_locked && ( territory.has_value() != m_smac_territory_rules.has_value()
		|| ( territory && !( *territory == *m_smac_territory_rules ) ) ) ) { THROW( "Host changed the frozen territory profile" ); }
	if ( m_ruleset_locked && ( specialists.has_value() != m_smac_specialist_rules.has_value()
		|| ( specialists && !( *specialists == *m_smac_specialist_rules ) ) ) ) { THROW( "Host changed the frozen specialist profile" ); }
	if ( m_ruleset_locked && ( movement.has_value() != m_smac_movement_rules.has_value()
		|| ( movement && !( *movement == *m_smac_movement_rules ) ) ) ) { THROW( "Host changed the frozen movement profile" ); }
	if ( m_ruleset_locked && ( units.has_value() != m_smac_predefined_unit_rules.has_value()
		|| ( units && !( *units == *m_smac_predefined_unit_rules ) ) ) ) { THROW( "Host changed the predefined unit profile of an initialized session" ); }
	if ( m_ruleset_locked && m_ruleset != ruleset ) {
		THROW( "Host changed the gameplay ruleset of an initialized session" );
	}
	if ( m_ruleset_locked && ( parsed.has_value() != m_smac_resource_rules.has_value() || ( parsed && !( *parsed == *m_smac_resource_rules ) ) ) ) {
		THROW( "Host changed the numeric rules of an initialized session" );
	}
	if ( m_ruleset_locked && ( m_smac_production_rules.has_value() != production.has_value()
		|| ( production && !( *m_smac_production_rules == *production ) ) ) ) {
		THROW( "Host changed the production profile of an initialized session" );
	}
	if ( m_ruleset_locked && ( m_smac_technology_rules.has_value() != technologies.has_value()
		|| ( technologies && !( *m_smac_technology_rules == *technologies ) ) ) ) {
		THROW( "Host changed the technology catalog of an initialized session" );
	}
	MapSettings next_map;
	if ( m_ruleset_locked && ( m_smac_world_size_rules.has_value() != world_sizes.has_value()
		|| ( world_sizes && !( *m_smac_world_size_rules == *world_sizes ) ) ) ) { THROW( "Host changed the world-size catalog of an initialized session" ); }
	if ( m_ruleset_locked && ( m_smac_faction_startup_rules.has_value() != factions.has_value()
		|| ( factions && !( *m_smac_faction_startup_rules == *factions ) ) ) ) { THROW( "Host changed the faction startup profile of an initialized session" ); }
	next_map.Deserialize( map.Serialize() );
	next_map.m_world_size_locked = map.m_world_size_locked;
	next_map.Deserialize( buf.ReadString() );
	const auto& next_selection = next_map.GetSmacWorldSize();
	if ( next_selection ) {
		if ( !world_sizes || next_map.type == MapSettings::MT_MAPFILE ) { THROW( "Unexpected SMAC world-size selection" ); }
		world_sizes->ValidateSelection( *next_selection );
	}
	const auto rules_data = buf.ReadString();
	// Default rules currently serialize no fields; V13 must not silently drop
	// unknown payload bytes. A future rules format requires an explicit version.
	if ( !rules_data.empty() ) { THROW( "Unexpected default rules fields in global settings" ); }
	const auto next_difficulty = buf.ReadInt();
	const auto next_name = buf.ReadString();
	if ( buf.lenr != buf.lenw ) {
		THROW( "Unexpected fields in global settings" );
	}
	// Parsing and identity validation above cannot partially change live data.
	map.Deserialize( next_map.Serialize() );
	rules.Deserialize( rules_data );
	difficulty_level = next_difficulty;
	game_name = next_name;
	m_ruleset = ruleset;
	m_smac_resource_rules = parsed;
	m_smac_production_rules = std::move( production );
	m_smac_technology_rules = std::move( technologies );
	m_smac_world_size_rules = std::move( world_sizes );
	m_smac_faction_startup_rules = std::move( factions );
	m_smac_predefined_unit_rules = std::move( units );
	m_smac_movement_rules = std::move( movement );
	m_smac_specialist_rules = std::move( specialists );
	m_smac_territory_rules = std::move( territory );
	m_smac_terraforming_rules = std::move( terraforming );
	m_ruleset_locked = true;
}

WRAPMAP( game_mode, LocalSettings::game_mode_t,
	{ LocalSettings::GM_SINGLEPLAYER, "single" },
	{ LocalSettings::GM_MULTIPLAYER, "multi" },
	{ LocalSettings::GM_SCENARIO, "scenario" },
)

WRAPMAP( network_type, LocalSettings::network_type_t,
	{ LocalSettings::NT_NONE, "none" },
	{ LocalSettings::NT_SIMPLE_TCPIP, "simple_tcpip" },
	{ LocalSettings::NT_HOTSEAT, "hotseat" },
)

WRAPMAP( network_role, LocalSettings::network_role_t,
	{ LocalSettings::NR_NONE, "none" },
	{ LocalSettings::NR_CLIENT, "client" },
	{ LocalSettings::NR_SERVER, "server" },
)

WRAPIMPL_DYNAMIC_GETTERS( LocalSettings )
			{
				"account",
				account.Wrap( GSE_CALL ),
			},
			WRAPIMPL_GET_MAPPED( game_mode )
			WRAPIMPL_GET_MAPPED( network_type )
			WRAPIMPL_GET_MAPPED( network_role )
			WRAPIMPL_GET_PTR( "player_name", player_name )
			WRAPIMPL_GET_PTR( "remote_address", remote_address )
WRAPIMPL_DYNAMIC_SETTERS( LocalSettings )
	WRAPIMPL_SET_MAPPED( game_mode )
	WRAPIMPL_SET_MAPPED( network_type )
	WRAPIMPL_SET_MAPPED( network_role )
	WRAPIMPL_SET_PTR( "player_name", String, player_name )
	WRAPIMPL_SET_PTR( "remote_address", String, remote_address )
WRAPIMPL_DYNAMIC_ON_SET( LocalSettings )
WRAPIMPL_DYNAMIC_END()

const types::Buffer LocalSettings::Serialize() const {
	types::Buffer buf;

	buf.WriteInt( game_mode );
	buf.WriteInt( network_type );
	buf.WriteInt( network_role );
	buf.WriteString( player_name );
	buf.WriteString( remote_address );

	return buf;
}

void LocalSettings::Deserialize( types::Buffer buf ) {
	game_mode = (game_mode_t)buf.ReadInt();
	network_type = (network_type_t)buf.ReadInt();
	network_role = (network_role_t)buf.ReadInt();
	player_name = buf.ReadString();
	remote_address = buf.ReadString();
}

WRAPIMPL_BEGIN( Settings )
	WRAPIMPL_PROPS
			{
				"global",
				global.Wrap( GSE_CALL ),
			},
			{
				"local",
				local.Wrap( GSE_CALL ),
			},
		};
WRAPIMPL_END_PTR()

void Settings::PrepareRuleset( const GlobalSettings::RulesSource& read_source ) {
	if ( local.network_role == LocalSettings::NR_CLIENT ) {
		// Joining accepts the host's identity and values, never a local rules file.
		global.ResetRuleset();
	}
	else { global.InitializeHostRules( read_source ); }
}

const types::Buffer Settings::Serialize() const {
	types::Buffer buf;

	buf.WriteString( global.Serialize().ToString() );
	buf.WriteString( local.Serialize().ToString() );

	return buf;
}

void Settings::Deserialize( types::Buffer buf ) {
	global.Deserialize( types::Buffer( buf.ReadString() ) );
	local.Deserialize( types::Buffer( buf.ReadString() ) );
}

}
}
}
