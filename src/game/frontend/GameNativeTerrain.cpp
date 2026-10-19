#include "Game.h"
#include "SmacResourceArt.h"
#include "NativeTerrainRenderer.h"
#include "SmacDevelopedOverlay.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "scene/actor/Instanced.h"
#include "game/backend/Game.h"
#include "game/backend/map/Consts.h"
#include "game/frontend/tile/TileManager.h"
#include "engine/Engine.h"
#include "config/Config.h"
#include "loader/texture/TextureLoader.h"
#include "scene/Scene.h"
#include "scene/Camera.h"
#include "scene/actor/Mesh.h"
#include "types/mesh/Rectangle.h"
#include "types/texture/Texture.h"
#include <algorithm>
#include <cmath>

namespace game::frontend {
void Game::UpdateNativeTerrainSpriteVisibility(const std::array<bool,3200>* covered,const std::array<bool,3200>* landmarks,const std::array<bool,3200>* resources) {
    std::unordered_set<size_t> hidden;
    if((covered || landmarks || resources) && m_tm && m_map_data.width==80 && m_map_data.height==80) {
        for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
            const auto index=static_cast<size_t>(y)*40+x/2;
            const bool developed=covered && covered->at(index),named=landmarks && landmarks->at(index);
            const bool resource=resources && resources->at(index);
            if(!developed && !named && !resource) {continue;}
            for(const auto& sprite:m_tm->GetTile(x,y)->GetRenderData().terrain_instances) {
                const auto entry=m_native_legacy_instances.find(sprite.id);
                if(entry!=m_native_legacy_instances.end() && entry->second.first==sprite.actor && ((developed && smac_developed_overlay::ReplacesLegacySprite(sprite.name)) ||
                    (named && (sprite.name=="Uranium" || sprite.name=="Geothermal")) ||
                    (resource && smac_resource_art::ReplacesLegacySprite(sprite.name)))) {hidden.insert(sprite.id);}
            }
        }}
    }
    for(const auto id:m_native_hidden_instances) {if(!hidden.count(id)) {
        const auto entry=m_native_legacy_instances.find(id);if(entry==m_native_legacy_instances.end()) {continue;}
        auto* actor=m_ism->GetInstancedSpriteByKey(entry->second.first)->actor;
        actor->SetInstance(id,entry->second.second);
    }}
    for(const auto id:hidden) {
        const auto& entry=m_native_legacy_instances.at(id);
        auto* actor=m_ism->GetInstancedSpriteByKey(entry.first)->actor;
        if(actor->HasInstance(id)) {actor->RemoveInstance(id);}
    }
    m_native_hidden_instances=std::move(hidden);
}
void Game::ClearNativeTerrain() {
    UpdateNativeTerrainSpriteVisibility(nullptr);m_native_legacy_instances.clear();
    if(m_native_actor) {m_world_scene->RemoveActor(m_native_actor);DELETE(m_native_actor);m_native_actor=nullptr;}
    if(m_native_texture) {DELETE(m_native_texture);m_native_texture=nullptr;}
    m_native_world.reset();m_native_source.reset();
    m_native_inspector=m_native_snapshot_dirty=m_native_frame_dirty=false;
}
void Game::UpdateNativeTerrain() {
    if(!m_native_inspector || m_world_retired) {return;}
    if(m_mt_ids.native_terrain) {
        auto response=m_game->MT_GetResponse(m_mt_ids.native_terrain);
        if(response.result!=backend::R_NONE) {
            m_mt_ids.native_terrain=0;
            if(response.world_ticket==m_world_ticket && response.result==backend::R_SUCCESS) {
                // Steal only the owned value. DestroyResponse handles cancelled or
                // obsolete worlds without leaving borrowed backend map pointers.
                m_native_world.reset(response.data.native_terrain);
                response.data.native_terrain=nullptr;m_native_frame_dirty=true;
            }
            m_game->MT_DestroyResponse(response);
        }
    }
    if(m_native_snapshot_dirty && !m_mt_ids.native_terrain) {
        m_native_snapshot_dirty=false;
        m_mt_ids.native_terrain=m_game->MT_GetNativeTerrain(m_world_ticket);
    }
    const bool admitted=m_native_world && m_viewport.window_width==1024 && m_viewport.window_height==768
        && std::abs(m_camera_position.z-0.125f)<0.000001f;
    if(!admitted) {if(m_native_actor) {m_native_actor->Hide();}UpdateNativeTerrainSpriteVisibility(nullptr);return;}
    if(!m_native_frame_dirty) {return;}
    if(!m_native_source || m_native_source->faction_files!=m_native_world->faction_files) {m_native_source=g_engine->GetTextureLoader()->GetSmacTerrainSource(m_native_world->faction_files);}
    const auto& matrix=m_camera->GetMatrix().m;
    std::vector<native_terrain::Placement> placements;
    // Preserve the original row/left-to-right painter order. Each horizontal
    // wrap copy is translated before clipping, without changing its record seed.
    for(int y=0;y<80;++y) {
        std::vector<native_terrain::Placement> row;
        for(int x=y&1;x<80;x+=2) {
            auto coord=m_tm->GetTile(x,y)->GetRenderData().coords.InvertY();
            coord.z=backend::map::s_consts.tile.scale.z;
            for(int copy=m_native_world->horizontal_wrap?-1:0;copy<=(m_native_world->horizontal_wrap?1:0);++copy) {
                const auto wx=coord.x+copy*80*backend::map::s_consts.tile.radius.x;
                const auto px=(matrix[0][0]*wx+matrix[0][1]*coord.y+matrix[0][2]*coord.z+matrix[0][3]+1)*512;
                const auto py=(1-(matrix[1][0]*wx+matrix[1][1]*coord.y+matrix[1][2]*coord.z+matrix[1][3]))*384;
                // Up to100 native height pixels can lift a tile into the view.
                if(px<-48 || px>1072 || py<-24 || py>635) {continue;}
                row.push_back({{{x,y}},{{static_cast<int>(std::lround(px)),static_cast<int>(std::lround(py))}},static_cast<uint32_t>(44*(40*y+x/2))});
            }
        }
        std::sort(row.begin(),row.end(),[](const auto& a,const auto& b){return a.flat_center[0]<b.flat_center[0];});
        placements.insert(placements.end(),row.begin(),row.end());
    }
    // Default controlled comparison base0x40100000. A measured original run
    // can override its low16 explicitly, without consuming any RNG stream.
    // The opt-in inspector presents resources/sites as well as improvements.
    // Keep the raw selector gate explicit; this is a presentation profile,
    // not a claim about unimplemented original display-option persistence.
    auto frame=native_terrain::Render(*m_native_world,*m_native_source,placements,g_engine->GetConfig()->GetNativeTerrainBaseLow16(),{{0,0}},0x10621);
    if(!m_native_texture) {
        NEW(m_native_texture,types::texture::Texture,"NativeTerrainViewport",1024,511);
        NEWV(quad,types::mesh::Rectangle,(types::Vec2<float>{-1,1}),(types::Vec2<float>{1,1-2*511.0f/768}),0.0f);
        // This quad is drawn directly in the world scene with face culling.
        // Rectangle's default second face has the opposite winding.
        quad->SetSurface(1,{0,2,1});
        NEW(m_native_actor,scene::actor::Mesh,"NativeTerrainViewport",quad);
        m_native_actor->SetPosition({0,0,0.2f});
        m_native_actor->SetRenderFlags(scene::actor::Actor::RF_IGNORE_CAMERA|scene::actor::Actor::RF_IGNORE_LIGHTING|scene::actor::Actor::RF_IGNORE_DEPTH);
        m_native_actor->SetTexture(m_native_texture);m_world_scene->AddActor(m_native_actor);
    }
    std::copy(frame.rgba.begin(),frame.rgba.end(),m_native_texture->GetBitmap());
    m_native_texture->FullUpdate();m_native_actor->SetTexture(m_native_texture);m_native_actor->Show();
    UpdateNativeTerrainSpriteVisibility(&frame.developed_tiles,&frame.landmark_tiles,&frame.resource_tiles);
    m_native_frame_dirty=false;
}
}
