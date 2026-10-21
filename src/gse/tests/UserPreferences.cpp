#include "UserPreferences.h"
#include "config/Config.h"
#include "gc/Space.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/context/GlobalContext.h"
#include "task/gsetests/GSETests.h"
#include "util/FinallyGuard.h"
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
                    "Enabled preference did not survive another configuration lifetime");GT_OK();
            }();
        });}catch(const std::exception& error) {return std::string("Preference lifecycle: ")+error.what();}
        return result;
    });
}
}
