#include "CampaignPersistence.h"
#include "ConventionalUnits.h"
#include "fixtures/PrimaryViewCursor.h"
#include "game/frontend/NativeViewContext.h"
#include <limits>
#include "task/gsetests/GSETests.h"
#include "game/backend/CampaignFile.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacPlanetMap.h"
#include "types/Buffer.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/unit/ColonyMarkerRender.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
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
    task->AddTest("Campaign cursor codec preserves legacy absence signed fields and original reopen policy",GT(){
        return ConventionalUnitFixture::WithControllerViewport([]()->std::string {
            using namespace game::backend;
            ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
            const auto path=Temporary();util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);});
            return fixture.Run([&](GSE_CALLABLE)->std::string {
                fixture.SaveCampaign(path);const auto legacy=campaign::Read(path);const auto old_bytes=campaign::Encode(legacy);
                GT_ASSERT(!legacy.cursor && campaign::Encode(campaign::Decode(old_bytes))==old_bytes,"Version-one campaign invented cursor state or changed bytes");
                for(const auto& control:fixtures::primary_cursor::Cases) {
                    auto record=legacy;record.cursor=control.cursor;const auto encoded=campaign::Encode(record);const auto loaded=campaign::Decode(encoded);
                    GT_ASSERT(loaded.cursor==record.cursor && loaded.world==legacy.world && loaded.settings==legacy.settings,
                        "Cursor round trip normalized its fields or rewrote simulation bytes");
                    game::frontend::native_terrain::ViewContext context;
                    context.index=7;context.active=0;context.flags=0x12345678;context.zoom=8;context.camera={{70,20}};
                    const auto preferences=context.preferences;context.Reopen(*loaded.cursor,{{80,80}});
                    const std::array<int,6> actual{{context.index,context.active,int(context.flags),context.zoom,context.camera[0],context.camera[1]}};
                    GT_ASSERT(actual==control.view && context.preferences==preferences,"Original primary reopen differs: "+std::string(control.id));
                    GT_ASSERT(Rejects([&]{campaign::Decode(encoded.substr(0,encoded.size()-1));}) && Rejects([&]{campaign::Decode(encoded+"x");}),
                        "Truncated or trailing version-two cursor data admitted");
                }
                auto edge=legacy;edge.cursor=game::ui_view::Cursor{{INT32_MIN,INT32_MAX}};
                GT_ASSERT(campaign::Decode(campaign::Encode(edge)).cursor==edge.cursor,"Signed cursor endpoints were normalized or narrowed");
                types::Buffer input(old_bytes);const auto magic=input.ReadString();input.ReadInt();const auto local=input.ReadInt();
                const auto settings=input.ReadString(),roster=input.ReadString(),world=input.ReadString();
                types::Buffer malformed;malformed.WriteString(magic);malformed.WriteInt(2);malformed.WriteInt(local);
                malformed.WriteString(settings);malformed.WriteString(roster);malformed.WriteString(world);
                malformed.WriteInt(std::numeric_limits<int64_t>::max());malformed.WriteInt(0);
                GT_ASSERT(Rejects([&]{campaign::Decode(malformed.ToString());}),"Wide cursor integer silently narrowed");GT_OK();
            });
        });
    });

    task->AddTest("Campaign file restores owned settings roster world and next actual turn",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]() -> std::string {
        return ConventionalUnitFixture::WithCampaignMapSources([&]() -> std::string {
        using namespace game::backend;using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
        const auto path=Temporary();util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);});
        std::string snapshot;
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* tile=game->GetMap()->GetTile(4,4);
            for(auto* corner:tile->elevation.corners){*corner=100;}
            for(size_t y=0;y<game->GetMap()->GetHeight();++y){for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2){game->GetMap()->GetTile(x,y)->Update();}}
            MoraleSet::morale_values_t names;for(int i=0;i<7;++i){names.emplace_back("morale"+std::to_string(i));}
            auto* um=game->GetUM();um->DefineMoraleSet(new MoraleSet("campaign",names));
            auto* def=new StaticDef("campaign-scout",um->GetMoraleSet("campaign"),"Scout",MT_LAND,1,
                new CvrScoutRender(2,0,1,1,{1,1},{0,0},CvrScoutRender::Composition::InitializedScout));um->DefineUnit(def);
            ConventionalUnitState state{2,1,1,0,1,true,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
            um->SpawnConventionalUnit(GSE_CALL,71,def,&game->GetState()->m_slots->GetSlot(0),tile,state);
            auto* colony=new StaticDef("campaign-pod",um->GetMoraleSet("campaign"),"Colony Pod",MT_LAND,1,new ColonyMarkerRender());um->DefineUnit(colony);
            um->SpawnConventionalUnit(GSE_CALL,72,colony,&game->GetState()->m_slots->GetSlot(0),tile,
                {0,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
            std::array<std::string,8> roles;roles[0]="campaign-worker";
            game->GetBM()->DefinePop(new base::PopDef(roles[0],"Worker",{},{},base::PopDef::PF_TILE_WORKER));
            for(uint8_t i=0;i<7;++i){roles[i+1]="campaign-specialist"+std::to_string(i);game->GetBM()->DefinePop(new base::PopDef(roles[i+1],"Specialist",{},{},base::PopDef::PF_NONE,i));}
            fixture.BasePresentationRoles(roles);
            map::SmacWorkerKnowledge::Data knowledge{int32_t(game->GetMap()->GetWidth()),int32_t(game->GetMap()->GetHeight()),true,true,0,{},{},{}};
            knowledge.tiles.resize(knowledge.width*knowledge.height/2,{2,std::nullopt});knowledge.observers[1]=map::SmacWorkerKnowledge::Observer{};knowledge.observers[1]->pact_with=0;
            knowledge.units={{71,0,false},{72,0,false}};
            knowledge.remembered[1].resize(knowledge.tiles.size(),map::SmacWorkerKnowledge::Improvements{0,0,false,false});
            game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(knowledge));
            snapshot=fixture.Snapshot();
            const auto saved=fixture.SaveCampaign(path);GT_ASSERT(std::filesystem::is_regular_file(saved),"campaign file was not written");
            GT_ASSERT(campaign::Read(path).world==snapshot,"file lost world bytes");
            // Replacing an existing file is the same atomic publication path.
            fixture.SaveCampaign(path);GT_ASSERT(campaign::Read(path).world==snapshot,"replacement changed snapshot");GT_OK();
        });if(!setup.empty()){return setup;}
        std::string first_decode,second_decode;
        fixture.Game()->GetRandom()->SetSeed(1);
        const auto first=fixture.CheckCampaignRestore(path,snapshot,&first_decode,false);if(!first.empty()){return first;}
        fixture.Game()->GetRandom()->SetSeed(987654321);
        const auto restored=fixture.CheckCampaignRestore(path,snapshot,&second_decode);if(!restored.empty()){return restored;}
        GT_ASSERT(first_decode==second_decode,"Saved map construction depends on the new process random seed");
        return fixture.CheckCampaignCallbackFailure(path);
        });
        });
    });
    task->AddTest("Campaign save waits for extracted events and retires old world completion",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]() -> std::string {
            ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
            const auto path=Temporary(),cancelled=Temporary();
            util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);std::filesystem::remove(cancelled,ignored);});
            return fixture.CheckCampaignSaveOwnership(path,cancelled);
        });
    });
    task->AddTest("Campaign cancellation retires frontend resources before initialization",GT(){
        return ConventionalUnitFixture::WithControllerViewport([&]() -> std::string {
            ConventionalUnitFixture fixture;fixture.Configure(true,3,true);
            return fixture.CheckCampaignFrontendCancellation();
        });
    });
    task->AddTest("Campaign malformed files and unstable saves preserve current world and existing file",GT(){
        using namespace game::backend;
        ConventionalUnitFixture fixture;fixture.Configure(true,3,true);fixture.ResourcePolicy(true);
        const auto path=Temporary();util::FinallyGuard cleanup([&]{std::error_code ignored;std::filesystem::remove(path,ignored);});
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();const auto snapshot=fixture.Snapshot();fixture.SaveCampaign(path);
            const auto saved=campaign::Read(path);const auto encoded=campaign::Encode(saved);
            auto truncated=encoded;truncated.pop_back();
            const auto bad_path=Temporary();util::FinallyGuard remove_bad([&]{std::error_code ignored;std::filesystem::remove(bad_path,ignored);});
            {std::ofstream output(bad_path,std::ios::binary);output.write(truncated.data(),static_cast<std::streamsize>(truncated.size()));}
            const auto failure=fixture.CheckCampaignRejectedLoad(GSE_CALL,bad_path);if(!failure.empty()){return failure;}
            // A checksummed policy envelope can still contradict accepted map
            // provenance. Reject through the real menu loader before replacing
            // its State/session, rather than during later backend restore.
            using Planet=map::SmacPlanetMap;using Policy=map::GlsmacResourceMapPolicy;
            std::string planet(Planet::FILE_BYTES,'\0');planet.replace(0,11,"TERRANMAP\0\x1a",11);
            const auto put=[&](size_t at,uint32_t value){for(size_t i=0;i<4;++i){planet.at(at+i)=char(value>>(i*8));}};
            put(11,5);put(15,80);put(19,80);put(15+0xa30,4);
            types::Buffer old_policy,new_policy;
            Policy::WriteOptional(old_policy,Policy::DisplayedTerrainV1(),true);
            Policy::WriteOptional(new_policy,Policy::InstalledPlanetV1(Planet::Decode(planet),false),true);
            auto forged=saved;const auto field=old_policy.ToString();const auto offset=forged.world.find(field);
            GT_ASSERT(offset!=std::string::npos && forged.world.find(field,offset+1)==std::string::npos,"policy fixture field is not unique");
            forged.world.replace(offset,field.size(),new_policy.ToString());campaign::Write(bad_path,forged);
            const auto provenance_failure=fixture.CheckCampaignRejectedLoad(GSE_CALL,bad_path);
            if(!provenance_failure.empty()){return provenance_failure;}
            GT_ASSERT(Rejects([&]{campaign::Decode(truncated);})&&Rejects([&]{campaign::Decode(encoded+"x");}),"truncated or trailing file admitted");
            auto invalid=saved;invalid.local_slot=1;GT_ASSERT(Rejects([&]{campaign::Write(path,invalid);}),"invalid replacement admitted");
            GT_ASSERT(campaign::Read(path).world==snapshot,"failed replacement damaged existing file");
            fixture.QueueProbeEvent(GSE_CALL,[]{});
            GT_ASSERT(Rejects([&]{fixture.SaveCampaign(path);}),"save omitted queued gameplay command");
            GT_ASSERT(campaign::Read(path).world==snapshot&&fixture.Snapshot()==snapshot,"rejected save changed file or current world");
            fixture.DrainEvents();
            fixture.SaveCampaign(path);GT_ASSERT(campaign::Read(path).world==snapshot,"rejected save retired the campaign or prevented retry");GT_OK();
        });
    });
}
}
