#include "SmacFirstBase.h"
#include "task/gsetests/GSETests.h"
namespace gse::tests {
void AddSmacFirstBaseTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC perihelion transitions enter actual multi-owner canonical upkeep at2190 and2210",GT(){
        return CheckSmacFirstBaseFixture(12,false);
    });
    task->AddTest("SMAC Planet ecology globals pass imported map through actual claimed Start",GT(){
        return CheckSmacFirstBaseFixture(0,false,3,true);
    });
    for(int difficulty=0;difficulty<6;++difficulty) {
        for(const int scenario:{0,2,4,5,6,7,8}) {
            task->AddTest("SMAC Planet first Base faction scenario "+std::to_string(scenario)+" difficulty "+std::to_string(difficulty)+" through claimed Start",
                GT(scenario,difficulty) {return CheckSmacFirstBaseFixture(scenario,false,difficulty,true);});
        }
    }
    task->AddTest("SMAC first Base all six difficulties copy governor and stored human target inputs",GT(){
        for (int difficulty=0;difficulty<6;++difficulty) {
            for (int scenario : {0,2}) {
                const auto result=CheckSmacFirstBaseFixture(scenario,false,difficulty);
                if(!result.empty()) {return "difficulty "+std::to_string(difficulty)+": "+result;}
            }
        } GT_OK();
    });
    task->AddTest("SMAC first Base claimed Start computes initial arid package before refresh",GT(){
        return CheckSmacFirstBaseFixture(0,false);
    });
    task->AddTest("SMAC first Base queued Start chooses an actual productive worker",GT(){
        return CheckSmacFirstBaseFixture(1,true);
    });
    task->AddTest("SMAC first Base owner packages and callback publication preserve authority",GT(){
        for (const int scenario : {2,4,5,6,7,8,9}) {const auto result=CheckSmacFirstBaseFixture(scenario,false); if(!result.empty()){return result;}} GT_OK();
    });
    task->AddTest("SMAC canonical upkeep late failure blocks retry and retires partial world",GT(){
        return CheckSmacFirstBaseFixture(10,false);
    });
    task->AddTest("SMAC canonical upkeep groups owners before saved Base record order",GT(){
        return CheckSmacFirstBaseFixture(11,false);
    });
    task->AddTest("SMAC first Base rejected preparation leaves identity map and knowledge unchanged",GT(){
        return CheckSmacFirstBaseFixture(3,false);
    });
}
}
