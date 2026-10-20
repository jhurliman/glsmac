#include "OwnedDesigns.h"
#include "gse/tests/fixtures/SmacInstalledDesignCheck.h"
#include <iostream>
int main() {
    try {const auto result=gse::tests::fixtures::installed_designs::Verify(owned_designs::Components(),owned_designs::Rows(),owned_designs::Names(),true);
        if(result.loader!=468 || result.localized!=357 || result.controller!=3844 || result.mark!=64) {throw std::runtime_error("Installed design native coverage differs");}
        std::cout<<"PASS: 2 full predefined pools,18 localized constructors,56 installed early controllers;4669 numeric and64 caller stores\n";
    }catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
