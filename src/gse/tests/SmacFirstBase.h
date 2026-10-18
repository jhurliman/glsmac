#pragma once
#include <string>
#include <cstdint>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddSmacFirstBaseTests(task::gsetests::GSETests* task);
std::string CheckSmacFirstBaseFixture(int scenario, bool queued, int32_t original_difficulty = 3, bool installed_planet = false);
}
