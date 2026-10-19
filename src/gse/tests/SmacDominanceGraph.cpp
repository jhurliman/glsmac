#include "SmacDominanceGraph.h"
#include "game/backend/faction/SmacDominanceGraph.h"
#include "task/gsetests/GSETests.h"
#include "types/texture/SmacIndexedLine.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/faction/SmacRankingScore.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/Array.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "Mesh.h"
#include "loader/texture/SDL2.h"
#include "loader/texture/SmacGraphPalette.h"
#include "engine/Engine.h"
#include "gse/GSE.h"
#include "gse/context/GlobalContext.h"
#include "ui/UI.h"
#include "ui/dom/Root.h"
#include "ui/dom/Surface.h"
#include "scene/actor/Cache.h"
#include "gc/Space.h"
#include "scene/Scene.h"
#include "scene/actor/Mesh.h"
#include "types/texture/Texture.h"
#include "util/FinallyGuard.h"
#include "util/UUID.h"
#include <filesystem>
#include <fstream>


namespace gse::tests {
namespace {
using Graph=game::backend::faction::SmacDominanceGraph;
using History=game::backend::faction::SmacDominanceHistory;
struct GraphCase {
    const char* id;size_t turn;int multiplier;Graph::Rect rect;
    std::array<int32_t,8> scores;int step,divisor;size_t lines;uint64_t fnv;
};
#include "fixtures/SmacDominanceGraphCases.inc"
struct LineCase {std::array<int32_t,4> line,clip;int32_t color;bool backed;size_t stores;uint64_t store_hash,frame_hash;};
#include "fixtures/SmacIndexedLineCases.inc"
uint64_t Hash(const std::vector<Graph::Line>& lines) {
    uint64_t hash=UINT64_C(14695981039346656037);
    for(const auto& line:lines) {for(const int32_t field:{line.x1,line.y1,line.x2,line.y2,int32_t(line.color)}) {
        const auto word=static_cast<uint32_t>(field);
        for(int shift=0;shift<32;shift+=8) {hash=(hash^((word>>shift)&255))*UINT64_C(1099511628211);}
    }}
    return hash;
}
}
static std::string CheckGraphPaletteSurface() {
    namespace fs=std::filesystem;
    const auto directory=fs::temp_directory_path()/("glsmac-graph-"+util::UUID::Generate());
    const auto data=directory/"default",path=data/"palette.pcx";
    fs::create_directory(directory);fs::create_directory(data);
    util::FinallyGuard remove([&]{std::error_code ignored;fs::remove(path,ignored);fs::remove(data,ignored);fs::remove(directory,ignored);});
    std::vector<uint8_t> bytes(128,0),pixels(638*32,0);
    bytes[0]=10;bytes[1]=5;bytes[2]=1;bytes[3]=8;bytes[65]=1;
    const auto word=[&](size_t at,int value){bytes[at]=value&255;bytes[at+1]=(value>>8)&255;};
    word(8,637);word(10,31);word(66,638);word(68,1);
    const std::array<uint8_t,8> normal{249,241,252,255,251,65,102,235},alternate{1,32,187,209,85,79,109,186};
    for(size_t slot=0;slot<8;++slot) {pixels[23*638+580+8*slot]=uint8_t(normal[slot]-10);pixels[31*638+580+8*slot]=uint8_t(alternate[slot]-10);}
    for(const auto value:pixels) {if(value>=192) {bytes.push_back(193);}bytes.push_back(value);}
    bytes.push_back(12);
    for(size_t index=0;index<256;++index) {
        const std::array<uint8_t,3> rgb=index==231?std::array<uint8_t,3>{0,255,0}:index==69?std::array<uint8_t,3>{255,250,245}:std::array<uint8_t,3>{uint8_t(index),uint8_t(index/2),uint8_t(index/3)};
        bytes.insert(bytes.end(),rgb.begin(),rgb.end());
    }
    {std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!file) {throw std::runtime_error("Cannot write graph palette fixture");}}
    std::shared_ptr<const loader::texture::SmacGraphPalette> retained;
    const auto check=WithWidgetTextureResources(directory.string(),[&]()->std::string {
        retained=g_engine->GetTextureLoader()->GetSmacGraphPalette();
        GT_ASSERT(retained && !retained->text_available && retained==g_engine->GetTextureLoader()->GetSmacGraphPalette() && retained->normal==normal && retained->alternate==alternate,"Actual graph metadata producer/cache differs");
        GT_ASSERT(retained->colors[241]==(std::array<uint8_t,3>{0,235,0}) && retained->colors[79]==(std::array<uint8_t,3>{235,235,235})
            && retained->colors[255]==(std::array<uint8_t,3>{255,255,255}),"Graph display changed Gamma100 common/reserved colors");
        GSE runtime;auto* gc_space=runtime.GetGCSpace();std::string result;
        gc_space->Accumulate(&runtime,[&] {
            auto* ctx=runtime.CreateGlobalContext();const si_t si;ExecutionPointer ep;
            auto* ui=new ::ui::UI(GSE_CALL);util::FinallyGuard cleanup([&]{ui->GetRoot()->Container::Destroy(GSE_CALL);});
            auto* scene=ui->GetRoot()->m_cache->GetScene();std::vector<scene::actor::Actor*> before;
            scene->WithActors([&](const auto& actors){before=actors;});
            auto* root=static_cast<value::Object*>(ui->GetRoot()->Wrap(GSE_CALL));
            auto* surface=static_cast<::ui::dom::Surface*>(::ui::dom::Object::Unwrap(static_cast<value::Callable*>(root->Get("surface"))->Run(GSE_CALL,{})));
            scene::actor::Mesh* actor=nullptr;
            scene->WithActors([&](const auto& actors){for(auto* candidate:actors) {if(std::find(before.begin(),before.end(),candidate)==before.end()) {actor=dynamic_cast<scene::actor::Mesh*>(candidate);}}});
            result=[&]()->std::string {
                GT_ASSERT(actor,"Actual graph surface actor missing");
                History history;history.samples[1][1]=2;history.samples[2][1]=4;history.samples[1][5]=20;history.samples[2][5]=21;
                value::array_elements_t scores,names;
                const std::array<std::string,8> keys{"","GAIANS","HIVE","UNIV","MORGAN","SPARTANS","BELIEVE","PEACE"};
                for(size_t slot=0;slot<8;++slot) {scores.push_back(VALUE(value::Int,,slot==5?21:slot==1?4:0));names.push_back(VALUE(value::String,,keys[slot]));}
                auto* frame=new value::Object(GSE_CALL,{{"kind",VALUE(value::String,,"smac-native-dominance-v1")},{"turn",VALUE(value::Int,,2)},
                    {"history",VALUE(value::String,,history.Serialize())},{"scores",VALUE(value::Array,,scores)},{"native_keys",VALUE(value::Array,,names)}});
                auto* render=static_cast<value::Callable*>(static_cast<value::Object*>(surface->Wrap(GSE_CALL))->Get("smacdominancegraph"));
                render->Run(GSE_CALL,{frame});const auto* texture=actor->GetTexture();const auto revision=actor->GetTextureRevision();
                GT_ASSERT(texture && texture->GetWidth()==220 && texture->GetHeight()==86
                    && texture->GetPixel(0,83)==types::Color::RGB(0,235,0) && texture->GetPixel(1,81)==types::Color::RGB(0,235,0)
                    && texture->GetPixel(0,65)==types::Color::RGB(235,235,235) && texture->GetPixel(1,64)==types::Color::RGB(235,235,235),"Real graph Surface lost native endpoints, alternate Spartans ink or palette");
                history.samples[1][1]=0;history.samples[2][1]=0;frame->Set("history",VALUE(value::String,,history.Serialize()),GSE_CALL);
                render->Run(GSE_CALL,{frame});
                GT_ASSERT(actor->GetTexture()==texture && actor->GetTextureRevision()>revision && texture->GetPixel(0,83)==types::Color::RGBA(0,0,0,0),"Graph redraw retained stale pixels or replaced owned texture");
                GT_OK();
            }();
        });
        return result;
    });
    if(!check.empty()) {return check;}
    GT_ASSERT(retained && retained->normal==normal && retained->colors[255]==(std::array<uint8_t,3>{255,255,255}),"Graph palette borrowed destroyed loader storage");GT_OK();
}

void AddSmacDominanceGraphTests(task::gsetests::GSETests* task) {
    task->AddTest("Dominance graph actual SDL metadata and Surface retain palette pixels and redraw ownership",GT(){return CheckGraphPaletteSurface();});

    task->AddTest("Dominance graph duplicate faction owners keep valid campaign upkeep and retained native history",GT(){
        using namespace game::backend;
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(smac_startup_fixture::OrdinarySource,true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* world=game->GetMap();std::string retained;
            fixture.ResourceState([&](auto& state) {
                if(state.roster.size()!=2 || state.roster[0].faction_id!=state.roster[1].faction_id) {throw std::runtime_error("fixture does not have duplicate valid faction owners");}
                state.dominance_history->samples[1][1]=777;state.dominance_history->samples[999][7]=-32768;
                retained=state.dominance_history->Serialize();
                for(auto& entry:state.economy) {entry.second.population_aggregate=17;entry.second.design_counts.emplace();}
            });
            const auto saved=fixture.Snapshot();
            auto* request=static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("request_smac_dominance_graph"));
            GT_ASSERT(!static_cast<value::Bool*>(request->Run(GSE_CALL,{VALUE(value::Int,,1)}))->value,"duplicate owners were merged into a fabricated native graph");
            GT_ASSERT(fixture.Snapshot()==saved,"unsupported graph admission mutated retained native history");
            map::SmacWorkerKnowledge::Data knowledge{int32_t(world->GetWidth()),int32_t(world->GetHeight()),true,true,0,{},{},{}};
            for(int owner=1;owner<=2;++owner) {knowledge.observers[owner]=map::SmacWorkerKnowledge::Observer{};knowledge.remembered[owner].resize(world->GetWidth()*world->GetHeight()/2);}
            knowledge.tiles.resize(world->GetWidth()*world->GetHeight()/2,{6,std::nullopt});
            game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(knowledge));
            std::array<std::string,8> roles;roles[0]="graph-worker";
            game->GetBM()->DefinePop(new base::PopDef(roles[0],"Worker",{},{},base::PopDef::PF_TILE_WORKER));
            for(uint8_t i=0;i<7;++i) {roles[i+1]="graph-specialist"+std::to_string(i);game->GetBM()->DefinePop(new base::PopDef(roles[i+1],"Specialist",{},{},base::PopDef::PF_NONE,i));}
            fixture.BasePresentationRoles(roles);
            const auto turn=game->GetTurnId();game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();bool exact=false;
            fixture.ResourceState([&](auto& state) {exact=game->GetTurnId()==turn+1 && state.rank_order
                && state.ranking_scores->at(1)==-1 && state.ranking_scores->at(2)==-1 && state.dominance_history->Serialize()==retained;});
            GT_ASSERT(exact,"unsupported graph projection broke canonical ranking/upkeep or replaced existing native samples");GT_OK();
        });
    });
    task->AddTest("Dominance graph actual queued rebuild refreshes native slots without reranking or inventing legacy identity",GT(){
        using namespace game::backend;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"UNIVERSITY");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();const auto& rules=game->GetState()->m_settings.global;
            int32_t expected=0;std::string previous;faction::SmacRankOrder retained;
            fixture.ResourceState([&](auto& state) {
                auto& economy=state.economy.at(1);economy.population_aggregate=13;economy.design_counts.emplace();
                expected=faction::CalculateSmacOrdinaryRankingScore(*state.technology.Find(1),13,*economy.design_counts,*rules.GetSmacTechnologyRules(),rules.GetSmacPredefinedUnitRules()->catalog,state.designs);
                state.rank_order.emplace();state.rank_order->faction_at_rank[4]=1;retained=*state.rank_order;
                state.ranking_scores->fill(-1);state.ranking_scores->at(0)=1234;
                state.dominance_history->samples[999][7]=-32768;previous=state.dominance_history->Serialize();
            });
            const auto random=fixture.GameplayRandom();
            auto* request=static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("request_smac_dominance_graph"));
            GT_ASSERT(static_cast<value::Bool*>(request->Run(GSE_CALL,{VALUE(value::Int,,1)}))->value,"Real canonical request unavailable");
            bool exact=false;
            fixture.ResourceState([&](auto& state) {exact=state.dominance_history->Serialize()==previous && state.ranking_scores->at(1)==-1;});
            GT_ASSERT(exact,"Queue-only request mutated historical state before owning dispatch");
            fixture.DrainEvents();
            fixture.ResourceState([&](auto& state) {
                const auto& row=state.dominance_history->samples.at(game->GetTurnId());
                exact=row[3]==expected && row[1]==0 && state.dominance_history->samples[999][7]==-32768
                    && state.ranking_scores->at(1)==expected && state.ranking_scores->at(0)==1234 && *state.rank_order==retained;
            });
            GT_ASSERT(exact && fixture.GameplayRandom()==random,"Score-only rebuild used dense native color/history slots, reranked, rewrote old data or drew RNG");
            fixture.ResourceState([&](auto& state) {state.roster.front().active=false;});
            request->Run(GSE_CALL,{VALUE(value::Int,,2)});fixture.DrainEvents();
            fixture.ResourceState([&](auto& state) {exact=state.dominance_history->samples.at(game->GetTurnId())[3]==0
                && state.dominance_history->samples[999][7]==-32768 && *state.rank_order==retained;});
            GT_ASSERT(exact,"Inactive current row did not become zero or old series was dropped");
            fixture.ResourceState([&](auto& state) {state.dominance_native_keys.reset();previous=state.dominance_history->Serialize();});
            GT_ASSERT(!static_cast<value::Bool*>(request->Run(GSE_CALL,{VALUE(value::Int,,3)}))->value,"V31 dense payload acquired guessed native identities");
            fixture.ResourceState([&](auto& state) {exact=state.dominance_history->Serialize()==previous;});
            GT_ASSERT(exact,"Unavailable legacy projection lost raw future samples");GT_OK();
        });
    });
    task->AddTest("Dominance indexed line kernel matches709 native controls and3362 ordered stores",GT(){
        size_t stores=0,empty=0;
        const auto hash=[](const std::vector<uint8_t>& bytes) {uint64_t value=UINT64_C(14695981039346656037);for(const auto byte:bytes) {value=(value^byte)*UINT64_C(1099511628211);}return value;};
        for(const auto& c:OriginalLineCases) {
            std::vector<uint8_t> guarded(16+40*24+16,199),stream;
            for(size_t y=0;y<24;++y) {for(size_t x=0;x<40;++x) {guarded[16+y*40+x]=x<32?17:253;}}
            bool rejected=false;
            try {types::texture::SmacIndexedLine(c.line,c.clip,c.color,[&](int32_t x,int32_t y,uint8_t color) {
                if(x<0 || x>=32 || y<0 || y>=24) {throw std::runtime_error("Native line escaped target");}
                guarded[16+y*40+x]=color;
                stream.push_back(x&255);stream.push_back((x>>8)&255);stream.push_back(y&255);stream.push_back((y>>8)&255);stream.push_back(color);
            },c.backed);}catch(const std::invalid_argument&) {rejected=true;}
            const bool empty_clip=c.clip[0]>=c.clip[2] || c.clip[1]>=c.clip[3];
            GT_ASSERT(rejected==empty_clip && stream.size()==c.stores*5 && hash(stream)==c.store_hash,"Native ordered line stores differ");
            const std::vector<uint8_t> frame(guarded.begin()+16,guarded.end()-16);
            GT_ASSERT(hash(frame)==c.frame_hash && std::all_of(guarded.begin(),guarded.begin()+16,[](auto byte){return byte==199;})
                && std::all_of(guarded.end()-16,guarded.end(),[](auto byte){return byte==199;}),"Native line changed pixels, guards or row padding");
            stores+=c.stores;empty+=empty_clip;
        }
        GT_ASSERT(stores==3362 && empty==9,"Native indexed line coverage changed");GT_OK();
    });
    task->AddTest("Dominance graph matches seventeen native builders and all6860 line requests",GT(){
        const std::array<std::string,8> names{"","FACTION1","FACTION2","FACTION3","FACTION4","SPARTANS","FACTION6","FACTION7"};
        const std::array<uint8_t,8> normal{249,111,252,255,251,65,102,187},alternate{1,175,4,248,3,57,109,192};
        size_t count=0;
        for(const auto& c:OriginalGraphCases) {
            History history;
            for(size_t turn=0;turn<1000;++turn) {
                history.samples[turn][0]=1234;
                for(size_t owner=1;owner<8;++owner) {history.samples[turn][owner]=static_cast<int16_t>(((turn*owner+17*owner)%300)*c.multiplier);}
            }
            history.Publish(c.turn,c.scores);
            const auto graph=Graph::Build(c.turn,c.rect,c.scores,history,names,normal,alternate);
            GT_ASSERT(graph.clip==c.rect && graph.horizontal_step==c.step && graph.vertical_divisor==c.divisor
                && graph.lines.size()==c.lines && Hash(graph.lines)==c.fnv,"Original graph line requests differ: "+std::string(c.id));
            count+=graph.lines.size();
        }
        GT_ASSERT(count==6860,"Native graph context coverage changed");GT_OK();
    });
    task->AddTest("Dominance graph refuses unclosed turn capacity and alternate columns",GT(){
        History history;std::array<int32_t,8> scores{};std::array<std::string,8> names{};std::array<uint8_t,8> colors{};
        for(const auto turn:{1000u,1001u}) {
            bool rejected=false;try {Graph::Build(turn,{501,76,721,162},scores,history,names,colors,colors);}catch(const std::invalid_argument&) {rejected=true;}
            GT_ASSERT(rejected,"High turn read adjoining history storage");
        }
        bool rejected=false;try {Graph::Build(2,{0,0,220,86},scores,history,names,colors,colors);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Unmeasured graph-column geometry admitted");GT_OK();
    });
}
}
