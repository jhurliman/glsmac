// Opt-in installed-file probe; not part of CTest and requires no game runtime.
// Build instructions and the supported envelope are in docs/cvr-container.md.
#include "loader/cvr/CvrContainerReader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

int main( const int argc, const char* const* argv ) {
	if ( argc != 2 ) {
		std::cerr << "usage: cvr_container_probe <installed asset directory>\n";
		return 2;
	}
	try {
		std::vector< std::filesystem::path > files;
		for ( const auto& file : std::filesystem::directory_iterator( argv[ 1 ] ) ) {
			if ( !file.is_regular_file() ) { continue; }
			auto extension = file.path().extension().string();
			for ( auto& c : extension ) { if ( c >= 'A' && c <= 'Z' ) { c += 'a' - 'A'; } }
			if ( extension == ".cvr" ) { files.push_back( file.path() ); }
		}
		std::sort( files.begin(), files.end() );
		if ( files.empty() ) { throw std::runtime_error( "no CVR files found" ); }
		size_t accepted = 0, rejected = 0;
		uint64_t total_bytes = 0;
		for ( const auto& path : files ) {
			try {
				std::ifstream stream( path, std::ios::binary );
				if ( !stream ) { throw std::runtime_error( "could not open file" ); }
				const std::string bytes{ std::istreambuf_iterator< char >( stream ), std::istreambuf_iterator< char >() };
				if ( stream.bad() ) { throw std::runtime_error( "could not read complete file" ); }
				const auto container = loader::cvr::CvrContainerReader::Read( bytes );
				std::cout << path.filename().string() << '\t' << bytes.size();
				for ( const auto& chunk : container.chunks ) { std::cout << '\t' << chunk.tag << ':' << chunk.payload.size(); }
				std::cout << '\n';
				total_bytes += bytes.size();
				++accepted;
			}
			catch ( const std::exception& error ) {
				std::cerr << path.filename().string() << ": " << error.what() << '\n';
				++rejected;
			}
		}
		std::cout << "accepted=" << accepted << " rejected=" << rejected << " accepted_bytes=" << total_bytes << '\n';
		return rejected ? 1 : 0;
	}
	catch ( const std::exception& error ) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
