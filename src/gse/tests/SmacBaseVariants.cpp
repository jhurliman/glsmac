#include "SmacBaseVariants.h"
#include "game/SmacBaseBody.h"
#include "game/SmacDetailMinimap.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "task/gsetests/GSETests.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacGraphPalette.h"
#include "Mesh.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "gse/value/Object.h"
#include "gse/value/Array.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "util/UUID.h"
#include "util/FinallyGuard.h"
#include <filesystem>
#include <cstdlib>
namespace gse::tests {
namespace {
namespace body=game::smac_base_body;namespace detail=game::smac_detail;
struct BankCase {int column,row;detail::Point offset,size;uint64_t hash;};
struct VariantCase {const char* id;int population;detail::Point limits;bool hq,sea,perimeter,tachyon,cdf;body::Access access;int owner;bool main,filled;int column,row,text;std::array<int,3> layout;int border;size_t samples;uint64_t stream,frame;};
#include "fixtures/SmacBaseVariants.inc"
uint64_t Fnv(const std::vector<uint8_t>& bytes) {uint64_t h=UINT64_C(14695981039346656037);for(const auto byte:bytes) {h=(h^byte)*UINT64_C(1099511628211);}return h;}
}
void AddSmacBaseVariantTests(task::gsetests::GSETests* task) {
    task->AddTest("Detail Base85 native body selectors access styles and population layouts",GT(){
        for(const auto& c:VARIANTS) {
            const auto selected=body::Select(c.population,c.limits[0],c.limits[1],c.hq,c.sea,c.perimeter,c.tachyon,c.cdf);
            GT_ASSERT(selected.column==c.column && selected.row==c.row,"native body bank differs: "+std::string(c.id));
            GT_ASSERT(body::Filled(c.access)==c.filled,"native population access differs: "+std::string(c.id));
            const auto layout=body::PopulationLayout(c.population,c.main?9:5,c.main?12:7);
            GT_ASSERT(layout.width==c.layout[0] && layout.height==c.layout[1] && layout.baseline==c.layout[2],"native 8/88 population layout differs: "+std::string(c.id));
        }GT_OK();
    });
    task->AddTest("Detail Base actual owner snapshot binds HQ defense population and native palette identity",GT(){
        using namespace game::backend;
        for(const bool duplicate:{false,true}) {
        ConventionalUnitFixture fixture;
        if(duplicate) {fixture.ConfigureTwoResourceParticipants(smac_startup_fixture::OrdinarySource,true,80,80,"GAIANS");}
        else {fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"UNIVERSITY",80,80);}
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* bm=game->GetBM();auto* owner=&game->GetState()->m_slots->GetSlot(0);auto* faction=owner->GetPlayer()->GetFaction();
            faction->m_bases_render={"univ.pcx",1,1,100,75,50,37,1,1,1};
            auto* specialist=new base::PopDef("DETAIL_SPECIALIST","Specialist",{},{},base::PopDef::PF_NONE,0);bm->DefinePop(specialist);
            base::Base::pops_t pops;for(size_t id=1;id<=9;++id) {pops.emplace(id,base::Pop(nullptr,id,specialist));}
            base::Base::facilities_t facilities;
            for(const int id:{1,4,5}) {auto* def=new base::FacilityDef("DETAIL_FACILITY"+std::to_string(id),"Facility","fixture",1,{"fixture",0,0,1,1},id);bm->DefineFacility(def);if(id!=5) {facilities.emplace(def->m_id,def);}}
            auto* tile=game->GetMap()->GetTile(40,40);tile->is_water_tile=false;*tile->elevation.center=100;
            base::SmacBaseAllocationState allocation{1,{{},9}};allocation.specialists.types.fill(0);
            auto* base=new base::Base(game,801,owner,faction,tile,"Live HQ",facilities,pops,10,0,{},base::SmacBaseResourceContext{1,false,0,0,0,false},allocation);bm->SpawnBase(GSE_CALL,base);
            map::SmacWorkerKnowledge::Data knowledge{80,80,true,true,0,{},{},{}};knowledge.observers[1]=map::SmacWorkerKnowledge::Observer{};knowledge.observers[1]->pact_with=0;knowledge.tiles.resize(3200,{0,std::nullopt});knowledge.tiles[40*40+20].explored_by=2;knowledge.bases.push_back({801,2,{}});
            game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(knowledge));
            auto* query=static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("get_smac_detail_minimap"));
            const auto check=[&](int row)->std::string {
                const auto before=fixture.Snapshot();auto* frame=static_cast<value::Object*>(query->Run(GSE_CALL,{}));const auto& bases=static_cast<value::Array*>(frame->Get("bases"))->value;
                GT_ASSERT(bases.size()==1,"live supported Base omitted from Detail");auto* record=static_cast<value::Object*>(bases[0]);
                const auto integer=[&](const char* name){return static_cast<value::Int*>(record->Get(name))->value;};
                GT_ASSERT(integer("population")==9 && integer("column")==3 && integer("row")==row,"live Detail ignored actual population/HQ/defense/retained altitude");
                GT_ASSERT(integer("native_owner")== (duplicate?0:3) && static_cast<value::Bool*>(record->Get("style_known"))->value==!duplicate && !static_cast<value::Bool*>(record->Get("filled"))->value,"live Detail confused dense owner1 with native University3 or invented duplicate-profile palette identity");
                GT_ASSERT(fixture.Snapshot()==before,"Detail Base query mutated retained campaign");GT_OK();
            };
            auto error=check(1);if(!error.empty()) {return error;}
            base->m_facilities.emplace("DETAIL_FACILITY5",bm->GetFacilityDef("DETAIL_FACILITY5"));
            return check(2);
        });if(!result.empty()) {return result;}
        }GT_OK();
    });
    task->AddTest("Detail Base24 actual indexed banks and74 native pre-font compositions",GT(){
        const auto* assets=std::getenv("GLSMAC_TEST_DETAIL_ASSET_ROOT");if(!assets) {GT_OK();}
        namespace fs=std::filesystem;const auto directory=fs::temp_directory_path()/("glsmac-base-variants-"+util::UUID::Generate());const auto data=directory/"default";
        fs::create_directory(directory);fs::create_directory(data);
        util::FinallyGuard cleanup([&] {std::error_code ignored;fs::remove(data/"univ.pcx",ignored);fs::remove(data/"palette.pcx",ignored);fs::remove(data,ignored);fs::remove(directory,ignored);});
        fs::copy_file(fs::path(assets)/"UNIV.PCX",data/"univ.pcx");fs::copy_file(fs::path(assets)/"PALETTE.PCX",data/"palette.pcx");
        return WithWidgetTextureResources(directory.string(),[&]()->std::string {
            const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();const auto shadow=game::frontend::smac_developed_overlay::ShadowLookup(palette->colors);
            for(const auto& b:BANKS) {
                const auto source=g_engine->GetTextureLoader()->GetSmacDetailBaseSprite("univ.pcx",1+76*b.row,b.column);
                GT_ASSERT(source->offset==b.offset && source->width==b.size[0] && source->height==b.size[1] && Fnv(source->pixels)==b.hash,"native retained Base bank differs");
                GT_ASSERT(source==g_engine->GetTextureLoader()->GetSmacDetailBaseSprite("univ.pcx",1+76*b.row,b.column),"indexed Base bank cache missed");
            }
            size_t contexts=0,total=0;
            for(const auto& c:VARIANTS) {
                const auto style=body::Colors(c.filled,c.owner==5,palette->normal.at(c.owner),palette->alternate.at(c.owner),palette->text.at(c.owner));
                GT_ASSERT(style.text==c.text && style.border==c.border,"native population palette differs: "+std::string(c.id));
                if(c.main) {continue;}
                detail::World world;for(auto& tile:world.tiles) {tile.known=true;tile.supported=true;tile.band=c.sea?1:3;}
                auto frame=detail::Terrain(world,{40,40});const auto source=g_engine->GetTextureLoader()->GetSmacDetailBaseSprite("univ.pcx",1+76*c.row,c.column);
                std::vector<uint8_t> stream;size_t count=0;
                const auto word=[&](int v) {stream.push_back(v&255);stream.push_back((v>>8)&255);};
                detail::Composite(frame,{112,42},*source,shadow,[&](int x,int y,int sx,int sy,uint8_t raw) {word(x);word(y);word(sx);word(sy);stream.push_back(raw);++count;});
                detail::PopulationBox(frame,{112,42},body::PopulationLayout(c.population,5,7),style);
                GT_ASSERT(count==c.samples && Fnv(stream)==c.stream && Fnv(std::vector<uint8_t>(frame.pixels.begin(),frame.pixels.end()))==c.frame,"native body/shadow/box frame differs: "+std::string(c.id));
                ++contexts;total+=count;
            }GT_ASSERT(contexts==74 && total==120058,"native Base matrix coverage changed");GT_OK();
        });
    });
}
}
