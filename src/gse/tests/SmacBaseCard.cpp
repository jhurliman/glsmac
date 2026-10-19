#include "SmacBaseCard.h"
#include "fixtures/SmacBaseCard.h"
#include "game/frontend/widget/SmacBaseCard.h"
#include "task/gsetests/GSETests.h"
#include "Mesh.h"
#include <SDL_image.h>
#include <cstdlib>
#include <limits>

namespace gse::tests {
void AddSmacBaseCardTests(task::gsetests::GSETests* task) {
    task->AddTest("Base console installed indexed art matches four original full-frame controls",GT(){
        const auto* path=std::getenv("GLSMAC_TEST_BASE_FACTION_ASSET");if(!path) {GT_OK();}
        return WithWidgetGraphics([&]()->std::string {
            using namespace loader::texture;
            const std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> image(IMG_Load(path),SDL_FreeSurface);
            GT_ASSERT(image && image->format && image->format->BitsPerPixel==8 && image->w>0 && image->h>0
                && image->pitch>=image->w,"installed Base source is not indexed8-bit");
            std::vector<uint8_t> pixels(size_t(image->w)*image->h);
            for(int y=0;y<image->h;++y) {for(int x=0;x<image->w;++x) {
                pixels[size_t(y)*image->w+x]=static_cast<uint8_t>(static_cast<const uint8_t*>(image->pixels)[size_t(y)*image->pitch+x]+10);
            }}
            std::array<types::Color::rgba_t,256> palette{};
            for(size_t i=0;i<256;++i) {palette[i]=types::Color::RGB(i,0,0);}
            const auto selected=MakeSmacIndexedBaseCard(pixels,image->w,image->h,1,1,false,palette);
            const auto roster=MakeSmacIndexedBaseCard(pixels,image->w,image->h,1,1,true,palette);
            GT_ASSERT(selected && roster,"installed retained Base art missing");
            struct Control {bool roster;std::array<int,4> clip;uint64_t hash;size_t changes;std::array<int,4> box;};
            // FNV64 of the full indexed canvases whose original SHA256 values
            // are frozen in base-sprite-scale-cases.json. Reference replay first
            // verifies those SHA256 values against the original native receipt.
            const Control controls[]{
                {false,{0,0,1024,768},0x7ebdf194b8971401ULL,565,{55,78,84,106}},
                {false,{58,83,77,97},0x1986f61c94ab9c60ULL,250,{58,83,77,97}},
                {true,{0,0,1024,768},0xfaba69b375241889ULL,249,{170,205,189,224}},
                {true,{180,214,199,228},0x68aa20879f114d56ULL,77,{180,214,189,224}},
            };
            const auto draw=[](std::vector<uint8_t>& canvas,const SmacBaseCard& card,int ax,int ay,const std::array<int,4>& clip) {
                for(size_t y=0;y<card.texture->GetHeight();++y) {for(size_t x=0;x<card.texture->GetWidth();++x) {
                    const int dx=ax+card.offset[0]+int(x),dy=ay+card.offset[1]+int(y);
                    const auto pixel=card.texture->GetPixel(x,y);
                    if(dx>=clip[0] && dy>=clip[1] && dx<clip[2] && dy<clip[3] && pixel>>24) {canvas[size_t(dy)*1024+dx]=pixel&255;}
                }}
            };
            for(const auto& c:controls) {
                std::vector<uint8_t> canvas(1024*768,100);
                if(c.roster) {
                    draw(canvas,*selected,34,66,{0,0,1024,768});
                    for(int x=34;x<48;++x) {canvas[66*1024+x]=0;canvas[81*1024+x]=0;}
                    for(int y=66;y<82;++y) {canvas[y*1024+34]=0;canvas[y*1024+47]=0;}
                }
                const auto before=canvas;
                draw(canvas,c.roster?*roster:*selected,c.roster?156:34,c.roster?197:66,c.clip);
                uint64_t hash=14695981039346656037ULL;size_t changes=0;std::array<int,4> box{{1024,768,0,0}};
                for(size_t i=0;i<canvas.size();++i) {
                    hash=(hash^canvas[i])*1099511628211ULL;
                    if(canvas[i]!=before[i]) {++changes;const int x=i%1024,y=i/1024;
                        box[0]=std::min(box[0],x);box[1]=std::min(box[1],y);box[2]=std::max(box[2],x+1);box[3]=std::max(box[3],y+1);}
                }
                GT_ASSERT(hash==c.hash && changes==c.changes && box==c.box,"installed Base copy differs from original indexed frame/clip phase");
            }
            GT_OK();
        });
    });

    task->AddTest("Base console native source stepping preserves indexed key and black remap",GT(){
        return WithWidgetGraphics([]()->std::string {
            using namespace loader::texture;
            const auto selected=SmacBaseCardSteps(false),roster=SmacBaseCardSteps(true);
            const std::array<int,3> original_period{{2,1,1}};
            for(size_t i=0;i<800;++i) {
                GT_ASSERT(selected[i]==original_period[i%3] && roster[i]==2,"original800-entry downscale table differs");
            }
            std::vector<uint8_t> indexed(100*75,9);
            std::array<types::Color::rgba_t,256> palette{};
            for(size_t i=0;i<256;++i) {palette[i]=types::Color::RGB(i,1,2);}
            for(int y=0;y<38;++y) {for(int x=0;x<41;++x) {indexed[(17+y)*100+27+x]=20+(y*41+x)%200;}}
            indexed[17*100+27]=9;indexed[17*100+29]=0;
            const std::array<int,8> selected_sources{{0,2,3,4,6,7,8,10}},roster_sources{{0,2,4,6,8,10,12,14}};
            for(bool small:{false,true}) {
                auto card=MakeSmacIndexedBaseCard(indexed,100,75,0,0,small,palette);
                GT_ASSERT(card,"indexed native Base source was rejected");
                const auto& sources=small?roster_sources:selected_sources;
                for(size_t x=0;x<sources.size();++x) {
                    const auto raw=indexed[17*100+27+sources[x]];
                    const auto expected=raw==9?types::Color::rgba_t(0):raw==0?types::Color::RGB(0,0,0):palette[raw];
                    GT_ASSERT(card->texture->GetPixel(x,0)==expected,"native sampler used rounded-size center/floor stepping or blended source0");
                }
                GT_ASSERT(card->texture->GetPixel(0,1)==palette[indexed[19*100+27]],"native vertical step phase differs");
            }
            GT_ASSERT(!MakeSmacIndexedBaseCard(indexed,99,75,0,0,false,palette),"indexed dimensions mismatch accepted");
            GT_OK();
        });
    });

    task->AddTest("Base console artwork rectangles match thirteen original selected and roster contexts",GT(){
        return WithWidgetGraphics([]()->std::string {
            using namespace game::frontend::widget;
            for(const auto& c:base_card_fixture::cases) {
                const auto rect=SmacBaseCardRectangle(c.offset,c.size,c.roster);
                const std::array<int,4> destination{{rect[0]+c.anchor[0],rect[1]+c.anchor[1],rect[2]+c.anchor[0],rect[3]+c.anchor[1]}};
                GT_ASSERT(destination==c.destination,"native Base rectangle differs for "+std::string(c.name));
                std::optional<SmacBaseCard> raster;
                {
                    types::texture::Texture atlas(128,128);
                    atlas.Fill(11+c.offset[0],13+c.offset[1],10+c.offset[0]+c.size[0],12+c.offset[1]+c.size[1],types::Color::FromRGB(20,40,60));
                    raster=MakeSmacBaseCard(atlas,11,13,c.roster);
                } // output must not borrow the source asset/atlas buffer
                const std::array<int,2> offset{{rect[0],rect[1]}};
                GT_ASSERT(raster && raster->offset==offset
                    && raster->texture->GetWidth()==static_cast<size_t>(rect[2]-rect[0])
                    && raster->texture->GetHeight()==static_cast<size_t>(rect[3]-rect[1]),"retained Base crop was fitted or rounded as a whole frame");
                GT_ASSERT(raster->texture->GetPixel(0,0)==types::Color::RGB(20,40,60),"Base card borrowed retired atlas pixels");
            }
            types::texture::Texture empty(100,75);
            GT_ASSERT(!MakeSmacBaseCard(empty,0,0,false) && !MakeSmacBaseCard(empty,1,0,false),"empty or out-of-bounds source was admitted");
            GT_OK();
        });
    });
}
}
