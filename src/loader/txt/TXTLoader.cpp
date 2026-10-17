#include "TXTLoader.h"

#include "util/FS.h"
#include "util/String.h"

#include <stdexcept>

namespace loader {
namespace txt {

std::vector< std::string > TXTLoader::ReadUniqueSection( const std::string& source, const std::string& name ) {
	std::vector< std::string > result;
	bool found = false;
	bool inside = false;
	for ( const auto& raw : util::String::Split( source, '\n' ) ) {
		const auto line = util::String::TrimCopy( raw );
		// alpha.txt uses both semicolon comments and ## translator comments.
		if ( line.empty() || line.front() == ';' || line.compare( 0, 2, "##" ) == 0 ) { continue; }
		if ( line.front() == '#' && line.find_first_of( " \t" ) == std::string::npos ) {
			inside = line == "#" + name;
			if ( inside ) {
				if ( found ) { throw std::invalid_argument( "Duplicate section #" + name ); }
				found = true;
			}
		}
		else if ( inside ) { result.push_back( line ); }
	}
	if ( !found ) { throw std::invalid_argument( "Missing section #" + name ); }
	return result;
}

const TXTLoader::txt_data_t& TXTLoader::GetTXTData( const std::string& path ) {
	auto it = m_txt_data.find( path );
	if ( it == m_txt_data.end() ) {
		it = m_txt_data.insert(
			{
				path,
				txt_data_t{ util::FS::ReadTextFile( path ) }
			}
		).first;
	}
	return it->second;
}

TXTLoader::txt_data_t::txt_data_t( const std::string& source ) {

	// raw lines
	lines = util::String::Split( source, '\n' );

	// sections
	sections_t::iterator section_it = sections.end();
	for ( const auto& line : lines ) {
		if ( !line.empty() && line[ 0 ] == '#' && line.find( ' ' ) == std::string::npos ) {
			if ( line.substr( 0, 4 ) == "#END" ) {
				section_it = sections.end();
			}
			else {
				const auto section_name = line.substr( 1 );
				ASSERT( sections.find( section_name ) == sections.end(), "section '" + section_name + "' already exists" );
				section_it = sections.insert(
					{
						section_name,
						{}
					}
				).first;
			}
		}
		else {
			if ( !line.empty() && section_it != sections.end() ) {
				section_it->second.push_back( line );
			}
		}
	}

}

}
}
