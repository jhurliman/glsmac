#include "Game.h"
#include "base/Base.h"
#include "base/BaseManager.h"
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
std::string EnergyRateState(const Game& game,const base::Base& anchor) {
    const auto owner=anchor.GetSmacResourceContext().owner_simulation_id;
    types::Buffer token;token.WriteInt(owner);token.WriteInt(game.GetTurnId());
    token.WriteString(game.GetCanonicalEconomy(anchor.m_owner).Serialize().ToString());return token.ToString();
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
    rates.Validate();return CalculateCanonicalBaseEnergy(base,base.PrepareSmacResourceCalculation(),&rates);
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

bool Game::HandleSmacEnergyRatesEvent(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="set_smac_energy_rates") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed energy-rate command");}
        return found->second->Deref();
    };
    if(args.size()!=4) {THROW("Energy rates require Base, expected faction state, Psych and Labs tenths");}
    const auto target=args.find("base");
    const auto* object=target!=args.end() && target->second ? dynamic_cast<const gse::value::Object*>(target->second->Deref()) : nullptr;
    if(!object || object->object_class!=base::Base::WRAP_CLASS || caller!=m_slot_num) {THROW("Energy rates require an actual local Base");}
    const auto id=object->value.find("id");
    if(id==object->value.end() || !id->second || id->second->type!=gse::VT_INT) {THROW("Energy rates require a stable Base identity");}
    const auto number=static_cast<const gse::value::Int*>(id->second)->value;
    if(number<0 || static_cast<uint64_t>(number)>SIZE_MAX) {THROW("Invalid energy-rate Base identity");}
    auto* anchor=m_bm->GetBase(static_cast<size_t>(number));if(!anchor || object->wrapobj!=anchor) {return true;}
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    const auto psych=static_cast<const gse::value::Int*>(field("psych",gse::VT_INT))->value;
    const auto labs=static_cast<const gse::value::Int*>(field("labs",gse::VT_INT))->value;
    if(expected.size()>2*1024*1024 || psych<0 || labs<0 || psych>10 || labs>10-psych) {THROW("Energy rates require nonnegative tenths totaling at most ten");}
    anchor->RequireSmacProductionSelection();
    if(apply) {
        CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Energy rates require the owning writable accumulation");}
    }
    if(expected!=EnergyRateState(*this,*anchor)) {return true;}
    // Existing ordinary command fences retain ownership of required questions.
    // The original accepted rate-control callback contract is still being frozen.
    if(m_canonical_continuation) {THROW("Energy rates are unavailable while a required Base choice is pending");}
    const base::SmacEnergyRates rates{static_cast<int32_t>(psych),static_cast<int32_t>(labs)};
    const auto results=PrepareEnergyRates(*this,*anchor,rates);
    auto& saved=m_simulation->value.economy.at(anchor->GetSmacResourceContext().owner_simulation_id);
    auto candidate=saved;candidate.rates=rates;candidate.Validate();
    if(!apply || candidate==saved) {return true;}
    auto updates=m_bm->m_base_updates;
    for(const auto& row:results) {base::BaseManager::QueueBaseUpdateInto(updates,row.first,base::BaseManager::BUO_REFRESH);}
    std::unique_lock lock(m_bm->m_updated_bases_mutex);auto triggers=m_bm->m_updated_bases;
    for(const auto& row:results) {triggers.insert(row.first);}
    static_assert(std::is_nothrow_swappable_v<decltype(candidate)>);
    std::swap(saved,candidate);m_bm->m_base_updates.swap(updates);m_bm->m_updated_bases.swap(triggers);
    return true;
}
}
