#include "Game.h"
#include "game/ActorVisibilitySnapshot.h"
#include "SmacResourceArt.h"
#include "NativeTerrainRenderer.h"
#include "NativeTerrainSession.h"
#include "NativeZoomPolicy.h"
#include "game/frontend/base/BaseManager.h"
#include "game/frontend/unit/UnitManager.h"
#include "input/Event.h"
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
void Game::UpdateNativeTerrainSpriteVisibility(const std::vector<bool>* covered,const std::vector<bool>* landmarks,const std::vector<bool>* resources,bool complete) {
    // The opaque native canvas owns the map. The legacy mesh must not leak
    // through the console edge or beyond a small zoomed-out world.
    if(m_actors.terrain) {if(complete) {m_actors.terrain->Hide();}else {m_actors.terrain->Show();}}
    std::unordered_set<size_t> hidden;
    if(complete) {for(const auto& row:m_native_legacy_instances) {hidden.insert(row.first);}}
    if(!complete && (covered || landmarks || resources) && m_tm && m_map_data.width==80 && m_map_data.height==80) {
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
    const bool was_masked=m_native_object_tiles.has_value();m_native_object_tiles.reset();
    if(m_native_view && m_tm) {for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {m_tm->GetTile(x,y)->SetNativeProjectorHeight(std::nullopt);}}}
    m_native_view.reset();
    if(was_masked) {
        if(m_bm) {m_bm->RefreshVisibility();}if(m_um) {m_um->RefreshVisibility();}
        if(m_world_scene && m_map_data.width && m_map_data.height) {UpdateMapInstances();}
    }
}
void Game::ClearNativeTerrain() {
    m_native_tile_at_response.reset();
    RestoreNativeTerrainProjection();
    UpdateNativeTerrainSpriteVisibility(nullptr);m_native_legacy_instances.clear();
    if(m_native_actor) {m_world_scene->RemoveActor(m_native_actor);DELETE(m_native_actor);m_native_actor=nullptr;}
    if(m_native_texture) {DELETE(m_native_texture);m_native_texture=nullptr;}
    m_native_world.reset();m_native_source.reset();m_native_context.reset();
    m_native_preview_tile.reset();m_native_preview_texture.reset();
    m_native_inspector=m_native_snapshot_dirty=m_native_frame_dirty=false;
    m_native_camera_from_legacy=true;m_native_canvas_blank=false;
}
bool Game::TryNativeTileAt(size_t screen_x,size_t screen_inverse_y) {
    if(!m_native_actor || !m_native_actor->IsVisible()) {return false;}
    // A displayed native frame owns its picking. A miss/unavailable native
    // result must not fall through to the different legacy GPU geometry.
    m_native_tile_at_response=tile_at_result_t{};
    if(m_world_retired || !m_native_view) {return true;}
    if(screen_x>INT32_MAX || screen_inverse_y>m_viewport.window_height) {return true;}
    const auto picked=m_native_view->Pick({{int(screen_x),int(m_viewport.window_height-screen_inverse_y)}});
    if(picked && picked->code==0 && picked->written) {
        m_native_tile_at_response=tile_at_result_t{true,{size_t((*picked->written)[0]),size_t((*picked->written)[1])}};
    }return true;
}
bool Game::IsNativeObjectTile(const types::Vec2<size_t>& tile) const {
    if(!m_native_object_tiles) {return true;}
    return tile.x<80 && tile.y<80 && !((tile.x^tile.y)&1) && m_native_object_tiles->at(tile.y*40+tile.x/2);
}
bool Game::HandleNativeZoom(const input::Event& event) {
    if(!m_map_data.native_projection || m_world_retired || !m_native_context || !m_native_view || !m_native_view->frame) {return false;}
    if(event.type!=input::EV_KEY_DOWN || (event.data.key.modifiers&input::KM_ALT)) {return false;}
    using Command=native_terrain::ZoomCommand;std::optional<Command> command;
    const auto& key=event.data.key;
    if(key.modifiers==(input::KM_CTRL|input::KM_SHIFT) && key.code==input::K_X) {
        m_scroller.Stop();m_native_context->ToggleFlattenedTerrain();
        m_native_camera_from_legacy=false;m_native_frame_dirty=true;return true;
    }
    if(key.modifiers==input::KM_CTRL) {
        if(key.code==input::K_Z) {command=Command::Maximum;}
        else if(key.code==input::K_X) {command=Command::Minimum;}
    }else if(!(key.modifiers&~input::KM_SHIFT)) {
        if(key.key=='z') {command=Command::In;}else if(key.key=='x') {command=Command::Out;}
        else if(key.key=='Z') {command=Command::Default;}else if(key.key=='X') {command=Command::Overview;}
    }
    if(!command) {return false;}
    native_terrain::ZoomViews views{};views[0]={true,true,m_native_context->zoom,m_native_context->camera};
    const auto next=native_terrain::ApplyZoom(views,*command,0);
    // Keep the native precheck result; do not silently clamp retained odd states.
    // The current raster/actor profile admits -14..16 (fresh keyboard views are even).
    ::game::native_zoom::Unit(next.views[0].zoom);
    if(m_bm) {m_bm->PrepareNativeActorZoom(next.views[0].zoom);}if(m_um) {m_um->PrepareNativeBodyZoom(next.views[0].zoom);}
    m_scroller.Stop();m_native_context->zoom=next.views[0].zoom;m_native_context->camera=next.views[0].camera;
    m_native_camera_from_legacy=false;m_native_frame_dirty=true;return true;
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
                    else {m_native_view->InvalidateAppearance();}
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
    const bool admitted=m_native_world && m_native_context && m_viewport.window_width==1024 && m_viewport.window_height==768
        && native_terrain::Session::Admits(*m_native_world);
    if(!admitted) {PresentNativeCanvas(nullptr);return;}
    if(!m_native_frame_dirty) {return;}
    if(!m_native_source || m_native_source->faction_files!=m_native_world->faction_files) {m_native_source=g_engine->GetTextureLoader()->GetSmacTerrainSource(m_native_world->faction_files);}
    const auto origin=m_tm->GetTile(0,0)->GetCanonicalCoords().InvertY();
    const auto projected_origin=[&]() {
        const auto& matrix=m_camera->GetMatrix().m;
        const float z=backend::map::s_consts.tile.scale.z;
        return types::Vec2<float>{(matrix[0][0]*origin.x+matrix[0][1]*origin.y+matrix[0][2]*z+matrix[0][3]+1)*512,
            (1-(matrix[1][0]*origin.x+matrix[1][1]*origin.y+matrix[1][2]*z+matrix[1][3]))*384};
    };
    if(m_native_camera_from_legacy) {
        const int old_zoom=m_native_view && m_native_view->frame?m_native_view->frame->zoom:0;
        const int old_q=::game::native_zoom::Unit(old_zoom);
        const auto basis=native_terrain::Layout(0,0,old_zoom,m_native_world->horizontal_wrap);
        const auto pixel=projected_origin();
        const int first_y=int(std::lround((basis.offset[1]-pixel.y)/(2*old_q)));
        const int cy=std::clamp(first_y+basis.span[1],0,79),parity=((first_y%2)+2)%2;
        const int first_x=2*int(std::lround(((basis.offset[0]-pixel.x)/(4*old_q)-parity)/2))+parity;
        int cx=(first_x+basis.span[0])%80;if(cx<0) {cx+=80;}
        m_native_context->camera={{cx,cy}};m_native_camera_from_legacy=false;
    }
    const int zoom=m_native_context->zoom,q=::game::native_zoom::Unit(zoom);
    if(m_bm) {m_bm->PrepareNativeActorZoom(zoom);}if(m_um) {m_um->PrepareNativeBodyZoom(zoom);}
    m_camera_position.z=q/96.0f;UpdateCameraRange();
    const auto layout=m_native_context->Layout(*m_native_world);
    const auto pixel=projected_origin();
    const int target_x=layout.offset[0]-layout.first[0]*4*q,target_y=layout.offset[1]-layout.first[1]*2*q;
    m_camera_position.x-=(pixel.x-target_x)/1024.0f;
    m_camera_position.y-=(pixel.y-target_y)*m_viewport.ratio.y/768.0f;ApplyCameraPosition();
    if(m_bm) {m_bm->SetNativeActorZoom(zoom);}if(m_um) {m_um->SetNativeBodyZoom(zoom);}
    auto placements=native_terrain::SelectView(layout);
    // Default controlled comparison base0x40100000. A measured original run
    // can override its low16 explicitly, without consuming any RNG stream.
    // The opt-in inspector presents resources/sites as well as improvements.
    // Keep the raw selector gate explicit; this is a presentation profile,
    // not a claim about unimplemented original display-option persistence.
    native_terrain::Frame frame;
    if(native_terrain::Session::Admits(*m_native_world)) {
        if(!m_native_view) {m_native_view=std::make_shared<native_terrain::Session>(*m_native_world);}
        const auto prepared=m_native_view->Prepare(*m_native_world,*m_native_context,
            m_native_inspector?native_terrain::Session::Observation::RevealedCanvas:native_terrain::Session::Observation::Observer);
        if(!prepared) {PresentNativeCanvas(nullptr);return;}
        const auto& plan=*prepared;
        frame=native_terrain::RenderPrepared(*m_native_world,*m_native_source,plan,
            g_engine->GetConfig()->GetNativeTerrainBaseLow16(),m_native_context->flags,0,
            plan.regions?&*m_native_view->frame:nullptr,m_native_context->preferences);
        // Reuse returned heights. Interior actors in a retained pan keep the
        // prior height just like retained pixels; do not add height queries.
        std::unordered_map<size_t,int> heights;
        for(const auto& object:plan.objects) {
            heights[size_t(object.tile[1])*40+object.tile[0]/2]=layout.offset[1]+(object.tile[1]-layout.first[1]-1)*2*q-object.projector[1];
        }
        std::vector<bool> objects(3200,false);
        for(const auto& placement:placements) {
            const auto x=placement.tile[0],y=placement.tile[1];const size_t index=size_t(y)*40+x/2;
            objects[index]=true;auto* tile=m_tm->GetTile(x,y);
            const auto fresh=heights.find(index);const auto height=fresh==heights.end()?tile->GetNativeProjectorHeight():std::optional<int>{fresh->second};
            const int wrap_x=(placement.flat_center[0]-layout.offset[0])/(4*q)-(x-layout.first[0]);
            tile->SetNativeProjectorHeight(height,zoom,wrap_x);
        }
        m_native_object_tiles=std::move(objects);
        m_native_view->Commit(frame,layout);
    } else {PresentNativeCanvas(nullptr);return;}
    PresentNativeCanvas(&frame);
    m_native_frame_dirty=false;
}
void Game::PresentNativeCanvas(const native_terrain::Frame* frame) {
    if(!frame && m_native_canvas_blank) {return;}
    if(!frame) {
        RestoreNativeTerrainProjection();m_native_object_tiles=std::vector<bool>(3200,false);
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
    if(frame) {std::copy(frame->rgba.begin(),frame->rgba.end(),m_native_texture->GetBitmap());}
    else {
        auto* bitmap=m_native_texture->GetBitmap();
        std::fill(bitmap,bitmap+1024*511*4,0);
        for(size_t i=0;i<1024*511;++i) {bitmap[4*i+3]=255;}
    }
    m_native_canvas_blank=frame==nullptr;
    m_native_texture->FullUpdate();m_native_actor->SetTexture(m_native_texture);m_native_actor->Show();
    UpdateNativeTerrainSpriteVisibility(nullptr,nullptr,nullptr,true);
    UpdateMapInstances();
    if(m_bm) {m_bm->RefreshVisibility();}if(m_um) {m_um->RefreshVisibility();}
}
}
