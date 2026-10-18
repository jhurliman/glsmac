#include "Base.h"
#include "BaseManager.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slot.h"
#include "gc/Space.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include <algorithm>
#include <limits>
#include <type_traits>

namespace game::backend {
bool Game::HandleSmacRenameEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool open=name=="open_smac_rename",cancel=name=="cancel_smac_rename";
    if(!open && !cancel && name!="commit_smac_rename") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed Base Rename command");}
        return found->second->Deref();
    };
    if(args.size()!=(open ? 2u : cancel ? 3u : 4u)) {THROW("Base Rename command has unexpected fields");}
    const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
    if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Base Rename requires an actual Base");}
    auto* base=base::Base::Unwrap(args.at("base"));
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>2*1024*1024) {THROW("Base Rename has invalid authority or state");}
    if(!base->CanSmacRename()) {THROW("Rename unavailable for a foreign Base or captured-AI diplomatic consequences");}
    base->RequireSmacSpecialistSelection();
    if(apply) {
        CheckRW(GSE_CALL);
        if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Base Rename requires its owning accumulation");}
    }
    if(open) {
        if(base->SmacSpecialistSelectionState()!=expected) {return true;}
        base->OpenSmacRename(GSE_CALL,apply);return true;
    }
    const auto id=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
    if(id<1) {THROW("Invalid Base Rename request identity");}
    const auto& pending=base->m_rename_request;
    if(!pending || pending->id!=id || pending->expected!=expected) {return true;}
    if(cancel) {if(apply) {base->m_rename_request.reset();}return true;}
    const auto& supplied=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
    if(supplied.size()>2*1024*1024) {THROW("Base Rename text exceeds its port command budget");}
    if(base->SmacSpecialistSelectionState()!=expected) {if(apply) {base->m_rename_request.reset();}return true;}
    // Original dialog admission: narrow bytes, first NUL, at most24 data bytes.
    // Current renderer only supports printable ASCII; reject unsupported display
    // bytes rather than publishing a name that asserts in the map-label renderer.
    auto accepted=supplied.substr(0,std::min(size_t(24),supplied.find('\0')));
    for(const unsigned char c:accepted) {if(c<32 || c>126) {THROW("Base Rename currently supports printable ASCII display bytes only");}}
    if(apply) {m_bm->RenameSmacBase(GSE_CALL,*base,std::move(accepted));base->m_rename_request.reset();}
    return true;
}
namespace base {
bool Base::CanSmacRename() const {
    if(!m_resource_context || !m_allocation || !m_production || !m_owner || !m_game->GetPlayer()
        || m_owner->GetPlayer()!=m_game->GetPlayer() || m_game->GetBM()->GetBase(m_id)!=this || !m_game->m_simulation) {return false;}
    if(!m_resource_context->conquest) {return true;}
    const auto former=m_resource_context->conquest->former_owner;
    return former==m_resource_context->owner_simulation_id || (m_game->m_simulation->value.human_membership & (uint8_t(1)<<former));
}
void Base::OpenSmacRename(GSE_CALLABLE,bool apply) {
    RequireSmacSpecialistSelection();
    if(!CanSmacRename()) {THROW("Base Rename has unadmitted captured-AI consequences");}
    if(m_next_rename_request==INT64_MAX) {THROW("Base Rename request identity exhausted");}
    const auto context=GetSmacResourceContext();const auto preliminary=CalculateSmacWorkers(*m_allocation,context);
    if(!apply) {return;}
    PublishSmacSpecialistState(GSE_CALL,preliminary,context);
    RenameRequest request{m_next_rename_request,SmacSpecialistSelectionState()};m_rename_request=std::move(request);++m_next_rename_request;
    m_game->GetState()->TriggerObject(m_game,"smac_rename_menu",ARGS_F(this) {
        {"base",Wrap(GSE_CALL)}, {"selection",CopySmacRenameChoices(GSE_CALL)}
    };});
}
gse::Value* Base::CopySmacRenameChoices(GSE_CALLABLE) const {
    if(!m_rename_request) {THROW("Base Rename view requires its native request");}
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"request_id",VALUE(gse::value::Int,,m_rename_request->id)},
        {"expected_state",VALUE(gse::value::String,,m_rename_request->expected)},
        {"name",VALUE(gse::value::String,,m_name)}, {"max_bytes",VALUE(gse::value::Int,,24)}
    }));
}
void BaseManager::RenameSmacBase(GSE_CALLABLE,Base& base,std::string name) {
    m_game->CheckRW(GSE_CALL);RequireSmacWorkerRecalculation();
    if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating() || GetBase(base.m_id)!=&base || !base.CanSmacRename()) {THROW("Base Rename requires current owned registry authority");}
    auto names=m_registered_base_names;auto updates=m_base_updates;
    const bool old_retained=std::any_of(m_bases.begin(),m_bases.end(),[&](const auto& entry){return entry.second!=&base && entry.second->m_name==base.m_name;});
    if(!old_retained) {names.erase(base.m_name);}names.insert(name);
    QueueBaseUpdateInto(updates,&base,BUO_REFRESH);
    std::lock_guard guard(m_updated_bases_mutex);auto triggers=m_updated_bases;triggers.insert(&base);
    static_assert(std::is_nothrow_swappable_v<decltype(names)> && std::is_nothrow_swappable_v<decltype(updates)> && std::is_nothrow_swappable_v<decltype(triggers)>);
    base.m_name.swap(name);m_registered_base_names.swap(names);m_base_updates.swap(updates);m_updated_bases.swap(triggers);
}
}
}
