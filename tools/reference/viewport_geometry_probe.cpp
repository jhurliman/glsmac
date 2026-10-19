#include "planet_geometry_input.h"
#include <iostream>
#include <stdexcept>
int Require(std::optional<int> value) {
    if(!value) {throw std::runtime_error("Unsupported viewport geometry query");}return *value;
}
int main(int argc,char** argv) {
    if(argc!=2) {throw std::invalid_argument("Expected owned Planet asset");}
    LandmarkGeometry model(80,80,true,PlanetCells(argv[1]),false,true);
    int count=0;std::cin>>count;
    if(!std::cin || count<0 || count>100) {throw std::runtime_error("Invalid viewport count");}
    for(int c=0;c<count;++c) {
        int zoom,relief,fx,fy,sx,sy,ox,oy,unit,n;
        std::cin>>zoom>>relief>>fx>>fy>>sx>>sy>>ox>>oy>>unit>>n;
        if(!std::cin || n<0 || n>100000 || unit<1 || unit>100) {throw std::runtime_error("Invalid view inputs");}
        model.Configure(zoom,relief);
        for(int e=0;e<n;++e) {
            int kind,x,y,v;std::cin>>kind>>x>>y>>v;
            if(!std::cin) {throw std::runtime_error("Truncated view query");}
            if(kind==0) {std::cout<<Require(model.Height(x,y,v))<<'\n';}
            else if(kind==1) {std::cout<<Require(model.Shade(x,y,v))<<'\n';}
            else if(kind==2) {
                int dx=x-fx;const int wrapped=(dx%80+80)%80;
                if(fy<=y && y<fy+sy+oy && wrapped<sx+ox) {dx=wrapped;}
                else {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
                std::cout<<(dx-1)*4*unit<<' '<<(y-fy-1)*2*unit-Require(model.ProjectorHeight(x,y))<<'\n';
            } else {throw std::runtime_error("Unknown view query");}
        }
    }
}
