#pragma once

#include <unordered_map>
#include <functional>

#include "TXTLoader.h"
#include "game/backend/settings/SmacFactionStartupRules.h"
#include "game/backend/faction/DisplayIdentity.h"

namespace loader {
namespace txt {

CLASS( FactionTXTLoader, TXTLoader )

	struct faction_data_t {
		struct {
			std::vector< std::string > land;
			std::vector< std::string > water;
		} bases_names;
	};
	const faction_data_t& GetFactionData( resource::resource_t res );
	const faction_data_t& GetFactionData( const std::string& filename );
	game::backend::faction::DisplayIdentity GetDisplayIdentity(const std::string& filename, const std::string& section);
	static game::backend::faction::DisplayIdentity ParseDisplayIdentity(const std::string& source, const std::string& section);
	// Pure, canonical SMAC startup metadata. No filesystem/cache access or
	// gameplay effects; callers provide the host's selected source texts.
	using StartupSource = std::function< std::string( const std::string& filename ) >;
	static game::backend::settings::SmacFactionStartupRules ParseStartupRules( const std::string& alpha,
		const StartupSource& read_source, const game::backend::settings::SmacTechnologyRules& technologies );
	static std::vector< game::backend::settings::SmacFactionStartupBonus > ParseStartupBonuses( const std::string& source,
		const std::string& section, const game::backend::settings::SmacTechnologyRules& technologies );

protected:

private:

	std::unordered_map< std::string, faction_data_t > m_faction_data = {};

	const faction_data_t& GetFactionDataImpl( const std::string& path );
	const std::vector< std::string >& GetSection( const txt_data_t& data, const std::string& name ) const;

};

}
}
