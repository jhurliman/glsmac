#pragma once
#include <string>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddSmacBaseResourceContextTests( task::gsetests::GSETests* task );
std::string CheckSmacBaseStartupFixture( bool queued, bool wrong_difficulty );
}
