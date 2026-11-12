#include "FiftyTurnCampaign.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/base/BaseManager.h"
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
            for(int turn=0;turn<50;++turn) {
                auto error=settle();GT_ASSERT(error.empty(),"Research: "+error);
                const auto before=game->GetTurnId();
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
                error=settle();GT_ASSERT(error.empty(),"Research: "+error);
                GT_ASSERT(game->GetTurnId()==before+1,"Human completion did not return through all computer turns at step "+std::to_string(turn));
                bool ready=false;fixture.ResourceState([&](const auto& state){ready=state.faction_turn
                    && state.faction_turn->Current().phase==game::backend::faction::SmacFactionTurn::Phase::OwnerCommand
                    && state.faction_turn->Current().owner==1;});
                GT_ASSERT(ready,"Campaign lost its human command boundary");
                error=fixture.CheckLocalComputerBoundary();GT_ASSERT(error.empty(),"Owner boundary: "+error);
            }
            GT_ASSERT(game->GetBM()->GetBases().size()>=7 && game->GetUM()->GetUnits().size()>=7,"Computer campaign lost its live world");
            GT_OK();
        });
    });
}
}
