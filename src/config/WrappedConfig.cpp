#include "WrappedConfig.h"

#include "util/ConfigManager.h"
#include "gse/value/Bool.h"

namespace config {
namespace wrapped {

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
        const auto& settings=manager->GetSettings();const auto row=settings.find("automatic-unit-designs");
        // Original preference bit0x4000 is enabled in every fresh-store variant.
        return VALUE(gse::value::Bool,,row==settings.end() || row->second.second=="true");
    }));
    properties.emplace("set_automatic_unit_designs",NATIVE_CALL(manager=m_manager,alive=m_alive) {
        if(!alive->load()) {GSE_ERROR(gse::EC.INVALID_CALL,"Configuration is no longer active");}
        N_EXPECT_ARGS(1);N_GETVALUE(enabled,0,Bool);
        try {manager->UpdateSetting("automatic-unit-designs",enabled ? "true":"false");}
        catch(const std::exception& error) {GSE_ERROR(gse::EC.INVALID_CALL,std::string("Cannot save preferences: ")+error.what());}
        return VALUE(gse::value::Undefined);
    }));
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Config )

}
}
