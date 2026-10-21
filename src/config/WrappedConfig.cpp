#include "WrappedConfig.h"

#include "util/ConfigManager.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"

namespace config {
namespace wrapped {
namespace {
gse::Value* PackPreferences(GSE_CALLABLE,const smac_preferences::Words& words) {
    const auto draft=smac_preferences::Decode(words);
    gse::value::object_properties_t values,audio;
    for(const auto& b:smac_preferences::Bindings) {values.emplace(std::to_string(b.id),VALUE(gse::value::Bool,,draft.checked[b.id]));}
    for(const auto& b:smac_preferences::Audio) {audio.emplace(std::to_string(b.id),VALUE(gse::value::Bool,,draft.audio[b.id]));}
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"values",VALUE(gse::value::Object,,GSE_CALL_NOGC,values)}, {"audio",VALUE(gse::value::Object,,GSE_CALL_NOGC,audio)}
    });
}
}

Config::Config( util::ConfigManager* const manager )
	: m_manager( manager ) {}

Config::~Config() {m_alive->store(false);}

WRAPIMPL_BEGIN( Config )
	WRAPIMPL_PROPS};
	for ( const auto& it : m_manager->GetSettings() ) {
		properties.insert( { it.first, VALUE( gse::value::String, , it.second.second ) } );
	}
    properties.emplace("get_automatic_unit_designs",NATIVE_CALL(manager=m_manager,alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(0);
        // Original preference bit0x4000 is enabled in every fresh-store variant.
        return VALUE(gse::value::Bool,,bool(manager->GetSmacPreferences(false,2)[0]&0x4000u));
    }));
    properties.emplace("set_automatic_unit_designs",NATIVE_CALL(manager=m_manager,alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(1);N_GETVALUE(enabled,0,Bool);
        try {
            std::map<std::string,std::string> updates{{"automatic-unit-designs",enabled?"true":"false"}};
            if(manager->GetSettings().count("smac-preferences")) {
                auto words=manager->GetSmacPreferences(false,2);
                if(enabled) {words[0]|=0x4000u;}else {words[0]&=~0x4000u;}
                updates.emplace("smac-preferences",smac_preferences::Store(words));
            }
            manager->UpdateSettings(updates);
        }
        catch(const std::exception& error) {GSE_ERROR(gse::EC.INVALID_CALL,std::string("Cannot save preferences: ")+error.what());}
        return VALUE(gse::value::Undefined);
    }));
    properties.emplace("get_smac_preferences",NATIVE_CALL(manager=m_manager,alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(2);N_GETVALUE(laptop,0,Bool);N_GETVALUE(difficulty,1,Int);
        if(difficulty<0 || difficulty>5) {GSE_ERROR(gse::EC.INVALID_CALL,"Preferences require difficulty0..5");}
        return PackPreferences(GSE_CALL,manager->GetSmacPreferences(laptop,int(difficulty)));
    }));
    properties.emplace("get_smac_preference_defaults",NATIVE_CALL(alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(1);N_GETVALUE(laptop,0,Bool);
        return PackPreferences(GSE_CALL,smac_preferences::Defaults(laptop,2,true));
    }));
    properties.emplace("set_smac_preferences",NATIVE_CALL(manager=m_manager,alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(2);N_GET(values,0,Object);N_GET(audio,1,Object);
        if(values->value.size()!=67 || audio->value.size()!=3) {GSE_ERROR(gse::EC.INVALID_CALL,"Preferences require all67 controls and3 audio states");}
        const auto checked=[&](gse::value::Object* object,int id) {
            const auto* value=object->Get(std::to_string(id))->Deref();
            if(value->type!=gse::VT_BOOL) {GSE_ERROR(gse::EC.INVALID_CALL,"Preference controls must be Boolean");}
            return static_cast<const gse::value::Bool*>(value)->value;
        };
        smac_preferences::Draft draft;
        for(const auto& b:smac_preferences::Bindings) {draft.checked[b.id]=checked(values,b.id);}
        for(const auto& b:smac_preferences::Audio) {draft.audio[b.id]=checked(audio,b.id);}
        const auto words=smac_preferences::Encode(draft);
        try {manager->UpdateSettings({{"smac-preferences",smac_preferences::Store(words)},
            {"automatic-unit-designs",bool(words[0]&0x4000u)?"true":"false"}});}
        catch(const std::exception& error) {GSE_ERROR(gse::EC.INVALID_CALL,std::string("Cannot save preferences: ")+error.what());}
        return VALUE(gse::value::Undefined);
    }));
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Config )

}
}
