#include "game/frontend/NativeTerrainView.h"
#include <iostream>
namespace native=game::frontend::native_terrain;
int main() {
    int count=0;std::cin>>count;if(!std::cin || count<0 || count>100) {return 1;}
    std::optional<native::ViewLayout> previous;
    for(int i=0;i<count;++i) {
        int x,y,zoom,mode;std::cin>>x>>y>>zoom>>mode;
        if(!std::cin || mode<0 || mode>2) {return 1;}
        const auto view=native::Layout(x,y,zoom,true);
        std::cout<<0<<' '<<i<<' '<<view.first[0]<<' '<<view.first[1]<<'\n';
        if(mode) {
            std::optional<native::ViewRegions> regions;
            if(mode==2) {
                if(!previous) {return 1;}regions=native::HorizontalPan(*previous,view);
                if(!regions) {return 1;}
                std::cout<<3<<' '<<i;for(const auto& r:*regions) {for(const int v:r) {std::cout<<' '<<v;}}std::cout<<'\n';
                std::cout<<4<<' '<<i<<' '<<view.first[0]<<' '<<view.first[1]<<' '<<previous->first[0]<<' '<<previous->first[1]<<'\n';
            }
            for(int objects=0;objects<2;++objects) {
                for(const auto& p:native::SelectView(view,regions,objects!=0)) {
                    std::cout<<objects+1<<' '<<i<<' '<<p.tile[0]<<' '<<p.tile[1]<<'\n';
                }
            }
        }
        previous=view;
    }
}
