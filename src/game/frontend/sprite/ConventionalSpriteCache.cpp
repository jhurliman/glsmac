#include "ConventionalSpriteCache.h"
#include "resource/ResourceManager.h"
#include "types/texture/Texture.h"
#include <fstream>
#include <stdexcept>
namespace game::frontend::sprite {
namespace {
std::string Read(const std::string& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file) {throw std::runtime_error("Cannot open conventional Unit asset: "+path);}
    const auto size=file.tellg();if(size<=0 || size>32*1024*1024) {throw std::runtime_error("Conventional Unit asset exceeds input domain");}
    std::string bytes(size_t(size),'\0');file.seekg(0);file.read(bytes.data(),std::streamsize(bytes.size()));
    if(!file || file.peek()!=std::char_traits<char>::eof()) {throw std::runtime_error("Conventional Unit asset changed while reading");}return bytes;
}
}
std::string ConventionalSpriteCache::ViewKey(const ConventionalUnitVisual& design,const loader::cvr::CvrScoutViewInput& view) {
    return design.Key()+":"+std::to_string(view.facing)+":"+std::to_string(view.zoom)+":"+std::to_string(view.scale_numerator)+":"+
        std::to_string(view.scale_denominator)+":"+std::to_string(view.anchor[0])+":"+std::to_string(view.anchor[1]);
}
ConventionalSpriteCache::Image ConventionalSpriteCache::Get(resource::ResourceManager& resources,const ConventionalUnitVisual& design,
    const loader::cvr::CvrScoutViewInput& view) {
    if(design.owner<1 || design.owner>7 || design.faction_art.empty() || design.faction_art.size()>256
        || design.faction_art.find_first_of("/\\:\0",0,4)!=std::string::npos) {throw std::invalid_argument("Conventional Unit needs its actual owner's faction art");}
    const auto key=ViewKey(design,view);const auto image=m_images.find(key);if(image!=m_images.end()) {return image->second;}
    const auto model_key=design.Key();auto model=m_models.find(model_key);
    if(model==m_models.end()) {
        const auto read=[&](const std::string& name) {return Read(resources.GetCustomPath(name));};
        auto prepared=loader::cvr::CvrScoutRenderer::LoadOrdinaryDesign(read,read(design.faction_art),design.chassis,design.weapon,design.armor,design.reactor,
            {design.weapon_power,design.weapon_mode,design.armor_strength,false,design.abilities});
        model=m_models.emplace(model_key,std::move(prepared)).first;
    }
    const auto frame=model->second.RenderOrdinaryBody(view);
    Image prepared{std::shared_ptr<types::texture::Texture>(frame.CreateTexture()),frame.target_anchor};
    return m_images.emplace(key,std::move(prepared)).first->second;
}
}
