#include "Base.h"
#include "BaseManager.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "gc/Space.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include <type_traits>

namespace game::backend {
bool Game::HandleSmacGovernorEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="apply_smac_governor") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed Governor command");}
        return found->second->Deref();
    };
    if(args.size()!=3) {THROW("Governor Apply requires Base, expected state and nineteen selections");}
    const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
    if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Governor Apply requires an actual Base");}
    auto* base=base::Base::Unwrap(args.at("base"));
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    const auto controls=static_cast<const gse::value::Int*>(field("controls",gse::VT_INT))->value;
    if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>2*1024*1024 || controls<0 || controls>0x7ffff) {THROW("Governor Apply has invalid authority or selections");}
    base->RequireSmacProductionSelection();
    if(apply) {
        CheckRW(GSE_CALL);
        if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Governor Apply requires its owning accumulation");}
    }
    if(base->SmacGovernorSelectionState()!=expected) {return true;}
    auto context=base->GetSmacResourceContext();
    context.governor_options=base::SmacBaseResourceContext::GovernorOptions(context.governor_options,static_cast<uint32_t>(controls));
    if(context.GovernorActive() && (context.governor_options&0xc1u)) {
        THROW("Active production, citizen and new-unit automation are not available yet; disable those options or deactivate the Governor");
    }
    if(!apply) {return true;}
    auto& remembered=m_simulation->value.remembered_governor.at(context.owner_simulation_id);
    std::optional<uint32_t> replacement=context.governor_options;
    auto updates=m_bm->m_base_updates;base::BaseManager::QueueBaseUpdateInto(updates,base,base::BaseManager::BUO_REFRESH);
    std::unique_lock lock(m_bm->m_updated_bases_mutex);auto triggers=m_bm->m_updated_bases;triggers.insert(base);
    static_assert(std::is_nothrow_copy_assignable_v<decltype(context)> && std::is_nothrow_swappable_v<decltype(replacement)>);
    // This is the actual local Apply no-active-management branch. Its outer
    // refresh is required even for unchanged settings. No config-only shortcut.
    *base->m_resource_context=context;remembered.swap(replacement);
    m_bm->m_base_updates.swap(updates);m_bm->m_updated_bases.swap(triggers);
    return true;
}
namespace base {
std::string Base::SmacGovernorSelectionState() const {
    RequireSmacProductionSelection();
    types::Buffer result;result.WriteString(SmacSpecialistSelectionState());
    const auto options=m_game->m_simulation->value.remembered_governor.at(GetSmacResourceContext().owner_simulation_id);
    result.WriteBool(options.has_value());if(options) {result.WriteInt(*options);}
    return result.ToString();
}
gse::Value* Base::CopySmacGovernorSettings(GSE_CALLABLE) const {
    RequireSmacProductionSelection();
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"expected_state",VALUE(gse::value::String,,SmacGovernorSelectionState())},
        {"controls",VALUE(gse::value::Int,,SmacBaseResourceContext::GovernorControls(GetSmacResourceContext().governor_options))}
    }));
}
}
}
