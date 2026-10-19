#include "SmacBaseCard.h"
#include "fixtures/SmacBaseCard.h"
#include "game/frontend/widget/SmacBaseCard.h"
#include "task/gsetests/GSETests.h"
#include "Mesh.h"

namespace gse::tests {
void AddSmacBaseCardTests(task::gsetests::GSETests* task) {
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
