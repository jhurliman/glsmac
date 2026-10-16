#pragma once

#include <string>

namespace task::gsetests {
class GSETests;
}

namespace gse::tests {

void AddMeshTests( task::gsetests::GSETests* task );
std::string CheckMeshCoordinateOutputs();

}
