#pragma once

#include <string>
#include <functional>

namespace task::gsetests {
class GSETests;
}

namespace gse::tests {

void AddMeshTests( task::gsetests::GSETests* task );
std::string CheckMeshCoordinateOutputs();
std::string WithWidgetGraphics( const std::function< std::string() >& check, unsigned short width=64, unsigned short height=64 );

}
