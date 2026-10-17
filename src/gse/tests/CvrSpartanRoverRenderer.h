#pragma once
#include <string>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
std::string MakeSpartanRoverTestAsset( const std::string& name );
void AddCvrSpartanRoverRendererTests( task::gsetests::GSETests* task );
}
