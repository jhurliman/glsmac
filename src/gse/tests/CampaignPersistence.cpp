#include "CampaignPersistence.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/CampaignFile.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/unit/MoraleSet.h"
#include "util/FinallyGuard.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace gse::tests {
namespace {
bool Rejects(const std::function<void()>& call) {try{call();}catch(const std::exception&){return true;}return false;}
std::string Temporary() {return (std::filesystem::temp_directory_path()/("glsmac-campaign-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".glsmac")).string();}
}
void AddCampaignPersistenceTests(task::gsetests::GSETests* task) {
    task->AddTest("Campaign file restores owned settings roster world and next actual turn",GT(){
        using namespace game::backend;using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
        const auto path=Temporary();util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);});
        std::string snapshot;
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* tile=game->GetMap()->GetTile(4,4);
            for(auto* corner:tile->elevation.corners){*corner=100;}tile->Update();
            MoraleSet::morale_values_t names;for(int i=0;i<7;++i){names.emplace_back("morale"+std::to_string(i));}
            auto* um=game->GetUM();um->DefineMoraleSet(new MoraleSet("campaign",names));
            auto* def=new StaticDef("campaign-scout",um->GetMoraleSet("campaign"),"Scout",MT_LAND,1,
                new CvrScoutRender(2,0,1,1,{1,1},{0,0},CvrScoutRender::Composition::InitializedScout));um->DefineUnit(def);
            ConventionalUnitState state{2,1,1,0,1,true,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
            um->SpawnConventionalUnit(GSE_CALL,71,def,&game->GetState()->m_slots->GetSlot(0),tile,state);
            snapshot=fixture.Snapshot();
            const auto saved=game->SaveCampaign(path);GT_ASSERT(std::filesystem::is_regular_file(saved),"campaign file was not written");
            GT_ASSERT(campaign::Read(path).world==snapshot,"file lost world bytes");
            // Replacing an existing file is the same atomic publication path.
            game->SaveCampaign(path);GT_ASSERT(campaign::Read(path).world==snapshot,"replacement changed snapshot");GT_OK();
        });if(!setup.empty()){return setup;}
        return fixture.CheckCampaignRestore(path,snapshot);
    });
    task->AddTest("Campaign malformed files and unstable saves preserve current world and existing file",GT(){
        using namespace game::backend;
        ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
        const auto path=Temporary();util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);});
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();const auto snapshot=fixture.Snapshot();game->SaveCampaign(path);
            const auto saved=campaign::Read(path);const auto encoded=campaign::Encode(saved);
            auto truncated=encoded;truncated.pop_back();
            const auto bad_path=Temporary();util::FinallyGuard remove_bad([&]{std::error_code ignored;std::filesystem::remove(bad_path,ignored);});
            {std::ofstream output(bad_path,std::ios::binary);output.write(truncated.data(),static_cast<std::streamsize>(truncated.size()));}
            const auto failure=fixture.CheckCampaignRejectedLoad(GSE_CALL,bad_path);if(!failure.empty()){return failure;}
            GT_ASSERT(Rejects([&]{campaign::Decode(truncated);})&&Rejects([&]{campaign::Decode(encoded+"x");}),"truncated or trailing file admitted");
            auto invalid=saved;invalid.local_slot=1;GT_ASSERT(Rejects([&]{campaign::Write(path,invalid);}),"invalid replacement admitted");
            GT_ASSERT(campaign::Read(path).world==snapshot,"failed replacement damaged existing file");
            game->Event(GSE_CALL,"complete_turn",{});
            GT_ASSERT(Rejects([&]{game->SaveCampaign(path);}),"save omitted queued gameplay command");
            GT_ASSERT(campaign::Read(path).world==snapshot&&fixture.Snapshot()==snapshot,"rejected save changed file or current world");
            fixture.DrainEvents();GT_OK();
        });
    });
}
}
