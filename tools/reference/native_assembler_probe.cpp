#include "game/frontend/NativeTerrainRenderer.h"
#include "game/frontend/SmacWaterCoast.h"
#include "game/backend/map/SmacLandmarkGeometry.h"
#include "game/backend/map/SmacTerrainLighting.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <algorithm>

namespace native=game::frontend::native_terrain;
namespace geometry=game::backend::map::smac_terrain;
std::vector<uint8_t> Read(const std::string& path) {
    std::ifstream file(path,std::ios::binary);
    if(!file) {throw std::runtime_error("Missing probe input");}
    return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv) {
    if(argc!=2) {throw std::invalid_argument("Expected owned temporary directory");}
    const std::string dir=argv[1];std::ifstream input(dir+"/input.txt");
    const auto number=[&](){int64_t value;if(!(input>>value)) {throw std::runtime_error("Truncated assembler input");}return value;};
    const int tw=int(number()),th=int(number()),sw=int(number()),sh=int(number()),rw=int(number()),rh=int(number());
    native::Source source;source.loaded={tw,th,Read(dir+"/texture.bin")};source.ter1={sw,sh,Read(dir+"/ter1.bin")};
    source.coast_mask=game::frontend::smac_water_coast::CoastMasks({rw,rh,Read(dir+"/rainfall.bin")})[0];
    const auto shadow=Read(dir+"/shadow.bin");if(shadow.size()!=256) {throw std::runtime_error("Invalid shadow table");}
    for(unsigned i=0;i<256;++i) {
        source.shadow_lookup[i]=shadow[i];
        // An injective diagnostic palette recovers exact output indices from
        // the public RGBA frame. It does not affect retained shadow lookup.
        source.palette[i]={uint8_t(i),0,0};
    }
    const int count=int(number());
    for(int c=0;c<count;++c) {
        game::native_terrain::World world;
        const int x=int(number()),y=int(number()),band=int(number()),contour=int(number()),moisture=int(number());
        const auto live=uint32_t(number()),selected=uint32_t(number()),landmark=uint32_t(number());
        const auto nutrients=number();world.resource_pattern=int32_t(number());world.automatic_land_resources=bool(number());
        const auto flags=uint32_t(number());const auto seed=uint16_t(number());
        const int center_band=int(number()),center_contour=int(number());
        const auto at=[&](int tx,int ty)->game::native_terrain::Tile& {
            tx=(tx%80+80)%80;
            if(ty<0 || ty>=80 || ((tx^ty)&1)) {throw std::runtime_error("Invalid probe coordinate");}
            return world.tiles[size_t(ty)*40+size_t(tx/2)];
        };
        for(auto& tile:world.tiles) {tile.band=uint8_t(band);tile.contour=uint8_t(contour);tile.ordinary=true;tile.selected_known=true;}
        const int patches=int(number());
        for(int p=0;p<patches;++p) {
            const int dx=int(number()),dy=int(number());auto& tile=at(x+dx,y+dy);
            tile.band=uint8_t(number());tile.contour=uint8_t(number());tile.items=uint32_t(number());tile.selected_items=tile.items;
        }
        const int words=int(number());
        for(int p=0;p<words;++p) {const int dx=int(number()),dy=int(number());at(x+dx,y+dy).landmarks=uint32_t(number());}
        auto& tile=at(x,y);tile.band=uint8_t(center_band);tile.contour=uint8_t(center_contour);tile.moisture=uint8_t(moisture);
        tile.items=live;tile.selected_items=selected;tile.landmarks=landmark;tile.nutrients=nutrients;
        tile.developed_ready=tile.resource_ready=true;tile.resource_items=live;
        std::vector<geometry::LandmarkGeometry::Cell> cells;
        for(const auto& cell:world.tiles) {cells.push_back({cell.band,cell.landmarks});}
        geometry::LandmarkGeometry model(80,80,true,std::move(cells));
        if(tile.band>=3) {
            std::array<int,5> heights{};
            auto light=geometry::OrdinaryLighting(x,y,[&](int tx,int ty)->std::optional<uint8_t> {
                if(ty<0 || ty>=80) {return {};}
                return at(tx,ty).band;
            });
            if(!light) {throw std::runtime_error("Unsupported captured lighting");}
            for(unsigned v=0;v<5;++v) {
                const auto height=model.Height(x,y,v),shade=model.ExtraShade(x,y,v);
                if(!height || !shade) {throw std::runtime_error("Unsupported captured geometry");}
                heights[v]=*height;(*light)[v]+=*shade;
            }
            tile.heights=heights;tile.lighting=*light;
        }
        const auto view=native::View(x,y,true);
        const auto placement=std::find_if(view.begin(),view.end(),[&](const auto& p){return p.tile==native::Point{{x,y}};});
        if(placement==view.end()) {throw std::runtime_error("Captured center is outside native view");}
        const auto frame=native::Render(world,source,{*placement},uint16_t(seed-placement->record_offset),{{0,0}},flags,250);
        std::vector<uint8_t> indexed(1024*511,250);
        for(size_t i=0;i<indexed.size();++i) {if(frame.rgba[4*i+3]) {indexed[i]=frame.rgba[4*i];}}
        std::ofstream output(dir+"/frame-"+std::to_string(c)+".bin",std::ios::binary);
        output.write(reinterpret_cast<const char*>(indexed.data()),indexed.size());
        std::cout<<c<<' '<<frame.attempts<<' '<<frame.writes<<'\n';
    }
}
