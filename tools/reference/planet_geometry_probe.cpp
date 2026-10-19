#include "game/backend/map/SmacLandmarkGeometry.h"
#include <fstream>
#include <iostream>
#include "planet_geometry_input.h"
#include <stdexcept>
using game::backend::map::smac_terrain::LandmarkGeometry;
int Value(std::optional<int> value) {if(!value) {throw std::runtime_error("Unsupported authored geometry query");}return *value;}
std::array<int,2> Projector(LandmarkGeometry& model,int x,int y) {
    int dx=x-28;const int wrapped=(dx%80+80)%80;
    if(y>=28 && y<52 && wrapped<22) {dx=wrapped;}
    else {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
    return {{(dx-1)*48,(y-29)*24-Value(model.ProjectorHeight(x,y))}};
}
int main(int argc,char** argv) {
    if(argc!=3) {throw std::invalid_argument("Expected pinned asset and owned output directory");}
    const auto cells=PlanetCells(argv[1]);const std::string dir=argv[2];
    for(int reverse=0;reverse<2;++reverse) {
        LandmarkGeometry model(80,80,true,cells,false,true);std::ofstream out(dir+"/traversal-"+std::to_string(reverse)+".txt");
        for(int position=0;position<3200;++position) {
            const int index=reverse?3199-position:position,x=2*(index%40)+(index/40&1),y=index/40;
            const auto p=Projector(model,x,y);out<<x<<' '<<y<<' '<<p[0]<<' '<<p[1];
            for(unsigned v=0;v<5;++v) {out<<' '<<Value(model.Height(x,y,v));}
            for(unsigned v=0;v<5;++v) {out<<' '<<Value(model.Shade(x,y,v));}
            out<<'\n';
        }
    }
    LandmarkGeometry model(80,80,true,cells,false,true);
    std::ifstream input(dir+"/renderer-input.txt");std::ofstream out(dir+"/renderer.txt");int count=0;input>>count;if(!input || count<0) {throw std::runtime_error("Invalid painter count");}
    constexpr int ids[4][3]={{0,1,2},{0,2,3},{0,3,4},{0,4,1}};
    constexpr int offsets[5][2]={{0,0},{-48,0},{0,-24},{48,0},{0,24}};
    for(int i=0;i<count;++i) {
        int x,y;input>>x>>y;if(!input) {throw std::runtime_error("Truncated painter order");}
        const auto p=Projector(model,x,y);const int mean=Value(model.ProjectorHeight(x,y));
        const int cx=p[0]+48,cy=p[1]+mean+24;out<<x<<' '<<y<<' '<<p[0]<<' '<<p[1];
        for(const auto& triangle:ids) {
            std::array<int,3> shades{};
            for(unsigned v=0;v<3;++v) {
                const auto k=triangle[v];
                out<<' '<<cx+offsets[k][0]<<' '<<cy+offsets[k][1]-Value(model.Height(x,y,k));
                shades[v]=Value(model.Shade(x,y,k));
            }
            for(const auto shade:shades) {out<<' '<<shade;}
        }
        out<<'\n';
    }
}
