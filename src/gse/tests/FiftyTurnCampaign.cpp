#include "FiftyTurnCampaign.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/Base.h"
#include "game/backend/unit/UnitManager.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <map>

namespace gse::tests {
void AddFiftyTurnCampaignTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC milestone ordinary seven-faction campaign advances fifty human turns",GT() {
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::map<std::string,std::string> sources;
        const auto read=[&](const std::string& name)->std::string {
            if(sources.count(name)) {return sources.at(name);}
            for(const auto& file:std::filesystem::directory_iterator(assets)) {
                auto key=file.path().filename().string();std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return char(std::tolower(c));});
                if(key==name) {std::ifstream in(file.path(),std::ios::binary);return sources[name]=std::string(std::istreambuf_iterator<char>(in),{});}
            }
            throw std::runtime_error("Missing installed milestone source: "+name);
        };
        const auto path=std::filesystem::path(assets)/"maps"/"planet.MP";
        std::ifstream planet(path,std::ios::binary);GT_ASSERT(planet.good(),"Installed Planet map is unavailable");
        const std::string bytes{std::istreambuf_iterator<char>(planet),{}};
        ConventionalUnitFixture fixture;const auto startup=fixture.StartLocalComputerCampaign(read,bytes);
        GT_ASSERT(startup.empty(),"Ordinary campaign startup: "+startup);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();
            std::cout<<fixture.CampaignSeedReport()<<std::endl;
            const auto settle=[&]()->std::string {
                for(size_t decision=0;decision<128;++decision) {
                    auto* wrapped=static_cast<value::Object*>(game->Wrap(GSE_CALL));
                    auto* pending=static_cast<value::Callable*>(wrapped->Get("get_pending_smac_research"))->Run(GSE_CALL,{});
                    if(pending->type==VT_NULL) {GT_OK();}
                    auto* request=static_cast<value::Object*>(pending);
                    const auto id=static_cast<value::Int*>(request->Get("request_id"))->value;
                    if(static_cast<value::Bool*>(request->Get("blind"))->value) {
                        game->Event(GSE_CALL,"set_research_priorities",{{"request_id",VALUE(value::Int,,id)},{"mask",VALUE(value::Int,,15)}});
                    } else {
                        const auto& choices=static_cast<value::Array*>(request->Get("choices"))->value;
                        GT_ASSERT(!choices.empty(),"Human research prompt has no offered choice");
                        auto* choice=static_cast<value::Object*>(choices.front());
                        game->Event(GSE_CALL,"choose_research",{{"request_id",VALUE(value::Int,,id)},{"technology_id",choice->Get("id")}});
                    }
                    fixture.DrainEvents();
                }
                GT_FAIL("Human research continuation did not settle");
            };
            auto initial_error=settle();GT_ASSERT(initial_error.empty(),"Initial research: "+initial_error);
            const auto initial_progress=fixture.CampaignProgress();
            const auto report=[&]() {
                const auto actions=fixture.CampaignActions();
                std::cout<<"Campaign turn "<<game->GetTurnId()<<": production="<<actions.production_choices
                    <<" moves="<<actions.moves<<" work="<<actions.work_dispatches<<" completed="<<actions.terrain_completions
                    <<" founded="<<actions.founded_bases<<std::endl;
                for(const auto& row:fixture.CampaignProgress()) {
                    if(!row.human && game->GetTurnId()<51) {continue;}
                    std::cout<<"  owner="<<row.owner<<" bases="<<row.bases<<" units="<<row.units
                        <<" technologies="<<row.technologies<<" labs="<<row.labs<<" improved="<<row.improved_tiles<<std::endl;
                }
                for(const auto& entry:game->GetBM()->GetBases()) {
                    const auto* base=entry.second;if(base->GetSmacResourceContext().owner_simulation_id!=1) {continue;}
                    std::cout<<"  human Base "<<entry.first<<" population="<<base->GetPopulation()
                        <<" target="<<base->GetProductionState()->current_item.smac_source_id.value_or(-999)
                        <<" minerals="<<base->GetProductionState()->minerals<<std::endl;
                }
            };
            report();
            for(int turn=0;turn<50;++turn) {
                try {
                auto error=settle();GT_ASSERT(error.empty(),"Research: "+error);
                error=fixture.DriveLocalHumanCampaign(GSE_CALL);GT_ASSERT(error.empty(),"Human commands: "+error);
                const auto before=game->GetTurnId();
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
                error=settle();GT_ASSERT(error.empty(),"Research: "+error);
                GT_ASSERT(game->GetTurnId()==before+1,"Human completion did not return through all computer turns at step "+std::to_string(turn));
                bool ready=false;fixture.ResourceState([&](const auto& state){ready=state.faction_turn
                    && state.faction_turn->Current().phase==game::backend::faction::SmacFactionTurn::Phase::OwnerCommand
                    && state.faction_turn->Current().owner==1;});
                GT_ASSERT(ready,"Campaign lost its human command boundary");
                error=fixture.CheckLocalComputerBoundary();GT_ASSERT(error.empty(),"Owner boundary: "+error);
                if((turn+1)%10==0) {report();}
                } catch(const std::exception& error) {
                    GT_FAIL("Campaign step "+std::to_string(turn)+", current turn "+std::to_string(game->GetTurnId())+": "+error.what());
                }
            }
            GT_ASSERT(game->GetBM()->GetBases().size()>=7 && game->GetUM()->GetUnits().size()>=7,"Computer campaign lost its live world");
            const auto actions=fixture.CampaignActions();
            GT_ASSERT(actions.production_choices && actions.moves && actions.work_dispatches && actions.terrain_completions && actions.founded_bases,
                "Human campaign must exercise production, movement, completed development and expansion");
            bool computer_expanded=false,computer_researched=false,computer_developed=false;
            for(const auto& after:fixture.CampaignProgress()) {
                if(after.human) {continue;}
                const auto before=std::find_if(initial_progress.begin(),initial_progress.end(),[&](const auto& row){return row.owner==after.owner;});
                GT_ASSERT(before!=initial_progress.end(),"Computer owner disappeared from progression inventory");
                computer_expanded|=after.bases>before->bases;
                computer_researched|=after.technologies>before->technologies;
                computer_developed|=after.improved_tiles>before->improved_tiles;
            }
            GT_ASSERT(computer_expanded && computer_researched && computer_developed,
                "Computer factions must actually expand, acquire knowledge and develop terrain");
            if(const auto* capture=std::getenv("GLSMAC_CAMPAIGN_CAPTURE");capture && *capture) {
                const auto saved=fixture.SaveCampaign(capture);
                GT_ASSERT(std::filesystem::is_regular_file(saved),"Campaign capture was not written");
                std::cout<<"Campaign checkpoint: "<<saved<<std::endl;
            }
            const auto replay=fixture.CheckLocalCampaignReplay(GSE_CALL,3);
            GT_ASSERT(replay.empty(),"Saved campaign continuation: "+replay);
            GT_OK();
        });
    });
}
}
