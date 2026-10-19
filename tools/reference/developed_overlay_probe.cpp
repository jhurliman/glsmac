#include "game/frontend/SmacDevelopedOverlay.h"
#include "game/frontend/SmacWaterCoast.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace raster=game::frontend::smac_terrain_raster;
namespace overlay=game::frontend::smac_developed_overlay;
std::vector<uint8_t> Read(const std::string& path) {
    std::ifstream f(path,std::ios::binary);if(!f) {throw std::runtime_error("Missing input: "+path);}
    return {std::istreambuf_iterator<char>(f),{}};
}
void Write(const std::string& path,const std::vector<uint8_t>& data) {
    std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());
    if(!f) {throw std::runtime_error("Cannot write output");}
}
int main(int argc,char** argv) {
    if(argc!=2) {throw std::invalid_argument("Expected owned temporary directory");}
    const std::string dir=argv[1];std::ifstream input(dir+"/input.txt");
    const auto number=[&]()->int64_t{int64_t v;if(!(input>>v)) {throw std::runtime_error("Truncated probe input");}return v;};
    const auto point=[&](){return raster::Point{int32_t(number()),int32_t(number())};};
    const auto crop=[&](){return overlay::Crop{int(number()),int(number()),int(number()),int(number())};};
    const auto colors=Read(dir+"/palette.bin");if(colors.size()!=768) {throw std::runtime_error("Invalid palette");}
    overlay::Palette palette{};for(size_t i=0;i<768;++i) {palette[i/3][i%3]=colors[i];}
    const auto shadow=overlay::ShadowLookup(palette);Write(dir+"/shadow-out.bin",{shadow.begin(),shadow.end()});
    const int tw=int(number()),th=int(number()),rw=int(number()),rh=int(number());
    const raster::Bitmap ter1{tw,th,Read(dir+"/ter1.bin")},rain{rw,rh,Read(dir+"/rainfall.bin")};
    const auto masks=game::frontend::smac_water_coast::CoastMasks(rain);
    const auto producers=number();std::ofstream selected(dir+"/selection.txt");
    for(int64_t i=0;i<producers;++i) {
        const auto items=uint32_t(number());const auto band=uint8_t(number());const auto nutrients=number();
        const auto word=uint32_t(number()),flags=uint32_t(number());const int zoom=int(number());
        const auto picks=overlay::Select(items,band,nutrients,word,flags,zoom);selected<<picks.size();
        for(const auto& p:picks) {for(auto v:p.crop) {selected<<' '<<v;}selected<<' '<<p.shadow;}selected<<'\n';
    }
    const int frames=int(number());
    for(int frame=0;frame<frames;++frame) {
        raster::Clip clip{};for(auto& v:clip) {v=int32_t(number());}
        const int background=int(number()),count=int(number());
        std::vector<raster::Bitmap> textures;textures.reserve(count);std::vector<raster::Command> commands;
        for(int i=0;i<count;++i) {
            raster::Command c;c.clip=clip;const int mode=int(number());
            if(mode>=6) {
                const auto rectangle=crop();const auto sprite=overlay::Decode(ter1,rectangle);const auto anchor=point();
                textures.push_back(sprite.bitmap);c.sprite_origin={anchor[0]+sprite.offset[0],anchor[1]+sprite.offset[1]};
                c.shader=mode==6?raster::Shader::Sprite:raster::Shader::SpriteShadow;c.shadow_lookup=&shadow;
                Write(dir+"/sprite-"+std::to_string(frame)+"-"+std::to_string(i)+".bin",sprite.bitmap.pixels);
                std::cout<<"S "<<frame<<' '<<i<<' '<<sprite.offset[0]<<' '<<sprite.offset[1]<<' '<<sprite.bitmap.width<<' '<<sprite.bitmap.height<<'\n';
            }else {
                const auto rectangle=crop();textures.push_back({rectangle[2],rectangle[3],Read(dir+"/texture-"+std::to_string(frame)+"-"+std::to_string(i)+".bin")});
                c.shader=static_cast<raster::Shader>(mode);c.polygon.count=uint8_t(number());
                for(size_t v=0;v<c.polygon.count;++v) {c.polygon.vertices[v]={point(),point(),int32_t(number())};}
                c.shade_limits={int32_t(number()),int32_t(number())};
                if(c.shader==raster::Shader::CoastMapped) {c.mask=&masks[0];for(auto& v:c.mask_mapping) {v=uint8_t(number());}}
            }
            c.texture=&textures.back();commands.push_back(c);
        }
        const auto out=raster::DrawBatch({1024,511,std::vector<uint8_t>(1024*511,uint8_t(background))},commands,clip,true);
        Write(dir+"/frame-"+std::to_string(frame)+".bin",out.bitmap.pixels);
        std::cout<<"F "<<frame<<' '<<out.attempts<<' '<<out.writes<<'\n';
    }
}
