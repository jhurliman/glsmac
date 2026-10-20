#include "SmacInstalledDesigns.h"
#include "fixtures/SmacInstalledDesignCheck.h"
#include "task/gsetests/GSETests.h"
namespace gse::tests {
void AddSmacInstalledDesignTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC installed predefined designs and localized early controller match native controls",GT(){
        try {const auto stats=fixtures::installed_designs::Verify(fixtures::installed_designs::Components(),fixtures::installed_designs::Rows(),fixtures::installed_designs::Names(),false);
            GT_ASSERT(stats.loader==468 && stats.localized==357 && stats.controller==3844 && stats.mark==64,"Installed design native coverage changed");
        }catch(const std::exception& error) {GT_ASSERT(false,"Native control: "+std::string(error.what()));}
        return {};
    });
}
}
