#include "Base.h"
#include "BaseManager.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/unit/Def.h"
#include "game/backend/unit/SmacProductionDesigns.h"
#include <algorithm>
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <set>
#include <type_traits>

namespace game::backend {
bool Game::HandleSmacQueueEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="edit_smac_queue") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed production queue command");}
        return found->second->Deref();
    };
    if(args.size()!=3) {THROW("Queue edit requires Base, expected state and entries");}
    const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
    if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Queue edit requires an actual Base");}
    auto* base=base::Base::Unwrap(args.at("base"));
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    const auto& entries=static_cast<const gse::value::Array*>(field("entries",gse::VT_ARRAY))->value;
    if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>2*1024*1024
        || entries.size()>base::ProductionState::MAX_QUEUE_ITEMS) {THROW("Queue edit has invalid authority or budget");}
    base->RequireSmacProductionSelection();
    if(apply) {
        CheckRW(GSE_CALL);
        if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Queue edit requires its owning accumulation");}
    }
    // Never install caller snapshots. Original positions are one-use tokens:
    // retaining an unavailable item must not allow extra copies of that item.
    if(base->SmacSpecialistSelectionState()!=expected) {return true;}
    auto replacement=base->GetProductionState();auto& queue=*replacement->queue;
    const auto original=queue;queue.clear();std::set<size_t> retained;
    const auto choices=base->GetSmacProductionChoices();
    for(auto* value:entries) {
        if(!value || value->Deref()->type!=gse::VT_OBJECT) {THROW("Queue entry requires one original position or source identity");}
        const auto& entry=static_cast<const gse::value::Object*>(value->Deref())->value;
        if(entry.size()!=1 || !entry.begin()->second || entry.begin()->second->Deref()->type!=gse::VT_INT) {THROW("Malformed queue entry");}
        const auto number=static_cast<const gse::value::Int*>(entry.begin()->second->Deref())->value;
        if(entry.begin()->first=="existing") {
            if(number<0 || static_cast<uint64_t>(number)>=original.size() || !retained.insert(static_cast<size_t>(number)).second) {THROW("Queue original position is invalid or reused");}
            queue.push_back(original.at(static_cast<size_t>(number)));
        } else if(entry.begin()->first=="source_id") {
            const auto choice=std::find_if(choices.begin(),choices.end(),[&](const auto& item){return item.source_id==number;});
            if(choice==choices.end()) {THROW("New queue item is not currently eligible");}
            queue.push_back(MakeSmacProductionItem(base->m_owner,number<0 ? base::ProductionItemKind::Facility : base::ProductionItemKind::Unit,
                static_cast<int32_t>(number<0 ? -number : number)));
        } else {THROW("Unknown queue entry identity");}
    }
    ValidateSmacProductionState(base->m_owner,*replacement);
    if(!apply || replacement==base->m_production) {return true;}
    auto references=base->PrepareSmacProductionReferences(replacement);auto* reference_target=base->SmacProductionReferencesTarget();
    auto updates=m_bm->m_base_updates; base::BaseManager::QueueBaseUpdateInto(updates,base,base::BaseManager::BUO_REFRESH);
    std::unique_lock lock(m_bm->m_updated_bases_mutex);auto triggers=m_bm->m_updated_bases;triggers.insert(base);
    static_assert(std::is_nothrow_swappable_v<decltype(replacement)>);
    base->m_production.swap(replacement);if(reference_target) {reference_target->swap(references);}m_bm->m_base_updates.swap(updates);m_bm->m_updated_bases.swap(triggers);
    return true;
}
namespace base {
gse::Value* Base::CopySmacQueueEditor(GSE_CALLABLE) const {
    RequireSmacProductionSelection();
    const auto choice_values=CopySmacProductionChoices(GSE_CALL);
    auto* choices=static_cast<gse::value::Object*>(choice_values);
    // Labels use the actual presentation provider, including retained items that
    // are no longer eligible. Queue edits never reinterpret their opaque IDs.
    gse::value::array_elements_t items;
    const auto& rules=m_game->GetState()->m_settings.global;
    for(size_t i=0;i<m_production->queue->size();++i) {
        const auto& item=m_production->queue->at(i);const auto id=*item.smac_source_id;
        const auto label=item.kind==ProductionItemKind::Unit ? (m_game->m_simulation && m_game->m_simulation->value.designs.native_registry
            ? *unit::ReadSmacProductionDesign(m_game->m_simulation->value.designs.native_registry->at(id),id).name
            : m_game->ResolveSmacProductionPresentation(m_owner,id)->m_name)
            : rules.GetSmacProductionRules()->facilities.at(id-1).name;
        items.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"existing",VALUE(gse::value::Int,,i)}, {"name",VALUE(gse::value::String,,label)}})));
    }
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"expected_state",VALUE(gse::value::String,,SmacSpecialistSelectionState())},
        {"items",VALUE(gse::value::Array,,items)}, {"choices",choices->Get("choices")},
        {"limit",VALUE(gse::value::Int,,ProductionState::MAX_QUEUE_ITEMS)}}));
}
}
}
