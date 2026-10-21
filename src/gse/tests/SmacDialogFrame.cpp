#include "fixtures/ConsolePanelBackdrop.h"
#include "util/SHA256.h"
#include "SmacDialogFrame.h"
#include "fixtures/SmacDialogFrame.h"
#include "fixtures/WorkshopStripFrames.h"
#include "loader/texture/SmacDialogFrame.h"
#include "task/gsetests/GSETests.h"
#include "util/FinallyGuard.h"
#include <SDL_image.h>
#include <cstdlib>
#include <memory>
namespace gse::tests {
using loader::texture::SmacDialogSource;
namespace frame=loader::texture::smac_dialog_frame;
void AddSmacDialogFrameTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC small dialog frame distinguishes source9 from destination7 and final copy",GT(){
        SmacDialogSource source;source.indexed.assign(750*900,9);source.palette[7]={192,192,192};source.palette[9]={9,0,0};
        source.indexed[229*750+1]=7; // TL pixel; null center suppresses overlays
        std::vector<uint8_t> bitmap(30*31,23);frame::Paint(bitmap,30,31,source,{0,0,0,10,11},{0,0,30,31});
        GT_ASSERT(bitmap[0]==7,"loaded7 was treated as sprite source transparency");
        source.indexed[229*750+1]=9;
        for(int y=0;y<24;++y) {for(int x=0;x<24;++x) {source.indexed[(236+y)*750+8+x]=static_cast<uint8_t>(31+(x+y)%3);}}
        bitmap.assign(30*31,7);frame::Paint(bitmap,30,31,source,{0,0,0,10,11},{0,0,30,31});
        GT_ASSERT(bitmap[0]==31 && bitmap[1]==32 && bitmap[30]==32,"key9 corners did not receive destination7 pattern overlays");
        source.indexed[242*750+15]=9;source.indexed[242*750+16]=7;
        bitmap.assign(30*31,23);frame::Paint(bitmap,30,31,source,{0,0,0,25,25},{0,0,30,31});
        GT_ASSERT(bitmap[6*30+7]==9 && bitmap[6*30+8]==7,"final center copy skipped literal7/9 pattern values");
        const auto rgba=frame::Rgba(std::vector<uint8_t>{7,9},source);
        GT_ASSERT(rgba[3]==255 && rgba[7]==0,"palette conversion globally hid destination7 or made key9 opaque");
        GT_OK();
    });
    task->AddTest("SMAC small dialog frame original indexed primitive controls",GT(){
        const auto* assets=std::getenv("GLSMAC_DIALOG_FRAME_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto path=std::string(assets)+"/interface.pcx";
        std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> image(IMG_Load(path.c_str()),SDL_FreeSurface);
        GT_ASSERT(image && image->format && image->format->BitsPerPixel==8 && image->w==750 && image->h==900 && image->pitch>=750,"Original frame atlas is unavailable or differs in shape");
        SmacDialogSource source;source.indexed.resize(750*900);
        GT_ASSERT(SDL_LockSurface(image.get())==0,"Cannot lock original frame atlas");
        util::FinallyGuard unlock([&]{SDL_UnlockSurface(image.get());});
        for(int y=0;y<900;++y) {const auto* row=static_cast<const uint8_t*>(image->pixels)+size_t(y)*image->pitch;
            for(int x=0;x<750;++x) {source.indexed[size_t(y)*750+x]=static_cast<uint8_t>(row[x]+10);}}
        size_t pixels=0;
        for(const auto& c:smac_frame_fixture::Cases) {
            std::vector<uint8_t> bitmap(size_t(c.w)*c.h);
            for(int y=0;y<c.h;++y) {for(int x=0;x<c.w;++x) {bitmap[size_t(y)*c.w+x]=c.background7?7:(x+y)%3==0?41:23;}}
            frame::Paint(bitmap,c.w,c.h,source,c.call,c.clip);
            uint64_t hash=14695981039346656037ull;for(auto v:bitmap) {hash=(hash^v)*1099511628211ull;}
            GT_ASSERT(hash==c.hash,"Original small-frame indexed control differs: "+std::string(c.name));
            pixels+=bitmap.size();
        }
        GT_ASSERT(pixels==43431,"Original frame control coverage changed");
        for(size_t style=0;style<4;++style) {
            std::vector<uint8_t> bitmap(70*50,9);frame::Paint(bitmap,70,50,source,{int(style),0,0,70,50},{0,0,70,50});
            uint64_t hash=14695981039346656037ull;for(auto value:bitmap) {hash=(hash^value)*1099511628211ull;}
            GT_ASSERT(hash==workshop_strip_fixture::FRAMES_STYLE[style],"Complete original Workshop frame style differs: "+std::to_string(style));
        }
        const auto digest=[](const auto& bytes){return util::SHA256(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));};
        for(const auto& c:console_panel_fixture::Cases) {
            std::vector<uint8_t> bitmap(1024*257);
            for(int y=0;y<257;++y) {for(int x=0;x<1024;++x) {bitmap[size_t(y)*1024+x]=(x+y)%3==0?41:23;}}
            frame::Paint(bitmap,1024,257,source,{21,c.outer[0],c.outer[1],c.outer[2],c.outer[3]},c.clip);
            frame::Paint(bitmap,1024,257,source,{18,c.inner[0],c.inner[1],c.inner[2],c.inner[3]},c.clip);
            GT_ASSERT(digest(bitmap)==c.full,"Original complete console panel differs: "+std::string(c.name));
            std::vector<uint8_t> crop;
            for(int y=c.crop[1];y<c.crop[3];++y) {crop.insert(crop.end(),bitmap.begin()+size_t(y)*1024+c.crop[0],bitmap.begin()+size_t(y)*1024+c.crop[2]);}
            GT_ASSERT(digest(crop)==c.cropped,"Original clipped console panel phase differs: "+std::string(c.name));
        }GT_OK();
    });
}
}
