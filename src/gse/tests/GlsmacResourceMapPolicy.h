#pragma once

#include <string>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddGlsmacResourceMapPolicyTests( task::gsetests::GSETests* task );
std::string CheckResourceMapPolicyBackend( int scenario );
}
