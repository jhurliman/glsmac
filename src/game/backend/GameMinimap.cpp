#include "Game.h"
#include "game/SmacDetailMinimap.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "unit/UnitManager.h"
#include "unit/Unit.h"
#include "faction/Faction.h"
#include "map/SmacPlanetMap.h"
#include "gse/value/Array.h"

#include "State.h"
#include "Player.h"
#include "slot/Slot.h"
#include "slot/Slots.h"
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
bool Game::AllowsLocalTileInspection(size_t x,size_t y) const {
    if(!m_state) {return false;}
    if(m_state->m_settings.global.GetRulesetId()!="smac") {return true;}
    if(!IsRunning() || !m_simulation || !m_slot || !m_player || !m_map || m_state->GetConnection()
        || m_player->GetRole()!=Player::PR_SINGLE || x>=m_map->GetWidth() || y>=m_map->GetHeight() || ((x^y)&1)) {return false;}
    const auto& participant=RequireSimulationParticipant(m_player);
    if(participant.slot!=static_cast<int32_t>(m_slot->GetIndex())) {throw std::runtime_error("Tile inspection local participant mismatch");}
    const auto& saved=m_map->GetSmacWorkerKnowledge();
    if(!saved || !m_map->m_worker_knowledge_ready) {return false;}
    const auto data=saved->CopyData();const auto viewer=participant.simulation_id;
    if(data.width!=static_cast<int64_t>(m_map->GetWidth()) || data.height!=static_cast<int64_t>(m_map->GetHeight())
        || viewer<1 || viewer>7 || !data.observers.at(viewer)) {throw std::runtime_error("Tile inspection knowledge differs from current local world");}
    const auto bit=static_cast<uint8_t>(1u<<viewer);
    const auto index=y*(m_map->GetWidth()/2)+x/2;
    // Survey geography is not permission to inspect climate/features or a
    // terrain preview. Only persistent exploration or this viewer's reveal is.
    return ((data.tiles.at(index).explored_by|data.all_map_revealed)&bit)!=0;
}
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
    if(!knowledge || !terrain || !m_map->m_worker_knowledge_ready) {return unavailable();}
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
        // Generated campaigns can retain no optional survey selection.
        // Known/revealed geography still has saved authority; unknown cells
        // require the explicit retained survey bit, never an assumed default.
        input.initial_survey=rules && (rules->primary&0x0100u)!=0;
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
gse::Value* Game::CopyLocalDetailMinimap(GSE_CALLABLE) const {
    if(!IsRunning() || !m_state || !m_simulation || !m_slot || !m_player || !m_map || m_state->GetConnection()
        || m_player->GetRole()!=Player::PR_SINGLE || m_map->GetWidth()!=80 || m_map->GetHeight()!=80) {return VALUE(gse::value::Undefined);}
    const auto& participant=RequireSimulationParticipant(m_player);
    if(participant.slot!=static_cast<int32_t>(m_slot->GetIndex())) {throw std::runtime_error("Detail local participant mismatch");}
    const auto& saved=m_map->GetSmacWorkerKnowledge();const auto& terrain=m_map->GetResourcePolicy();
    if(!saved || !terrain || !m_map->m_worker_knowledge_ready) {return VALUE(gse::value::Undefined);}
    const auto data=saved->CopyData();const auto viewer=participant.simulation_id;
    if(data.width!=80 || data.height!=80 || viewer<1 || viewer>7 || !data.observers.at(viewer)) {throw std::runtime_error("Detail saved observer differs from live world");}
    const auto& rules=m_state->m_settings.global.GetSmacCampaignRules();
    const uint8_t bit=static_cast<uint8_t>(1u<<viewer);
    const bool survey=rules && (rules->primary&0x0100u),revealed=(data.all_map_revealed&bit)!=0;
    const auto* original=terrain->OriginalPlanet();
    const auto coordinate=[&](int x,int y)->std::optional<std::array<int,2>> {
        if(y<0 || y>=80) {return {}; }if(data.horizontal_wrap) {x=(x%80+80)%80;}
        if(x<0 || x>=80 || ((x^y)&1)) {return {}; }return std::array<int,2>{x,y};
    };
    const auto band_at=[&](int x,int y)->std::optional<uint8_t> {
        const auto q=coordinate(x,y);if(!q) {return {};}
        return static_cast<uint8_t>(terrain->Altitude(*m_map->GetTile((*q)[0],(*q)[1])));
    };
    std::string tiles(3200*12,'\0');
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
        const size_t index=size_t(y)*40+x/2;const bool explored=(data.tiles.at(index).explored_by&bit)!=0;
        const bool known=explored || revealed || survey;if(!known) {continue;}
        bool supported=true;const auto band=*band_at(x,y);
        const auto ordinary=[&](int px,int py) {const auto q=coordinate(px,py);return !q || !original || !(original->At((*q)[0],(*q)[1]).Landmarks()&0xb03u);};
        if(band>=3) {
            supported=ordinary(x,y);
            for(const auto& corner:map::smac_terrain::CORNERS) {for(const auto& delta:corner) {supported=supported && ordinary(x+delta[0],y+delta[1]);}}
        }
        tiles[index*12]=static_cast<char>(band);tiles[index*12+1]=static_cast<char>(1u|((explored || revealed)?2u:0u)|(supported?4u:0u));
        if(supported) {const auto heights=::game::smac_detail::Heights(x,y,band_at);
            for(size_t v=0;v<5;++v) {tiles[index*12+2+v*2]=static_cast<char>(heights[v]&255);tiles[index*12+3+v*2]=static_cast<char>((heights[v]>>8)&255);}}
    }}
    const auto& resource_rules=m_state->m_settings.global.GetSmacResourceRules();
    const auto& profiles=m_state->m_settings.global.GetSmacFactionStartupRules();
    gse::value::array_elements_t bases;
    for(const auto& entry:m_bm->GetBases()) {
        const auto* base=entry.second;const auto* tile=base->GetTile();
        if(!base->HasSmacResourceContext() || !tile || tile->base!=base || tile->coord.x>=80 || tile->coord.y>=80 || !base->m_faction) {continue;}
        const size_t index=size_t(tile->coord.y)*40+tile->coord.x/2;
        if((static_cast<uint8_t>(tiles[index*12+1])&6u)!=6u) {continue;}
        const auto context=base->GetSmacResourceContext();const auto owner=context.owner_simulation_id;
        const auto memory=std::find_if(data.bases.begin(),data.bases.end(),[&](const auto& row){return row.id==base->m_id;});
        if(!::game::smac_detail::AdmitsBase(true,true,(static_cast<uint8_t>(tiles[index*12+1])&2u)!=0,owner==viewer,memory!=data.bases.end() && (memory->known_by&bit))) {continue;}
        const auto source=m_bm->GetSmacBodySourceSlot(*base);if(!source) {continue;}
        // Body art follows saved former-owner history; label colors still use owner.
        const auto& art=m_state->m_slots->GetSlot(*source).GetPlayer()->GetFaction()->m_bases_render;
        const auto population=base->GetPopulation();
        if(!resource_rules || !profiles || population>127
            || resource_rules->hab_complex_limit<0 || resource_rules->hab_complex_limit>126
            || resource_rules->hab_dome_limit<0 || resource_rules->hab_dome_limit>126
            || art.grid_x!=1 || art.grid_y!=1 || art.cell_width!=100 || art.cell_height!=75
            || art.cell_padding!=1 || art.cell_cx!=50 || art.cell_cy!=37 || art.scale_x!=1 || art.scale_y!=1 || art.file.empty()
            || m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {continue;}
        const auto* profile=profiles->Find(base->m_faction->m_id);if(!profile) {continue;}
        // Frozen startup validation pins the canonical source binding order.
        // Dense simulation/transport IDs are not palette slots.
        const bool unique_identity=std::count_if(m_simulation->value.roster.begin(),m_simulation->value.roster.end(),[&](const auto& row){return row.faction_id==profile->port_id;})==1;
        const int native_owner=unique_identity?int(profile-profiles->factions.data())+1:0;
        bool hq=false,perimeter=false,tachyon=false;
        for(const auto& facility:base->m_facilities) {const auto id=facility.second->m_smac_source_id;
            hq|=id && *id==1;perimeter|=id && *id==4;tachyon|=id && *id==5;}
        const auto body=::game::smac_base_body::Select(int(population),resource_rules->hab_complex_limit,resource_rules->hab_dome_limit,
            hq,static_cast<uint8_t>(tiles[index*12])<3,perimeter,tachyon,false); // saved no-project capability proves no CDF
        ::game::smac_base_body::Access access;access.owned=owner==viewer;access.revealed=revealed;
        access.governor=m_simulation->value.resource_modifiers.planetary_governor==viewer;
        bool unit_authority=true;
        for(const auto& live:m_um->GetUnits()) {
            const auto* actor=live.second;const auto* position=actor->GetTile();if(!position) {continue;}
            int dx=int(position->coord.x)-int(tile->coord.x),dy=int(position->coord.y)-int(tile->coord.y);
            if(data.horizontal_wrap) {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
            const bool on_base=dx==0 && dy==0,neighbor=(dy==0 && std::abs(dx)==2) || (dx==0 && std::abs(dy)==2) || (std::abs(dx)==1 && std::abs(dy)==1);
            if(!on_base && !neighbor) {continue;}
            if(!actor->IsConventional() && !actor->IsNative()) {unit_authority=false;continue;}
            const auto unit_owner=actor->SimulationOwner();
            access.unit_here|=on_base && unit_owner>=0 && unit_owner<8;
            access.neighbor_unit|=neighbor && unit_owner==viewer;
        }
        const auto own_relations=m_simulation->value.hostility.owners.find(owner),viewer_relations=m_simulation->value.hostility.owners.find(viewer);
        const bool relation_authority=own_relations!=m_simulation->value.hostility.owners.end() && viewer_relations!=m_simulation->value.hostility.owners.end();
        if(relation_authority) {access.owner_pact=(own_relations->second.directions.at(viewer).primary&1u)!=0;
            access.viewer_infiltration=(viewer_relations->second.directions.at(owner).primary&0x1000u)!=0;}
        const bool style_known=unique_identity && (access.unit_here || (unit_authority && (access.owned || access.revealed || access.governor || access.neighbor_unit || relation_authority)));
        bases.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"x",VALUE(gse::value::Int,,tile->coord.x)},{"y",VALUE(gse::value::Int,,tile->coord.y)},
            {"file",VALUE(gse::value::String,,art.file)},{"owned",VALUE(gse::value::Bool,,owner==viewer)},
            {"population",VALUE(gse::value::Int,,population)},{"column",VALUE(gse::value::Int,,body.column)},{"row",VALUE(gse::value::Int,,body.row)},
            {"native_owner",VALUE(gse::value::Int,,native_owner)},{"spartans",VALUE(gse::value::Bool,,profile->filename=="spartans.txt")},
            {"filled",VALUE(gse::value::Bool,,::game::smac_base_body::Filled(access))},{"style_known",VALUE(gse::value::Bool,,style_known)}
        })));
    }
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"kind",VALUE(gse::value::String,,"smac-detail-ordinary-v2")},{"horizontal_wrap",VALUE(gse::value::Bool,,data.horizontal_wrap)},
        {"tiles",VALUE(gse::value::String,,tiles)},{"bases",VALUE(gse::value::Array,,bases)}
    }));
}

}
