#include "gse/tests/fixtures/SmacWaterCoast.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace raster=game::frontend::smac_terrain_raster;
std::vector<uint8_t> Read(const std::string& path) {
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Missing indexed input: "+path);
 return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});
}
void Write(const std::string& path,const std::vector<uint8_t>& pixels) {
 std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());if(!f)throw std::runtime_error("Cannot write indexed output");
}
int main(int argc,char** argv) {
 if(argc!=5)throw std::invalid_argument("Expected loaded directory, output prefix and rainfall width/height");
 const std::string directory=argv[1],output=argv[2];
 const raster::Bitmap rainfall{std::stoi(argv[3]),std::stoi(argv[4]),Read(directory+"/rainfall.bin")};
 const auto masks=game::frontend::smac_water_coast::CoastMasks(rainfall);
 for(size_t i=0;i<2;++i)Write(output+"mask-"+std::to_string(i)+".bin",masks[i].pixels);
 const auto frames=gse::tests::water_coast_fixture::Frames();
 for(size_t index=0;index<frames.size();++index) {
  const auto& frame=frames[index];std::vector<raster::Bitmap> textures;textures.reserve(frame.draws.size());
  for(size_t i=0;i<frame.draws.size();++i) {
   const auto& crop=frame.draws[i].pass.crop;
   textures.push_back({crop[2],crop[3],Read(directory+"/texture-"+std::to_string(index)+"-"+std::to_string(i)+".bin")});
  }
  std::vector<raster::Command> commands;
  for(size_t i=0;i<frame.draws.size();++i) {
   const auto& pass=frame.draws[i].pass;
   raster::Command command{pass.polygon,&textures[i],pass.shader,frame.clip,pass.shade_limits};
   if(pass.shader==raster::Shader::CoastMapped) {command.mask=&masks[0];command.mask_mapping=pass.mapping;}
   commands.push_back(command);
  }
  const auto result=raster::DrawBatch({1024,511,std::vector<uint8_t>(1024*511,250)},commands,frame.clip,true);
  Write(output+std::to_string(index)+".bin",result.bitmap.pixels);
  std::cout<<index<<":"<<result.attempts<<":"<<result.writes<<"\n";
 }
}
