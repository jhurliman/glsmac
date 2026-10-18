#include "Base.h"
#include "BaseManager.h"
#include "PopDef.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "gc/Space.h"
#include "gse/value/Object.h"
#include "gse/value/Array.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/String.h"
#include <limits>
#include <type_traits>

namespace game::backend {
bool Game::HandleSmacSpecialistEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool open=name=="open_smac_specialist";
    const bool cancel=name=="cancel_smac_specialist";
    if(!open && !cancel && name!="select_smac_specialist") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed manual specialist command");}
        return found->second->Deref();
    };
    if(args.size()!=(open ? 3u : cancel ? 4u : 5u)) {THROW("Manual specialist command requires Base, population and expected state");}
    const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
    if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Manual specialist command requires an actual Base");}
    auto* base=base::Base::Unwrap(args.at("base"));
    const auto population=static_cast<const gse::value::Int*>(field("pop_id",gse::VT_INT))->value;
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    if(!base || base->GetGame()!=this || caller!=m_slot_num || population<1
        || static_cast<uint64_t>(population)>std::numeric_limits<size_t>::max() || expected.size()>2*1024*1024) {
        THROW("Manual specialist command has invalid authority or arguments");
    }
    base->RequireSmacSpecialistSelection();
    if(apply) {
        CheckRW(GSE_CALL);
        if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Manual specialist command requires its owning accumulation");}
    }
    // Never decode/install caller snapshots. A selection must also name the
    // native-issued optional view request, not merely guess a current snapshot.
    if(open) {
        if(base->SmacSpecialistSelectionState()!=expected) {return true;}
        base->OpenSmacSpecialistSelection(GSE_CALL,static_cast<size_t>(population),apply);
        return true;
    }
    const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
    if(request<1) {THROW("Invalid manual specialist request identity");}
    const auto& pending=base->m_specialist_request;
    if(!pending || pending->id!=request || pending->pop_id!=static_cast<size_t>(population) || pending->expected!=expected) {return true;}
    if(cancel) {if(apply) {base->m_specialist_request.reset();}return true;}
    const auto type=static_cast<const gse::value::Int*>(field("type",gse::VT_INT))->value;
    if(type< -1 || type>6) {THROW("Manual specialist type is outside Worker/installed catalog");}
    if(base->SmacSpecialistSelectionState()!=expected) {if(apply) {base->m_specialist_request.reset();}return true;}
    base->SelectSmacSpecialist(GSE_CALL,static_cast<size_t>(population),static_cast<int32_t>(type),apply);
    if(apply) {base->m_specialist_request.reset();}
    return true;
}
namespace base {
void Base::RequireSmacSpecialistSelection() const {
    RequireSmacProductionSelection();
    m_game->GetBM()->RequireSmacWorkerRecalculation();
    if(m_pops.empty() || !m_game->HasSmacBasePresentations()) {THROW("Manual specialist selection requires current population/presentation authority");}
    ValidateAllocationProjection(true);
}
std::string Base::SmacSpecialistSelectionState() const {
    types::Buffer state;state.WriteInt(m_game->GetTurnId());state.WriteString(Base::Serialize(this).ToString());return state.ToString();
}
Base::SmacAllocationRoles Base::SpecialistSelectionRoles() const {
    if(!m_game->HasSmacBasePresentations()) {THROW("Manual specialist selection has no admitted population roles");}
    const auto& source=m_game->GetSmacBasePresentations();SmacAllocationRoles result;result.worker=source[0];
    for(size_t i=0;i<7;++i) {result.specialists[i]=source[i+1];}return result;
}
void Base::PublishSmacSpecialistState(GSE_CALLABLE,const SmacBaseAllocationState& allocation,const SmacBaseResourceContext& context) {
    m_game->CheckRW(GSE_CALL);
    if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Manual specialist publication requires its owning accumulation");}
    // Resource recalculation normalizes food but never collects upkeep food,
    // credits or research. Stage the complete allocation/cache before publishing.
    std::optional<SmacBaseUpkeepCache> cache{SmacBaseUpkeepCache{
        CalculateSmacResourceCalculation(SmacBaseResourcePhase::Ordinary,&allocation),
        m_game->CalculateCanonicalUnitSupport(*this,m_pops.size(),nullptr)}};
    auto prepared=PrepareSmacWorkerState(GSE_CALL,allocation,SpecialistSelectionRoles(),context,m_production,cache->calculation.nutrients.normalized_stored);
    static_assert(std::is_nothrow_swappable_v<decltype(cache)>);
    prepared.Commit();m_smac_upkeep_cache.swap(cache);prepared.Finish();
}
void Base::OpenSmacSpecialistSelection(GSE_CALLABLE,size_t pop_id,bool apply) {
    RequireSmacSpecialistSelection();
    if(m_next_specialist_request==INT64_MAX) {THROW("Manual specialist request identity exhausted");}
    if(!m_pops.count(pop_id)) {THROW("Manual specialist population is no longer in this Base");}
    const auto context=GetSmacResourceContext();
    const auto preliminary=CalculateSmacWorkers(*m_allocation,context);
    if(!apply) {return;}
    PublishSmacSpecialistState(GSE_CALL,preliminary,context);
    SpecialistSelectionRequest request{m_next_specialist_request,pop_id,SmacSpecialistSelectionState()};
    m_specialist_request=std::move(request);++m_next_specialist_request;
    // This optional UI question does not suspend a turn. Cancel/same-choice
    // retains the original wrapper's completed preliminary recalculation.
    m_game->GetState()->TriggerObject(m_game,"smac_specialist_menu",ARGS_F(this,pop_id) {
        {"base",Wrap(GSE_CALL)}, {"selection",CopySmacSpecialistChoices(GSE_CALL,pop_id)}
    };});
}
gse::Value* Base::CopySmacSpecialistChoices(GSE_CALLABLE,size_t pop_id) const {
    if(!m_specialist_request || m_specialist_request->pop_id!=pop_id) {THROW("Specialist choices require their native view request");}
    const auto menu=GetSmacSpecialistMenu(pop_id);
    const auto& rules=*m_game->GetState()->m_settings.global.GetSmacSpecialistRules();
    const bool full=!menu.current && m_allocation->specialists.total_count>=16;
    gse::value::array_elements_t choices;
    choices.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"type",VALUE(gse::value::Int,,-1)}, {"name",VALUE(gse::value::String,,"Worker")}})));
    if(!full) {for(uint8_t i=0;i<7;++i) {if(menu.eligible[i]) {
        choices.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"type",VALUE(gse::value::Int,,i)}, {"name",VALUE(gse::value::String,,rules.specialists[i].singular)}})));
    }}}
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"pop_id",VALUE(gse::value::Int,,pop_id)}, {"request_id",VALUE(gse::value::Int,,m_specialist_request->id)},
        {"expected_state",VALUE(gse::value::String,,m_specialist_request->expected)},
        {"current_type",VALUE(gse::value::Int,,menu.current ? static_cast<int32_t>(*menu.current) : -1)},
        {"minimum_size",VALUE(gse::value::Int,,rules.minimum_base_size)},
        {"size_explanation_requested",VALUE(gse::value::Bool,,menu.size_explanation_requested)},
        {"creation_limited",VALUE(gse::value::Bool,,full)}, {"choices",VALUE(gse::value::Array,,choices)}
    }));
}
void Base::SelectSmacSpecialist(GSE_CALLABLE,size_t pop_id,int32_t type,bool apply) {
    RequireSmacSpecialistSelection();
    const auto found=m_pops.find(pop_id);
    if(found==m_pops.end()) {THROW("Manual specialist population is no longer in this Base");}
    const auto menu=GetSmacSpecialistMenu(pop_id);
    const auto current=menu.current ? static_cast<int32_t>(*menu.current) : -1;
    if(type==current) {return;}
    if(type< -1 || type>6 || (type>=0 && !menu.eligible[type])) {THROW("Manual specialist type is not currently eligible");}
    auto next=*m_allocation;auto context=GetSmacResourceContext();
    if(menu.current) {
        if(type== -1) {--next.specialists.total_count;} // Drop last counted slot; do not shift the clicked type.
        else {
            size_t position=0;for(auto it=m_pops.begin();it!=found;++it) {++position;}
            const auto workers=AllocationCoordinates(next).size();
            if(position<workers || position-workers>=static_cast<size_t>(next.specialists.total_count)) {THROW("Manual specialist projection changed");}
            next.specialists.types[position-workers]=static_cast<uint8_t>(type);
        }
    } else {
        if(next.specialists.total_count>=16) {THROW("Live manual specialist creation beyond sixteen stored slots is unavailable");}
        const auto coordinate=found->second.GetWorkedTileCoords();
        if(!coordinate) {THROW("Manual specialist Worker has no current square");}
        const auto* map=m_game->GetMap();
        const SmacWorkerRadiusMap radius{{static_cast<int32_t>(m_tile->coord.x),static_cast<int32_t>(m_tile->coord.y)},static_cast<int32_t>(map->GetWidth()),static_cast<int32_t>(map->GetHeight()),true};
        int32_t remove=0;
        for(int32_t i=1;i<=20;++i) {const auto xy=SmacWorkedSquareCollection::Resolve(radius,i);
            if(xy && static_cast<size_t>((*xy)[0])==coordinate->x && static_cast<size_t>((*xy)[1])==coordinate->y) {remove=i;}}
        if(!remove || !(next.selected&(uint32_t(1)<<remove))) {THROW("Manual specialist Worker square is not selected");}
        next.selected&=~(uint32_t(1)<<remove);next.specialists.types[next.specialists.total_count++]=static_cast<uint8_t>(type);
    }
    context.governor_options&=~SmacBaseResourceContext::MANAGE_CITIZENS;
    next=CalculateSmacWorkers(next,context);
    if(apply) {PublishSmacSpecialistState(GSE_CALL,next,context);}
}
}
}
