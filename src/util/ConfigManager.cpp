#include "ConfigManager.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

#include "util/FS.h"
#include "util/String.h"
#include "config/Config.h"

namespace util {

ConfigManager::ConfigManager( const std::string& path, const config::Config* const config )
	: m_path( path )
	, m_config( config ) {}

void ConfigManager::AddRule( const std::string& argument, const std::string& description, const arg_handler_t handler ) {
	m_arg_rules[ argument ] = {
		argument,
		false,
		"",
		description,
		handler
	};
	size_t sz = argument.size() + 6;
	while ( m_offset < sz ) {
		m_offset += m_offset_step;
	}
}

void ConfigManager::AddRule( const std::string& argument, const std::string& value_name, const std::string& description, const arg_handler_t handler ) {
	m_arg_rules[ argument ] = {
		argument,
		true,
		value_name,
		description,
		handler
	};
	size_t sz = argument.size() + value_name.size() + 6;
	while ( m_offset < sz ) {
		m_offset += m_offset_step;
	}
}

const std::string ConfigManager::GetHelpString() const {
	std::string result = (std::string)"\n" +
		"Usage:\n\t" + m_path + " [options]\n" +
		"\n" +
		"Options:\n";
	for ( auto& rule : m_arg_rules ) {
		result +=
			std::string( m_offset_step, ' ' ) +
				"--" + rule.first +
				' ' +
				rule.second.value_name +
				std::string( std::max< ssize_t >( 1, (ssize_t)m_offset - rule.first.size() - rule.second.value_name.size() - 4 ), ' ' ) +
				rule.second.description
				+ "\n";
	}
	return result;
}

const std::string ConfigManager::GetUnknownArgumentNote() const {
	return (std::string)"Run\n\t" + m_path + " --help\nfor list of supported options.";
}

static const std::string s_errprefix_args = "argument --";
void ConfigManager::ParseArgs( const int argc, char* const argv[] ) {
	ASSERT( argc >= 1, "argc is zero" );

	const char* c;
	std::string kv, k;
	std::optional< std::string > v;
	for ( int i = 1 ; i < argc ; i++ ) {
		c = argv[ i ];
		if ( c[ 0 ] != '-' || c[ 1 ] != '-' ) {
			throw std::runtime_error( (std::string)"unknown argument \"" + c + "\"!" );
		}
		kv = ( c + 2 );
		size_t splitpos = kv.find( '=' );
		if ( splitpos != std::string::npos ) {
			k = kv.substr( 0, splitpos );
			v = kv.substr( splitpos + 1 );
		}
		else {
			k = kv;
			if (
				( i < argc - 1 )
				) {
				v = argv[ i + 1 ];
			}
			else {
				v = {};
			}
		}
		const auto& it = Set( k, v, s_errprefix_args, false, true );
		if ( it->second.has_value ) {
			i++;
		}
	}
}

void ConfigManager::ParseFile( const std::string& path ) {
	const auto& p = path.empty()
		? GetConfigPath()
		: path;
	if ( util::FS::FileExists( p ) ) {
		const auto errprefix = "setting " + p + ": ";
		YAML::Node root = YAML::LoadFile( p );
		for ( const auto& it : root ) {
			const auto& k = it.first.as< std::string >();
			if ( !it.second.IsNull() ) {
				Set( k, it.second.as< std::string >(), errprefix, true, false );
			}
			else {
				Set( k, {}, errprefix, true, false );
			}
		}
	}
}

void ConfigManager::UpdateSetting( const std::string& key, const std::string& value ) {
    UpdateSettings({{key,value}});
}

void ConfigManager::UpdateSettings(const std::map<std::string,std::string>& values) {
    if(values.empty()) {return;}
    auto previous=m_settings;
    try {
        for(const auto& [key,value]:values) {m_settings.insert_or_assign(key,std::make_pair(true,value));}
        Save();
    } catch(...) {m_settings.swap(previous);throw;}
}

const ConfigManager::settings_t& ConfigManager::GetSettings() const {
	return m_settings;
}

config::smac_preferences::Words ConfigManager::GetSmacPreferences(bool laptop,int difficulty) const {
    const auto packed=m_settings.find("smac-preferences");
    if(packed!=m_settings.end()) {return config::smac_preferences::Load(packed->second.second);}
    auto words=config::smac_preferences::Defaults(laptop,difficulty);
    const auto legacy=m_settings.find("automatic-unit-designs");
    if(legacy!=m_settings.end()) {
        if(legacy->second.second=="true") {words[0]|=0x4000u;}else {words[0]&=~0x4000u;}
    }
    return words;
}

const std::map< std::string, ConfigManager::arg_rule_t >::iterator ConfigManager::Set( const std::string& k, const std::optional< std::string >& v, const std::string& err_prefix, const bool is_saveable, const bool can_override ) {
	const auto& it = m_arg_rules.find( k );
	if ( it == m_arg_rules.end() ) {
		throw std::runtime_error( (std::string)"invalid " + err_prefix + k );
	}
	if ( it->second.has_value && !v.has_value() ) {
		throw std::runtime_error( (std::string)err_prefix + k + " requires a value!" );
	}
	if ( !can_override && m_settings.find( k ) != m_settings.end() ) {
		return it;
	}
	it->second.handler( v.value_or( "" ) );
	m_settings.insert_or_assign(
		k, std::make_pair(
			is_saveable,
			it->second.has_value
				? v.value_or( "" )
				: ""
		)
	);
	return it;
}

void ConfigManager::Save() {
	YAML::Node root;
	for ( const auto& kv : m_settings ) {
		if ( kv.second.first ) {
			root[ kv.first ] = kv.second.second;
		}
	}
	// Preference acceptance must not silently succeed on a failed write.
    std::ofstream out;
    out.exceptions(std::ios::failbit|std::ios::badbit);
    out.open(GetConfigPath(),std::ios::binary|std::ios::trunc);
    out << Dump(root) << util::String::EOLN();
    out.close();
}

const std::string ConfigManager::GetConfigPath() const {
	return m_config->GetPrefix() + "config.yml";
}

}
