#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "map/tile/Tile.h"
#include <algorithm>
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"

namespace game::backend {
base::ProductionItem Game::SelectSmacFoundingProduction(const slot::Slot* owner, const map::tile::Tile* tile) const {
    RequireSmacLocalHumanOwner(owner);
    const auto& participant=RequireSmacProductionOwner(owner);
    const auto& catalog=m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
    std::vector<unit::SmacCustomDesign> designs;
    for (const auto& row:catalog.units) {
        designs.push_back(ResolveConventionalDesign({static_cast<int32_t>(row.source_id),participant.simulation_id},*m_simulation));
    }
    const auto* custom=m_simulation->value.designs.Find(participant.simulation_id);
    if (!custom) { THROW("Founding owner has no saved custom design table"); }
    designs.insert(designs.end(),custom->designs.begin(),custom->designs.end());
    std::sort(designs.begin(),designs.end(),[](const auto& a,const auto& b){return a.id<b.id;});
    int64_t best=-1; int32_t winner=2;
    for (const auto& design:designs) {
        if (design.id>=64 && !design.prototyped) {continue;}
        const auto item=MakeSmacProductionItem(owner,base::ProductionItemKind::Unit,design.id);
        if (!IsSmacUnitProductionEligibleAt(owner,tile,item)) {continue;}
        int64_t score=int64_t(catalog.defenses.defenses.at(design.defense).rating)*32;
        score*=design.plan==2 ? 4 : design.plan==1 ? 3 : design.plan==3 ? 2 : 1;
        if (catalog.chassis.chassis.at(design.chassis).triad!=loader::txt::ChassisTriad::Land) {score/=4;}
        score-=design.cost;
        if (score>=best) {best=score;winner=design.id;}
    }
    return MakeSmacProductionItem(owner,base::ProductionItemKind::Unit,winner);
}

bool Game::HandleSmacFoundingEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool recovery=name=="found_base_with_default_governor";
    if(name!="found_base" && !recovery) {return false;}
    const auto found=args.find("unit");
    if(args.size()!=(recovery ? 2u : 1u) || found==args.end() || !found->second) {THROW("Founding requires the current Colony Pod and optional confirmed settings");}
    const auto* object=dynamic_cast<const gse::value::Object*>(found->second->Deref());
    if(!object || object->object_class!=unit::Unit::WRAP_CLASS) {THROW("Founding requires an actual Colony Pod");}
    const auto identity=object->value.find("id");
    if(identity==object->value.end() || !identity->second || identity->second->type!=gse::VT_INT) {THROW("Founding requires a stable Pod identity");}
    const auto id=static_cast<const gse::value::Int*>(identity->second)->value;
    if(id<0 || uint64_t(id)>SIZE_MAX) {THROW("Invalid Colony Pod identity");}
    auto* colony=m_um ? m_um->GetUnit(static_cast<size_t>(id)) : nullptr;
    if(!colony || object->wrapobj!=colony) {return true;}
    if(caller!=m_slot_num || colony->m_owner->GetIndex()!=caller || colony->m_owner->GetPlayer()!=m_player) {
        THROW("Founding requires the actual local owner and registered Colony Pod");
    }
    m_bm->ValidateSmacFounding(*colony);
    const auto& roles=GetSmacBasePresentations();
    const auto owner=colony->Conventional().owner_simulation_id;
    const bool missing=!m_simulation->value.remembered_governor.at(owner);
    const bool has_base=std::any_of(m_bm->GetBases().begin(),m_bm->GetBases().end(),[&](const auto& row){return row.second->m_owner==colony->m_owner;});
    if(missing && has_base && !recovery) {THROW("Apply Governor settings in an owned Base before founding a new Base");}
    types::Buffer state;state.WriteInt(m_active_admission ? m_active_admission->ticket : 0);state.WriteInt(GetTurnId());
    state.WriteInt(colony->m_id);state.WriteInt(colony->GetTile()->coord.x);state.WriteInt(colony->GetTile()->coord.y);colony->Conventional().Serialize(state,true,true,true,true,true);
    if(recovery) {
        const auto expected=args.find("expected_state");
        if(expected==args.end() || !expected->second || expected->second->Deref()->type!=gse::VT_STRING) {THROW("Default Governor choice requires its current Pod state");}
        const auto& token=static_cast<const gse::value::String*>(expected->second->Deref())->value;
        if(token.size()>2*1024*1024) {THROW("Default Governor choice exceeds its state bound");}
        if(token!=state.ToString() || !missing || has_base) {return true;}
        if(apply) {m_bm->FoundSmacBase(GSE_CALL,*colony,roles,base::SmacBaseResourceContext::NewGameGovernorOptions(m_simulation->value.designs.global_difficulty));}
    } else if(missing && !has_base) {
        if(apply) {
            const auto expected=state.ToString();
            m_state->TriggerObject(this,"smac_governor_foundation",ARGS_F(colony,expected) {
                {"unit",colony->Wrap(GSE_CALL)},{"expected_state",VALUE(gse::value::String,,expected)}
            };});
        }
    } else if(apply) {m_bm->FoundSmacBase(GSE_CALL,*colony,roles);}
    return true;
}
}
