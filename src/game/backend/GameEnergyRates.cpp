#include "Game.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "State.h"
#include "gc/Space.h"
#include "gse/value/Bool.h"
#include "gse/value/Array.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <type_traits>
#include <stdexcept>

namespace game::backend {
namespace {
std::string EnergyRateState(const Game& game,const base::Base& anchor,const base::SmacEnergyRates* rates=nullptr) {
    const auto owner=anchor.GetSmacResourceContext().owner_simulation_id;
    types::Buffer token;token.WriteInt(owner);token.WriteInt(game.GetTurnId());
    auto economy=game.GetCanonicalEconomy(anchor.m_owner);if(rates) {economy.rates=*rates;}
    token.WriteString(economy.Serialize().ToString());return token.ToString();
}
using EnergyPreview=std::vector<std::pair<base::Base*,base::SmacBaseEnergyCalculation>>;
EnergyPreview PrepareEnergyRates(const Game& game,const base::Base& anchor,base::SmacEnergyRates rates) {
    rates.Validate();EnergyPreview result;
    auto* bm=game.GetBM();
    for(const auto id:bm->GetBaseRecordOrder()) {
        auto* base=bm->GetBase(id);
        if(!base) {throw std::runtime_error("Energy rate preview requires the current Base record order");}
        if(base->m_owner!=anchor.m_owner) {continue;}
        result.emplace_back(base,game.PreviewCanonicalBaseEnergy(*base,rates));
    }
    return result;
}
}
base::SmacBaseEnergyCalculation Game::PreviewCanonicalBaseEnergy(const base::Base& base,base::SmacEnergyRates rates) const {
    rates.Validate();
    if(base.m_retained_population) {
        const auto allocation=base.CalculateSmacWorkers(*base.m_allocation,base.GetSmacResourceContext());
        const auto resources=base.CalculateSmacResourceCalculation(base::SmacBaseResourcePhase::Ordinary,&allocation);
        return CalculateCanonicalBaseEnergy(base,resources,&rates,&allocation);
    }
    return CalculateCanonicalBaseEnergy(base,base.PrepareSmacResourceCalculation(),&rates);
}

gse::Value* Game::CopySmacEnergyRatePreview(GSE_CALLABLE,const base::Base& anchor,std::optional<base::SmacEnergyRates> candidate) const {
    anchor.RequireSmacProductionSelection();const auto saved=GetCanonicalEconomy(anchor.m_owner);
    const auto rates=candidate ? *candidate : saved.rates;const auto results=PrepareEnergyRates(*this,anchor,rates);
    gse::value::array_elements_t bases;
    for(const auto& [base,energy]:results) {
        bases.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"id",VALUE(gse::value::Int,,base->m_id)}, {"name",VALUE(gse::value::String,,base->m_name)},
            {"economy",VALUE(gse::value::Int,,energy.tail.final.economy)}, {"psych",VALUE(gse::value::Int,,energy.tail.final.psych)},
            {"labs",VALUE(gse::value::Int,,energy.tail.final.labs)}, {"maintenance",VALUE(gse::value::Int,,energy.maintenance)}
        })));
    }
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"expected_state",VALUE(gse::value::String,,EnergyRateState(*this,anchor))},
        {"psych",VALUE(gse::value::Int,,rates.psych)}, {"labs",VALUE(gse::value::Int,,rates.labs)},
        {"economy",VALUE(gse::value::Int,,10-rates.psych-rates.labs)}, {"bases",VALUE(gse::value::Array,,bases)}
    }));
}

void Game::ApplySmacEnergyRates(GSE_CALLABLE,base::Base& anchor,base::SmacEnergyRates rates,bool apply) {
    rates.Validate();const auto roles=anchor.SpecialistSelectionRoles();
    auto& saved=m_simulation->value.economy.at(anchor.GetSmacResourceContext().owner_simulation_id);
    auto candidate=saved;candidate.rates=rates;candidate.Validate();
    struct Row {
        base::Base* base;
        size_t frame_index;
        base::SmacBaseAllocationState allocation;
        int64_t food;
        std::optional<base::SmacBaseUpkeepCache> cache;
    };
    GetSmacWorkerAvailability(anchor); // authenticate saved knowledge before dereferencing it
    auto frame=ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,anchor.GetSmacResourceContext().owner_simulation_id).CopyData();
    std::map<size_t,size_t> indices;size_t index=0;
    for(const auto& row:m_bm->GetBases()) {indices.emplace(row.first,index++);}
    std::vector<Row> rows;
    for(const auto id:m_bm->GetBaseRecordOrder()) {
        auto* base=m_bm->GetBase(id);
        if(!base) {THROW("Energy recalculation requires the current global Base record order");}
        if(base->m_owner!=anchor.m_owner) {continue;}
        base->RequireSmacProductionSelection();
        rows.push_back({base,indices.at(id),*base->GetSmacAllocation(),base->m_accumulated_nutrients,{}});
    }
    if(rows.empty()) {THROW("Energy recalculation requires an owned Base");}
    // Two actual ordered forced passes. Detached availability includes every
    // preceding candidate assignment without publishing transient live Pops.
    for(int pass=0;pass<2;++pass) {
        for(auto& row:rows) {
            const auto availability=map::SmacWorkerMapFrame::Create(frame).Evaluate(row.frame_index);
            row.allocation=row.base->CalculateSmacWorkers(row.allocation,row.base->GetSmacResourceContext(),
                base::SmacBaseResourcePhase::Ordinary,{},nullptr,nullptr,{},nullptr,availability.unavailable);
            frame.bases.at(row.frame_index).selected_radius=row.allocation.selected&map::SmacWorkerMapFrame::RADIUS_MASK;
            const auto resources=row.base->CalculateSmacResourceCalculation(base::SmacBaseResourcePhase::Ordinary,&row.allocation,{},row.food);
            const auto support=CalculateCanonicalUnitSupport(*row.base,row.base->GetPopulation(),nullptr);
            CalculateCanonicalBaseEnergy(*row.base,resources,&rates,&row.allocation);
            row.food=resources.nutrients.normalized_stored;
            row.cache=base::SmacBaseUpkeepCache{resources,support};
        }
    }
    if(!apply) {return;}
    std::optional<base::Base::PreparedSmacWorkerState> batch;
    for(auto& row:rows) {
        auto prepared=row.base->PrepareSmacWorkerState(GSE_CALL,row.allocation,roles,
            row.base->GetSmacResourceContext(),row.base->m_production,row.food,{}, {},nullptr,{},false,batch ? &*batch : nullptr);
        batch.emplace(std::move(prepared));
    }
    static_assert(std::is_nothrow_swappable_v<decltype(candidate)>);
    static_assert(std::is_nothrow_swappable_v<std::optional<base::SmacBaseUpkeepCache>>);
    batch->Commit();std::swap(saved,candidate);
    for(auto& row:rows) {row.base->m_smac_upkeep_cache.swap(row.cache);}
    batch->Finish();
}

bool Game::AllowsSmacEnergyRateEvent(const std::string& name) const {
    return !m_energy_rate_session || name=="open_smac_energy_rates" || name=="adjust_smac_energy_rates" || name=="close_smac_energy_rates";
}
void Game::NotifySmacEnergyRates(GSE_CALLABLE,int64_t id,const base::Base* anchor) {
    m_state->TriggerObject(this,"smac_energy_rates_menu",ARGS_F(this,id,anchor) {
        {"request_id",VALUE(gse::value::Int,,id)}, {"closed",VALUE(gse::value::Bool,,anchor==nullptr)},
        {"selection",anchor ? CopySmacEnergyRatePreview(GSE_CALL,*anchor,{}) : VALUE(gse::value::Undefined)}
    };});
}

bool Game::HandleSmacEnergyRatesEvent(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool open=name=="open_smac_energy_rates",adjust=name=="adjust_smac_energy_rates",close=name=="close_smac_energy_rates";
    if(!open && !adjust && !close && name!="set_smac_energy_rates") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed energy-rate command");}
        return found->second->Deref();
    };
    if(caller!=m_slot_num) {THROW("Energy rates require the authenticated local human owner");}
    if(adjust || close) {
        if(args.size()!=(adjust ? 3u : 2u)) {THROW("Energy edit requires its native request and button/locks or close choice");}
        const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
        if(request<1) {THROW("Invalid energy allocation request identity");}
        // Retired and superseded request IDs own no Base pointer or rollback.
        if(!m_energy_rate_session || m_energy_rate_session->id!=request) {return true;}
        base::Base* anchor=nullptr;
        for(const auto id:m_bm->GetBaseRecordOrder()) {
            auto* candidate=m_bm->GetBase(id);
            if(candidate && candidate->GetSmacResourceContext().owner_simulation_id==m_energy_rate_session->owner) {anchor=candidate;break;}
        }
        if(!anchor || m_energy_rate_session->turn!=GetTurnId() || m_energy_rate_session->expected!=EnergyRateState(*this,*anchor)) {
            if(apply) {m_energy_rate_session.reset();NotifySmacEnergyRates(GSE_CALL,request,nullptr);}return true;
        }
        anchor->RequireSmacProductionSelection();
        if(apply) {
            CheckRW(GSE_CALL);
            if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Energy edit requires its owning accumulation");}
        }
        if(close) {
            const bool cancel=static_cast<const gse::value::Bool*>(field("cancel",gse::VT_BOOL))->value;
            if(cancel) {ApplySmacEnergyRates(GSE_CALL,*anchor,m_energy_rate_session->opening,apply);}
            if(apply) {m_energy_rate_session.reset();NotifySmacEnergyRates(GSE_CALL,request,nullptr);}return true;
        }
        const auto button=static_cast<const gse::value::Int*>(field("button",gse::VT_INT))->value;
        const auto locks=static_cast<const gse::value::Int*>(field("locks",gse::VT_INT))->value;
        if(button<29 || button>34 || locks<0 || locks>7) {THROW("Invalid energy allocation button or locks");}
        const auto rates=base::AdjustSmacEnergyRates(GetCanonicalEconomy(anchor->m_owner).rates,static_cast<int32_t>(button),static_cast<uint32_t>(locks));
        auto next=EnergyRateState(*this,*anchor,&rates);
        ApplySmacEnergyRates(GSE_CALL,*anchor,rates,apply);
        if(apply) {m_energy_rate_session->expected.swap(next);NotifySmacEnergyRates(GSE_CALL,request,anchor);}return true;
    }
    if(args.size()!=(open ? 2u : 4u)) {THROW("Energy rates require Base and expected faction state");}
    const auto target=args.find("base");
    const auto* object=target!=args.end() && target->second ? dynamic_cast<const gse::value::Object*>(target->second->Deref()) : nullptr;
    if(!object || object->object_class!=base::Base::WRAP_CLASS || caller!=m_slot_num) {THROW("Energy rates require an actual local Base");}
    const auto id=object->value.find("id");
    if(id==object->value.end() || !id->second || id->second->type!=gse::VT_INT) {THROW("Energy rates require a stable Base identity");}
    const auto number=static_cast<const gse::value::Int*>(id->second)->value;
    if(number<0 || static_cast<uint64_t>(number)>SIZE_MAX) {THROW("Invalid energy-rate Base identity");}
    auto* anchor=m_bm->GetBase(static_cast<size_t>(number));if(!anchor || object->wrapobj!=anchor) {return true;}
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    if(expected.size()>2*1024*1024) {THROW("Energy allocation state exceeds its bound");}
    anchor->RequireSmacProductionSelection();
    if(apply) {
        CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Energy rates require the owning writable accumulation");}
    }
    if(expected!=EnergyRateState(*this,*anchor)) {return true;}
    // Existing ordinary command fences retain ownership of required questions.
    // The optional rates window yields to existing required-choice authority.
    if(m_canonical_continuation) {THROW("Energy rates are unavailable while a required Base choice is pending");}
    if(open) {
        if(m_energy_rate_session) {
            if(apply && m_energy_rate_session->owner==anchor->GetSmacResourceContext().owner_simulation_id && m_energy_rate_session->turn==GetTurnId()) {
                NotifySmacEnergyRates(GSE_CALL,m_energy_rate_session->id,anchor);
            }
            return true;
        }
        {std::lock_guard lock(m_admission_mutex);if(m_campaign_save) {return true;}}
        ApplySmacEnergyRates(GSE_CALL,*anchor,GetCanonicalEconomy(anchor->m_owner).rates,false);
        if(m_next_energy_rate_session==INT64_MAX) {THROW("Energy allocation request identities exhausted");}
        if(apply) {
            m_energy_rate_session=EnergyRateSession{m_next_energy_rate_session,anchor->GetSmacResourceContext().owner_simulation_id,
                GetTurnId(),GetCanonicalEconomy(anchor->m_owner).rates,expected};
            ++m_next_energy_rate_session;
            try {NotifySmacEnergyRates(GSE_CALL,m_energy_rate_session->id,anchor);}
            catch(...) {m_energy_rate_session.reset();throw;}
        }
        return true;
    }
    const auto psych=static_cast<const gse::value::Int*>(field("psych",gse::VT_INT))->value;
    const auto labs=static_cast<const gse::value::Int*>(field("labs",gse::VT_INT))->value;
    if(psych<0 || labs<0 || psych>10 || labs>10-psych) {THROW("Energy rates require nonnegative tenths totaling at most ten");}
    const base::SmacEnergyRates rates{static_cast<int32_t>(psych),static_cast<int32_t>(labs)};
    ApplySmacEnergyRates(GSE_CALL,*anchor,rates,apply);
    return true;
}
}
