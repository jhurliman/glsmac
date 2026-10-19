#include "game/frontend/NativeViewGeometry.h"
#include "planet_geometry_input.h"
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=2) {return 1;}
    game::native_terrain::World world;const auto cells=PlanetCells(argv[1]);
    for(size_t i=0;i<cells.size();++i) {world.tiles[i].band=cells[i].band;world.tiles[i].landmarks=cells[i].landmarks;}
    namespace native=game::frontend::native_terrain;native::ViewGeometry geometry(world);
    int count=0;std::cin>>count;if(!std::cin || count<0 || count>100) {return 1;}
    for(int i=0;i<count;++i) {
        int x,y,zoom,relief,mode;std::cin>>x>>y>>zoom>>relief>>mode;
        if(!std::cin || mode<0 || mode>2) {return 1;}
        geometry.Prepare(native::Layout(x,y,zoom,true),zoom,relief,mode==2,mode!=0,[&](const auto& q) {
            std::cout<<i<<' '<<int(q.kind)<<' '<<q.tile[0]<<' '<<q.tile[1]<<' '<<q.vertex<<' '<<q.result[0];
            if(q.kind==native::ViewGeometry::Query::Kind::Projector) {std::cout<<' '<<q.result[1];}std::cout<<'\n';
        });
    }
}
