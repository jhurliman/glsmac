#include "Game.h"
#include "Player.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "map/SmacNativeTerrainItems.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include <type_traits>

#include "faction/Faction.h"
#include "faction/SmacDiplomacy.h"
#include "slot/Slot.h"
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend {
namespace {
std::string ContactUnavailable(const faction::SmacSimulationState& state,int32_t actor,int32_t target,size_t turn) {
    if(!state.diplomacy_metadata || !state.diplomacy_metadata->sunspot_countdown) {return "Saved communications authority is unavailable";}
    if(*state.diplomacy_metadata->sunspot_countdown>0) {return "Sunspots disrupt communications";}
    const auto& direction=state.hostility.owners.at(actor).directions.at(target);
    if(!(direction.primary&8u)) {return "No commlink frequency";}
    //573070: future directed dates suppress opening too. No write on suppression.
    if(!(direction.primary&0x2000u) && direction.transition_turn>=0 && int64_t(turn)-direction.transition_turn<16) {
        return "Negotiations resume in "+std::to_string(16-(int64_t(turn)-direction.transition_turn))+" turns";
    }
    return {};
}
std::string DiplomacyToken(const faction::SmacSimulationState& state,int32_t actor,int32_t target,size_t turn) {
    types::Buffer token;token.WriteInt(actor);token.WriteInt(target);token.WriteInt(turn);
    for(const auto pair:{std::pair<int32_t,int32_t>{actor,target},{target,actor}}) {
        const auto& owner=state.hostility.owners.at(pair.first);const auto& d=owner.directions.at(pair.second);
        for(const auto word:{d.primary,d.secondary,d.paired_a,d.paired_b,owner.integrity,owner.bookkeeping}) {token.WriteInt(word);}
        token.WriteInt(d.transition_turn);
    }
    token.WriteBool(state.IsHuman(target));
    token.WriteBool(state.diplomacy_metadata.has_value());
    if(state.diplomacy_metadata) {
        const auto& data=*state.diplomacy_metadata;token.WriteBool(data.sunspot_countdown.has_value());
        if(data.sunspot_countdown) {token.WriteInt(*data.sunspot_countdown);}
        token.WriteInt(data.sanctions[actor]);token.WriteInt(data.sanctions[target]);
        token.WriteInt(data.grievance[actor][target]);token.WriteInt(data.grievance[target][actor]);
    }
    return token.ToString();
}
std::string RelationText(uint32_t flags) {
    std::string result;
    for(const auto& entry:{std::pair<uint32_t,const char*>{1,"Pact"},{2,"Treaty"},{4,"Truce"},{16,"Vendetta"}}) {
        if(flags&entry.first) {if(!result.empty()) {result+=" / ";}result+=entry.second;}
    }
    return result.empty()?"Contact established":result;
}
// GLSMAC AI policy, deliberately separate from the source transition recipes.
// Free Truce ends fighting; a peaceable counterparty trades with a reputable,
// unsanctioned partner. This is not the original scored negotiation algorithm.
std::string PeaceRefusal(const faction::SmacSimulationState& state,int32_t actor,int32_t target,bool treaty) {
    if(state.IsHuman(target)) {return "Another human controls this faction";}
    if(!treaty) {return {};}
    const auto& own=state.hostility.owners.at(actor);const auto& peer=state.hostility.owners.at(target);
    if((own.directions[target].primary|peer.directions[actor].primary)&16u) {return "We must agree to a Truce before discussing a Treaty";}
    if(uint64_t(own.integrity)>uint64_t(peer.integrity)+1u) {return "Your record of broken agreements prevents a Treaty";}
    if(state.diplomacy_metadata->sanctions[actor] || state.diplomacy_metadata->sanctions[target]) {return "Economic sanctions prevent a useful trade agreement";}
    return {};
}
}

gse::Value* Game::CopySmacDiplomacy(GSE_CALLABLE) const {
    ValidateCurrentSimulation();RequireSmacLocalHumanOwner(m_slot);
    const auto owner=RequireSimulationParticipant(m_player).simulation_id;const auto& state=m_simulation->value;
    gse::value::array_elements_t rows;
    for(const auto& participant:state.roster) {
        const auto target=participant.simulation_id;
        if(target==owner) {continue;}
        const auto& relation=state.hostility.owners.at(owner).directions.at(target);
        if(!(relation.primary&8u)) {continue;} // never reveal unmet identities
        const auto* definition=GetFaction(participant.faction_id);
        if(!definition) {throw std::runtime_error("Commlink faction definition is unavailable");}
        const auto unavailable=participant.active?ContactUnavailable(state,owner,target,GetTurnId()):"Faction eliminated";
        rows.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"target",VALUE(gse::value::Int,,target)}, {"name",VALUE(gse::value::String,,definition->m_name)},
            {"relation",VALUE(gse::value::String,,RelationText(relation.primary))}, {"unavailable",VALUE(gse::value::String,,unavailable)},
            {"expected_state",VALUE(gse::value::String,,DiplomacyToken(state,owner,target,GetTurnId()))}
        })));
    }
    return VALUE(gse::value::Array,,rows);
}

bool Game::HandleSmacDiplomacyEvent(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="offer_smac_peace") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {throw std::runtime_error("Malformed diplomatic offer");}
        return found->second->Deref();
    };
    if(args.size()!=3 || caller!=m_slot_num || !IsRunning()) {throw std::runtime_error("Diplomacy requires an authenticated local offer");}
    ValidateCurrentSimulation();RequireSmacLocalHumanOwner(m_slot);RequireSmacGameplayActive();
    if(m_canonical_continuation || m_canonical_turn_in_progress || m_canonical_turn_failed) {throw std::runtime_error("Finish the current turn choice before negotiations");}
    const auto target=static_cast<const gse::value::Int*>(field("target",gse::VT_INT))->value;
    const auto& kind=static_cast<const gse::value::String*>(field("kind",gse::VT_STRING))->value;
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    const auto owner=RequireSimulationParticipant(m_player).simulation_id;auto& state=m_simulation->value;
    if(target<1 || target>7 || target==owner || (kind!="truce" && kind!="treaty") || expected.size()>4096) {throw std::runtime_error("Diplomatic offer has invalid target or terms");}
    const auto participant=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==target;});
    if(participant==state.roster.end() || !participant->active) {throw std::runtime_error("Diplomatic target is not an active participant");}
    if(expected!=DiplomacyToken(state,owner,static_cast<int32_t>(target),GetTurnId())) {return true;}
    const auto unavailable=ContactUnavailable(state,owner,static_cast<int32_t>(target),GetTurnId());
    if(!unavailable.empty()) {throw std::runtime_error(unavailable);}
    const auto refusal=PeaceRefusal(state,owner,static_cast<int32_t>(target),kind=="treaty");
    if(apply) {CheckRW(GSE_CALL);if(!GetGCSpace()->IsAccumulating()) {throw std::runtime_error("Diplomacy requires its owning accumulation");}}
    int32_t violations=0;
    if(refusal.empty()) {violations=ApplySmacAcceptedPeace(GSE_CALL,owner,static_cast<int32_t>(target),kind=="treaty",apply);}
    if(!apply) {return true;}
    auto text=refusal.empty()?std::string(kind=="treaty"?"Treaty accepted":"Truce accepted"):refusal;
    if(violations) {text+="; units in foreign territory have been marked";}

    m_state->TriggerObject(this,"smac_diplomacy_response",ARGS_F(target,text,refusal) {
        {"target",VALUE(gse::value::Int,,target)},{"accepted",VALUE(gse::value::Bool,,refusal.empty())},{"text",VALUE(gse::value::String,,text)}
    };});
    return true;
}
int32_t Game::ApplySmacAcceptedPeace(GSE_CALLABLE,int32_t actor,int32_t target,bool treaty,bool apply) {
    auto& saved=m_simulation->value;
    if(!saved.diplomacy_metadata || GetTurnId()>INT32_MAX) {throw std::runtime_error("Accepted peace requires saved diplomatic metadata and turn");}
    auto hostility=saved.hostility;auto metadata=*saved.diplomacy_metadata;auto status=saved.faction_status;
    auto random=saved.gameplay_random;
    faction::GrantSmacRelation(hostility,actor,target,treaty?0x01000202u:0x204u);
    hostility.owners.at(actor).directions.at(target).transition_turn=static_cast<int32_t>(GetTurnId());
    ++hostility.owners.at(actor).bookkeeping;++hostility.owners.at(target).bookkeeping;
    if(treaty) {metadata.grievance[target][actor]=0;(void)random.NextSharedLow16();}
    if(!m_map || !m_map->m_worker_knowledge || !m_map->m_territory_topology || !m_um || !m_bm) {throw std::runtime_error("Accepted peace requires current territory and actor authority");}
    auto knowledge=m_map->m_worker_knowledge->CopyData();
    // Worker-facing exemptions and recognition are projections of these changed
    // raw directions. Retain every other observer/knowledge/memory bit.
    for(const auto pair:{std::pair<int32_t,int32_t>{actor,target},{target,actor}}) {
        auto& observer=knowledge.observers.at(pair.first);
        if(!observer) {throw std::runtime_error("Peace lacks an admitted worker observer");}
        const auto flags=hostility.owners.at(pair.first).directions.at(pair.second).primary;
        observer->relations[pair.second].foreign_unit_exemption=(flags&3u)!=0;
        observer->relations[pair.second].territory_recognition=(flags&0x08000008u)==0x08000008u;
        if(observer->pact_with) {
            auto& pacts=*observer->pact_with;
            pacts=static_cast<uint8_t>((pacts&~(1u<<pair.second))|((flags&1u)?1u<<pair.second:0u));
        }
    }
    auto prepared_knowledge=std::optional<map::SmacWorkerKnowledge>{map::SmacWorkerKnowledge::Create(knowledge)};
    std::vector<std::pair<unit::Unit*,uint32_t>> words;
    auto updates=m_um->m_unit_updates;
    int32_t weighted=0;
    const auto flags=hostility.owners.at(target).directions.at(actor).primary;
    //551f40(A,B) calls575020(B,A,2,0,0). Pact/Vendetta skip even the global clear.
    if(treaty && !(flags&0x11u)) {
        const auto& topology=*m_map->m_territory_topology;
        const auto& regions=topology.Regions();std::array<bool,64> exempt{};
        const auto index=[&](const map::tile::Tile* tile) {
            if(!tile || tile->tiles!=m_map->GetTilesPtr() || tile->coord.x>=m_map->GetWidth() || tile->coord.y>=m_map->GetHeight()) {throw std::runtime_error("Treaty Unit has invalid map membership");}
            return size_t(tile->coord.y)*size_t(m_map->GetWidth()/2)+tile->coord.x/2;
        };
        if(!(flags&0x20u) && hostility.owners.at(actor).integrity<5) {
            for(const auto id:m_bm->GetBaseRecordOrder()) {
                const auto* base=m_bm->GetBase(id);if(!base) {throw std::runtime_error("Treaty Base order is incomplete");}
                const auto third=base->GetSmacResourceContext().owner_simulation_id;
                if(third==actor || third==target) {continue;}
                if((hostility.owners.at(actor).directions[third].primary&16u) && (hostility.owners.at(target).directions[third].primary&16u)) {
                    const auto region=regions.at(index(base->GetTile()));if(region<64) {exempt[region]=true;}
                }
            }
        }
        const auto& definitions=m_state->m_settings.global.GetSmacPredefinedUnitRules();
        if(!definitions) {throw std::runtime_error("Treaty requires frozen Unit plans");}
        for(const auto id:m_um->GetUnitRecordOrder()) {
            auto* current=m_um->GetUnit(id);if(!current || current->m_id!=id) {throw std::runtime_error("Treaty Unit record order is incomplete");}
            uint32_t original=0;
            if(current->IsNative()) {original=current->Native().state;}
            else if(current->IsConventional() && current->Conventional().retained_state_word) {original=*current->Conventional().retained_state_word;}
            else {throw std::runtime_error("Treaty requires complete retained Unit state words");}
            uint32_t replacement=original&~0x400u;
            if(current->SimulationOwner()==actor) {
                const auto& record=current->Conventional();int32_t plan=-1;
                if(record.design_id<64) {plan=definitions->catalog.units.at(record.design_id).plan;}
                else {
                    const auto* owner=saved.designs.Find(actor);
                    if(!owner) {throw std::runtime_error("Treaty Unit has no design owner");}
                    const auto design=std::find_if(owner->designs.begin(),owner->designs.end(),[&](const auto& row){return row.id==record.design_id;});
                    if(design==owner->designs.end()) {throw std::runtime_error("Treaty Unit design is unavailable");}plan=design->plan;
                }
                const auto* tile=current->GetTile();const auto at=index(tile);const auto region=regions.at(at);
                if(((plan>=0 && plan<=8) || (plan==10 && static_cast<int32_t>(record.order)==3)) && !tile->is_water_tile && region<64 && !exempt[region]
                    && knowledge.tiles.at(at).claimant==target) {
                    //4f97a0 forced disclosure uses the raw claim, with a separate
                    // same-region, all-owner nearest-Base output. Later ties win.
                    const base::Base* nearest=nullptr;int64_t distance=9999;
                    for(const auto base_id:m_bm->GetBaseRecordOrder()) {
                        const auto* base=m_bm->GetBase(base_id);if(!base) {throw std::runtime_error("Treaty nearest Base registry is incomplete");}
                        if(regions.at(index(base->GetTile()))!=region) {continue;}
                        auto dx=std::abs(int64_t(tile->coord.x)-int64_t(base->GetTile()->coord.x));const auto dy=std::abs(int64_t(tile->coord.y)-int64_t(base->GetTile()->coord.y));
                        if(topology.Wraps()) {dx=std::min(dx,int64_t(m_map->GetWidth())-dx);}
                        const auto candidate=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
                        if(candidate<=distance) {distance=candidate;nearest=base;}
                    }
                    if(nearest) {
                        replacement|=0x400u;++weighted;
                        if(map::NativeTerrainItems(tile->terraforming,tile->features)&0x81608850u) {++weighted;}
                    }
                }
            }
            if(original!=replacement) {
                words.emplace_back(current,replacement);
                auto& update=updates.try_emplace(id,unit::UnitManager::unit_update_t{unit::UnitManager::UUO_NONE,current}).first->second;
                if(update.unit!=current || (update.ops&unit::UnitManager::UUO_DESPAWN)) {throw std::runtime_error("Treaty Unit has incompatible queued updates");}
                update.ops=static_cast<unit::UnitManager::unit_update_op_t>(update.ops|unit::UnitManager::UUO_REFRESH);
            }
        }
        if(weighted) {
            auto& word=status.values.at(target);if(!word) {throw std::runtime_error("Treaty violation requires saved faction status");}*word|=0x10000u;
        }
    }
    if(!apply) {return weighted;}
    // No callbacks/allocations from here through complete native publication.
    saved.hostility.owners.swap(hostility.owners);saved.diplomacy_metadata=metadata;saved.faction_status=status;saved.gameplay_random=random;
    m_um->CommitDiplomacyStateWords(words,updates);
    CommitConventionalSight(prepared_knowledge); // marks map/actor views dirty
    return weighted;
}
namespace unit {
void UnitManager::CommitDiplomacyStateWords(const std::vector<std::pair<Unit*,uint32_t>>& words,
    std::unordered_map<size_t,unit_update_t>& updates) noexcept {
    for(const auto& [actor,word]:words) {
        if(actor->IsNative()) {std::get<SmacNativeUnitRecord>(actor->m_state).state=word;}
        else {std::get<ConventionalUnitState>(actor->m_state).retained_state_word=word;}
    }
    m_unit_updates.swap(updates);
}
}

}
