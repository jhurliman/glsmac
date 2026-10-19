#include "SmacDetailMinimap.h"
#include "game/SmacDetailMinimap.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Bool.h"
#include "gse/value/Array.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacGraphPalette.h"
#include "Mesh.h"
#include "util/UUID.h"
#include "util/FinallyGuard.h"
#include "gse/GSE.h"
#include "gse/context/GlobalContext.h"
#include "gc/Space.h"
#include "gse/value/Int.h"
#include "ui/UI.h"
#include "ui/dom/Root.h"
#include "ui/dom/Widget.h"
#include "game/frontend/widget/Minimap.h"
#include "types/texture/Texture.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
namespace gse::tests {
namespace {
namespace detail=game::smac_detail;
struct TerrainCase {const char* id;detail::Point camera;bool flat,mixed,checker;int known;bool survey,revealed;int background;size_t entries;uint64_t entry_hash;size_t fills;uint64_t fill_hash;size_t writes;uint64_t write_hash,frame_hash;};
struct BaseCase {const char* id;int known;bool survey,revealed;int owner,visited;bool no_base,unit,cdf,admitted;detail::Point tile,anchor,offset,size;size_t samples;uint64_t sample_hash,source_hash,frame_before_text;};
#include "fixtures/SmacDetailMinimap.inc"
struct Hash {uint64_t value=UINT64_C(14695981039346656037);void Byte(uint8_t byte) {value=(value^byte)*UINT64_C(1099511628211);}void Word(int v) {const auto word=static_cast<uint32_t>(v);for(int bit=0;bit<32;bit+=8) {Byte((word>>bit)&255);}}void Short(int v) {Byte(v&255);Byte((v>>8)&255);}};
template<class Bytes> uint64_t Fnv(const Bytes& bytes) {Hash h;for(const auto byte:bytes) {h.Byte(static_cast<uint8_t>(byte));}return h.value;}
detail::World World(bool flat,bool mixed,bool checker,int known,bool survey,bool revealed) {
    detail::World world;world.wrap=!flat;
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {auto& tile=world.tiles[y*40+x/2];tile.band=mixed?(x/2+y)%8:3;
        tile.known=(checker?bool((x/2+y)&1):bool(known&8)) || survey || revealed;tile.base_known=bool(known&8)||revealed;}}
    const auto band=[&](int x,int y)->std::optional<uint8_t> {if(y<0 || y>=80) {return {}; }if(!flat) {x=(x%80+80)%80;}if(x<0 || x>=80) {return {}; }return world.tiles[y*40+x/2].band;};
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {world.tiles[y*40+x/2].heights=detail::Heights(x,y,band);}}return world;
}
}
void AddSmacDetailMinimapTests(task::gsetests::GSETests* task) {
    task->AddTest("Detail minimap eleven complete native terrain frames preserve geometry edge phase and indexed backing",GT(){
        size_t total_fills=0,total_writes=0;
        for(const auto& c:TERRAIN_CASES) {
            const auto world=World(c.flat,c.mixed,c.checker,c.known,c.survey,c.revealed);Hash entries,fills,writes;
            const auto order=detail::Entries(world,c.camera);
            for(const auto& e:order) {entries.Word(e.tile[0]);entries.Word(e.tile[1]);entries.Word(e.anchor[0]);entries.Word(e.anchor[1]);}
            size_t count=0,stored=0;
            const auto frame=detail::Terrain(world,c.camera,static_cast<uint8_t>(c.background),[&](const auto& entry,const auto& points,uint8_t color) {
                ++count;fills.Word(entry.tile[0]);fills.Word(entry.tile[1]);for(const auto& p:points) {fills.Word(p[0]);fills.Word(p[1]);}fills.Word(color);
                detail::Fill(points,{0,0,239,119},[&](int y,int left,int right) {for(int x=left;x<right;++x) {writes.Short(x);writes.Short(y);writes.Byte(color);++stored;}});
            });
            GT_ASSERT(order.size()==c.entries && entries.value==c.entry_hash,"native Detail tile order/anchor differs: "+std::string(c.id));
            GT_ASSERT(count==c.fills && fills.value==c.fill_hash,"native Detail physical fills differ: "+std::string(c.id));
            GT_ASSERT(stored==c.writes && writes.value==c.write_hash && Fnv(frame.pixels)==c.frame_hash,"native Detail edge stepping, stores, clip or padding differs: "+std::string(c.id));
            total_fills+=count;total_writes+=stored;
        }GT_ASSERT(total_fills==410 && total_writes==217303,"native Detail terrain coverage changed");GT_OK();
    });
    task->AddTest("Detail minimap native Base visibility excludes survey-only foreign-unvisited and all units",GT(){
        size_t admitted=0;
        for(const auto& c:BASE_CASES) {
            const bool visible=detail::AdmitsBase(true,!c.no_base,bool(c.known&8)||c.revealed,c.owner==3,bool(c.visited&8));
            GT_ASSERT(visible==c.admitted,"native Base survey/exploration/visited gate differs: "+std::string(c.id));
            GT_ASSERT(!detail::AdmitsBase(false,true,true,true,true),"inactive game drew a Base");
            if(visible) {
                const auto world=World(false,false,false,c.known,c.survey,c.revealed);const auto entries=detail::Entries(world,{40,40});
                const auto e=std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return entry.tile==c.tile;});
                GT_ASSERT(e!=entries.end() && e->anchor[0]==c.anchor[0] && e->anchor[1]-14==c.anchor[1],"native Detail Base anchor differs");++admitted;
            }
        }GT_ASSERT(admitted==7,"native Detail Base coverage changed");GT_OK();
    });
    task->AddTest("Detail minimap actual indexed Base loader matches seven native clipped sample streams",GT(){
        const auto* assets=std::getenv("GLSMAC_TEST_DETAIL_ASSET_ROOT");if(!assets) {GT_OK();}
        namespace fs=std::filesystem;const auto directory=fs::temp_directory_path()/("glsmac-detail-"+util::UUID::Generate());const auto data=directory/"default";
        fs::create_directory(directory);fs::create_directory(data);
        util::FinallyGuard cleanup([&] {std::error_code ignored;fs::remove(data/"univ.pcx",ignored);fs::remove(data/"palette.pcx",ignored);fs::remove(data,ignored);fs::remove(directory,ignored);});
        fs::copy_file(fs::path(assets)/"UNIV.PCX",data/"univ.pcx");fs::copy_file(fs::path(assets)/"PALETTE.PCX",data/"palette.pcx");
        return WithWidgetTextureResources(directory.string(),[&]()->std::string {
            const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();const auto shadow=game::frontend::smac_developed_overlay::ShadowLookup(palette->colors);size_t total=0;
            for(const auto& c:BASE_CASES) {
                auto world=World(false,false,false,c.known,c.survey,c.revealed);auto frame=detail::Terrain(world,{40,40});Hash stream;
                if(c.admitted) {
                    const auto source=g_engine->GetTextureLoader()->GetSmacDetailBaseSprite("univ.pcx",c.cdf?77:1);
                    GT_ASSERT(source && source==g_engine->GetTextureLoader()->GetSmacDetailBaseSprite("univ.pcx",c.cdf?77:1)
                        && source->offset==c.offset && source->width==c.size[0] && source->height==c.size[1] && Fnv(source->pixels)==c.source_hash,"actual Detail source trim/cache differs");
                    size_t count=0;detail::Composite(frame,c.anchor,*source,shadow,[&](int x,int y,int sx,int sy,uint8_t raw) {stream.Short(x);stream.Short(y);stream.Short(sx);stream.Short(sy);stream.Byte(raw);++count;});
                    GT_ASSERT(count==c.samples && stream.value==c.sample_hash,"native9/16 prefix phase/transparent/shadow sample stream differs: "+std::string(c.id));
                    detail::PopulationBox(frame,c.anchor,c.owner==3);total+=count;
                }
                GT_ASSERT(Fnv(frame.pixels)==c.frame_before_text,"native Detail body/shadow/population-box frame differs before declared font boundary: "+std::string(c.id));
            }GT_ASSERT(total==2663,"native Detail sample coverage changed");
            return WithWidgetGraphics([&]()->std::string {
            GSE runtime;auto* gc_space=runtime.GetGCSpace();std::string result;
            gc_space->Accumulate(&runtime,[&] {
                auto* ctx=runtime.CreateGlobalContext();const si_t si;ExecutionPointer ep;
                auto* ui=new ::ui::UI(GSE_CALL);
                auto presenter=std::make_unique<game::frontend::widget::Minimap>(nullptr,ui);
                util::FinallyGuard cleanup([&]{ui->GetRoot()->Container::Destroy(GSE_CALL);});
                auto* root=static_cast<value::Object*>(ui->GetRoot()->Wrap(GSE_CALL));
                auto* properties=new value::Object(GSE_CALL,{{"type",VALUE(value::String,,"minimap")},{"width",VALUE(value::Int,,239)},{"height",VALUE(value::Int,,119)}});
                auto* widget=static_cast<::ui::dom::Widget*>(::ui::dom::Object::Unwrap(static_cast<value::Callable*>(root->Get("widget"))->Run(GSE_CALL,{properties})));
                const auto pixels=[&]{auto* texture=widget->GetTexture(0);return std::string(reinterpret_cast<const char*>(texture->GetBitmap()),texture->GetBitmapSize());};
                result=[&]()->std::string {
                    std::string packed(3200*12,'\0'),rgba(3200*4,'\0');
                    for(size_t i=0;i<3200;++i) {packed[i*12]=3;packed[i*12+1]=7;rgba[i*4]=static_cast<char>(255);rgba[i*4+3]=static_cast<char>(255);}
                    const auto frame=[&](const std::string& bytes) {return new value::Object(GSE_CALL,{{"kind",VALUE(value::String,,"smac-detail-ordinary-v2")},{"horizontal_wrap",VALUE(value::Bool,,true)},{"tiles",VALUE(value::String,,bytes)},{"bases",new value::Array(gc_space,{})}});};
                    auto* overview=new value::Object(GSE_CALL,{{"kind",VALUE(value::String,,"smac-coarse-physical-v1")},{"width",VALUE(value::Int,,80)},{"height",VALUE(value::Int,,80)},{"horizontal_wrap",VALUE(value::Bool,,true)},{"rgba",VALUE(value::String,,rgba)}});
                    widget->WrapSet("data",new value::Object(GSE_CALL,{{"overview",overview},{"detail",frame(packed)},{"mode",VALUE(value::String,,"detail")}}),GSE_CALL);
                    const auto known=pixels();
                    GT_ASSERT(known.size()==239*119*4 && static_cast<uint8_t>(known[(60*239+120)*4])==palette->colors[22][0],"Detail widget did not paint native physical palette");
                    widget->WrapSet("data",new value::Object(GSE_CALL,{{"mode",VALUE(value::String,,"world")}}),GSE_CALL);
                    GT_ASSERT(pixels()!=known,"World mode retained Detail pixels");
                    widget->WrapSet("data",new value::Object(GSE_CALL,{{"mode",VALUE(value::String,,"detail")}}),GSE_CALL);
                    GT_ASSERT(pixels()==known,"mode-only Detail switch lost retained local view");
                    widget->WrapSet("data",new value::Object(GSE_CALL,{{"detail",frame(std::string(3200*12,'\0'))}}),GSE_CALL);
                    const auto unknown=pixels();
                    for(size_t i=0;i<unknown.size();i+=4) {GT_ASSERT(static_cast<uint8_t>(unknown[i])==palette->colors[0][0] && static_cast<uint8_t>(unknown[i+1])==palette->colors[0][1] && static_cast<uint8_t>(unknown[i+2])==palette->colors[0][2],"Detail retained unknown geography");}
                    NEWV(capture,types::texture::Texture,239,119);capture->Clear();presenter->UpdateCapture(capture);
                    GT_ASSERT(pixels()==unknown,"late GPU capture replaced unknown Detail");
                    widget->Hide();widget->Show();GT_ASSERT(pixels()==unknown,"reattach changed retained Detail");
                    widget->Hide();presenter.reset();widget->WrapSet("data",new value::Object(GSE_CALL,{}),GSE_CALL);GT_OK();
                }();
            });return result;
            },1024,768);
        });
    });
    task->AddTest("Detail minimap actual observer query retains known geometry without disclosing unknown terrain",GT(){
        using namespace game::backend;ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"UNIVERSITY",80,80);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* callable=static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("get_smac_detail_minimap"));
            GT_ASSERT(callable->Run(GSE_CALL,{})->type==VT_UNDEFINED,"missing knowledge supplied a Detail view");
            map::SmacWorkerKnowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=map::SmacWorkerKnowledge::Observer{};data.observers[1]->pact_with=0;data.tiles.resize(3200,{0,std::nullopt});data.tiles[40*40+20].explored_by=2;
            auto* tile=game->GetMap()->GetTile(40,40);tile->is_water_tile=false;*tile->elevation.center=100;
            game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(data));const auto saved=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            auto* frame=static_cast<value::Object*>(callable->Run(GSE_CALL,{}));const auto packed=static_cast<value::String*>(frame->Get("tiles"))->value;
            GT_ASSERT(packed.size()==3200*12 && static_cast<uint8_t>(packed[(40*40+20)*12+1])==7,"actual known Detail tile lost geography/Base flags");
            for(size_t i=0;i<3200;++i) {if(i!=40*40+20) {GT_ASSERT(packed.substr(i*12,12)==std::string(12,'\0'),"unknown Detail cell leaked retained geography");}}
            frame->Assign("tiles",VALUE(value::String,,"mutated copy"));
            GT_ASSERT(fixture.Snapshot()==saved && fixture.GameplayRandom()==random,"Detail read changed retained world/random");
            GT_ASSERT(static_cast<value::String*>(static_cast<value::Object*>(callable->Run(GSE_CALL,{}))->Get("tiles"))->value==packed,"Detail snapshot aliased mutable consumer data");GT_OK();
        });
    });
}
}
