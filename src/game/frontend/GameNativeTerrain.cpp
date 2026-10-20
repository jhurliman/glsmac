#include "Game.h"
#include "game/ActorVisibilitySnapshot.h"
#include "SmacResourceArt.h"
#include "NativeTerrainRenderer.h"
#include "NativeTerrainSession.h"
#include "SmacDevelopedOverlay.h"
#include "SmacTilePreview.h"
#include "widget/TilePreview.h"
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
bool Game::AllowsNativeTileInspection(int x,int y) const {
    if(m_native_inspector) {return true;}
    if(m_actor_visibility) {return !m_world_retired && m_actor_visibility->Known(x,y);}
    if(!m_map_data.native_projection) {return true;}
    if(m_world_retired || !m_native_world || x<0 || x>=80 || y<0 || y>=80 || ((x^y)&1)) {return false;}
    return m_native_world->KnowledgeAt(size_t(y)*40+size_t(x)/2)==::game::native_terrain::World::Knowledge::Known;
}
std::shared_ptr<types::texture::Texture> Game::GetNativeTilePreview(int x,int y) {
    if(m_world_retired || !m_native_world) {return {};}
    const std::array<int,2> tile{{x,y}};
    if(m_native_preview_tile==tile) {return m_native_preview_texture;}
    m_native_preview_tile=tile;m_native_preview_texture.reset();
    if(!m_native_source || m_native_source->faction_files!=m_native_world->faction_files) {
        m_native_source=g_engine->GetTextureLoader()->GetSmacTerrainSource(m_native_world->faction_files);
    }
    if(!m_native_source) {return {};}
    const smac_terrain_raster::Bitmap backing{89,117,std::vector<uint8_t>(89*117,0)};
    const auto frame=smac_tile_preview::Paint(*m_native_world,*m_native_source,x,y,
        g_engine->GetConfig()->GetNativeTerrainBaseLow16(),backing);
    if(!frame) {return {};}
    auto texture=std::make_shared<types::texture::Texture>("Native console tile preview",89,117);
    std::copy(frame->rgba.begin(),frame->rgba.end(),texture->GetBitmap());
    m_native_preview_texture=std::move(texture);return m_native_preview_texture;
}
void Game::UpdateNativeTerrainSpriteVisibility(const std::vector<bool>* covered,const std::vector<bool>* landmarks,const std::vector<bool>* resources) {
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
void Game::RestoreNativeTerrainProjection() {
    if(m_native_view && m_tm) {for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {m_tm->GetTile(x,y)->SetNativeProjectorHeight(std::nullopt);}}}
    m_native_view.reset();
}
void Game::ClearNativeTerrain() {
    RestoreNativeTerrainProjection();
    UpdateNativeTerrainSpriteVisibility(nullptr);m_native_legacy_instances.clear();
    if(m_native_actor) {m_world_scene->RemoveActor(m_native_actor);DELETE(m_native_actor);m_native_actor=nullptr;}
    if(m_native_texture) {DELETE(m_native_texture);m_native_texture=nullptr;}
    m_native_world.reset();m_native_source.reset();
    m_native_preview_tile.reset();m_native_preview_texture.reset();
    m_native_inspector=m_native_snapshot_dirty=m_native_frame_dirty=false;
}
void Game::UpdateNativeTerrain() {
    if(!m_map_data.native_projection || m_world_retired) {return;}
    if(m_mt_ids.native_terrain) {
        auto response=m_game->MT_GetResponse(m_mt_ids.native_terrain);
        if(response.result!=backend::R_NONE) {
            m_mt_ids.native_terrain=0;
            if(response.world_ticket==m_world_ticket && response.result==backend::R_SUCCESS) {
                // Steal only the owned value. DestroyResponse handles cancelled or
                // obsolete worlds without leaving borrowed backend map pointers.
                m_native_world.reset(response.data.native_terrain);
                if(m_native_view) {
                    if(!m_native_world || !m_native_view->Matches(*m_native_world)) {RestoreNativeTerrainProjection();}
                    else {m_native_view->frame.reset();m_native_view->layout.reset();}
                }
                response.data.native_terrain=nullptr;m_native_frame_dirty=true;
                // Cache input identity includes this complete owned snapshot,
                // viewer and asset-family table. No old-world result survives.
                m_native_preview_tile.reset();m_native_preview_texture.reset();
                if(m_widgets.TilePreview) {m_widgets.TilePreview->Refresh();}
                if(m_tm && m_tm->GetSelectedTile()) {UpdateTilePreview(m_tm->GetSelectedTile());}
            }
            m_game->MT_DestroyResponse(response);
        }
    }
    if(m_native_snapshot_dirty && !m_mt_ids.native_terrain) {
        m_native_snapshot_dirty=false;
        m_mt_ids.native_terrain=m_game->MT_GetNativeTerrain(m_world_ticket,m_native_inspector);
    }
    if(!m_native_inspector) {return;}
    const bool admitted=m_native_world && m_viewport.window_width==1024 && m_viewport.window_height==768
        && std::abs(m_camera_position.z-0.125f)<0.000001f;
    if(!admitted) {RestoreNativeTerrainProjection();if(m_native_actor) {m_native_actor->Hide();}UpdateNativeTerrainSpriteVisibility(nullptr);return;}
    if(!m_native_frame_dirty) {return;}
    if(!m_native_source || m_native_source->faction_files!=m_native_world->faction_files) {m_native_source=g_engine->GetTextureLoader()->GetSmacTerrainSource(m_native_world->faction_files);}
    const auto& matrix=m_camera->GetMatrix().m;
    // Translate the port's flat camera origin onto the native doubled-X grid.
    // The original selector owns padded rows/columns; clipping flat centers
    // first would discard tiles whose geometry or art reaches into the view.
    auto origin=m_tm->GetTile(0,0)->GetRenderData().coords.InvertY();
    origin.z=backend::map::s_consts.tile.scale.z;
    const auto px=(matrix[0][0]*origin.x+matrix[0][1]*origin.y+matrix[0][2]*origin.z+matrix[0][3]+1)*512;
    const auto py=(1-(matrix[1][0]*origin.x+matrix[1][1]*origin.y+matrix[1][2]*origin.z+matrix[1][3]))*384;
    const int first_y=std::clamp(static_cast<int>(std::lround(-py/24)),-2,60);
    const int parity=(first_y%2+2)%2;
    int first_x=2*static_cast<int>(std::lround((-px/48-parity)/2))+parity;
    if(!m_native_world->horizontal_wrap) {first_x=std::clamp(first_x,-4,62);}
    int camera_x=(first_x+11)%80;if(camera_x<0) {camera_x+=80;}
    const auto layout=native_terrain::Layout(camera_x,first_y+12,0,m_native_world->horizontal_wrap);
    if(!m_native_world->horizontal_wrap) {first_x=layout.first[0];}
    auto placements=native_terrain::SelectView(layout);
    // Align the actual camera, including GPU units and Bases, with the native
    // pixel grid. Translating only the candidate list leaves edge wedges when
    // the port's smooth camera falls between native rows/columns.
    m_camera_position.x-=(px+first_x*48)/1024.0f;
    m_camera_position.y-=(py+first_y*24)*m_viewport.ratio.y/768.0f;
    ApplyCameraPosition();
    // Default controlled comparison base0x40100000. A measured original run
    // can override its low16 explicitly, without consuming any RNG stream.
    // The opt-in inspector presents resources/sites as well as improvements.
    // Keep the raw selector gate explicit; this is a presentation profile,
    // not a claim about unimplemented original display-option persistence.
    native_terrain::Frame frame;
    if(native_terrain::Session::Admits(*m_native_world)) {
        if(!m_native_view) {m_native_view=std::make_shared<native_terrain::Session>(*m_native_world);}
        const bool incremental=m_native_view->frame && m_native_view->layout &&
            bool(native_terrain::HorizontalPan(*m_native_view->layout,layout));
        const auto plan=m_native_view->geometry.Prepare(layout,0,25,incremental);
        frame=native_terrain::RenderPrepared(*m_native_world,*m_native_source,plan,
            g_engine->GetConfig()->GetNativeTerrainBaseLow16(),0x10621,0,incremental?&*m_native_view->frame:nullptr);
        // The object pass already performed its own projector query. Reuse
        // that result for bodies, badges and names without touching caches.
        for(const auto& object:plan.objects) {
            const int height=(object.tile[1]-layout.first[1]-1)*24-object.projector[1];
            m_tm->GetTile(object.tile[0],object.tile[1])->SetNativeProjectorHeight(height);
        }
        m_native_view->frame=frame;m_native_view->layout=layout;
    } else {
        RestoreNativeTerrainProjection();
        frame=native_terrain::Render(*m_native_world,*m_native_source,placements,g_engine->GetConfig()->GetNativeTerrainBaseLow16(),{{0,0}},0x10621);
    }
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
