#include "UserPreferences.h"
#include "config/Config.h"
#include "config/WrappedConfig.h"
#include "util/ConfigManager.h"
#include "gc/Space.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/context/GlobalContext.h"
#include "task/gsetests/GSETests.h"
#include "util/FinallyGuard.h"
#include "ui/dom/SmacPreferencesText.h"
#include "util/FS.h"
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace gse::tests {
namespace {
bool Rejects(const std::function<void()>& call) {try {call();}catch(const std::exception&) {return true;}return false;}
std::string Read(const std::filesystem::path& path) {std::ifstream in(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});}
}
void AddUserPreferencesTests(task::gsetests::GSETests* task) {
    task->AddTest("User preferences reproduce original defaults inversion and accepted word reconstruction",GT() {
        namespace prefs=::config::smac_preferences;
        const prefs::Words desktop{{0xbbe1dd96u,0x003a7168u,0x0003c3a9u}};
        GT_ASSERT(prefs::Defaults(false,2)==desktop && prefs::Defaults(false,0)[0]==0xbbe1ddb6u
            && prefs::Defaults(true,1)[0]==0xa3e1dd36u && prefs::Defaults(true,5)[1]==0x00327168u,
            "Original Laptop/difficulty default branches differ");
        GT_ASSERT(prefs::Defaults(false,0,true)==desktop && prefs::Defaults(true,0,true)[0]==0xa3e1dd16u,
            "Restore Defaults incorrectly inherited the low-difficulty tutorial bit");
        GT_ASSERT((prefs::Encode(prefs::Draft{})==prefs::Words{{0x20048004u,0x03400000u,0u}}),
            "All unchecked controls lost the original seven inverted bits");
        const auto zero=prefs::Decode({{0u,0u,0u}});size_t inverted=0;
        for(bool checked:zero.checked) {inverted+=checked;}
        GT_ASSERT(inverted==7 && zero.checked[13] && zero.checked[14] && zero.checked[15] && zero.checked[42]
            && zero.checked[66] && zero.checked[71] && zero.checked[79],"Original inverted control identities differ");
        GT_ASSERT((prefs::Encode(zero)==prefs::Words{}),"Zero words did not round-trip through inverted controls");
        GT_ASSERT((prefs::Encode(prefs::Decode({{0xffffffffu,0xffffffffu,0xffffffffu}}))==prefs::Words{{0xfbffdfafu,0x03fffffdu,0x0003feffu}}),
            "Original accepted reconstruction retained unrepresented bits");
        GT_ASSERT((prefs::Encode(prefs::Decode(desktop))==prefs::Words{{0xbbe1dd86u,0x003a7168u,0x0003c2a9u}}),
            "Unchanged original OK did not clear the two unrepresented default bits");
        GT_ASSERT(prefs::Load(prefs::Store(desktop))==desktop,"Local preference words did not round-trip");
        for(const std::string bad:{"", "1,2", "1,2,3,4", "-1,2,3", "4294967296,2,3", "1,,3", "1,2,3x"}) {
            GT_ASSERT(Rejects([&]{prefs::Load(bad);}),"Malformed preference word store was accepted");
        }
        GT_OK();
    });
    task->AddTest("User preferences load all67 constructed controls from installed templates",GT() {
        std::string source;
        const std::array<std::string,6> tags{{"BASICPREFS","ADVANCEDPREFS","BASEWARNINGS","AUTOMATIONPREFS","AUDIOVISUALPREFS","MAPPREFS"}};
        const std::array<int,6> counts{{8,12,17,14,11,8}};
        for(size_t page=0;page<tags.size();++page) {
            source+="#"+tags[page]+"\r\n#xs 440\r\n#caption Title"+std::to_string(page)+"\r\n";
            // Map has explanatory body text; only #itemlist rows are controls.
            source+="Not a checkbox.\r\n#itemlist\r\n#checkbox 17\r\n";
            for(int row=0;row<counts[page];++row) {source+="label"+std::to_string(page)+"_"+std::to_string(row)+"\r\n";}
            source+="; ignored comment\r\n## translator comment\r\n\r\n";
        }
        const auto pages=::ui::dom::ReadSmacPreferencesText(source);
        size_t total=0;std::vector<int> ids;
        for(const auto& page:pages) {for(const auto& row:page.rows) {++total;ids.push_back(row.id);}}
        GT_ASSERT(total==67 && std::count(ids.begin(),ids.end(),53)==0,"Constructed control inventory differs");
        GT_ASSERT(pages[1].rows[5].id==28 && pages[1].rows[5].label=="label1_5"
            && pages[3].rows[6].id==42 && pages[3].rows[6].label=="label3_6","Nonsequential native IDs lost their template ordinals");
        GT_ASSERT(pages[4].rows[0].id==64 && pages[4].rows[0].label=="label4_3"
            && pages[4].rows[7].id==22 && pages[4].rows[7].label=="label4_10","Audio template-only rows became controls");
        GT_ASSERT(pages[2].rows.size()==13 && pages[2].rows[10].id==54 && pages[2].rows[10].label=="label2_10"
            && pages[6].rows.size()==4 && pages[6].rows[0].id==57 && pages[6].rows[0].label=="label2_13"
            && pages[6].title==pages[2].title,"Warning continuation or missing53 shifted labels");
        GT_ASSERT(pages[5].rows[2].id==78 && pages[5].rows[2].label=="label5_2","Ocean grid control was reordered");
        GT_ASSERT(Rejects([&]{::ui::dom::ReadSmacPreferencesText(source+"#BASICPREFS\n");}),"Duplicate template was accepted");
        GT_ASSERT(Rejects([&]{::ui::dom::ReadSmacPreferencesText(source+"extra checkbox\n");}),"Extra checkbox shifted the contract");
        GT_ASSERT(Rejects([&]{::ui::dom::ReadSmacPreferencesText(source.substr(source.find("#ADVANCEDPREFS")));}),"Missing page was accepted");
        GT_ASSERT(Rejects([&]{::ui::dom::ReadSmacPreferencesText(source+std::string(1,'\0'));}),"Embedded NUL was accepted");
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");
        if(assets && *assets) {
            std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,std::string(assets)+"/script.txt");
            const auto original=::ui::dom::ReadSmacPreferencesText(std::string(bytes.begin(),bytes.end()));
            total=0;for(const auto& page:original) {total+=page.rows.size();}
            GT_ASSERT(total==67 && original[0].rows[1].id==8 && !original[0].rows[1].label.empty(),"Installed original Preferences catalog differs");
        }
        GT_OK();
    });
    task->AddTest("User preferences persist disabled automatic designs and fence retired configuration callbacks",GT() {
        const auto parent=std::filesystem::temp_directory_path();
        const auto path=parent/("glsmac-preferences-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if(path.parent_path()!=parent || !std::filesystem::create_directory(path)) {GT_FAIL("Cannot create isolated preference profile");}
        util::FinallyGuard cleanup([&] {std::error_code ignored;std::filesystem::remove_all(path,ignored);});
        const auto file=path/"config.yml",backup=path/"config.previous";
        {std::ofstream out(file);out << "smacpath: retained-install-path\n";}
        GSE runtime;std::string result;auto* gc_space=runtime.GetGCSpace();
        try {gc_space->Accumulate(&runtime,[&] {
            auto* ctx=runtime.CreateGlobalContext();ExecutionPointer ep;const si_t si{};
            const auto create=[&] {
                auto cfg=std::make_unique<::config::Config>("preferences-test");
                std::string prefix="--prefix="+path.string();char name[]="preferences-test";
                char* args[]{name,prefix.data()};cfg->Init(2,args);return cfg;
            };
            result=[&]() -> std::string {
                auto cfg=create();auto* object=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                auto* get=static_cast<value::Callable*>(object->Get("get_automatic_unit_designs"));
                auto* set=static_cast<value::Callable*>(object->Get("set_automatic_unit_designs"));
                const auto enabled=[&](value::Callable* getter) {return static_cast<value::Bool*>(getter->Run(GSE_CALL,{}))->value;};
                GT_ASSERT(enabled(get),"Fresh profile did not use the original enabled default");
                const auto original=Read(file);
                GT_ASSERT(Rejects([&] {set->Run(GSE_CALL,{VALUE(value::Int,,0)});}) && Read(file)==original,
                    "Non-Boolean preference changed the configuration");
                set->Run(GSE_CALL,{VALUE(value::Bool,,false)});
                GT_ASSERT(!enabled(get) && Read(file).find("retained-install-path")!=std::string::npos,
                    "Disabled preference was not live or rewrote unrelated settings");
                auto* second=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                GT_ASSERT(!enabled(static_cast<value::Callable*>(second->Get("get_automatic_unit_designs"))),
                    "A new wrapper reset the stored preference");
                cfg.reset();
                GT_ASSERT(Rejects([&] {get->Run(GSE_CALL,{});}) && Rejects([&] {set->Run(GSE_CALL,{VALUE(value::Bool,,true)});}),
                    "Retained configuration callbacks accessed their destroyed manager");
                cfg=create();object=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                get=static_cast<value::Callable*>(object->Get("get_automatic_unit_designs"));
                set=static_cast<value::Callable*>(object->Get("set_automatic_unit_designs"));
                GT_ASSERT(!enabled(get),"Fresh configuration lost explicit false from disk");
                set->Run(GSE_CALL,{VALUE(value::Bool,,true)});const auto saved=Read(file);
                std::filesystem::rename(file,backup);std::filesystem::create_directory(file);
                GT_ASSERT(Rejects([&] {set->Run(GSE_CALL,{VALUE(value::Bool,,false)});}) && enabled(get) && Read(backup)==saved,
                    "Failed preference save was reported as success or changed live state");
                std::filesystem::remove(file);std::filesystem::rename(backup,file);
                cfg.reset();cfg=create();object=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                GT_ASSERT(enabled(static_cast<value::Callable*>(object->Get("get_automatic_unit_designs"))),
                    "Enabled preference did not survive another configuration lifetime");

                // A shared Preferences acceptance must save all represented
                // fields together, not issue independently visible saves.
                {util::ConfigManager batch("preferences-test",cfg.get());
                ::config::wrapped::Config facade(&batch);
                auto* batch_object=static_cast<value::Object*>(facade.Wrap(GSE_CALL));
                auto* batch_get=static_cast<value::Callable*>(batch_object->Get("get_automatic_unit_designs"));
                batch.UpdateSettings({{"automatic-unit-designs","false"},{"smacpath","batch-install"},{"ruleset","smac"}});
                auto* snapshot=static_cast<value::Object*>(facade.Wrap(GSE_CALL));
                GT_ASSERT(!enabled(batch_get) && static_cast<value::String*>(snapshot->Get("smacpath"))->value=="batch-install"
                    && static_cast<value::String*>(snapshot->Get("ruleset"))->value=="smac",
                    "Batch acceptance did not expose all fields together");
                const auto batch_saved=Read(file);
                std::filesystem::rename(file,backup);std::filesystem::create_directory(file);
                GT_ASSERT(Rejects([&]{batch.UpdateSettings({{"automatic-unit-designs","true"},{"smacpath","failed-install"},{"ruleset","smax"}});})
                    && !enabled(batch_get) && Read(backup)==batch_saved,"Failed batch save changed live values or the previous file");
                snapshot=static_cast<value::Object*>(facade.Wrap(GSE_CALL));
                GT_ASSERT(static_cast<value::String*>(snapshot->Get("smacpath"))->value=="batch-install"
                    && static_cast<value::String*>(snapshot->Get("ruleset"))->value=="smac","Failed batch rolled back only one field");
                // Empty acceptance does not attempt IO even at an unwritable path.
                batch.UpdateSettings({});
                std::filesystem::remove(file);std::filesystem::rename(backup,file);}
                cfg.reset();cfg=create();object=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                GT_ASSERT(!enabled(static_cast<value::Callable*>(object->Get("get_automatic_unit_designs")))
                    && static_cast<value::String*>(object->Get("smacpath"))->value=="batch-install"
                    && static_cast<value::String*>(object->Get("ruleset"))->value=="smac","Accepted batch did not survive a fresh configuration lifetime");

                auto* full_get=static_cast<value::Callable*>(object->Get("get_smac_preferences"));
                auto* full_set=static_cast<value::Callable*>(object->Get("set_smac_preferences"));
                auto* reset=static_cast<value::Callable*>(object->Get("get_smac_preference_defaults"));
                const auto get_full=[&](int difficulty) {return static_cast<value::Object*>(full_get->Run(GSE_CALL,{VALUE(value::Bool,,false),VALUE(value::Int,,difficulty)}));};
                auto* state=get_full(0);auto* values=static_cast<value::Object*>(state->Get("values"));
                auto* audio=static_cast<value::Object*>(state->Get("audio"));
                const auto flag=[](value::Object* values,const char* key) {return static_cast<value::Bool*>(values->Get(key))->value;};
                GT_ASSERT(values->value.size()==67 && audio->value.size()==3 && !flag(values,"8") && flag(values,"7")
                    && cfg->GetSmacPreference(10),"Legacy Boolean migration or complete draft shape differs");
                auto* default_state=static_cast<value::Object*>(reset->Run(GSE_CALL,{VALUE(value::Bool,,false)}));
                GT_ASSERT(!flag(static_cast<value::Object*>(default_state->Get("values")),"7"),"Reset incorrectly enabled low-difficulty tutorial");
                values->Assign("8",VALUE(value::Bool,,true));values->Assign("10",VALUE(value::Bool,,false));
                GT_ASSERT(cfg->GetSmacPreference(10),"Detached preference edit changed live settings before acceptance");
                full_set->Run(GSE_CALL,{values,audio});
                GT_ASSERT(!cfg->GetSmacPreference(10) && cfg->GetSmacPreference(8)
                    && enabled(static_cast<value::Callable*>(object->Get("get_automatic_unit_designs"))),"Full acceptance did not update shared consumers");
                const auto full_saved=Read(file);
                GT_ASSERT(Rejects([&]{full_set->Run(GSE_CALL,{VALUE(value::Object,,GSE_CALL_NOGC),audio});}) && Read(file)==full_saved,
                    "Incomplete preference draft changed persisted settings");
                std::filesystem::rename(file,backup);std::filesystem::create_directory(file);
                values->Assign("8",VALUE(value::Bool,,false));values->Assign("10",VALUE(value::Bool,,true));
                GT_ASSERT(Rejects([&]{full_set->Run(GSE_CALL,{values,audio});}) && cfg->GetSmacPreference(8)
                    && !cfg->GetSmacPreference(10) && Read(backup)==full_saved,"Failed full acceptance partially changed consumers");
                std::filesystem::remove(file);std::filesystem::rename(backup,file);
                cfg.reset();
                GT_ASSERT(Rejects([&]{full_get->Run(GSE_CALL,{});}) && Rejects([&]{full_set->Run(GSE_CALL,{});})
                    && Rejects([&]{reset->Run(GSE_CALL,{});}),"Full preference callbacks survived their configuration lifetime");
                cfg=create();object=static_cast<value::Object*>(cfg->Wrap(GSE_CALL));
                get=static_cast<value::Callable*>(object->Get("get_automatic_unit_designs"));
                set=static_cast<value::Callable*>(object->Get("set_automatic_unit_designs"));
                GT_ASSERT(enabled(get) && !cfg->GetSmacPreference(10),"Full preference words did not survive reload");
                set->Run(GSE_CALL,{VALUE(value::Bool,,false)});
                GT_ASSERT(!enabled(get) && !cfg->GetSmacPreference(10),"Legacy setter clobbered unrelated packed preferences");GT_OK();
            }();
        });}catch(const std::exception& error) {return std::string("Preference lifecycle: ")+error.what();}
        return result;
    });
}
}
