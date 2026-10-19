#include "gse/tests/fixtures/SmacTerritoryBoundary.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace raster=game::frontend::smac_terrain_raster;
namespace boundary=game::frontend::smac_territory_boundary;
std::vector<uint8_t> Read(const std::string& path) {
    std::ifstream file(path,std::ios::binary);if(!file) {throw std::runtime_error("Missing indexed input: "+path);}
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
}
void Write(const std::string& path,const std::vector<uint8_t>& bytes) {
    std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!file) {throw std::runtime_error("Cannot write indexed output");}
}
void WriteRows(const std::string& path,const raster::OwnerClut& rows) {
    std::vector<uint8_t> bytes;for(const auto& row:rows) {bytes.insert(bytes.end(),row.begin(),row.end());}Write(path,bytes);
}
int main(int argc,char** argv) {
    if(argc!=5) {throw std::invalid_argument("Expected loaded directory, output prefix and rainfall dimensions");}
    const std::string directory=argv[1],output=argv[2];
    const auto palette=Read(directory+"/palette.bin"),targets=Read(directory+"/targets.bin"),system=Read(directory+"/system.txt"),factions=Read(directory+"/factions.bin");
    if(palette.size()!=768 || targets.size()!=8 || factions.size()%3) {throw std::invalid_argument("Invalid palette metadata inputs");}
    game::frontend::smac_land_material::Palette colors{};
    for(size_t i=0;i<256;++i) {for(size_t c=0;c<3;++c) {colors[i][c]=palette[3*i+c];}}
    std::array<uint8_t,8> sampled{};std::copy(targets.begin(),targets.end(),sampled.begin());
    const auto initial=boundary::Rows(colors,sampled,boundary::ReadOpacities(std::string_view(reinterpret_cast<const char*>(system.data()),system.size())));
    auto active=initial;
    for(size_t i=0;i<factions.size();i+=3) {
        if(factions[i]>=8) {throw std::invalid_argument("Invalid owner metadata");}
        active[factions[i]]=boundary::Row(colors,factions[i+1],static_cast<uint8_t>(factions[i+2]-10)*100/255);
    }
    WriteRows(output+"initial.bin",initial);WriteRows(output+"active.bin",active);
    const raster::Bitmap rainfall{std::stoi(argv[3]),std::stoi(argv[4]),Read(directory+"/rainfall.bin")};
    const auto classes=boundary::Mask(rainfall);const auto masks=game::frontend::smac_water_coast::CoastMasks(rainfall);
    Write(output+"classes.bin",classes.pixels);
    const auto frames=gse::tests::boundary_fixture::Frames();
    for(size_t index=0;index<frames.size();++index) {
        const auto& frame=frames[index];std::vector<raster::Bitmap> textures;textures.reserve(frame.draws.size());
        for(size_t i=0;i<frame.draws.size();++i) {
            const auto& pass=frame.draws[i].pass;
            if(pass.shader==raster::Shader::OwnerClut) {textures.push_back({});continue;}
            textures.push_back({pass.crop[2],pass.crop[3],Read(directory+"/texture-"+std::to_string(index)+"-"+std::to_string(i)+".bin")});
        }
        std::vector<raster::Command> commands;
        for(size_t i=0;i<frame.draws.size();++i) {
            const auto& draw=frame.draws[i];const auto& pass=draw.pass;
            raster::Command command{pass.polygon,pass.shader==raster::Shader::OwnerClut?&classes:&textures[i],pass.shader,frame.clip,pass.shade_limits};
            if(pass.shader==raster::Shader::CoastMapped) {command.mask=&masks[0];command.mask_mapping=pass.mapping;}
            if(pass.shader==raster::Shader::OwnerClut) {command.owner_clut=&active;command.owner_mapping=draw.owner_mapping;}
            commands.push_back(command);
        }
        raster::Bitmap initial_image{1024,511,std::vector<uint8_t>(1024*511,250)};
        if(frame.gradient) {for(size_t i=0;i<initial_image.pixels.size();++i) {initial_image.pixels[i]=static_cast<uint8_t>(i%1024);}}
        const auto result=raster::DrawBatch(initial_image,commands,frame.clip,true);
        Write(output+std::to_string(index)+".bin",result.bitmap.pixels);
        std::cout<<index<<":"<<result.attempts<<":"<<result.writes<<"\n";
    }
}
