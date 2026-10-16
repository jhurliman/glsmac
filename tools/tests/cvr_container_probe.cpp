// Opt-in installed-file probe; not part of CTest and requires no game runtime.
// Build instructions and the supported envelope are in docs/cvr-container.md.
#include "loader/cvr/CvrContainerReader.h"
#include "loader/cvr/CvrSceneReader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

int main( const int argc, const char* const* argv ) {
	const bool inspect_scene = argc >= 2 && std::string( argv[ 1 ] ) == "--scene";
	if ( argc != ( inspect_scene ? 3 : 2 ) ) {
		std::cerr << "usage: cvr_container_probe [--scene] <installed asset directory>\n";
		return 2;
	}
	try {
		std::vector< std::filesystem::path > files;
		for ( const auto& file : std::filesystem::directory_iterator( argv[ inspect_scene ? 2 : 1 ] ) ) {
			if ( !file.is_regular_file() ) { continue; }
			auto extension = file.path().extension().string();
			for ( auto& c : extension ) { if ( c >= 'A' && c <= 'Z' ) { c += 'a' - 'A'; } }
			if ( extension == ".cvr" ) { files.push_back( file.path() ); }
		}
		std::sort( files.begin(), files.end() );
		if ( files.empty() ) { throw std::runtime_error( "no CVR files found" ); }
		size_t accepted = 0, rejected = 0;
		uint64_t total_bytes = 0;
		uint64_t total_objects = 0, geometry_objects = 0, frame_records = 0;
		for ( const auto& path : files ) {
			try {
				std::ifstream stream( path, std::ios::binary );
				if ( !stream ) { throw std::runtime_error( "could not open file" ); }
				const std::string bytes{ std::istreambuf_iterator< char >( stream ), std::istreambuf_iterator< char >() };
				if ( stream.bad() ) { throw std::runtime_error( "could not read complete file" ); }
				const auto container = loader::cvr::CvrContainerReader::Read( bytes );
				std::optional< loader::cvr::CvrScene > scene;
				if ( inspect_scene ) { scene = loader::cvr::CvrSceneReader::Read( container ); }
				std::cout << path.filename().string() << '\t' << bytes.size();
				for ( const auto& chunk : container.chunks ) { std::cout << '\t' << chunk.tag << ':' << chunk.payload.size(); }
				if ( scene ) {
					std::cout << "\tobjects:" << scene->objects.size() << "\tframes:" << scene->frame_count;
					total_objects += scene->objects.size();
					for ( const auto& object : scene->objects ) {
						geometry_objects += object.voxel_data.has_value();
						frame_records += object.frames.size();
					}
				}
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
		if ( inspect_scene ) {
			std::cout << "objects=" << total_objects << " geometry_objects=" << geometry_objects << " frame_records=" << frame_records << '\n';
		}
		return rejected ? 1 : 0;
	}
	catch ( const std::exception& error ) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
