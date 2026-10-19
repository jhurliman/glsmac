#include "SmacCardOverlay.h"
#include "game/backend/settings/SmacBadgeSlots.h"
#include "game/FrontendRequest.h"
#include "loader/texture/SmacUnitBadge.h"
#include "loader/texture/SmacPalette.h"
#include "task/gsetests/GSETests.h"
#include <memory>

namespace gse::tests {
void AddSmacCardOverlayTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC card badge original loaded slots and retained identity admission",GT(){
        using namespace game::backend::settings;
        SmacFactionStartupRules profiles;
        for(size_t i=0;i<7;++i) {const auto& source=SmacFactionStartupRules::Bindings()[i];
            profiles.factions[i].port_id=source.port_id;profiles.factions[i].filename=source.filename;profiles.factions[i].section=source.section;}
        std::array<std::optional<std::string>,7> players{};players[0]="UNIVERSITY";players[6]="GAIANS";
        auto mapped=SmacBadgeSlots(profiles,players);
        GT_ASSERT(mapped[0]==3 && mapped[6]==1 && mapped[1]==255,"Badge palette slot followed dense/transport owner instead of frozen loaded definition");
        std::optional<std::array<std::string,8>> retained{std::in_place};
        for(size_t i=1;i<8;++i) {retained->at(i)=profiles.factions[i-1].section;}
        GT_ASSERT(SmacBadgeSlots(profiles,players,retained)==mapped,"Consistent saved native identity was discarded");
        std::swap(retained->at(1),retained->at(3));
        auto absent=SmacBadgeSlots(profiles,players,retained);
        for(auto owner:absent) {GT_ASSERT(owner==255,"Saved custom order was overwritten by canonical profile lookup");}
        players[1]="UNIVERSITY";absent=SmacBadgeSlots(profiles,players);
        for(auto owner:absent) {GT_ASSERT(owner==255,"Duplicate port owners were guessed into original loaded slots");}
        players[1]="UNKNOWN";absent=SmacBadgeSlots(profiles,players);
        for(auto owner:absent) {GT_ASSERT(owner==255,"Unknown profile received fabricated badge colors");}
        players[1].reset();std::swap(profiles.factions[0],profiles.factions[2]);absent=SmacBadgeSlots(profiles,players);
        for(auto owner:absent) {GT_ASSERT(owner==255,"Reordered source catalog silently canonicalized");}
        game::FrontendRequest::slot_define_t legacy{0,"UNIVERSITY"};
        GT_ASSERT(legacy.native_palette_slot==255,"Legacy/default slot publication invented source identity");
        game::FrontendRequest message(game::FrontendRequest::FR_SLOT_DEFINE);
        message.data.slot_define.slotdefs=new game::FrontendRequest::slot_defines_t{{0,"UNIVERSITY",3},legacy};
        game::FrontendRequest copy(message);
        GT_ASSERT(copy.data.slot_define.slotdefs!=message.data.slot_define.slotdefs
            && copy.data.slot_define.slotdefs->at(0).native_palette_slot==3 && copy.data.slot_define.slotdefs->at(1).native_palette_slot==255,
            "Owned slot request copy lost explicit palette identity or absence");
        GT_OK();
    });
    task->AddTest("SMAC card badge indexed UNITS recoloring and software display palette",GT(){
        using namespace loader::texture;
        std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> units(SDL_CreateRGBSurfaceWithFormat(0,170,762,8,SDL_PIXELFORMAT_INDEX8),SDL_FreeSurface);
        GT_ASSERT(units && units->pixels,"Cannot allocate indexed badge fixture");
        for(int y=0;y<units->h;++y) {std::fill_n(static_cast<uint8_t*>(units->pixels)+size_t(y)*units->pitch,units->w,uint8_t{255});}
        const auto put=[&](int x,int y,uint8_t game_index) {static_cast<uint8_t*>(units->pixels)[size_t(y)*units->pitch+x]=static_cast<uint8_t>(game_index-10);};
        put(25,731,4);put(26,731,3);put(27,731,65);put(28,731,6);put(29,731,7);put(30,731,155);
        const auto source=ExtractSmacUnitBadge(units.get(),1,1);
        GT_ASSERT(source[0]==4 && source[1]==3 && source[2]==65 && source[3]==9 && source[4]==9,"Wrong native template bank/preprocessing/wrapped source bytes");
        std::array<SDL_Color,256> entries{};entries[145]={216,224,244,255};entries[199]={251,7,239,255};entries[55]={0,0,0,255};
        SDL_Palette common{};common.ncolors=256;common.colors=entries.data();
        const auto palette=MakeSmacSoftwarePalette(&common);
        const auto colored=ColorSmacUnitBadge(source,255,209,palette);
        GT_ASSERT(colored.indexed[0]==255 && colored.rgba[0]==255 && colored.rgba[3]==255,"Reserved primary white was capped or RGB-matched");
        GT_ASSERT(colored.indexed[1]==209 && colored.rgba[4]==235 && colored.rgba[5]==7 && colored.rgba[6]==235,"Secondary common palette skipped Gamma100 channel cap");
        GT_ASSERT(colored.indexed[2]==65 && colored.rgba[8]==0 && colored.rgba[11]==255,"Opaque black became a health hole or semitransparent shadow");
        GT_ASSERT(colored.rgba[15]==0 && colored.rgba[19]==0 && colored.rgba[20]==216 && colored.rgba[22]==235,"Key preprocessing or active display palette differs");
        const auto alternate=ColorSmacUnitBadge(source,241,32,palette);
        GT_ASSERT(alternate.indexed[0]==241 && alternate.indexed[1]==32 && source[0]==4,"Owner recolor mutated retained template or used faction-name colors");
        bool rejected=false;try {ExtractSmacUnitBadge(units.get(),7,0);} catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Out-of-bank badge admitted");
        GT_OK();
    });
}
}
