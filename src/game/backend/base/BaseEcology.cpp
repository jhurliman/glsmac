#include "Base.h"
#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacEcology.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include <limits>
#include <stdexcept>

namespace game::backend::base {

std::optional<int32_t> Base::CalculateSmacEcologyDamage(const SmacBaseResourceCalculation& calculation,
    const SmacBaseAllocationState* allocation, size_t population, const FacilityDef* granted) const {
    m_game->ValidateCurrentSimulation();
    const auto& simulation=m_game->m_simulation;
    const auto& rules=m_game->m_state->m_settings.global.GetSmacEcologyRules();
    if(!simulation || !rules || !simulation->value.ecology_world || !simulation->value.ecology_history
        || !simulation->value.perihelion) {return std::nullopt;}
    const auto& state=simulation->value;
    // The caller authenticates all installed local facilities and this explicit
    // campaign capability. Project absence is authority, not a missing lookup.
    if(state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Live ecology requires admitted project authority");
    }
    const auto* selected=allocation ? allocation : (m_allocation ? &*m_allocation : nullptr);
    if(!selected) {return std::nullopt;}
    (void)AllocationCoordinates(*selected,population);
    const auto* tiles=m_game->GetMap()->GetTilesPtr();
    if(tiles->GetWidth()<8) {return std::nullopt;} // narrower radius aliasing is not measured
    if(population>127) {throw std::runtime_error("Live ecology population exceeds 127");}
    const auto owner=GetSmacResourceContext().owner_simulation_id;
    const auto* technology=state.technology.Find(owner);
    const auto* social=state.social.Find(owner);
    if(!technology || !social || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed) {
        throw std::runtime_error("Live ecology requires current owner technology and PLANET");
    }
    bool tree=false,hybrid=false,nano=false,preserve=false,temple=false;
    const auto role=[&](const FacilityDef* facility) {
        m_game->GetBM()->ValidateSmacFacilityDef(facility);
        if(!facility->m_smac_source_id) {throw std::runtime_error("Ecology facility has no source identity");}
        switch(*facility->m_smac_source_id) {
            case 13:tree=true;break; case 14:hybrid=true;break; case 20:nano=true;break;
            case 31:preserve=true;break; case 32:temple=true;break; default:break;
        }
    };
    for(const auto& item:m_facilities) {role(item.second);}
    if(granted) {role(granted);}
    const SmacWorkerRadiusMap radius{{static_cast<int32_t>(m_tile->coord.x),static_cast<int32_t>(m_tile->coord.y)},
        static_cast<int32_t>(tiles->GetWidth()),static_cast<int32_t>(tiles->GetHeight()),true};
    std::array<std::optional<std::array<int32_t,2>>,21> worked{};
    for(int32_t i=0;i<=20;++i) {
        if(selected->selected & (uint32_t(1)<<i)) {worked[i]=SmacWorkedSquareCollection::Resolve(radius,i);}
    }
    const auto terraforming=CalculateSmacTerraformingEcology(radius,tree,hybrid,[&](int32_t x,int32_t y) {
        using namespace map::tile;
        const auto flags=m_game->GetMap()->GetTile(x,y)->terraforming;
        uint32_t improvements=0;
        const std::array<std::pair<terraforming_t,uint32_t>,10> mappings{{
            {TERRAFORMING_ROAD,SmacEcologySquare::Road},{TERRAFORMING_MAG_TUBE,SmacEcologySquare::MagTube},
            {TERRAFORMING_MINE,SmacEcologySquare::Mine},{TERRAFORMING_SOLAR,SmacEcologySquare::Solar},
            {TERRAFORMING_FARM,SmacEcologySquare::Farm},{TERRAFORMING_SOIL_ENRICHER,SmacEcologySquare::SoilEnricher},
            {TERRAFORMING_CONDENSER,SmacEcologySquare::Condenser},{TERRAFORMING_MIRROR,SmacEcologySquare::EchelonMirror},
            {TERRAFORMING_BOREHOLE,SmacEcologySquare::Borehole},{TERRAFORMING_FOREST,SmacEcologySquare::Forest}}};
        for(const auto& mapping:mappings) {if(flags & mapping.first) {improvements|=mapping.second;}}
        bool is_worked=false;
        for(const auto& coordinate:worked) {
            if(coordinate && (*coordinate)[0]==x && (*coordinate)[1]==y) {is_worked=true;break;}
        }
        return SmacEcologySquare{improvements,is_worked};
    });
    const auto burden=CalculateSmacEcologyBurden({state.ecology_history->clean_minerals.at(owner),
        terraforming.raw_points,calculation.minerals.intake,static_cast<int32_t>(population),
        state.GetSatelliteCounts(owner).resources[1],nano,preserve,temple,false,false});
    const auto* player=m_owner->GetPlayer();
    if(!player) {throw std::runtime_error("Live ecology owner has no player difficulty");}
    const auto& world=*state.ecology_world;
    return base::CalculateSmacEcologyDamage(burden,{state.IsHuman(owner),
        unit::SmacStartupDesignState::OriginalDifficulty(player->GetDifficultyLevel()),state.designs.global_difficulty,
        state.ecology_history->major_atrocities.at(owner),world.sea_level_offset,rules->sea_level_rises,
        world.native_life,technology->score,technology->future_count,social->current.at(7),
        false,false,*state.perihelion}).damage;
}

}
