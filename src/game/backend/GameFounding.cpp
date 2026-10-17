#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include <algorithm>

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
    if (name!="found_base") {return false;}
    const auto found=args.find("unit");
    if (args.size()!=1 || found==args.end() || !found->second) {THROW("Founding requires exactly one current Colony Pod");}
    auto* colony=unit::Unit::Unwrap(found->second);
    if (!colony || !m_um || m_um->GetUnit(colony->m_id)!=colony || caller!=m_slot_num
        || colony->m_owner->GetIndex()!=caller || colony->m_owner->GetPlayer()!=m_player) {
        THROW("Founding requires the actual local owner and registered Colony Pod");
    }
    m_bm->ValidateSmacFounding(*colony);
    const auto& roles=GetSmacBasePresentations();
    if (apply) {m_bm->FoundSmacBase(GSE_CALL,*colony,roles);}
    return true;
}
}
