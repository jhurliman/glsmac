#pragma once
#include <string>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddSmacFirstBaseTests(task::gsetests::GSETests* task);
std::string CheckSmacFirstBaseFixture(int scenario, bool queued);
}
