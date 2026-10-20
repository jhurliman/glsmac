#include "game/frontend/NativeTerrainSession.h"
#include "game/frontend/SmacWaterCoast.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "loader/texture/SmacPalette.h"
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
namespace native=game::frontend::native_terrain;
std::vector<uint8_t> Read(const std::string& path) {
    std::ifstream input(path,std::ios::binary);
    if(!input) {throw std::runtime_error("Missing owned appearance input");}
    return {std::istreambuf_iterator<char>(input),{}};
}
void Write(const std::string& path,const std::vector<uint8_t>& bytes) {
    std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    if(!output) {throw std::runtime_error("Appearance output failed");}
}
uint32_t Word(const std::vector<uint8_t>& bytes,size_t offset) {
    return uint32_t(bytes.at(offset))|(uint32_t(bytes.at(offset+1))<<8)|(uint32_t(bytes.at(offset+2))<<16)|(uint32_t(bytes.at(offset+3))<<24);
}
int main(int argc,char** argv) {
    if(argc!=2) {return 1;}const std::string dir=argv[1];std::ifstream input(dir+"/input.txt");
    int tw,th,sw,sh,rw,rh,seed;if(!(input>>tw>>th>>sw>>sh>>rw>>rh>>seed) || seed<0 || seed>65535) {return 1;}
    native::Source source;source.loaded={tw,th,Read(dir+"/texture.bin")};source.ter1={sw,sh,Read(dir+"/ter1.bin")};
    source.coast_mask=game::frontend::smac_water_coast::CoastMasks({rw,rh,Read(dir+"/rainfall.bin")})[0];
    const auto common=Read(dir+"/common-palette.bin");if(common.size()!=768) {return 1;}
    std::unique_ptr<SDL_Palette,decltype(&SDL_FreePalette)> palette(SDL_AllocPalette(256),SDL_FreePalette);
    if(!palette) {return 1;}std::array<SDL_Color,256> colors{};
    for(size_t i=0;i<256;++i) {colors[i]={common[3*i],common[3*i+1],common[3*i+2],255};}
    if(SDL_SetPaletteColors(palette.get(),colors.data(),0,256)!=0) {return 1;}
    source.palette=loader::texture::MakeSmacSoftwarePalette(palette.get());
    source.shadow_lookup=game::frontend::smac_developed_overlay::ShadowLookup(source.palette);
    std::vector<uint8_t> active;for(const auto& rgb:source.palette) {active.insert(active.end(),rgb.begin(),rgb.end());}Write(dir+"/palette.rgb.bin",active);
    const auto bytes=Read(dir+"/planet.bin");if(bytes.size()!=15+0xaa4+3200*44+128) {return 1;}
    game::native_terrain::World world;world.retained_geometry=true;world.viewer=3;
    world.resource_pattern=static_cast<int32_t>(Word(bytes,23));
    world.exploration_ready=true;world.observers_present=8;world.initial_survey=false;
    for(size_t i=0;i<3200;++i) {
        const size_t p=2739+i*44;auto& tile=world.tiles[i];
        tile.band=bytes[p]>>5;tile.moisture=(bytes[p]>>3)&3;tile.rockiness=bytes[p+5]>>6;tile.contour=bytes[p+1];
        tile.items=tile.selected_items=tile.resource_items=Word(bytes,p+8);tile.landmarks=Word(bytes,p+12);tile.region=bytes[p+3];
        tile.ordinary=tile.material_admitted=tile.resource_ready=tile.developed_ready=tile.selected_known=true;tile.explored_by=8;
    }
    for(const int relief:{25,8}) {
        native::Session session(world);native::ViewContext context;
        context.camera={{22,38}};context.flags=0x10621;context.preferences={0,relief==8?0x10000u:0u};context.relief=relief;
        const auto plan=session.Prepare(world,context,native::Session::Observation::RevealedCanvas);
        if(!plan) {return 1;}
        const auto frame=native::RenderPrepared(world,source,*plan,uint16_t(seed),context.flags,65,nullptr,context.preferences);
        const std::string name=relief==8?"low_relief":"normal_relief";Write(dir+"/"+name+".indexed.bin",frame.indexed);
        std::vector<uint8_t> rgb;rgb.reserve(frame.indexed.size()*3);
        for(size_t i=0;i<frame.indexed.size();++i) {rgb.insert(rgb.end(),frame.rgba.begin()+4*i,frame.rgba.begin()+4*i+3);}
        Write(dir+"/"+name+".rgb.bin",rgb);
    }
}
