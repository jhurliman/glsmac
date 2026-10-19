#include "Game.h"
#include "game/ScoutBattlePresentation.h"
#include "AnimationDef.h"
#include "game/backend/animation/FramesRow.h"
#include "game/frontend/unit/Unit.h"
#include "game/frontend/unit/UnitManager.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sound.h"
#include "engine/Engine.h"
#include "audio/Audio.h"
#include "loader/texture/TextureLoader.h"
#include <chrono>
#include <memory>
namespace game::frontend {
struct Game::ScoutBattle {
    scout_presentation::Playback playback;
    std::array<std::unique_ptr<AnimationDef>,2> definitions;
    sprite::InstancedSprite* sprite=nullptr;size_t instance=0;
    explicit ScoutBattle(const scout_presentation::Plan& plan):playback(plan) {}
    void Clear() {if(sprite) {sprite->actor->RemoveInstance(instance);sprite=nullptr;}}
    ~ScoutBattle() {Clear();}
};
void Game::StartScoutBattle(const scout_presentation::Plan& plan) {
    for(const auto id:plan.units) {if(!m_um->GetUnitById(id)) {return;}}
    auto battle=std::make_shared<ScoutBattle>(plan);
    for(size_t i=0;i<2;++i) {
        const uint16_t width=i ? 100:43,height=i ? 150:47;
        backend::animation::FramesRow definition(i ? "ScoutDeath":"ScoutHit",i ? "xf.pcx":"xi.pcx",2,2,width,height,
            width/2,height-16,2,16,i ? 8:16,float(width)/64,float(height)/64,i ? 800:400,i ? "boom.wav":"energy1.wav");
        battle->definitions[i]=std::make_unique<AnimationDef>(m_ism,&definition,g_engine->GetTextureLoader()->GetSmacEffectTexture(i!=0));
        (void)battle->definitions[i]->GetSprites();
        if(g_engine->GetAudio()->IsPlaybackAvailable()) {(void)battle->definitions[i]->GetSound();}
    }
    m_map_control.is_dragging=false;
    m_scout_battle=std::move(battle);IterateScoutBattle();
}
void Game::IterateScoutBattle() {
    for(auto it=m_scout_sounds.begin();it!=m_scout_sounds.end();) {
        if((*it)->IsFinished()) {g_engine->GetAudio()->RemoveAndDeleteActor(*it);it=m_scout_sounds.erase(it);}else {++it;}
    }
    if(!m_scout_battle) {return;}
    const auto now=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    auto& battle=*m_scout_battle;
    while(const auto action=battle.playback.Next(now)) {
        using Kind=scout_presentation::Playback::Kind;
        if(action->kind==Kind::Finished) {m_scout_battle.reset();return;}
        if(action->kind==Kind::Health) {
            for(size_t i=0;i<2;++i) {auto* unit=m_um->GetUnitById(battle.playback.GetPlan().units[i]);
                if(unit) {unit->SetHealth(float(action->step.health[i])/10);unit->Refresh();if(m_um->GetSelectedUnit()==unit) {UpdateUnitPreview(unit);}}
            }continue;
        }
        battle.Clear();if(action->kind==Kind::Clear) {continue;}
        const size_t index=action->step.effect==scout_presentation::Effect::Death ? 1:0;
        auto* unit=m_um->GetUnitById(battle.playback.GetPlan().units[action->step.target]);if(!unit) {continue;}
        if(action->frame==0 && g_engine->GetAudio()->IsPlaybackAvailable()) {
            auto sound=std::make_unique<scene::actor::Sound>("Scout battle",battle.definitions[index]->GetSound());
            m_scout_sounds.reserve(m_scout_sounds.size()+1);g_engine->GetAudio()->AddActor(sound.get());
            m_scout_sounds.push_back(sound.release());m_scout_sounds.back()->Play();
        }
        auto* sprite=battle.definitions[index]->GetSprites().at(action->frame);
        const auto instance=sprite->actor->AddInstance(unit->GetTile()->GetRenderData().coords.InvertY());
        battle.sprite=sprite;battle.instance=instance;
    }
}
void Game::CancelScoutBattle() {
    m_scout_battle.reset();m_deferred_frontend.clear();
    for(auto* sound:m_scout_sounds) {sound->Stop();g_engine->GetAudio()->RemoveAndDeleteActor(sound);}m_scout_sounds.clear();
}
}
