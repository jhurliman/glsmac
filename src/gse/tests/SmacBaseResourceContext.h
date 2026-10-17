#pragma once
#include <string>
#include <cstdint>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddSmacBaseResourceContextTests( task::gsetests::GSETests* task );
std::string CheckSmacBaseStartupFixture( bool queued, int32_t original_difficulty );
}
