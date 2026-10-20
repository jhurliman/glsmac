#include "UIClass.h"
#include "Mesh.h"
#include "gse/context/GlobalContext.h"
#include "task/gsetests/GSETests.h"
#include "ui/UI.h"
#include "ui/dom/Root.h"
#include "ui/dom/Object.h"
#include "ui/dom/Input.h"
#include "input/Event.h"
#include "gc/Space.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/value/Array.h"
#include "gse/value/Callable.h"
#include "util/FinallyGuard.h"

namespace gse::tests {
namespace {
value::Object* AsObject(Value* v) {return static_cast<value::Object*>(v);}
Value* Call(GSE_CALLABLE,value::Object* object,const std::string& name,const value::function_arguments_t& args) {
    return static_cast<value::Callable*>(object->value.at(name))->Run(GSE_CALL,args);
}
int64_t Width(GSE_CALLABLE,value::Object* object) {
    auto* node=::ui::dom::Object::Unwrap(object);
    return static_cast<value::Int*>(AsObject(node->Wrap(GSE_CALL))->value.at("width"))->value;
}
std::string Exercise(GSE_CALLABLE,::ui::UI& gui,bool inherited) {
    auto* wrap=AsObject(gui.Wrap(GSE_CALL));
    const auto object=[&](const value::object_properties_t& p) {return VALUE(gse::value::Object,,GSE_CALL_NOGC,p);};
    const auto props=[&](int v) -> Value* {return object({{"width",VALUE(gse::value::Int,,v)}});};
    auto* parent=AsObject(Call(GSE_CALL,wrap,"class",{
        VALUE(gse::value::String,,"modifier-test"),
        object({{"width",VALUE(gse::value::Int,,10)},
            {"height",VALUE(gse::value::Int,,20)},{"_hover",props(15)},{"_active",props(20)}})
    }));
    std::string style="modifier-test";
    if(inherited) {
        Call(GSE_CALL,wrap,"class",{VALUE(gse::value::String,,"modifier-child"),VALUE(gse::value::String,,style)});
        style="modifier-child";
    }
    auto* root=AsObject(wrap->value.at("root"));
    const auto make=[&](bool manual) {
        value::object_properties_t properties{{"class",VALUE(gse::value::String,,style)}};
        if(manual) {properties.emplace("width",VALUE(gse::value::Int,,35));}
        return AsObject(Call(GSE_CALL,root,"area",{object(properties)}));
    };
    auto* idle=make(false);auto* active=make(false);auto* manual=make(true);
    auto* node=::ui::dom::Object::Unwrap(active);
    node->AddModifier(GSE_CALL,::ui::CM_HOVER);node->AddModifier(GSE_CALL,::ui::CM_ACTIVE);
    ::ui::dom::Object::Unwrap(manual)->AddModifier(GSE_CALL,::ui::CM_ACTIVE);
    GT_ASSERT(Width(GSE_CALL,idle)==10 && Width(GSE_CALL,active)==20 && Width(GSE_CALL,manual)==35,"initial widths: " + std::to_string(Width(GSE_CALL,idle)) + "," + std::to_string(Width(GSE_CALL,active)) + "," + std::to_string(Width(GSE_CALL,manual)));
    Call(GSE_CALL,parent,"set",{object({{"_active",props(22)}})});
    GT_ASSERT(Width(GSE_CALL,idle)==10 && Width(GSE_CALL,active)==22 && Width(GSE_CALL,manual)==35,"reapplication ignored live modifier priority or overwrote a manual value");
    Call(GSE_CALL,parent,"set",{object({{"_hover",props(16)}})});
    GT_ASSERT(Width(GSE_CALL,active)==22,"hover refresh overwrote the higher-priority active style");
    Call(GSE_CALL,parent,"unset",{VALUE(gse::value::Array,,value::array_elements_t{VALUE(gse::value::String,,"_active")})});
    GT_ASSERT(Width(GSE_CALL,active)==16 && Width(GSE_CALL,manual)==35,"modifier removal did not restore hover/manual fallback");
    Call(GSE_CALL,parent,"set",{object({{"_active",props(24)}})});
    GT_ASSERT(Width(GSE_CALL,active)==24,"removed active payload could not be reapplied");
    node->RemoveModifier(GSE_CALL,::ui::CM_ACTIVE);node->RemoveModifier(GSE_CALL,::ui::CM_HOVER);
    GT_ASSERT(Width(GSE_CALL,active)==10,"removing state did not restore the base style");
    if(!inherited) {
        auto* early=AsObject(Call(GSE_CALL,wrap,"class",{
            VALUE(gse::value::String,,"cascade-early"),
            object({{"width",VALUE(gse::value::Int,,10)},{"height",VALUE(gse::value::Int,,20)},
                {"_hover",props(16)},{"_active",props(20)}})
        }));
        auto* late=AsObject(Call(GSE_CALL,wrap,"class",{
            VALUE(gse::value::String,,"cascade-late"),
            object({{"width",VALUE(gse::value::Int,,12)},{"_hover",props(18)},{"_active",props(28)}})
        }));
        auto* both=AsObject(Call(GSE_CALL,root,"area",{object({{"class",VALUE(gse::value::String,,"cascade-early cascade-late")}})}));
        auto* both_node=::ui::dom::Object::Unwrap(both);
        GT_ASSERT(Width(GSE_CALL,both)==12,"later base class did not win");
        both_node->AddModifier(GSE_CALL,::ui::CM_HOVER);
        both_node->AddModifier(GSE_CALL,::ui::CM_ACTIVE);
        GT_ASSERT(Width(GSE_CALL,both)==28,"later active class did not win");
        Call(GSE_CALL,early,"set",{object({{"_active",props(22)}})});
        GT_ASSERT(Width(GSE_CALL,both)==28,"earlier active refresh overrode later active class");
        Call(GSE_CALL,late,"set",{object({{"_active",props(30)}})});
        GT_ASSERT(Width(GSE_CALL,both)==30,"later active refresh was ignored");
        Call(GSE_CALL,late,"unset",{VALUE(gse::value::Array,,value::array_elements_t{VALUE(gse::value::String,,"_active")})});
        GT_ASSERT(Width(GSE_CALL,both)==22,"later base overrode earlier active fallback");
        Call(GSE_CALL,late,"set",{object({{"_hover",props(36)}})});
        GT_ASSERT(Width(GSE_CALL,both)==22,"later hover refresh overrode earlier active class");
        both_node->RemoveModifier(GSE_CALL,::ui::CM_ACTIVE);
        GT_ASSERT(Width(GSE_CALL,both)==36,"active removal did not restore later hover");
        both_node->RemoveModifier(GSE_CALL,::ui::CM_HOVER);
        GT_ASSERT(Width(GSE_CALL,both)==12,"hover removal did not restore later base");
    }
    GT_OK();
}
}
void AddUIClassTests(task::gsetests::GSETests* task) {
    task->AddTest("UI input narrow byte limits clamp defaults assignments and keyboard edits",GT(){
        return WithWidgetGraphics([&]() -> std::string {
            GSE runtime;std::string result;
            auto* gc_space=runtime.GetGCSpace();
            try {gc_space->Accumulate(&runtime,[&] {
                auto* ctx=runtime.CreateGlobalContext();ExecutionPointer ep;const si_t si{};
                auto* gui=new ::ui::UI(GSE_CALL);
                util::FinallyGuard cleanup([&]{gui->GetRoot()->Container::Destroy(GSE_CALL);});
                result=[&]() -> std::string {
                    auto* root=AsObject(AsObject(gui->Wrap(GSE_CALL))->value.at("root"));
                    auto* wrapped=AsObject(Call(GSE_CALL,root,"input",{VALUEEXT(value::Object,GSE_CALL,(value::object_properties_t{
                        {"value",VALUE(value::String,,std::string(127,'a'))},{"max_bytes",VALUE(value::Int,,23)}
                    }))}));
                    auto* node=static_cast<::ui::dom::Input*>(::ui::dom::Object::Unwrap(wrapped));
                    const auto text=[&]{return static_cast<value::String*>(AsObject(node->Wrap(GSE_CALL))->value.at("value"))->value;};
                    GT_ASSERT(text()==std::string(23,'a'),"default was not truncated during control construction");
                    node->WrapSet("value",VALUE(value::String,,std::string(90,'a')),GSE_CALL);
                    GT_ASSERT(text()==std::string(23,'a'),"repeated oversized assignment leaked raw text into the public value");
                    node->OnFocus();input::Event key;key.SetType(input::EV_KEY_DOWN);key.data.key.is_printable=true;key.data.key.key='b';
                    node->ProcessEvent(GSE_CALL,key);GT_ASSERT(text()==std::string(23,'a'),"full control accepted another printable byte");
                    key.data.key.is_printable=false;key.data.key.code=input::K_BACKSPACE;node->ProcessEvent(GSE_CALL,key);
                    key.data.key.is_printable=true;key.data.key.code=input::K_NONE;node->ProcessEvent(GSE_CALL,key);
                    GT_ASSERT(text()==std::string(22,'a')+"b","backspace did not free one byte for insertion");
                    node->WrapSet("value",VALUE(value::String,,std::string("ab\0tail",7)),GSE_CALL);
                    GT_ASSERT(text()=="ab","bounded narrow default did not stop at its first NUL");
                    node->WrapSet("max_bytes",VALUE(value::Int,,39),GSE_CALL);
                    node->WrapSet("value",VALUE(value::String,,std::string(80,'f')),GSE_CALL);
                    GT_ASSERT(text().size()==39,"Formal-size control did not enforce its own limit");
                    node->WrapSet("max_bytes",VALUE(value::Int,,23),GSE_CALL);
                    GT_ASSERT(text().size()==23,"lowering the limit did not trim current text");
                    node->WrapSet("max_bytes",VALUE(value::Int,,0),GSE_CALL);
                    node->WrapSet("value",VALUE(value::String,,std::string(80,'u')),GSE_CALL);
                    GT_ASSERT(text().size()==80,"unbounded legacy control was unexpectedly limited");
                    GT_OK();
                }();
            });}catch(const std::exception& error){result=std::string("Input byte limit check threw: ")+error.what();}
            return result;
        });
    });
    for(bool inherited:{false,true}) {
        task->AddTest(inherited ? "UI class modifier refresh preserves inherited live styles" : "UI class modifier refresh preserves live and manual styles",GT(inherited){
            return WithWidgetGraphics([&]() -> std::string {
                GSE runtime;
                std::string result;
                auto* gc_space=runtime.GetGCSpace();
                try {
                    gc_space->Accumulate(&runtime,[&] {
                    auto* ctx=runtime.CreateGlobalContext();
                    ExecutionPointer ep;const si_t si{};
                    auto* gui=new ::ui::UI(GSE_CALL);
                    util::FinallyGuard cleanup([&] {gui->GetRoot()->Container::Destroy(GSE_CALL);});
                    result=Exercise(GSE_CALL,*gui,inherited);
                    });
                } catch (const std::exception& error) {
                    result=std::string("UI class refresh threw: ")+error.what();
                }
                return result;
            });
        });
    }
}
}
