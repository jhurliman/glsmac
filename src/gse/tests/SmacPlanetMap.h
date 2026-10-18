#pragma once
#include <string>
namespace task::gsetests { class GSETests; }
namespace gse::tests {
void AddSmacPlanetMapTests(task::gsetests::GSETests* task);
std::string CheckInstalledPlanetBackend(const std::string& installed_asset = {});
}
