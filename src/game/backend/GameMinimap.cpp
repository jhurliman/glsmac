#include "Game.h"
#include "State.h"
#include "Player.h"
#include "slot/Slot.h"
#include "map/Map.h"
#include "map/SmacMinimap.h"
#include "map/tile/Tile.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Bool.h"
#include "gse/value/Undefined.h"
#include "gse/value/Object.h"
#include <stdexcept>

namespace game::backend {
gse::Value* Game::CopyLocalMinimap(GSE_CALLABLE) const {
    const auto unavailable=[&]()->gse::Value* {
        // An explicit empty frame prevents an existing saved unknown mask from
        // falling back to a whole-world capture. No-knowledge diagnostic/legacy
        // worlds retain their existing presentation, without claiming SMAC fog.
        if(m_map && m_map->GetSmacWorkerKnowledge()) {return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{}));}
        return VALUE(gse::value::Undefined);
    };
    if(!IsRunning() || !m_state || !m_simulation || !m_slot || !m_player || !m_map
        || m_state->GetConnection() || m_player->GetRole()!=Player::PR_SINGLE) {return unavailable();}
    const auto& participant=RequireSimulationParticipant(m_player);
    if(participant.slot!=static_cast<int32_t>(m_slot->GetIndex())) {throw std::runtime_error("Minimap local participant mismatch");}
    const auto& knowledge=m_map->GetSmacWorkerKnowledge();
    const auto& terrain=m_map->GetResourcePolicy();
    const auto& rules=m_state->m_settings.global.GetSmacCampaignRules();
    // Neither current installed files nor the rendered whole-world mesh can
    // fill unavailable saved survey/knowledge/terrain authorities on a read.
    if(!knowledge || !terrain || !rules || !m_map->m_worker_knowledge_ready) {return unavailable();}
    const auto data=knowledge->CopyData();
    const auto observer=participant.simulation_id;
    if(data.width!=static_cast<int64_t>(m_map->GetWidth()) || data.height!=static_cast<int64_t>(m_map->GetHeight())
        || observer<1 || observer>7 || !data.observers.at(observer)) {
        throw std::runtime_error("Minimap knowledge differs from current local world");
    }
    // Original overview request geometry is established for even ordinary
    // worlds; do not feed unsupported tiny/odd geometry to the live widget.
    if(data.height<=4 || (data.height&1)) {return unavailable();}
    const auto bit=static_cast<uint8_t>(1u<<observer);
    std::string rgba(data.tiles.size()*4,'\0');
    for(int32_t y=0;y<data.height;++y) {for(int32_t x=y&1;x<data.width;x+=2) {
        const auto index=static_cast<size_t>(y)*(data.width/2)+x/2;
        const auto* tile=m_map->GetTile(x,y);
        map::smac_minimap::Input input;
        input.band=static_cast<int32_t>(terrain->Altitude(*tile));
        input.viewer=observer;
        input.explored=(data.tiles.at(index).explored_by&bit)!=0;
        input.initial_survey=(rules->primary&0x0100u)!=0;
        input.revealed=(data.all_map_revealed&bit)!=0;
        // Default World overview is physical. No button binding establishes
        // a political/rainfall selector, and map-color assignment is separate.
        if(const auto color=map::smac_minimap::Resolve(input)) {
            for(size_t channel=0;channel<3;++channel) {rgba[index*4+channel]=static_cast<char>(color->rgb[channel]);}
            rgba[index*4+3]=static_cast<char>(255);
        }
    }}
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"kind",VALUE(gse::value::String,,"smac-coarse-physical-v1")},
        {"width",VALUE(gse::value::Int,,data.width)},{"height",VALUE(gse::value::Int,,data.height)},
        {"observer",VALUE(gse::value::Int,,observer)},{"horizontal_wrap",VALUE(gse::value::Bool,,data.horizontal_wrap)},
        {"rgba",VALUE(gse::value::String,,rgba)}
    }));
}
}
