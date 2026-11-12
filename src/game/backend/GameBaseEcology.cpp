#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacEcology.h"
#include "base/SmacEcologyNativeSpawn.h"
#include "map/Map.h"
#include "map/SmacTerrainAuthority.h"
#include "map/SmacShapingOffsets.h"
#include "util/FinallyGuard.h"
#include "map/tile/Tile.h"
#include "unit/UnitManager.h"
#include <limits>
#include <stdexcept>

namespace game::backend {
void Game::ProcessCanonicalBaseEcology(GSE_CALLABLE,base::Base& current) {
    CheckRW(GSE_CALL);
    auto& state=m_simulation->value;const auto& global=m_state->m_settings.global;
    // Older diagnostic worlds without ecology calculation authority cannot
    // manufacture new aggregate history from an unavailable resource result.
    if(m_map->GetWidth()<8 || !state.ecology_history || !state.ecology_world || !state.perihelion || !global.GetSmacEcologyRules()) {return;}
    const auto owner=current.GetSmacResourceContext().owner_simulation_id;
    const auto& cache=current.GetSmacUpkeepCache();const auto* social=state.social.Find(owner);
    const auto& rules=global.GetSmacCampaignRules();
    if(!cache || !cache->calculation.eco_damage || !social || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed) {
        throw std::runtime_error("Ecology requires the retained final resource pass and current PLANET");
    }
    auto history=*state.ecology_history;auto random=state.gameplay_random;
    const auto contribution=base::SmacEcologyResearchContribution(*cache->calculation.eco_damage,social->current.at(7));
    const auto aggregate=int64_t(history.research_aggregate.at(owner))+contribution;
    if(aggregate<INT32_MIN || aggregate>INT32_MAX) {throw std::overflow_error("Ecology aggregate exceeds measured signed32");}
    history.research_aggregate.at(owner)=int32_t(aggregate);
    const auto draw=[&]()->int32_t {return random.NextCrtRaw15();};
    // The threshold consumes the existing CRT stream even for damage0 and
    // disabled events. Do not refresh damage merely because research awarded.
    // Legacy diagnostic worlds did not retain the handler's saved rule words.
    // Preserve their older aggregate capability without inventing event gates.
    if(!rules) {*state.ecology_history=history;return;}
    if(!base::AdmitSmacEcologyBloom(*cache->calculation.eco_damage,*state.perihelion,rules->primary,rules->secondary,draw)) {
        *state.ecology_history=history;state.gameplay_random=random;return;
    }
    if(!m_map->GetResourcePolicy() || !m_map->GetResourcePolicy()->NativeTerrainSource()
        || !m_map->GetSmacWorkerKnowledge() || !m_map->GetTerritoryTopology()) {
        throw std::runtime_error("Ecology bloom requires native terrain and saved ordinary campaign authority");
    }
    const auto knowledge=m_map->GetSmacWorkerKnowledge()->CopyData();
    const base::SmacWorkerRadiusMap radius{{int32_t(current.GetTile()->coord.x),int32_t(current.GetTile()->coord.y)},
        int32_t(m_map->GetWidth()),int32_t(m_map->GetHeight()),knowledge.horizontal_wrap};
    std::vector<base::SmacBloomBase> bases;std::vector<base::SmacNativeSpawnBase> spawn_bases;int32_t selected_base=-1;
    for(const auto id:m_bm->GetBaseRecordOrder()) {
        const auto* base=m_bm->GetBase(id);const auto* tile=base->GetTile();
        if(base==&current) {selected_base=int32_t(bases.size());}
        const std::array<int32_t,2> coordinate{{int32_t(tile->coord.x),int32_t(tile->coord.y)}};
        bases.push_back({int32_t(bases.size()),coordinate});
        spawn_bases.push_back({coordinate,base->GetSmacResourceContext().owner_simulation_id});
    }
    if(selected_base<0) {throw std::runtime_error("Ecology Base is outside native record order");}
    const auto cell=[&](int32_t x,int32_t y){return m_map->CopyEcologyBloomCell({{x,y}},knowledge);};
    const auto selected=base::SelectSmacEcologyBloom(radius,selected_base,bases,[&](int32_t x,int32_t y) {
        const auto tile=cell(x,y);uint32_t improvements=0;
        for(const auto& bit:std::array<std::pair<uint32_t,uint32_t>,5>{{{0x10,base::SmacEcologySquare::Mine},
            {0x40,base::SmacEcologySquare::Solar},{0x8000,base::SmacEcologySquare::Farm},
            {0x80000,base::SmacEcologySquare::SoilEnricher},{0x200000,base::SmacEcologySquare::Forest}}}) {
            if(tile.items&bit.first) {improvements|=bit.second;}
        }
        return base::SmacBloomTile{tile.altitude_class,bool(tile.items&0x20u),bool(tile.items&1u)&&tile.affiliation<=7,improvements};
    },draw);
    if(!selected) {*state.ecology_history=history;state.gameplay_random=random;return;}
    const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(current.m_owner->GetPlayer()->GetDifficultyLevel());
    const auto plan=base::PrepareSmacBloomTerrain({selected->coordinate,radius.width,radius.height,radius.horizontal_wrap},
        history.clean_minerals.at(owner),difficulty,GetTurnId()!=0,cell,draw);
    history.clean_minerals.at(owner)=plan.History();
    const auto bounded=[](int64_t value)->int32_t {
        if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Ecology event arithmetic exceeds measured signed32");}return int32_t(value);
    };
    bool post=true;
    if(!state.IsHuman(owner)) {
        const auto first=draw();
        if(!(first&1) && state.designs.global_difficulty>0 && draw()%(state.designs.global_difficulty+1)!=0) {
            --history.clean_minerals.at(owner);post=false; // terrain/CRT stay; only this H increment is undone
        }
    }
    auto status=state.faction_status.values.at(owner);
    if(!status) {throw std::runtime_error("Ecology event lacks saved faction status");}
    if(post && (*status&0x4000u)) {*status&=~0x4000u;post=false;}
    if(post && history.clean_minerals.at(owner)<7) {post=false;}
    if(post) {
        if(!state.victory_history) {throw std::runtime_error("Ecology event lacks saved Voice project history");}
        if(state.victory_history->voice_host!=-1) {post=false;}
    }
    // Direct disclosure is solely the focused/viewer faction, independently
    // of which owner generated this bloom. No new exploration is granted.
    const auto viewer=RequireSimulationParticipant(GetPlayer()).simulation_id;
    std::optional<uint8_t> remembered;
    bool notice=false;
    if(plan.FungusCell() && viewer>0) {
        const auto point=*plan.FungusCell();const auto index=size_t(point[1])*(radius.width/2)+point[0]/2;
        const auto viewer_status=state.faction_status.values.at(viewer);
        if(!viewer_status) {throw std::runtime_error("Ecology viewer lacks saved reveal status");}
        if((*viewer_status&0x200u) || (knowledge.tiles.at(index).explored_by&(1u<<viewer)) || (rules->secondary&0x80u)) {
            remembered=uint8_t(viewer);
            const auto selected_cell=cell(selected->coordinate[0],selected->coordinate[1]);
            notice=owner==viewer || ((selected_cell.items&2u) && selected_cell.affiliation==viewer);
        }
    }
    auto warming=state.ecology_warming_count;
    auto schedule=state.ecology_climate_schedule;
    std::optional<int32_t> climate_extra;
    bool warming_notice=false;
    if(post && (state.IsHuman(owner) || state.designs.global_difficulty>=4)) {
        const auto& ecology=*global.GetSmacEcologyRules();
        if(!warming || !ecology.warming_frequency) {throw std::runtime_error("Ecology warming requires retained count and frozen frequency");}
        *warming=bounded(int64_t(*warming)+1);
        const auto offset=state.ecology_world->sea_level_offset;
        const auto threshold=6+3*std::clamp(offset,0,100)/std::clamp(ecology.sea_level_rises,1,99);
        const auto frequency=*ecology.warming_frequency;
        const auto scaled=bounded(int64_t(frequency[0])*(*warming))/frequency[1];
        if(scaled>=threshold) {
            if(!schedule) {throw std::runtime_error("Ecology threshold lacks retained pending sea schedule");}
            *warming=0;
            const auto severity=std::clamp(history.clean_minerals.at(owner)/6-1,1,3);
            climate_extra=bounded(int64_t(m_map->GetResourcePolicy()->NativeTerrainSource()->Info().climate_extra)+severity);
            schedule->pending=bounded(int64_t(schedule->pending)+ecology.sea_level_rises*severity/3);
            if(schedule->pending==INT32_MIN) {throw std::overflow_error("Ecology pending magnitude exceeds signed32");}
            schedule->increment=std::abs(schedule->pending);schedule->divisor=20;schedule->accumulator=0;
            warming_notice=true;
        }
    }
    auto terrain=m_map->PrepareEcologyBloom(plan,remembered,climate_extra);
    std::optional<unit::UnitManager::PreparedNativeBirths> births;
    if(post) {
        const auto economy=state.economy.find(owner);
        if(!state.retained_base_totals || economy==state.economy.end() || !economy->second.regional_base_counts) {
            throw std::runtime_error("Ecology spawning lacks retained Base counts");
        }
        m_map->QueryEcologyBloom(*terrain,[&] {
            const auto staged=m_map->GetSmacWorkerKnowledge()->CopyData();
            const auto spawn=base::PrepareSmacNativeSpawn({{selected->coordinate,radius.width,radius.height,radius.horizontal_wrap},
                owner,history.clean_minerals.at(owner),state.retained_base_totals->at(owner),int32_t(current.GetPopulation())},spawn_bases,[&](int32_t x,int32_t y) {
                const auto tile=m_map->CopyEcologyBloomCell({{x,y}},staged);
                const auto region=m_map->GetTerritoryTopology()->Regions().at(size_t(y)*(radius.width/2)+x/2);
                return base::SmacNativeSpawnTile{tile.altitude_class,bool(tile.items&0x20u),region,
                    economy->second.regional_base_counts->at(region),bool(tile.items&3u),tile.affiliation};
            });
            births.emplace(m_um->PrepareEcologyNativeBirths(GSE_CALL,spawn));
            if(!warming_notice && history.clean_minerals.at(owner)>=10 && !(rules->secondary&0x20000u)) {
                births->Query([&] {
                    const auto after_birth=m_map->GetSmacWorkerKnowledge()->CopyData();
                    const auto x0=radius.width>1?draw()%radius.width:0,y=radius.height>1?draw()%radius.height:0;
                    const auto x=(x0&~1)|(y&1);
                    if(m_map->CopyEcologyBloomCell({{x,y}},after_birth).altitude_class>=3) {return;}
                    for(size_t i=0;i<49;++i) {
                        auto px=x+map::smac_shaping::OFFSETS[i][0];const auto py=y+map::smac_shaping::OFFSETS[i][1];
                        if(radius.horizontal_wrap) {px=(px+radius.width)%radius.width;}
                        if(px<0 || px>=radius.width || py<0 || py>=radius.height) {continue;}
                        const auto tile=m_map->CopyEcologyBloomCell({{px,py}},after_birth);
                        if((tile.items&3u) && tile.affiliation<=7) {return;}
                    }
                    throw std::runtime_error("Volcanic ecology requires complete live climate and cross-category terrain publication");
                });
            }
        });
    }
    auto frontend=*m_pending_frontend_requests;
    std::unique_ptr<map::TerrainRenderUpdate> payload;
    if(const auto* render=m_map->GetEcologyBloomTerrainPayload(*terrain)) {
        payload=std::make_unique<map::TerrainRenderUpdate>(*render);
        FrontendRequest update(FrontendRequest::FR_UPDATE_TERRAIN);update.data.update_terrain.update=payload.get();frontend.push_back(update);
    }
    std::unique_ptr<FrontendRequest::tile_updates_t> tiles;
    if(m_map->m_map_state && !plan.Changes().empty()) {
        tiles=std::make_unique<FrontendRequest::tile_updates_t>();tiles->reserve(plan.Changes().size());
        for(const auto& change:plan.Changes()) {
            auto* tile=m_map->GetTile(change.coordinate[0],change.coordinate[1]);
            tiles->emplace_back(tile,m_map->GetTileState(tile));
        }
        FrontendRequest update(FrontendRequest::FR_UPDATE_TILES);update.data.update_tiles.tile_updates=tiles.get();frontend.push_back(update);
    }
    m_map->CommitEcologyBloom(*terrain);
    if(births) {births->Commit();}
    *state.ecology_history=history;state.faction_status.values.at(owner)=status;state.ecology_warming_count=warming;state.ecology_climate_schedule=schedule;state.gameplay_random=random;
    m_pending_frontend_requests->swap(frontend);payload.release();tiles.release();
    if(births) {births->Notify(GSE_CALL);}
    if(notice) {Message("Xenofungus has bloomed near "+current.m_name+".");}
    if(warming_notice) {Message("Global warming has scheduled a sea-level change.");}
}
void Game::ProcessCanonicalClimateStep() {
    if(!m_simulation || !m_simulation->value.ecology_climate_schedule) {return;}
    auto schedule=*m_simulation->value.ecology_climate_schedule;
    if(!schedule.pending) {return;}
    if(schedule.increment<0 || schedule.divisor<=0) {throw std::runtime_error("Deferred climate has invalid retained schedule");}
    const auto sum=int64_t(schedule.accumulator)+schedule.increment;
    if(sum<INT32_MIN || sum>INT32_MAX) {throw std::overflow_error("Deferred climate accumulator exceeds signed32");}
    schedule.accumulator=int32_t(sum);
    if(schedule.accumulator>=schedule.divisor) {
        throw std::runtime_error("Deferred sea-level step requires complete live climate and cross-category terrain publication");
    }
    m_simulation->value.ecology_climate_schedule=schedule;
}
}
