#include "SmacMinimap.h"
#include "fixtures/SmacMinimap.h"
#include "fixtures/SmacMinimapNavigation.h"
#include "fixtures/SmacMinimapPlacement.h"
#include "game/frontend/widget/SmacMinimapRaster.h"
#include "Mesh.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Random.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/unit/Unit.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/GSE.h"
#include "gse/context/GlobalContext.h"
#include "gc/Space.h"
#include "ui/UI.h"
#include "ui/geometry/Geometry.h"
#include "ui/dom/Root.h"
#include "ui/dom/Widget.h"
#include "game/frontend/widget/Minimap.h"
#include "types/texture/Texture.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>

namespace gse::tests {
namespace {
using namespace game::backend;
Value* Query(GSE_CALLABLE,Game* game,const value::function_arguments_t& args={}) {
    return static_cast<value::Callable*>(static_cast<value::Object*>(game->Wrap(GSE_CALL))->Get("get_smac_minimap"))->Run(GSE_CALL,args);
}
bool Rejects(const std::function<void()>& f) {try {f();}catch(const std::exception&) {return true;}return false;}
const std::string& Pixels(value::Object* result) {return static_cast<value::String*>(result->Get("rgba"))->value;}

std::string CheckOverviewWidget() {
    return WithWidgetGraphics([]()->std::string {
        GSE runtime;auto* gc_space=runtime.GetGCSpace();std::string result;
        gc_space->Accumulate(&runtime,[&] {
            auto* ctx=runtime.CreateGlobalContext();const si_t si;ExecutionPointer ep;
            auto* ui=new ::ui::UI(GSE_CALL);
            auto presenter=std::make_unique<game::frontend::widget::Minimap>(nullptr,ui);
            util::FinallyGuard cleanup([&]{ui->GetRoot()->Container::Destroy(GSE_CALL);});
            auto* root=static_cast<value::Object*>(ui->GetRoot()->Wrap(GSE_CALL));
            auto* properties=new value::Object(GSE_CALL,{
                {"type",VALUE(value::String,,"minimap")},{"width",VALUE(value::Int,,239)},{"height",VALUE(value::Int,,119)}
            });
            auto* widget=static_cast<::ui::dom::Widget*>(::ui::dom::Object::Unwrap(
                static_cast<value::Callable*>(root->Get("widget"))->Run(GSE_CALL,{properties})));
            const auto pixels=[&]{auto* texture=widget->GetTexture(0);return std::string(reinterpret_cast<const char*>(texture->GetBitmap()),texture->GetBitmapSize());};
            result=[&]()->std::string {
                std::string rgba(3200*4,'\0');
                for(size_t i=0;i<rgba.size();i+=4) {rgba[i]=80;rgba[i+1]=65;rgba[i+2]=51;rgba[i+3]=static_cast<char>(255);}
                auto* frame=new value::Object(GSE_CALL,{
                    {"kind",VALUE(value::String,,"smac-coarse-physical-v1")},{"width",VALUE(value::Int,,80)},{"height",VALUE(value::Int,,80)},
                    {"horizontal_wrap",VALUE(value::Bool,,true)},{"rgba",VALUE(value::String,,rgba)}
                });
                widget->WrapSet("data",new value::Object(GSE_CALL,{{"overview",frame}}),GSE_CALL);
                const auto known=pixels();
                GT_ASSERT(std::count(known.begin(),known.end(),static_cast<char>(255))>100,"admitted terrain did not reach real minimap texture");
                widget->Hide();widget->Show();GT_ASSERT(pixels()==known,"native overview redraw changed retained snapshot: "+std::to_string(known.size())+" vs "+std::to_string(pixels().size())+" bytes, "+std::to_string(widget->GetTexture(0)->GetWidth())+"x"+std::to_string(widget->GetTexture(0)->GetHeight()));
                // A late capture response contains the whole world. It must
                // never replace an observer-filtered snapshot or its absence.
                NEWV(capture,types::texture::Texture,239,119);
                for(size_t y=0;y<119;++y) {for(size_t x=0;x<239;++x) {capture->SetPixel(x,y,types::Color::RGBA(255,0,255,255));}}
                presenter->UpdateCapture(capture);GT_ASSERT(pixels()==known,"GPU whole-world capture replaced observer raster");
                widget->WrapSet("data",new value::Object(GSE_CALL,{{"overview",new value::Object(GSE_CALL,{})}}),GSE_CALL);
                const auto unknown=pixels();GT_ASSERT(std::all_of(unknown.begin(),unknown.end(),[](char byte){return byte==0;}),"unavailable observer retained previous map pixels");
                widget->Hide();widget->Show();GT_ASSERT(pixels()==unknown,"reattach fell back to full-world capture");
                widget->Hide();presenter.reset();
                widget->WrapSet("data",new value::Object(GSE_CALL,{}),GSE_CALL); // retained callback must skip expired presenter
                GT_OK();
            }();
        });
        return result;
    },1024,768);
}
}
void AddSmacMinimapTests(task::gsetests::GSETests* task) {
    task->AddTest("minimap actual console caller remains right anchored on wider presentation stages",GT(){
        for(const auto size:std::array<std::array<unsigned short,2>,2>{{{{1024,768}},{{1440,1080}}}}) {
            const auto error=WithWidgetGraphics([&]()->std::string {
                GSE runtime;auto* gc_space=runtime.GetGCSpace();std::string result;
                gc_space->Accumulate(&runtime,[&] {
                    auto* ctx=runtime.CreateGlobalContext();const si_t si;ExecutionPointer ep;
                    auto* ui=new ::ui::UI(GSE_CALL);util::FinallyGuard cleanup([&]{ui->GetRoot()->Container::Destroy(GSE_CALL);});
                    auto* root=static_cast<value::Object*>(ui->GetRoot()->Wrap(GSE_CALL));
                    auto* state=static_cast<value::Object*>(runtime.RunScript(GSE_CALL,"./GLSMAC_data/tests/_minimap_layout"));
                    result=[&]()->std::string {
                        ui->GetRoot()->GetGeometry()->NeedUpdate();
                        const auto width=ui->GetRoot()->GetGeometry()->GetWidth();
                        GT_ASSERT(width==size[0],"unexpected logical presentation width");
                        const std::array<const char*,3> fields{{"geometry","info_geometry","world_geometry"}};
                        const std::array<int,3> source_left{{780,785,904}},source_right{{1019,901,958}};
                        for(size_t i=0;i<fields.size();++i) {
                            auto* source=static_cast<value::Object*>(state->Get(fields[i]));
                            value::object_properties_t props;
                            for(const auto* field:{"align","right","top","width","height"}) {
                                auto* v=source->Get(field);
                                // The synchronous factory omits class expansion;
                                // retain the measured World artwork dimensions.
                                if(v->type==VT_UNDEFINED && i==2) {v=VALUE(value::Int,,std::string(field)=="width"?54:32);}
                                props.emplace(field,v);
                            }
                            auto* panel=::ui::dom::Object::Unwrap(static_cast<value::Callable*>(root->Get("panel"))->Run(GSE_CALL,{new value::Object(GSE_CALL,props)}));
                            ui->GetRoot()->GetGeometry()->NeedUpdate();
                            const auto area=panel->GetGeometry()->GetEffectiveArea();
                            GT_ASSERT(area.left==source_left[i]+width-1024 && area.right==source_right[i]+width-1024,
                                "actual minimap caller detached a child from the right console");
                        }
                        GT_OK();
                    }();
                });return result;
            },size[0],size[1]);GT_ASSERT(error.empty(),"console stage: "+error);
        }GT_OK();
    });
    task->AddTest("minimap Planet source matches native clipping ink and complete indexed control",GT(){
        namespace raster=game::frontend::widget::smac_minimap;
        for(const auto& c:minimap_placement_fixture::cases) {
            GT_ASSERT(raster::PlanetAnchor(c.tile)==c.anchor,"native base-pass tile admission or anchor differs");
            int count=0;std::optional<std::array<int32_t,4>> bounds;
            raster::PlanetPixels(c.tile,[&](int32_t x,int32_t y) {
                if(!bounds) {bounds=std::array<int32_t,4>{{x,y,x+1,y+1}};}
                else {(*bounds)[0]=std::min((*bounds)[0],x);(*bounds)[1]=std::min((*bounds)[1],y);(*bounds)[2]=std::max((*bounds)[2],x+1);(*bounds)[3]=std::max((*bounds)[3],y+1);}
                ++count;
            });
            GT_ASSERT(bounds==c.bbox && count==c.count,"isolated native diamond painted different pixels");
        }
        std::string tiles(3200*4,'\0');for(size_t i=0;i<tiles.size();i+=4) {tiles[i]=80;tiles[i+1]=65;tiles[i+2]=51;tiles[i+3]=static_cast<char>(255);}
        const auto source=raster::PlanetSource(tiles);std::string indexed(320*152,100);size_t count=0;
        for(size_t i=0;i<indexed.size();++i) {if(source.rgba[i*4+3]) {indexed[i]=22;++count;}}
        uint64_t hash=14695981039346656037ull;for(const auto byte:indexed) {hash=(hash^static_cast<uint8_t>(byte))*1099511628211ull;}
        // Portable digest of the bitmap whose SHA256 equals the original
        // e5147aee...7f12 receipt. Optional dump permits independent SHA check
        // without adding a crypto dependency or native assets to the engine.
        GT_ASSERT(count==47980 && hash==0x22b180631ce1ef5dull,"complete native source coverage/control differs");
        if(const auto* path=std::getenv("GLSMAC_TEST_MINIMAP_INDEX_DUMP")) {
            std::ofstream out(path,std::ios::binary);out.write(indexed.data(),indexed.size());GT_ASSERT(out.good(),"could not write opt-in minimap index control");
        }
        const auto padded=raster::OverviewTarget({{80,80}},{{400,200}},tiles);
        GT_ASSERT(!padded.rgba[3] && static_cast<uint8_t>(padded.rgba[(24*400+40)*4+3])==255,"native unscaled source padding request differs");
        for(size_t channel=0;channel<4;++channel) {tiles[(2*40+1)*4+channel]=0;}
        const auto unknown=raster::PlanetSource(tiles);
        GT_ASSERT(!unknown.rgba[(1*320+3)*4+3],"unknown tile acquired its retained geography color");
        std::string sparse(3200*4,'\0');sparse[3]=static_cast<char>(255);
        GT_ASSERT(!raster::OverviewTarget({{80,80}},{{239,119}},sparse).rgba[3]
            && static_cast<uint8_t>(raster::OverviewTarget({{80,80}},{{239,119}},sparse,false).rgba[3])==255,
            "flat port fallback silently inherited round Planet row omission");
        std::string other(128*64*2,'\0');other[3]=static_cast<char>(255);
        const auto fallback=raster::OverviewTarget({{128,64}},{{239,119}},other);
        GT_ASSERT(static_cast<uint8_t>(fallback.rgba[3])==255 && !fallback.rgba.back(),"other-size fallback lost known geography or exposed unknowns");GT_OK();
    });
    task->AddTest("minimap observer raster owns snapshots and rejects late whole-world captures",GT(){return CheckOverviewWidget();});
    task->AddTest("minimap navigation matches original signed parity padding and seam helpers",GT(){
        namespace navigation=game::frontend::widget::smac_minimap;
        for(const auto& c:minimap_navigation_fixture::cases) {
            const auto layout=navigation::OverviewLayout({{80,80}},c.window);
            const auto actual=c.reverse?navigation::TileToPixel(c.input,layout,c.flat):navigation::PixelToTile(c.input,layout);
            GT_ASSERT(actual==c.expected,"original minimap coordinate mapping differs");
        }
        const auto stretched=navigation::OverviewLayout({{80,80}},{{239,119}});
        const auto padded=navigation::OverviewLayout({{80,80}},{{400,200}});
        GT_ASSERT(stretched.source==navigation::Point({{320,152}}) && stretched.image==stretched.window && stretched.offset==navigation::Point({{0,0}})
            && padded.source==padded.image && padded.offset==navigation::Point({{40,24}}),"overview producer stretch/centering request differs");
        GT_OK();
    });
    task->AddTest("minimap coarse colors match 28 original physical rainfall territory and knowledge controls",GT(){
        const auto colors=minimap_fixture::Colors();
        for(const auto& c:minimap_fixture::cases) {
            GT_ASSERT(map::smac_minimap::Resolve(c.input,colors)==c.expected,"original minimap decision differs: "+std::string(c.name));
        }
        map::smac_minimap::Input own;own.claimant=3;own.flags|=0x10000u;
        GT_ASSERT(Rejects([&]{(void)map::smac_minimap::Resolve(own);}),"political view fabricated absent assigned map color");
        own.claimant=0;GT_ASSERT(map::smac_minimap::Resolve(own)==map::smac_minimap::Color({22,{{80,65,51}}}),"native claim0 acquired slot0 red");
        for(int owner=1;owner<=7;++owner) {
            own.claimant=owner;own.viewer=owner;
            GT_ASSERT(map::smac_minimap::Resolve(own,colors)==colors[owner],"explicit map assignment was replaced by inferred faction tint");
        }
        own.viewer=3;own.display_faction=1;own.claimant=1;
        GT_ASSERT(map::smac_minimap::Resolve(own,colors)==colors[1],"political owner compared against renderer viewer instead of native focus faction");
        own.flags=0x80010010u;GT_ASSERT(Rejects([&]{(void)map::smac_minimap::Resolve(own,colors);}),"unmeasured combined scan flags admitted");GT_OK();
    });
    task->AddTest("minimap real local query preserves unknowns retained survey and restore without mutation",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"UNIVERSITY",80,80);
        std::string saved, expected;
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();
            GT_ASSERT(Query(GSE_CALL,game)->type==VT_UNDEFINED,"missing knowledge acquired full current map");
            game->GetState()->m_settings.global.SelectSmacCampaignRules(false);
            using Knowledge=map::SmacWorkerKnowledge;
            Knowledge::Data data{80,80,true,true,0,{},{},{}};data.observers[1]=Knowledge::Observer{};data.observers[1]->pact_with=0;
            data.tiles.resize(3200,{0,std::nullopt});data.tiles[0].explored_by=2;
            auto* tile=game->GetMap()->GetTile(0,0);tile->is_water_tile=false;*tile->elevation.center=100;
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            const auto before=fixture.Snapshot(), random=game->GetRandom()->GetStateString();
            const auto gameplay=fixture.GameplayRandom();const auto id=unit::Unit::GetNextId();const auto queued=fixture.QueuedFrontendRequests().size();
            auto* frame=Query(GSE_CALL,game);GT_ASSERT(frame->type==VT_OBJECT,"admitted physical frame missing");
            auto* result=static_cast<value::Object*>(frame);const auto original=Pixels(result);
            GT_ASSERT(static_cast<value::Int*>(result->Get("observer"))->value==1 && original.size()==3200*4,"query substituted faction index or grid size");
            GT_ASSERT(static_cast<uint8_t>(original[0])==80 && static_cast<uint8_t>(original[1])==65 && static_cast<uint8_t>(original[2])==51
                && static_cast<uint8_t>(original[3])==255 && std::all_of(original.begin()+4,original.end(),[](char byte){return byte==0;}),"unknown retained geography leaked into overview");
            result->Assign("rgba",VALUE(value::String,,"changed copy"));
            GT_ASSERT(Pixels(static_cast<value::Object*>(Query(GSE_CALL,game)))==original && fixture.Snapshot()==before
                && game->GetRandom()->GetStateString()==random && fixture.GameplayRandom()==gameplay && unit::Unit::GetNextId()==id
                && fixture.QueuedFrontendRequests().size()==queued,"minimap query aliased authority or changed world/RNG/queue");
            GT_ASSERT(Rejects([&]{Query(GSE_CALL,game,{VALUE(value::Int,,2)});}),"query accepted another observer selector");
            data.all_map_revealed=2;game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            const auto revealed=Pixels(static_cast<value::Object*>(Query(GSE_CALL,game)));
            for(size_t i=3;i<revealed.size();i+=4) {GT_ASSERT(static_cast<uint8_t>(revealed[i])==255,"saved all-map reveal did not admit physical geography");}
            saved=fixture.Snapshot();expected=revealed;
            fixture.ResourcePolicy(false);auto* unavailable=Query(GSE_CALL,game);
            GT_ASSERT(unavailable->type==VT_OBJECT && static_cast<value::Object*>(unavailable)->value.empty(),"missing terrain policy leaked a legacy whole-world fallback");GT_OK();
        });
        if(!setup.empty()) {return setup;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(smac_startup_fixture::OrdinarySource,"UNIVERSITY",80,80);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            auto* game=restored.Game();game->GetState()->m_settings.global.SelectSmacCampaignRules(false);
            restored.Restore(GSE_CALL,saved,true);
            auto* result=Query(GSE_CALL,game);GT_ASSERT(result->type==VT_OBJECT,"restored map knowledge unavailable after admission");
            GT_ASSERT(Pixels(static_cast<value::Object*>(result))==expected,"restore replaced actual knowledge frame");GT_OK();
        });
    });
}
}
