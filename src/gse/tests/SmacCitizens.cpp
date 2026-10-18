#include "SmacCitizens.h"
#include "game/backend/base/SmacCitizens.h"
#include "task/gsetests/GSETests.h"
#include <climits>
namespace gse::tests {
using namespace game::backend::base;
void AddSmacCitizensTests(task::gsetests::GSETests* task) {
task->AddTest("SMAC citizen ordinary and assimilation prefixes match original controls",GT(){
{const auto r=CalculateSmacCitizenPrefix(7,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==6 && r.faction_talents==0),"CITIZEN-INPUT-001");}
{const auto r=CalculateSmacCitizenPrefix(1,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==0 && r.after_faction==0 && r.faction_talents==0),"CITIZEN-INPUT-002");}
{const auto r=CalculateSmacCitizenPrefix(2,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==1 && r.after_faction==1 && r.faction_talents==0),"CITIZEN-INPUT-003");}
{const auto r=CalculateSmacCitizenPrefix(6,true,0,0,0,0);GT_ASSERT((r.content_allowance==6 && r.ordinary==0 && r.after_faction==0 && r.faction_talents==0),"CITIZEN-INPUT-004");}
{const auto r=CalculateSmacCitizenPrefix(7,true,0,0,0,0);GT_ASSERT((r.content_allowance==6 && r.ordinary==1 && r.after_faction==1 && r.faction_talents==0),"CITIZEN-INPUT-005");}
{const auto r=CalculateSmacCitizenPrefix(4,false,0,0,0,0);GT_ASSERT((r.content_allowance==3 && r.ordinary==1 && r.after_faction==1 && r.faction_talents==0),"CITIZEN-INPUT-006");}
{const auto r=CalculateSmacCitizenPrefix(4,false,5,0,0,0);GT_ASSERT((r.content_allowance==3 && r.ordinary==1 && r.after_faction==1 && r.faction_talents==0),"CITIZEN-INPUT-007");}
{const auto r=CalculateSmacCitizenPrefix(3,true,5,4,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==2 && r.after_faction==2 && r.faction_talents==0),"CITIZEN-INPUT-008");}
{const auto r=CalculateSmacCitizenPrefix(4,true,5,4,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==3 && r.after_faction==4 && r.faction_talents==0),"CITIZEN-INPUT-009");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,4,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==7 && r.faction_talents==0),"CITIZEN-INPUT-010");}
{const auto r=CalculateSmacCitizenPrefix(1,true,5,0,4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==0 && r.after_faction==0 && r.faction_talents==1),"CITIZEN-INPUT-011");}
{const auto r=CalculateSmacCitizenPrefix(4,true,5,0,4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==3 && r.after_faction==3 && r.faction_talents==1),"CITIZEN-INPUT-012");}
{const auto r=CalculateSmacCitizenPrefix(5,true,5,0,4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==4 && r.after_faction==4 && r.faction_talents==2),"CITIZEN-INPUT-013");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,4,4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==7 && r.faction_talents==2),"CITIZEN-INPUT-014");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,1,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==13 && r.faction_talents==0),"CITIZEN-INPUT-015");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==6 && r.faction_talents==0),"CITIZEN-INPUT-016");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,-4,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==5 && r.faction_talents==0),"CITIZEN-INPUT-017");}
{const auto r=CalculateSmacCitizenPrefix(1,true,5,0,-4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==0 && r.after_faction==0 && r.faction_talents==1),"CITIZEN-INPUT-018");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==6 && r.faction_talents==0),"CITIZEN-INPUT-019");}
{const auto r=CalculateSmacCitizenPrefix(7,true,5,0,4,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==6 && r.after_faction==6 && r.faction_talents==2),"CITIZEN-INPUT-020");}
{const auto r=CalculateSmacCitizenPrefix(0,true,5,0,0,0);GT_ASSERT((r.content_allowance==1 && r.ordinary==0 && r.after_faction==0 && r.faction_talents==0),"CITIZEN-INPUT-021");}
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,11).assimilation==2),"ASSIM-TIMER-001");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,10).assimilation==1),"ASSIM-TIMER-002");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,1).assimilation==1),"ASSIM-TIMER-003");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,2).assimilation==1),"ASSIM-TIMER-004");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,11).assimilation==2),"ASSIM-TIMER-005");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,2).assimilation==1),"ASSIM-TIMER-006");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,1).assimilation==1),"ASSIM-TIMER-007");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,0).assimilation==0),"ASSIM-TIMER-008");
GT_ASSERT((CalculateSmacCitizenPrefix(7,true,5,0,0,0).assimilation==0),"ASSIM-TIMER-009");
GT_ASSERT((CalculateSmacCitizenPrefix(1,true,5,0,0,255).assimilation==1),"ASSIM-TIMER-010");
GT_ASSERT((CalculateSmacCitizenPrefix(1,true,0,0,0,255).assimilation==0),"ASSIM-TIMER-011");
GT_ASSERT((CalculateSmacCitizenPrefix(127,true,5,0,0,255).assimilation==26),"ASSIM-TIMER-012");
GT_ASSERT((CalculateSmacCitizenPrefix(20,true,5,0,0,20).assimilation==2),"ASSIM-TIMER-013");
GT_ASSERT((CalculateSmacCitizenPrefix(20,true,5,0,0,21).assimilation==3),"ASSIM-TIMER-014");
GT_ASSERT((CalculateSmacCitizenPrefix(7,false,0,0,0,11).assimilation==2),"ASSIM-TIMER-015");
GT_ASSERT((CalculateSmacCitizenPrefix(7,false,5,0,0,11).assimilation==2),"ASSIM-TIMER-016");
return "";});
task->AddTest("SMAC citizen bureaucracy preserves original global list distribution",GT(){
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,3);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-001");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,4,3);GT_ASSERT((r.interval==4 && r.drones==0),"BUREAU-DRONE-002");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,0);GT_ASSERT((r.interval==4 && r.drones==0),"BUREAU-DRONE-003");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,4);GT_ASSERT((r.interval==4 && r.drones==0),"BUREAU-DRONE-004");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,7);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-005");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,8);GT_ASSERT((r.interval==4 && r.drones==0),"BUREAU-DRONE-006");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,8,0);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-007");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,9,3);GT_ASSERT((r.interval==4 && r.drones==2),"BUREAU-DRONE-008");}
{const auto r=CalculateSmacCitizenBureaucracy(1,true,5,0,80,40,100,3);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-009");}
{const auto r=CalculateSmacCitizenBureaucracy(0,true,5,0,80,40,5,3);GT_ASSERT((r.interval==4 && r.drones==0),"BUREAU-DRONE-010");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,-4,80,40,5,3);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-011");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,4,80,40,5,3);GT_ASSERT((r.interval==8 && r.drones==0),"BUREAU-DRONE-012");}
{const auto r=CalculateSmacCitizenBureaucracy(7,false,0,0,80,40,8,6);GT_ASSERT((r.interval==7 && r.drones==1),"BUREAU-DRONE-013");}
{const auto r=CalculateSmacCitizenBureaucracy(7,false,5,0,80,40,8,6);GT_ASSERT((r.interval==7 && r.drones==1),"BUREAU-DRONE-014");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,3);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-015");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,3);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-016");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,8,8,100,511);GT_ASSERT((r.interval==0 && r.drones==0),"BUREAU-DRONE-017");}
{const auto r=CalculateSmacCitizenBureaucracy(7,true,5,0,80,40,5,511);GT_ASSERT((r.interval==4 && r.drones==1),"BUREAU-DRONE-018");}
return "";});
task->AddTest("SMAC citizen facility and project inputs match original controls",GT(){
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-001");}
{std::array<bool,34> f{};f[6]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-002");}
{std::array<bool,34> f{};f[8]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-003");}
{std::array<bool,34> f{};f[11]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-004");}
{std::array<bool,34> f{};f[12]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==2 && r.project_talents==0,"CITIZEN-FAC-005");}
{std::array<bool,34> f{};f[17]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-1 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-006");}
{std::array<bool,34> f{};f[18]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-1 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-007");}
{std::array<bool,34> f{};f[22]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==1 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-008");}
{std::array<bool,34> f{};f[8]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-009");}
{std::array<bool,34> f{};f[8]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,true,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-010");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,true,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-011");}
{std::array<bool,34> f{};f[8]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-012");}
{std::array<bool,34> f{};f[8]=true;f[11]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,true,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-013");}
{std::array<bool,34> f{};f[6]=true;f[11]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-4 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-014");}
{std::array<bool,34> f{};f[17]=true;f[18]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-015");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(3,f,1,false,true,false,false);GT_ASSERT(r.drone_adjustment==-1 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-016");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(4,f,1,false,true,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-017");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(3,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-018");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,true,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==1,"CITIZEN-FAC-019");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,true);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==2,"CITIZEN-FAC-020");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,true,true);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==3,"CITIZEN-FAC-021");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-022");}
{std::array<bool,34> f{};f[6]=true;f[8]=true;f[11]=true;f[12]=true;f[17]=true;f[18]=true;f[22]=true;const auto r=CalculateSmacCitizenFacilityInputs(3,f,1,true,true,true,true);GT_ASSERT(r.drone_adjustment==-6 && r.paradise_talents==2 && r.project_talents==3,"CITIZEN-FAC-023");}
{std::array<bool,34> f{};const auto r=CalculateSmacCitizenFacilityInputs(7,f,1,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-024");}
{std::array<bool,34> f{};f[22]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,0,false,false,false,false);GT_ASSERT(r.drone_adjustment==0 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-025");}
{std::array<bool,34> f{};f[22]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,-2,false,false,false,false);GT_ASSERT(r.drone_adjustment==-2 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-026");}
{std::array<bool,34> f{};f[22]=true;const auto r=CalculateSmacCitizenFacilityInputs(7,f,4,false,false,false,false);GT_ASSERT(r.drone_adjustment==4 && r.paradise_talents==0 && r.project_talents==0,"CITIZEN-FAC-027");}
return "";});
task->AddTest("SMAC citizen early ordered conversions match original stage snapshots",GT(){
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{0,6,0} && r.facilities==SmacCitizenCounts{0,6,0}),"CITIZEN-CONVERT-001");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,3,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{2,5,0} && r.psych==SmacCitizenCounts{2,5,0} && r.facilities==SmacCitizenCounts{2,5,0}),"CITIZEN-CONVERT-002");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,-3,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,7,0} && r.psych==SmacCitizenCounts{0,7,0} && r.facilities==SmacCitizenCounts{0,7,0}),"CITIZEN-CONVERT-003");}
{const auto r=ConvertSmacCitizens({7,0,14,0,0,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,7,0} && r.psych==SmacCitizenCounts{0,7,0} && r.facilities==SmacCitizenCounts{0,7,0}),"CITIZEN-CONVERT-004");}
{const auto r=ConvertSmacCitizens({7,0,14,3,0,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,7,3} && r.psych==SmacCitizenCounts{0,7,3} && r.facilities==SmacCitizenCounts{0,7,3}),"CITIZEN-CONVERT-005");}
{const auto r=ConvertSmacCitizens({7,0,14,3,1,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,7,2} && r.psych==SmacCitizenCounts{0,7,2} && r.facilities==SmacCitizenCounts{0,7,2}),"CITIZEN-CONVERT-006");}
{const auto r=ConvertSmacCitizens({7,0,14,3,5,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{1,6,0} && r.psych==SmacCitizenCounts{1,6,0} && r.facilities==SmacCitizenCounts{1,6,0}),"CITIZEN-CONVERT-007");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,1,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{0,6,0} && r.facilities==SmacCitizenCounts{0,6,0}),"CITIZEN-CONVERT-008");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,2,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{1,6,0} && r.facilities==SmacCitizenCounts{1,6,0}),"CITIZEN-CONVERT-009");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,3,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{1,6,0} && r.facilities==SmacCitizenCounts{1,6,0}),"CITIZEN-CONVERT-010");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,100,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{4,3,0} && r.facilities==SmacCitizenCounts{4,3,0}),"CITIZEN-CONVERT-011");}
{const auto r=ConvertSmacCitizens({7,0,6,0,3,0,-3,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{2,5,0} && r.psych==SmacCitizenCounts{1,5,0} && r.facilities==SmacCitizenCounts{1,5,0}),"CITIZEN-CONVERT-012");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,0,-4,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{0,6,0} && r.facilities==SmacCitizenCounts{0,2,0}),"CITIZEN-CONVERT-013");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,0,8,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{0,6,0} && r.facilities==SmacCitizenCounts{0,7,0}),"CITIZEN-CONVERT-014");}
{const auto r=ConvertSmacCitizens({7,0,6,0,0,0,4,0,2});GT_ASSERT((r.initial==SmacCitizenCounts{0,6,0} && r.psych==SmacCitizenCounts{1,5,0} && r.facilities==SmacCitizenCounts{2,4,0}),"CITIZEN-CONVERT-015");}
{const auto r=ConvertSmacCitizens({7,0,7,3,0,0,0,-4,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,7,3} && r.psych==SmacCitizenCounts{0,7,3} && r.facilities==SmacCitizenCounts{0,3,3}),"CITIZEN-CONVERT-016");}
{const auto r=ConvertSmacCitizens({7,3,6,0,2,0,0,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,4,0} && r.psych==SmacCitizenCounts{0,4,0} && r.facilities==SmacCitizenCounts{0,4,0}),"CITIZEN-CONVERT-017");}
{const auto r=ConvertSmacCitizens({7,7,6,7,0,0,20,0,2});GT_ASSERT((r.initial==SmacCitizenCounts{0,0,0} && r.psych==SmacCitizenCounts{0,0,0} && r.facilities==SmacCitizenCounts{0,0,0}),"CITIZEN-CONVERT-018");}
{const auto r=ConvertSmacCitizens({1,0,2,1,0,0,2,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,1,1} && r.psych==SmacCitizenCounts{0,1,0} && r.facilities==SmacCitizenCounts{0,1,0}),"CITIZEN-CONVERT-019");}
{const auto r=ConvertSmacCitizens({127,0,127,127,127,0,254,0,0});GT_ASSERT((r.initial==SmacCitizenCounts{0,127,0} && r.psych==SmacCitizenCounts{63,63,0} && r.facilities==SmacCitizenCounts{63,63,0}),"CITIZEN-CONVERT-020");}
return "";});
task->AddTest("SMAC citizen police priority and pacifism match original snapshots",GT(){
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-001");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-3,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-002");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-2,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-003");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-1,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,5,0} && r.pacifism==SmacCitizenCounts{0,5,0} && r.normalized==SmacCitizenCounts{0,5,0}),"CITIZEN-POLICE-004");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,5,0} && r.pacifism==SmacCitizenCounts{0,5,0} && r.normalized==SmacCitizenCounts{0,5,0}),"CITIZEN-POLICE-005");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,1,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,4,0} && r.pacifism==SmacCitizenCounts{0,4,0} && r.normalized==SmacCitizenCounts{0,4,0}),"CITIZEN-POLICE-006");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,2,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,3,0} && r.pacifism==SmacCitizenCounts{0,3,0} && r.normalized==SmacCitizenCounts{0,3,0}),"CITIZEN-POLICE-007");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,3,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,0,0} && r.pacifism==SmacCitizenCounts{0,0,0} && r.normalized==SmacCitizenCounts{0,0,0}),"CITIZEN-POLICE-008");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,4,0,false,{{3,false},{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,0,0} && r.pacifism==SmacCitizenCounts{0,0,0} && r.normalized==SmacCitizenCounts{0,0,0}),"CITIZEN-POLICE-009");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,4,0} && r.pacifism==SmacCitizenCounts{0,4,0} && r.normalized==SmacCitizenCounts{0,4,0}),"CITIZEN-POLICE-010");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,3,0,false,{{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,3,0} && r.pacifism==SmacCitizenCounts{0,3,0} && r.normalized==SmacCitizenCounts{0,3,0}),"CITIZEN-POLICE-011");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{3,false},{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,4,0} && r.pacifism==SmacCitizenCounts{0,4,0} && r.normalized==SmacCitizenCounts{0,4,0}),"CITIZEN-POLICE-012");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{4,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-013");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,2,0,false,{{8,false},{9,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-014");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-015");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,5,0} && r.pacifism==SmacCitizenCounts{0,5,0} && r.normalized==SmacCitizenCounts{0,5,0}),"CITIZEN-POLICE-016");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,5,0} && r.pacifism==SmacCitizenCounts{0,5,0} && r.normalized==SmacCitizenCounts{0,5,0}),"CITIZEN-POLICE-017");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,true,{{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,5,0} && r.pacifism==SmacCitizenCounts{0,5,0} && r.normalized==SmacCitizenCounts{0,5,0}),"CITIZEN-POLICE-018");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-2,0,true,{{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-019");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,3,0,true,{{3,true},{3,true},{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,0,0} && r.pacifism==SmacCitizenCounts{0,0,0} && r.normalized==SmacCitizenCounts{0,0,0}),"CITIZEN-POLICE-020");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,0,0,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-021");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-3,1,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-022");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-3,2,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,7,0} && r.normalized==SmacCitizenCounts{0,7,0}),"CITIZEN-POLICE-023");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-4,2,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,8,0} && r.normalized==SmacCitizenCounts{0,7,0}),"CITIZEN-POLICE-024");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-5,2,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,10,0} && r.normalized==SmacCitizenCounts{0,7,0}),"CITIZEN-POLICE-025");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-6,2,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,10,0} && r.normalized==SmacCitizenCounts{0,7,0}),"CITIZEN-POLICE-026");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,6,0},7,0,-2,4,false,{});GT_ASSERT((r.garrison==SmacCitizenCounts{0,6,0} && r.pacifism==SmacCitizenCounts{0,6,0} && r.normalized==SmacCitizenCounts{0,6,0}),"CITIZEN-POLICE-027");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,7,2},7,0,2,0,false,{{3,false},{3,false},{3,false}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,4,2} && r.pacifism==SmacCitizenCounts{0,4,2} && r.normalized==SmacCitizenCounts{0,4,2}),"CITIZEN-POLICE-028");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{3,4,0},7,0,1,0,false,{{3,false},{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{3,1,0} && r.pacifism==SmacCitizenCounts{3,1,0} && r.normalized==SmacCitizenCounts{3,1,0}),"CITIZEN-POLICE-029");}
{const auto r=PoliceSmacCitizens(SmacCitizenCounts{0,0,0},7,7,3,0,false,{{3,true},{3,true},{3,true}});GT_ASSERT((r.garrison==SmacCitizenCounts{0,0,0} && r.pacifism==SmacCitizenCounts{0,0,0} && r.normalized==SmacCitizenCounts{0,0,0}),"CITIZEN-POLICE-030");}
return "";});
task->AddTest("SMAC citizen late projects and suppression preserve original ordering",GT(){
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{1,5,0} && r.final==SmacCitizenCounts{1,5,0}),"CITIZEN-FINAL-001");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,1,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{2,5,0} && r.suppressed==SmacCitizenCounts{2,5,0} && r.final==SmacCitizenCounts{2,5,0}),"CITIZEN-FINAL-002");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,2,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{3,5,0} && r.suppressed==SmacCitizenCounts{3,5,0} && r.final==SmacCitizenCounts{2,4,0}),"CITIZEN-FINAL-003");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,3,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{4,5,0} && r.suppressed==SmacCitizenCounts{4,5,0} && r.final==SmacCitizenCounts{3,4,0}),"CITIZEN-FINAL-004");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{1,5,0} && r.final==SmacCitizenCounts{1,5,0}),"CITIZEN-FINAL-005");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,4,0} && r.suppressed==SmacCitizenCounts{1,4,0} && r.final==SmacCitizenCounts{1,4,0}),"CITIZEN-FINAL-006");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,1,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{1,5,0} && r.final==SmacCitizenCounts{1,5,0}),"CITIZEN-FINAL-007");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,2,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,3,0} && r.suppressed==SmacCitizenCounts{1,3,0} && r.final==SmacCitizenCounts{1,3,0}),"CITIZEN-FINAL-008");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,3,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,4,0} && r.suppressed==SmacCitizenCounts{1,4,0} && r.final==SmacCitizenCounts{1,4,0}),"CITIZEN-FINAL-009");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,-1,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,4,0} && r.suppressed==SmacCitizenCounts{1,4,0} && r.final==SmacCitizenCounts{1,4,0}),"CITIZEN-FINAL-010");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,true,4,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,4,0} && r.suppressed==SmacCitizenCounts{1,4,0} && r.final==SmacCitizenCounts{1,4,0}),"CITIZEN-FINAL-011");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,0,0},7,0,0,true,2,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,-2,0} && r.suppressed==SmacCitizenCounts{1,-2,0} && r.final==SmacCitizenCounts{1,0,0}),"CITIZEN-FINAL-012");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{0,7,3},7,0,2,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{2,7,3} && r.suppressed==SmacCitizenCounts{2,7,3} && r.final==SmacCitizenCounts{0,7,1}),"CITIZEN-FINAL-013");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,3,false,0,true,0);GT_ASSERT((r.projects==SmacCitizenCounts{4,5,0} && r.suppressed==SmacCitizenCounts{0,0,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-014");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,false,1);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{0,0,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-015");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,false,128);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{0,0,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-016");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,false,255);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{0,0,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-017");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,0,0,false,0,true,10);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,0} && r.suppressed==SmacCitizenCounts{0,0,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-018");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,3},7,0,0,false,0,true,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,3} && r.suppressed==SmacCitizenCounts{0,0,3} && r.final==SmacCitizenCounts{0,0,3}),"CITIZEN-FINAL-019");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,3},7,7,0,false,0,true,0);GT_ASSERT((r.projects==SmacCitizenCounts{1,5,3} && r.suppressed==SmacCitizenCounts{0,0,3} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-020");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,5,0},7,7,2,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{3,5,0} && r.suppressed==SmacCitizenCounts{3,5,0} && r.final==SmacCitizenCounts{0,0,0}),"CITIZEN-FINAL-021");}
{const auto r=FinishSmacCitizens(SmacCitizenCounts{1,0,0},1,0,3,false,0,false,0);GT_ASSERT((r.projects==SmacCitizenCounts{4,0,0} && r.suppressed==SmacCitizenCounts{4,0,0} && r.final==SmacCitizenCounts{1,0,0}),"CITIZEN-FINAL-022");}
return "";});
task->AddTest("SMAC citizen Golden Age and riot status match original caller controls",GT(){
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{4,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-001");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{4,0,0},true,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-002");}
{const auto r=CalculateSmacCitizenStatus(2,2,SmacCitizenCounts{2,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-003");}
{const auto r=CalculateSmacCitizenStatus(2,3,SmacCitizenCounts{1,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-004");}
{const auto r=CalculateSmacCitizenStatus(2,3,SmacCitizenCounts{2,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-005");}
{const auto r=CalculateSmacCitizenStatus(2,4,SmacCitizenCounts{2,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-006");}
{const auto r=CalculateSmacCitizenStatus(2,5,SmacCitizenCounts{2,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-007");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{4,1,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-008");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{2,0,0},true,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==true && r.riots_over==false),"CITIZEN-STATUS-009");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{1,3,0},true,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==true && r.riot_dispatch==true && r.map_refresh==false && r.riot_new==true && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-010");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{2,2,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-011");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{2,3,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==true && r.riot_dispatch==true && r.map_refresh==false && r.riot_new==true && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-012");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,true,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==true && r.riot_dispatch==true && r.map_refresh==false && r.riot_new==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-013");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{2,2,0},false,true,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==true && r.golden_start==false && r.golden_over==false && r.riots_over==true),"CITIZEN-STATUS-014");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{4,0,0},false,true,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==true && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-015");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,false,true,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-016");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==true && r.riot_dispatch==true && r.map_refresh==false && r.riot_new==true && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-017");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,true,true,0);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==true && r.golden_start==false && r.golden_over==false && r.riots_over==true),"CITIZEN-STATUS-018");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,false,false,1);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-019");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{0,3,0},false,false,false,128);GT_ASSERT((r.admitted==true && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-020");}
{const auto r=CalculateSmacCitizenStatus(2,7,SmacCitizenCounts{4,0,3},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-021");}
{const auto r=CalculateSmacCitizenStatus(0,7,SmacCitizenCounts{4,0,0},false,false,false,0);GT_ASSERT((r.admitted==false && r.golden==false && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-022");}
{const auto r=CalculateSmacCitizenStatus(1,7,SmacCitizenCounts{4,0,0},false,true,false,0);GT_ASSERT((r.admitted==false && r.golden==false && r.riots==true && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==false && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-023");}
{const auto r=CalculateSmacCitizenStatus(2,127,SmacCitizenCounts{64,0,0},false,false,false,0);GT_ASSERT((r.admitted==true && r.golden==true && r.riots==false && r.riot_dispatch==false && r.map_refresh==false && r.golden_start==true && r.golden_over==false && r.riots_over==false),"CITIZEN-STATUS-024");}
return "";});
}
}
