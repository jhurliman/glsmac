#pragma once
#include "game/backend/faction/SmacDominance.h"
namespace gse::tests::dominance_fixture {
namespace f=game::backend::faction;
// council-vote-counts original SHA256 ca4edad23426385da540e23e56478b888c668f3ba23398ff9cc9b9d10b665d98
// potential-diplomatic-majority-selector original SHA256 289f7554f464a139572bb20a1f39804dbc8ebe9b03f8ee2a6844339b17ea4b6f
// leading-human-dominance-score-path original SHA256 f3987cf89763a31213e42286ddbfb4fd2a700c708c5a87a5ba928baf9bf69774
// leading-human-victory-pressure-shortcut original SHA256 bc03414db3eacad19af674081b291044af256ddc8a7342b1e5e424e4f50cb6be
struct Vote{const char* id;int owner;std::vector<f::SmacCouncilBase> bases;bool empath,clinical;std::vector<int32_t> modifiers;int expected;};
inline std::vector<Vote> Votes(){return {
{"COUNCIL-VOTES-001",1,{{1,3},{2,9}},false,false,{},3},
{"COUNCIL-VOTES-002",1,{{1,3},{1,4},{2,9}},false,false,{},7},
{"COUNCIL-VOTES-003",1,{{1,3},{2,9}},true,false,{},4},
{"COUNCIL-VOTES-004",1,{{1,1},{1,1},{2,9}},true,false,{},3},
{"COUNCIL-VOTES-005",1,{{1,1},{2,9}},true,false,{},1},
{"COUNCIL-VOTES-006",1,{{1,3},{2,9}},false,true,{},6},
{"COUNCIL-VOTES-007",1,{{1,3},{2,9}},true,true,{},8},
{"COUNCIL-VOTES-008",1,{{1,3},{2,9}},false,false,{},3},
{"COUNCIL-VOTES-009",1,{{1,3},{2,9}},false,false,{},3},
{"COUNCIL-VOTES-010",1,{{1,3},{2,9}},true,false,{},4},
{"COUNCIL-VOTES-011",1,{{1,3},{2,9}},false,false,{2},6},
{"COUNCIL-VOTES-012",1,{{1,3},{2,9}},true,true,{2},16},
{"COUNCIL-VOTES-013",1,{{1,3},{2,9}},false,false,{2,3},18},
{"COUNCIL-VOTES-014",1,{{1,3},{2,9}},false,false,{0},0},
{"COUNCIL-VOTES-015",1,{{1,3},{2,9}},false,false,{-1},3},
{"COUNCIL-VOTES-016",1,{{1,3},{2,9}},false,false,{2,-1,3},18},
{"COUNCIL-VOTES-017",1,{{1,3},{2,9}},false,false,{0,3},0},
{"COUNCIL-VOTES-018",1,{{1,3},{2,9}},false,false,{1},3},
{"COUNCIL-VOTES-019",1,{},false,false,{},0},
{"COUNCIL-VOTES-020",1,{{1,0},{2,9}},true,true,{},0},
{"COUNCIL-VOTES-021",1,{{0,50},{1,3},{2,9}},false,false,{},3},
{"COUNCIL-VOTES-022",7,{{7,3},{1,9}},true,false,{2},8},
{"COUNCIL-VOTES-023",1,{{1,-1},{2,9}},true,false,{},-1},
};}
struct Majority{const char* id;f::SmacVictoryContext context;int query,exclude,rule,expected;};
inline std::vector<Majority> Majorities(){std::vector<Majority> rows;
{Majority r{"MAJORITY-001",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-002",{},1,0,0,2};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,3}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-003",{},1,0,0,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,3}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,3}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-004",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-005",{},2,0,0,2};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[2][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-006",{},3,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[3][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-007",{},1,1,0,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,6}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-008",{},1,1,0,2};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-009",{},1,3,0,0};r.context.rules=4104;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,6}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.relations[1][3]=1;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-010",{},1,3,0,1};r.context.rules=4104;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,6}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.relations[3][1]=1;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-011",{},1,3,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,6}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.relations[1][3]=1;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-012",{},1,0,0,1};r.context.rules=4104;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.relations[1][3]=1;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-013",{},1,0,0,0};r.context.rules=0;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-014",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][39]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-015",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][52]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-016",{},1,0,0,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-017",{},3,0,0,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
r.context.technologies[2][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-018",{},1,0,0,2};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,3}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-019",{},1,0,1,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-020",{},1,0,2,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-021",{},0,0,1,0};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-022",{},4,0,0,4};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,0}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,0}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[4][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-023",{},4,4,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,0}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,0}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[4][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-024",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-025",{},1,0,0,1};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,3}},false,false,{2});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,4}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Majority r{"MAJORITY-026",{},1,0,0,2};r.context.rules=8;
r.context.votes[1]=f::SmacCouncilVotes(1,{{1,4}},false,false,{0});
r.context.votes[2]=f::SmacCouncilVotes(2,{{2,3}},false,false,{});
r.context.votes[3]=f::SmacCouncilVotes(3,{{3,0}},false,false,{});
r.context.votes[4]=f::SmacCouncilVotes(4,{{4,0}},false,false,{});
r.context.votes[5]=f::SmacCouncilVotes(5,{{5,0}},false,false,{});
r.context.votes[6]=f::SmacCouncilVotes(6,{{6,0}},false,false,{});
r.context.votes[7]=f::SmacCouncilVotes(7,{{7,0}},false,false,{});
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
return rows;}
struct Score{const char* id;f::SmacVictoryContext context;f::SmacDominanceQuery query;bool expected;};
inline std::vector<Score> Scores(){std::vector<Score> rows;
{Score r{"DOMINANCE-001",{},{1,1,6,2,0,9999,150,100},true};r.context.turn=251;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-002",{},{1,1,6,2,0,9999,149,100},false};r.context.turn=251;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-003",{},{1,1,6,2,0,9999,150,100},false};r.context.turn=251;r.context.humans=0;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-004",{},{1,2,6,2,0,9999,150,100},false};r.context.turn=251;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-005",{},{1,1,5,2,0,9999,150,100},false};r.context.turn=251;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-006",{},{1,1,6,2,0,9999,150,100},false};r.context.turn=250;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-007",{},{1,1,6,3,0,9999,150,100},false};r.context.turn=200;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-008",{},{1,1,6,3,0,9999,150,100},true};r.context.turn=201;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-009",{},{1,1,7,1,0,9999,150,100},false};r.context.turn=301;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-010",{},{1,1,7,1,1,9999,150,100},true};r.context.turn=301;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-011",{},{1,1,7,1,1,9999,150,100},false};r.context.turn=300;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-012",{},{1,1,5,2,0,9999,125,100},true};r.context.turn=101;r.context.humans=2;r.context.rules=66;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-013",{},{1,1,5,2,0,9999,124,100},false};r.context.turn=101;r.context.humans=2;r.context.rules=66;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-014",{},{1,1,5,2,0,9999,125,100},false};r.context.turn=100;r.context.humans=2;r.context.rules=66;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-015",{},{1,1,5,0,0,9999,125,100},true};r.context.turn=101;r.context.humans=2;r.context.rules=66;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-016",{},{1,1,6,2,0,9999,199,100},false};r.context.turn=251;r.context.humans=2;r.context.rules=0;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-017",{},{1,1,6,2,0,9999,200,100},true};r.context.turn=251;r.context.humans=2;r.context.rules=0;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-018",{},{1,1,6,2,0,1000,150,100},true};r.context.turn=251;r.context.humans=2;r.context.rules=0;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-019",{},{1,1,6,2,0,1001,150,100},false};r.context.turn=251;r.context.humans=2;r.context.rules=0;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-020",{},{1,1,6,3,0,9999,150,100},true};r.context.turn=251;r.context.humans=2;r.context.rules=0;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-021",{},{1,1,4,2,0,9999,150,100},false};r.context.turn=101;r.context.humans=2;r.context.rules=66;rows.push_back(std::move(r));}
{Score r{"DOMINANCE-022",{},{1,1,6,2,0,9999,0,0},true};r.context.turn=251;r.context.humans=2;r.context.rules=2;rows.push_back(std::move(r));}
return rows;}
struct Pressure{const char* id;f::SmacVictoryContext context;f::SmacDominanceQuery query;bool pressure,dominance;int majority;};
inline std::vector<Pressure> Pressures(){std::vector<Pressure> rows;
{Pressure r{"ordinary_no_pressure",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_1_False",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=0;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_1_True",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_2_False",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=252;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_2_True",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=6;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=252;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_7_False",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=252;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_7_True",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=130;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=252;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_-1",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=-1;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_0",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_250",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=250;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_251",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=251;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_252",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"deadline_10000",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=10000;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_with_no_cost",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_with_saved_cost",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_economic_rule_off",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6666;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_dead_roster_human",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"future_diplomatic_rule_off",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6662;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_0",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_1",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=1;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_2",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=2;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_3",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=3;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_100",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=100;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_101",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=101;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"cooperative_history_word_-1",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=-1;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"odd_history_coop_off",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=2574;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=1;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=1;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=1;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=1;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=1;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=1;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=1;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"majority_rival_1",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"majority_rival_6",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=6;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"majority_rival_7",{},{1,1,6,2,0,9999,100,100},true,false,2};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=7;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"leader_tech_missing",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
rows.push_back(std::move(r));}
{Pressure r{"voice_-1_None",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_-1_56",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_-1_57",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_0_None",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_0_56",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_0_57",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_6_None",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=6;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_6_56",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=6;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_6_57",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=6;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_9999_None",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=9999;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_9999_56",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=9999;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_9999_57",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=9999;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_other_human_technology",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=10;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[3][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_other_AI_technology",{},{1,1,6,2,0,9999,100,100},false,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[3][57]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_transcendence_rule_off",{},{1,1,6,2,0,9999,100,100},true,true,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"voice_no_majority_technology",{},{1,1,6,2,0,9999,100,100},true,false,0};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=0;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=0;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][56]=true;
rows.push_back(std::move(r));}
{Pressure r{"admission_exact_time",{},{1,1,6,2,0,9999,100,100},true,false,1};
r.context.turn=250;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
{Pressure r{"admission_insufficient_bases",{},{1,1,5,2,0,9999,100,100},true,false,1};
r.context.turn=251;r.context.humans=2;r.context.rules=6670;r.context.voice_host=-1;
r.context.votes[1]=6;r.context.votes[2]=1;
r.context.economic_deadlines[1]=252;r.context.last_base_expansion_or_advice_turn[1]=0;
r.context.economic_deadlines[2]=0;r.context.last_base_expansion_or_advice_turn[2]=0;
r.context.economic_deadlines[3]=0;r.context.last_base_expansion_or_advice_turn[3]=0;
r.context.economic_deadlines[4]=0;r.context.last_base_expansion_or_advice_turn[4]=0;
r.context.economic_deadlines[5]=0;r.context.last_base_expansion_or_advice_turn[5]=0;
r.context.economic_deadlines[6]=0;r.context.last_base_expansion_or_advice_turn[6]=0;
r.context.economic_deadlines[7]=0;r.context.last_base_expansion_or_advice_turn[7]=0;
r.context.technologies[1][41]=true;
rows.push_back(std::move(r));}
return rows;}
}
