#include "Game.h"
#include "Player.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "faction/SmacDominance.h"
#include "map/Map.h"
#include "map/SmacTerritoryRules.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend {

bool Game::PrepareCanonicalLeadingHumanDominance(const faction::SmacSimulationState& state) const {
    if(GetTurnId()>INT32_MAX) {throw std::runtime_error("Dominance turn exceeds signed32");}
    // Every mode0 difficulty/rivalry branch rejects through turn100. Do not
    // demand later-only histories or manufacture substitutes before that gate.
    if(GetTurnId()<=100) {return false;}
    if(!state.rank_order || !state.retained_base_totals) {throw std::runtime_error("Dominance lacks retained rank or Base totals");}
    const auto leader=state.rank_order->Leader();
    if(!leader || !state.IsHuman(leader)) {return false;}
    const auto participant=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==leader;});
    if(participant==state.roster.end()) {throw std::runtime_error("Rank leader has no accepted controller");}
    const auto& global=m_state->m_settings.global;
    if(!global.IsOrdinarySmacCampaign() || state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Live dominance requires the admitted ordinary no-project campaign");
    }
    const auto turn=static_cast<int32_t>(GetTurnId());
    const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(m_state->m_slots->GetSlot(participant->slot).GetPlayer()->GetDifficultyLevel());
    // Legacy port campaigns explicitly lacked option flags; do not relabel
    // them original Standard Rules. New campaigns own their actual rule word.
    const auto rules=global.GetSmacCampaignRules() ? global.GetSmacCampaignRules()->primary : 0u;
    const bool intense=(rules&0x40)!=0;
    const auto count=state.retained_base_totals->at(leader);
    if(count<0) {throw std::runtime_error("Negative live dominance Base total is outside the ordinary profile");}
    if(count<=std::max<int64_t>(4,(int64_t(turn)+25)/50) || (difficulty<2 && !intense)
        || turn<=(intense ? 100 : 350-difficulty*50)) {return false;}
    if(!state.victory_history || state.victory_history->voice_host!=-1) {
        throw std::runtime_error("Live no-project dominance lacks its saved pressure authority");
    }
    faction::SmacVictoryContext context;
    context.turn=turn;context.rules=rules;context.humans=state.human_membership;
    context.voice_host=state.victory_history->voice_host;
    context.economic_deadlines=state.victory_history->economic_deadlines;
    context.last_base_expansion_or_advice_turn=state.victory_history->last_base_expansion_or_advice_turn;
    for(const auto& owner:state.technology.factions) {context.technologies.at(owner.admission.simulation_id)=owner.owned;}
    for(const auto& owner:state.hostility.owners) {for(size_t peer=0;peer<8;++peer) {context.relations.at(owner.first)[peer]=owner.second.directions[peer].primary;}}
    faction::SmacVictoryRequirements requirements{{settings::SmacTechnologyReference::Kind::UNKNOWN,{}},-1,-1};
    if(faction::SmacVictoryPressure(context,requirements)) {
        if(!global.GetSmacCouncilRules()) {throw std::runtime_error("Victory-pressure majority lacks frozen proposal requirements");}
        requirements.supreme_leader=global.GetSmacCouncilRules()->SupremeLeader(*global.GetSmacTechnologyRules());
        std::vector<faction::SmacCouncilBase> bases;
        for(const auto id:m_bm->GetBaseRecordOrder()) {
            const auto* base=m_bm->GetBase(id);
            if(!base || base->GetPopulation()>INT32_MAX) {throw std::runtime_error("Council Base record is invalid");}
            bases.push_back({base->GetSmacResourceContext().owner_simulation_id,static_cast<int32_t>(base->GetPopulation())});
        }
        for(const auto& owner:state.roster) {
            const auto* profile=global.GetSmacFactionStartupRules()->Find(owner.faction_id);
            if(!profile) {throw std::runtime_error("Council faction has no frozen profile");}
            std::vector<int32_t> votes;
            for(const auto& bonus:profile->Records()) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::VOTES) {votes.push_back(bonus.parameter);}}
            // Project absence is the admitted campaign capability, not a
            // missing-project or inactive-faction inference.
            context.votes[owner.simulation_id]=faction::SmacCouncilVotes(owner.simulation_id,bases,false,false,votes);
        }
    }
    if(!state.ranking_scores) {throw std::runtime_error("Dominance lacks the retained score table");}
    return faction::SmacLeadingHumanDominance(context,{leader,leader,count,difficulty,0,9999,state.ranking_scores->at(leader),state.ranking_scores->at(state.rank_order->RunnerUp())},
        requirements,*global.GetSmacTechnologyRules());
}

std::optional<faction::SmacRegionalStrategyRow> Game::PrepareCanonicalRegionalStrategy(int32_t owner,const faction::SmacSimulationState& state) {
    if(!state.regional_strategy) {return {};}
    const auto& topology=m_map->GetTerritoryTopology();
    // Partial diagnostic/legacy geometry has no territory-summary authority.
    // It does not admit this phase; do not reconstruct it during valuation.
    if(!topology || !topology->Summary() || !topology->Areas()) {return {};}
    if(!state.regional_military || !state.retained_base_totals || !state.retained_land_region_concentration
        || !state.strategy || !state.strategy->count(owner) || !m_map->GetSmacWorkerKnowledge()) {
        throw std::runtime_error("Regional strategy lacks retained producer inputs");
    }
    const auto& military=*state.regional_military;
    if(!military.factions.at(owner-1).base_phase_flags) {throw std::runtime_error("Regional strategy lacks retained flag8");}
    std::array<faction::SmacRegionalStrategyRegion,128> regions{};
    for(size_t region=1;region<regions.size();++region) {
        auto& out=regions[region];
        out.stored_area=topology->Areas()->at(region);out.maximum_rating=topology->Summary()->maximum_rating[region];
        const auto& territory=topology->Summary()->owners[owner];
        out.owned_suitable=territory.suitable[region];out.covered_suitable=territory.covered_suitable[region];
        out.neutral_suitable=topology->Summary()->owners[0].suitable[region];
        out.flags=military.factions.at(owner-1).base_phase_flags->at(region);
        for(int32_t peer=1;peer<=7;++peer) {
            const auto saved=state.economy.find(peer);
            // The port roster is immutable for a campaign. Never-admitted
            // slots have no Base-count or relation writers and remain fresh0;
            // an admitted but unavailable historical row must reject instead.
            if(saved!=state.economy.end()) {
                if(!saved->second.regional_base_counts) {throw std::runtime_error("Regional strategy lacks peer Base history");}
                out.peers[peer].bases=saved->second.regional_base_counts->at(region);
            }
            out.peers[peer].units=military.factions.at(peer-1).units[region];
            out.peers[peer].attack=military.factions.at(peer-1).attack[region];
            out.peers[peer].relation=state.hostility.owners.at(owner).directions[peer].primary;
        }
    }
    if(GetTurnId()>INT32_MAX) {throw std::runtime_error("Regional strategy turn exceeds signed32");}
    const auto& priorities=state.strategy->at(owner).priorities;
    faction::SmacRegionalStrategyContext context{owner,static_cast<int32_t>(GetTurnId()),static_cast<int32_t>(m_map->GetWidth()*m_map->GetHeight()/2),
        state.retained_base_totals->at(owner),priorities[0],priorities[2],PrepareCanonicalLeadingHumanDominance(state),*state.retained_land_region_concentration};
    namespace routes=map::smac_territory;
    std::optional<routes::RegionalRouteMap> route_map;
    std::vector<routes::Base> bases;
    return faction::RefreshSmacRegionalStrategy(context,regions,state.regional_strategy->at(owner),[&](int32_t peer,uint8_t region,int32_t mode) {
        if(!route_map) {
            routes::RegionalRelations relations{};
            for(const auto& faction:state.hostility.owners) {for(size_t target=0;target<8;++target) {relations.at(faction.first)[target]=faction.second.directions[target].primary;}}
            route_map=routes::ProjectRegionalRoutes(*m_map->GetSmacWorkerKnowledge(),m_map->TerritorySea(),topology->Regions(),relations,state.human_membership);
            for(const auto id:m_bm->GetBaseRecordOrder()) {
                const auto* base=m_bm->GetBase(id);const auto* tile=base ? base->GetTile() : nullptr;
                if(!tile) {throw std::runtime_error("Regional route has an invalid live Base record");}
                bases.push_back({{{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)}},base->GetSmacResourceContext().owner_simulation_id});
            }
        }
        return routes::RegionalBasesReachable(*route_map,bases,owner,peer,region,mode,regions[region].peers[owner].bases,regions[region].peers[peer].bases,
            m_map->GetRegionalPathWorkspace(),[&](int32_t prototype) {
                const auto& catalog=m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
                const auto found=std::find_if(catalog.units.begin(),catalog.units.end(),[&](const auto& row){return row.source_id==prototype;});
                if(found==catalog.units.end()) {throw std::runtime_error("Regional path prototype lacks its current definition");}
                return found->plan;
            });
    });
}
}
