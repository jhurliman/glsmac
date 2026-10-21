#include "game/frontend/SmacDevelopedOverlay.h"
#include "loader/cvr/CvrActorMaterial.h"
#include "ConventionalSpriteCache.h"
#include "resource/ResourceManager.h"
#include "types/texture/Texture.h"
#include <fstream>
#include <algorithm>
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
    const auto key=ViewKey(design,view);const auto image=m_images.find(key);if(image!=m_images.end()) {return image->second;}
    auto& model=Model(resources,design);
    const auto frame=model.RenderOrdinaryBody(view);
    Image prepared{std::shared_ptr<types::texture::Texture>(frame.CreateTexture()),frame.target_anchor};
    return m_images.emplace(key,std::move(prepared)).first->second;
}
loader::cvr::CvrScoutRenderer& ConventionalSpriteCache::Model(resource::ResourceManager& resources,const ConventionalUnitVisual& design) {
    if(design.owner<1 || design.owner>7 || design.faction_art.empty() || design.faction_art.size()>256
        || design.faction_art.find_first_of("/\\:\0",0,4)!=std::string::npos) {throw std::invalid_argument("Conventional Unit needs its actual owner's faction art");}
    const auto model_key=design.Key();auto model=m_models.find(model_key);
    if(model==m_models.end()) {
        const auto read=[&](const std::string& name) {return Read(resources.GetCustomPath(name));};
        auto prepared=loader::cvr::CvrScoutRenderer::LoadOrdinaryDesign(read,read(design.faction_art),design.chassis,design.weapon,design.armor,design.reactor,
            {design.weapon_power,design.weapon_mode,design.armor_strength,false,design.abilities});
        model=m_models.emplace(model_key,std::move(prepared)).first;
    }
    return model->second;
}
std::shared_ptr<types::texture::Texture> ConventionalSpriteCache::GetCard(resource::ResourceManager& resources,const ConventionalUnitVisual& design,
    int facing,bool roster,const std::array<int,2>& origin,const loader::texture::SmacDialogSource& source) {
    const int width=roster?65:116,height=roster?48:176;
    if(origin[0]<-4096 || origin[0]>4096 || origin[1]!=(roster?196:66)) {
        throw std::invalid_argument("Native Unit card exceeds the console bitmap");
    }
    const loader::cvr::CvrScoutViewInput view{facing,roster?-8:-4,0,1,{0,0}};
    const int phase_key=roster && origin[0]>=148 && origin[0]+width<=755 ? origin[0]%24:origin[0]+4096;
    const auto key=ViewKey(design,view)+":"+std::to_string(phase_key)+":"+std::to_string(origin[1]);
    const auto found=m_cards.find(key);if(found!=m_cards.end()) {return found->second;}
    if(m_console.empty()) {
        auto backdrop=loader::cvr::CvrActorMaterial::ReadConsoleBackdrop(Read(resources.GetCustomPath("console2.pcx")));
        using namespace loader::texture;
        for(const auto& call:std::array<smac_dialog_frame::Call,4>{{{21,8,60,136,248},{18,11,63,133,245},{21,139,192,764,248},{18,142,195,761,245}}}) {
            smac_dialog_frame::Paint(backdrop,1024,257,source,call,{0,0,1024,257});
        }
        m_shadow_lookup=smac_developed_overlay::ShadowLookup(source.palette);m_console=std::move(backdrop);
    }
    const auto image=Model(resources,design).RenderOrdinary(view);
    const std::array<int,2> offset{(roster?38:57)-128,(roster?26:37)-128};
    std::vector<uint8_t> target(size_t(width)*height);
    for(int y=0;y<height;++y) {for(int x=0;x<width;++x) {
        const int dx=x+origin[0];target[size_t(y)*width+x]=dx>=0 && dx<1024 ? m_console[size_t(y+origin[1])*1024+dx]:9;
    }}
    image.Composite(target,width,height,width,m_shadow_lookup,offset,{0,0,width,height});
    auto texture=std::make_shared<types::texture::Texture>("Native indexed Unit card",width,height);texture->Clear();
    auto* rgba=texture->GetBitmap();
    for(int y=0;y<height;++y) {for(int x=0;x<width;++x) {
        const int sx=x-offset[0],sy=y-offset[1];
        if(sx<0 || sy<0 || sx>=256 || sy>=256) {continue;}
        const auto i=size_t(sy)*256+sx;
        if(image.shadow.mask[i]==0 && image.body.indexed.color[i]==9) {continue;}
        const auto out=(size_t(y)*width+x)*4;const auto color=source.palette[target[size_t(y)*width+x]];
        for(size_t c=0;c<3;++c) {rgba[out+c]=color[c];}rgba[out+3]=255;
    }}
    texture->FullUpdate();
    // Widgets retain borrowed card textures independently of this bounded cache.
    if(m_cards.size()>=512) {m_cards.erase(m_cards.begin());}
    return m_cards.emplace(key,std::move(texture)).first->second;
}

}
