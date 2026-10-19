#include "SmacComputerGoals.h"
#include "game/backend/faction/SmacComputerGoals.h"
#include "task/gsetests/GSETests.h"
#include <yaml-cpp/yaml.h>
#include <vector>

namespace gse::tests {
namespace {
using Goals=game::backend::faction::SmacComputerGoals;
using Goal=game::backend::faction::SmacComputerGoal;
Goal Read(const YAML::Node& row) {return {row[0].as<int16_t>(),row[1].as<int16_t>(),row[2].as<int32_t>(),row[3].as<int32_t>(),row[4].as<int32_t>()};}
std::vector<Goal> Active(const Goals& goals) {
    std::vector<Goal> result;for(const auto& row:goals.active) {if(row.type>=0) {result.push_back(row);}}return result;
}
}
void AddSmacComputerGoalsTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC computer goals renewal matches ten original planning controls",GT(){
        const auto source=YAML::LoadFile("docs/behavior/computer-upkeep-expansion-planning-cases.json");
        for(const auto* group:{"production_controls","visibility_controls"}) {for(const auto& control:source[group]) {
            Goals goals;size_t index=0;
            for(const auto& row:control["goals_before"]) {
                const auto record=Read(row);goals.active.at(index++)=record;
                // Native territory refill inserts these type3 goals through
                // the persistent helper; Base/other planner tasks are active.
                if(record.type==3) {goals.InsertPersistent(record.type,record.priority,record.x,record.y,64,80);}
            }
            std::vector<Goal> expected;for(const auto& row:control["goals_after"]) {expected.push_back(Read(row));}
            const auto persistent=goals.persistent;goals.Renew(64,80);
            GT_ASSERT(Active(goals)==expected && goals.persistent==persistent,"Original planning renewal differs: "+control["variant"].as<std::string>());
        }}GT_OK();
    });
    task->AddTest("SMAC computer goals duplicates retain auxiliary and defer persistent refresh",GT(){
        Goals goals;goals.persistent[0].auxiliary=1234;
        goals.InsertPersistent(3,1,2,2,64,80);
        GT_ASSERT(goals.persistent[0].auxiliary==1234 && goals.active[0].auxiliary==-1,"Persistent insertion overwrote its unwritten DWORD or active auxiliary differs");
        goals.active[0].auxiliary=77;
        goals.InsertPersistent(3,4,2,2,64,80);
        GT_ASSERT(goals.persistent[0].priority==4 && goals.active[0].priority==1 && goals.active[0].auxiliary==77,"Persistent duplicate immediately refreshed active state");
        goals.Renew(64,80);
        GT_ASSERT(goals.active[0].priority==4 && goals.active[0].auxiliary==77,"Renewal failed to raise duplicate priority or overwrote auxiliary");
        goals.InsertActive(3,2,2,2,999,64,80);
        GT_ASSERT(goals.active[0].priority==4 && goals.active[0].auxiliary==77,"Lower active duplicate changed priority/auxiliary");
        goals.active[1]={9,0,3,3,88};goals.active[2]={9,-32768,4,4,99};
        goals.Renew(64,80);
        GT_ASSERT(goals.active[1].type==9 && goals.active[1].priority==0 && goals.active[2].type==-1 && goals.active[2].priority==-32768,"Signed expiry or zero-priority retention differs");GT_OK();
    });
    task->AddTest("SMAC computer goals replacement uses strict scores first ties and independent bounds",GT(){
        Goals goals;
        for(size_t i=0;i<goals.active.size();++i) {goals.active[i]={1,10,int32_t(i),0,1};}
        goals.active[5].priority=-1;goals.active[6].priority=-1;goals.active[7].priority=-100;
        goals.InsertActive(3,1,20,20,8,64,80);
        GT_ASSERT(goals.active[5].type==3 && goals.active[6].type==1 && goals.active[7].type==1,"Active first positive-score tie or zero-score rejection differs");
        for(size_t i=0;i<goals.persistent.size();++i) {goals.persistent[i]={1,20,int32_t(i),0,55};}
        const auto full=goals;goals.InsertPersistent(3,21,30,30,64,80);
        GT_ASSERT(goals==full,"Nonpositive replacement score was accepted");
        goals.persistent[3].priority=-1;goals.persistent[4].priority=-1;
        goals.InsertPersistent(3,1,30,30,64,80);
        GT_ASSERT(goals.persistent[3].type==3 && goals.persistent[4].type==1 && goals.persistent[3].auxiliary==55,"Persistent replacement differs from independent score/first tie");
        const auto before=goals.active;
        goals.InsertPersistent(3,2,-2,2,64,80);
        GT_ASSERT(goals.persistent[4].x==-2 && goals.active==before,"Persistent out-of-bounds retention or active bounds guard differs");
        Goals empty;empty.active[0]={1,-32768,0,0,0};
        empty.InsertActive(3,1,2,2,42,64,80);
        GT_ASSERT(empty.active[1].type==3 && empty.active[0].type==1,"Unused score1000 did not outrank negative occupied score");GT_OK();
    });
    task->AddTest("SMAC computer goals compare incoming DWORD before signed word storage",GT(){
        Goals goals;goals.InsertActive(3,32768,2,2,9,64,80);
        GT_ASSERT(goals.active[0].priority==-32768,"Incoming priority was not stored as signed low16");
        goals.InsertActive(3,65537,2,2,10,64,80);
        GT_ASSERT(goals.active[0].priority==1 && goals.active[0].auxiliary==9,"Duplicate compared narrowed priority or overwrote auxiliary");
        Goals forwarded;forwarded.InsertPersistent(65539,65537,2,2,64,80);
        GT_ASSERT(forwarded.persistent[0].type==3 && forwarded.persistent[0].priority==1 && forwarded.active[0].type==3 && forwarded.active[0].priority==1,"Persistent insertion did not forward sign-extended stored fields");GT_OK();
    });
}
}
