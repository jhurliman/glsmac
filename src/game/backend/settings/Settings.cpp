#include "Settings.h"

#include "util/FS.h"
#include "loader/txt/SmacRulesTXTLoader.h"

#include <limits>

#include "gse/value/Float.h"
#include "gse/value/Ptr.h"

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
	WRAPIMPL_SET_MAPPED( type )
	WRAPIMPL_SET_PTR( "filename", String, filename )
	WRAPIMPL_SET_PTR( "size_x", Int, size_x )
	WRAPIMPL_SET_PTR( "size_y", Int, size_y )
	WRAPIMPL_SET_PTR( "ocean_coverage", Float, ocean_coverage )
	WRAPIMPL_SET_PTR( "erosive_forces", Float, erosive_forces )
	WRAPIMPL_SET_PTR( "native_lifeforms", Float, native_lifeforms )
	WRAPIMPL_SET_PTR( "cloud_cover", Float, cloud_cover )
WRAPIMPL_DYNAMIC_ON_SET( MapSettings )
WRAPIMPL_DYNAMIC_END()

const types::Buffer MapSettings::Serialize() const {
	types::Buffer buf;

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
	type = (type_t)buf.ReadInt();
	filename = buf.ReadString();
	size_x = buf.ReadInt();
	size_y = buf.ReadInt();
	ocean_coverage = buf.ReadFloat();
	erosive_forces = buf.ReadFloat();
	native_lifeforms = buf.ReadFloat();
	cloud_cover = buf.ReadFloat();
	if ( buf.lenr != buf.lenw ) { THROW( "Unexpected fields in map settings" ); }
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
	m_smac_base_square_rules.reset();
}

void GlobalSettings::InitializeHostRules( const RulesSource& read_source ) {
	if ( m_ruleset_locked ) { return; } // Already frozen for this session.
	if ( m_ruleset == RULESET_UNSELECTED ) { THROW( "Select a gameplay ruleset before starting a game" ); }
	std::optional< resource::smac::BaseSquareRules > parsed;
	if ( m_ruleset == RULESET_SMAC ) {
		parsed = loader::txt::SmacRulesTXTLoader::ParseBaseSquareRules( read_source( "alpha.txt" ) );
	}
	// Only publish after file reading and all parsing succeeded.
	m_smac_base_square_rules = parsed;
	LockRuleset();
}

const std::optional< resource::smac::BaseSquareRules >& GlobalSettings::GetSmacBaseSquareRules() const {
	return m_smac_base_square_rules;
}

void GlobalSettings::LockRuleset() {
	if ( m_ruleset == RULESET_UNSELECTED ) {
		THROW( "Select a gameplay ruleset before starting a game" );
	}
	if ( m_ruleset == RULESET_SMAC && !m_smac_base_square_rules ) {
		THROW( "SMAC base-square rules have not been loaded" );
	}
	m_ruleset_locked = true;
}

bool GlobalSettings::IsRulesetLocked() const {
	return m_ruleset_locked;
}

void GlobalSettings::ResetRuleset() {
	m_ruleset = RULESET_UNSELECTED;
	m_ruleset_locked = false;
	m_smac_base_square_rules.reset();
}

WRAPIMPL_DYNAMIC_GETTERS( GlobalSettings )
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
	if ( ( m_ruleset == RULESET_SMAC ) != m_smac_base_square_rules.has_value() ) {
		THROW( "Gameplay ruleset and SMAC base-square rules do not match" );
	}
	// The integer prefix rejects the old string-first format in both directions.
	buf.WriteInt( 2 );
	buf.WriteString( GetRulesetId() );
	buf.WriteBool( m_smac_base_square_rules.has_value() );
	if ( m_smac_base_square_rules ) {
		for ( const auto& row : { m_smac_base_square_rules->base_square, m_smac_base_square_rules->bonus_square, m_smac_base_square_rules->recycling } ) {
			for ( const auto value : row ) { buf.WriteInt( value ); }
		}
	}
	buf.WriteString( map.Serialize().ToString() );
	buf.WriteString( rules.Serialize().ToString() );
	buf.WriteInt( difficulty_level );
	buf.WriteString( game_name );

	return buf;
}

static bool SameSmacRules( const std::optional< resource::smac::BaseSquareRules >& a, const std::optional< resource::smac::BaseSquareRules >& b ) {
	return a.has_value() == b.has_value() && ( !a || ( a->base_square == b->base_square && a->bonus_square == b->bonus_square && a->recycling == b->recycling ) );
}

void GlobalSettings::Deserialize( types::Buffer buf ) {
	if ( buf.ReadInt() != 2 ) {
		THROW( "Unsupported global settings format (expected version 2)" );
	}
	const auto ruleset = ParseRuleset( buf.ReadString() );
	const auto has_smac_rules = buf.ReadBool();
	if ( ( ruleset == RULESET_SMAC ) != has_smac_rules ) {
		THROW( "Gameplay ruleset and SMAC base-square rules do not match" );
	}
	std::optional< resource::smac::BaseSquareRules > parsed;
	if ( has_smac_rules ) {
		std::array< resource::smac::RuleTriple, 3 > rows;
		for ( auto& row : rows ) {
			for ( auto& field : row ) {
				const auto value = buf.ReadInt();
				if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
					THROW( "SMAC base-square rule is outside signed int32 range" );
				}
				field = static_cast< int32_t >( value );
			}
		}
		parsed.emplace( rows[ 0 ], rows[ 1 ], rows[ 2 ] );
	}
	if ( m_ruleset_locked && m_ruleset != ruleset ) {
		THROW( "Host changed the gameplay ruleset of an initialized session" );
	}
	if ( m_ruleset_locked && !SameSmacRules( m_smac_base_square_rules, parsed ) ) {
		THROW( "Host changed the numeric rules of an initialized session" );
	}
	MapSettings next_map;
	next_map.Deserialize( buf.ReadString() );
	const auto rules_data = buf.ReadString();
	// Default rules currently serialize no fields; V2 must not silently drop
	// unknown payload bytes. A future rules format requires an explicit version.
	if ( !rules_data.empty() ) { THROW( "Unexpected default rules fields in global settings" ); }
	const auto next_difficulty = buf.ReadInt();
	const auto next_name = buf.ReadString();
	if ( buf.lenr != buf.lenw ) {
		THROW( "Unexpected fields in global settings" );
	}
	// Parsing and identity validation above cannot partially change live data.
	map = next_map;
	rules.Deserialize( rules_data );
	difficulty_level = next_difficulty;
	game_name = next_name;
	m_ruleset = ruleset;
	m_smac_base_square_rules = parsed;
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
