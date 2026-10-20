#include "game/frontend/NativeTerrainRenderer.h"
#include "game/frontend/SmacWaterCoast.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace native=game::frontend::native_terrain;
std::vector<uint8_t> Read(const std::string& name) {
    std::ifstream input(name,std::ios::binary);if(!input) {throw std::runtime_error("Missing owned probe input");}
    return {std::istreambuf_iterator<char>(input),{}};
}
uint32_t Word(const std::vector<uint8_t>& bytes,size_t offset) {
    return uint32_t(bytes.at(offset))|(uint32_t(bytes.at(offset+1))<<8)|(uint32_t(bytes.at(offset+2))<<16)|(uint32_t(bytes.at(offset+3))<<24);
}
int main(int argc,char** argv) {
    if(argc!=3) {return 1;}const std::string dir=argv[1];std::ifstream input(dir+"/input.txt");
    int tw,th,sw,sh,rw,rh,seed,automatic,initial,observer0_alias,viewer;
    if(!(input>>tw>>th>>sw>>sh>>rw>>rh>>seed>>automatic>>initial>>observer0_alias>>viewer) || seed<0 || seed>65535 || initial<0 || initial>255) {return 1;}
    int partial=0;if(input>>partial) {if(partial<0 || partial>3 || (partial && viewer!=3)) {return 1;}}
    native::ViewGeometry::Knowledge known(3200,false);
    native::Source source;source.loaded={tw,th,Read(dir+"/texture.bin")};source.ter1={sw,sh,Read(dir+"/ter1.bin")};
    source.coast_mask=game::frontend::smac_water_coast::CoastMasks({rw,rh,Read(dir+"/rainfall.bin")})[0];
    const auto shadow=Read(dir+"/shadow.bin");if(shadow.size()!=256) {return 1;}
    for(size_t i=0;i<256;++i) {source.shadow_lookup[i]=shadow[i];source.palette[i]={uint8_t(i),0,0};}
    const auto bytes=Read(dir+"/planet.bin");if(bytes.size()!=15+0xaa4+3200*44+128) {return 1;}
    game::native_terrain::World world;world.retained_geometry=true;world.viewer=viewer;world.automatic_land_resources=automatic;
    const auto pattern=Word(bytes,23);world.resource_pattern=pattern<=INT32_MAX?int32_t(pattern):int32_t(int64_t(pattern)-4294967296LL);
    for(size_t i=0;i<3200;++i) {
        const size_t p=2739+i*44;auto& tile=world.tiles[i];
        tile.band=bytes[p]>>5;tile.moisture=(bytes[p]>>3)&3;tile.rockiness=bytes[p+5]>>6;tile.contour=bytes[p+1];
        tile.items=tile.selected_items=tile.resource_items=Word(bytes,p+8);tile.landmarks=Word(bytes,p+12);tile.region=bytes[p+3];
        if(observer0_alias) {tile.selected_items=tile.landmarks;}
        if(partial) {tile.selected_items=Word(bytes,p+24);known[i]=(bytes[p+4]&8)!=0;}
        tile.ordinary=tile.material_admitted=tile.resource_ready=tile.developed_ready=tile.selected_known=true;
    }
    const int zoom=std::stoi(argv[2]);
    if(zoom!=-1 && zoom!=1 && zoom!=3 && zoom!=8) {throw std::invalid_argument("Unmeasured diagnostic zoom profile");}
    if(zoom>0) {
        using Geometry=game::backend::map::smac_terrain::LandmarkGeometry;
        std::vector<Geometry::Cell> cells;for(const auto& t:world.tiles) {cells.push_back({t.band,t.landmarks});}
        Geometry geometry(80,80,true,std::move(cells),false,true);geometry.Configure(zoom,25);
        const native::Point tile=zoom==1?native::Point{{22,38}}:zoom==3?native::Point{{21,35}}:native::Point{{19,39}};
        native::ViewGeometry::Terrain terrain;terrain.placement.tile=tile;
        terrain.placement.record_offset=44u*uint32_t(40*tile[1]+tile[0]/2);
        int sum=0;for(unsigned v=4;v>0;--v) {sum+=geometry.Height(tile[0],tile[1],v).value();}
        for(unsigned v=0;v<5;++v) {terrain.heights[v]=geometry.Height(tile[0],tile[1],v).value();terrain.lighting[v]=geometry.Shade(tile[0],tile[1],v).value();}
        native::ViewGeometry::Plan plan;plan.zoom=zoom;plan.unit=native::Layout(22,38,zoom,true).unit;plan.opaque_background=false;
        terrain.projector={{100,100}};terrain.flat_center={{100+4*plan.unit,100+2*plan.unit+Geometry::FloorQuarter(sum)}};terrain.placement.flat_center=terrain.flat_center;
        plan.terrain.push_back(terrain);
        if(zoom==3) {plan.clip={{113,87,190,157}};}else if(zoom==8) {plan.clip={{117,97,200,177}};}
        const auto frame=native::RenderPrepared(world,source,plan,uint16_t(seed),0x60010ee3u,73);
        std::ofstream output(dir+"/tile-z"+std::to_string(zoom)+".indexed",std::ios::binary);output.write(reinterpret_cast<const char*>(frame.indexed.data()),frame.indexed.size());
        std::cout<<"tile "<<zoom<<" stores "<<frame.writes<<" heights";for(const auto h:terrain.heights) {std::cout<<' '<<h;}std::cout<<'\n';return 0;
    }
    native::ViewGeometry geometry(world);std::optional<native::Frame> previous;
    constexpr int cameras[4][2]={{22,38},{22,38},{24,38},{22,38}};
    for(int i=0;i<4;++i) {
        if(zoom==-1 && i==1) {continue;}
        std::ofstream queries(dir+"/frame-"+std::to_string(i)+".queries");
        const bool pan=i>=2;const auto plan=geometry.Prepare(native::Layout(cameras[i][0],cameras[i][1],zoom,true),zoom,25,pan,true,
            [&](const native::ViewGeometry::Query& query) {
                queries<<int(query.kind)<<' '<<query.tile[0]<<' '<<query.tile[1]<<' '<<query.vertex<<' '<<query.result[0]<<' '<<query.result[1]<<'\n';
            },partial?&known:nullptr,partial>=2?std::optional<int>{2}:std::nullopt);
        const auto cache=geometry.CacheBytes();std::ofstream cache_file(dir+"/frame-"+std::to_string(i)+".cache",std::ios::binary);
        cache_file.write(reinterpret_cast<const char*>(cache.data()),cache.size());
        auto frame=native::RenderPrepared(world,source,plan,uint16_t(seed),partial?0x60010ee3u:0x10621u,uint8_t(initial),pan?&*previous:nullptr);
        std::ofstream output(dir+"/frame-"+std::to_string(i)+".bin",std::ios::binary);
        output.write(reinterpret_cast<const char*>(frame.indexed.data()),frame.indexed.size());
        std::cout<<i<<' '<<frame.tiles<<' '<<frame.attempts<<' '<<frame.writes<<'\n';previous=std::move(frame);
    }
}
