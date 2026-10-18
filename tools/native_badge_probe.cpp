// Opt-in installed-asset probe; no proprietary pixels are stored in the repo.
#include "../src/loader/texture/SmacNativeBadge.h"
#include <SDL_image.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
int main(int argc,char** argv) {
    if(argc!=3) {std::cerr<<"usage: native_badge_probe ASSET_DIRECTORY OUTPUT_DIRECTORY\n";return 2;}
    try {
        using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)>;
        const std::filesystem::path assets(argv[1]),output(argv[2]);std::filesystem::create_directories(output);
        Surface units(IMG_Load((assets/"units.pcx").string().c_str()),SDL_FreeSurface);
        Surface palette(IMG_Load((assets/"palette.pcx").string().c_str()),SDL_FreeSurface);
        if(!units || !palette) {throw std::runtime_error(IMG_GetError());}
        for(int level=0;level<7;++level) {for(int variant=0;variant<2;++variant) {
            const auto badge=loader::texture::DecodeSmacNativeBadge(units.get(),palette->format->palette,level,variant);
            const auto stem="native-"+std::to_string(level)+"-"+std::to_string(variant);
            std::ofstream indices(output/(stem+".indices"),std::ios::binary);indices.write(reinterpret_cast<const char*>(badge.indexed.data()),badge.indexed.size());
            std::ofstream rgba(output/(stem+".rgba"),std::ios::binary);rgba.write(reinterpret_cast<const char*>(badge.rgba.data()),badge.rgba.size());
            if(!indices || !rgba) {throw std::runtime_error("Could not write badge output");}
        }}
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
